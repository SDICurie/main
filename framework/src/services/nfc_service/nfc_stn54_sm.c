/**
 * COPYRIGHT(c) 2014 STMicroelectronics
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

#include <cfw/cfw.h>
#include <cfw/cfw_service.h>
#include <infra/port.h>
#include <string.h>
#include "services/services_ids.h"
#include "infra/log.h"
#include "infra/tcmd/handler.h"

#include <util/misc.h>

#include "services/nfc_service/nfc_service.h"
#include "nfc_service_private.h"
#include "nfc_stn54_sm.h"
#include "nfc_stn54_seq.h"

#include "drivers/gpio.h"
#include "drivers/nfc_stn54.h"

#ifdef CONFIG_NFC_STN54_FW_UPDATE
#include <drivers/spi_flash.h>

/* taken from bootloader ota.h; need to keep it in sync */
enum OTA_TYPE {
	ARC = 0,
	QUARK = 1,
	BLE_CORE = 2,
	BOOTLOADER = 3,
	NFC_FW = 4,
	OTA_TYPE_MAX = 5
};

struct ota_binary_header {
	uint8_t magic[3];
	uint8_t type;
	uint32_t version;
	uint32_t offset;
	uint32_t length;
};

struct ota_header {
	uint8_t magic[3];
	uint8_t hdr_version;
	uint16_t hdr_length;
	uint16_t platform;
	uint32_t crc;
	uint32_t pl_length;
	uint32_t build_version;
	uint32_t min_version;
	uint32_t app_min_version;
	struct ota_binary_header binaries[OTA_TYPE_MAX];
};
#endif

typedef enum nfc_fsm_state {
	ST_OFF,     /* controller is powered down, wait for init */
	ST_INIT,    /* controller is powered up and initialized, wait for mode config */
	ST_CONFIG,  /* mode configuration is done*/
	ST_READY,   /* controller is ready to start the RF loop */
	ST_ACTIVE,  /* the RF loop is active */
	ST_DISABLE, /* stop ongoing commands, start cleanup scenario, power off */
	ST_HIBER,       /* stop ongoing commands, enter hibernation mode */
#ifdef CONFIG_NFC_STN54_FW_UPDATE
	ST_FW_UPDATE, /* firmware update ongoing */
#endif
	ST_ERROR,   /* something's not right! */
	ST_ANY,     /* "any state" placeholder in the FSM table */
} fsm_state_t;

static const char *state_name[] = {
	"OFF",
	"INIT",
	"CONFIG",
	"READY",
	"ACTIVE",
	"DISABLE",
	"HIBERNATE",
#ifdef CONFIG_NFC_STN54_FW_UPDATE
	"FW_UPDATE",
#endif
	"ERROR",
	"ANY"
};

static uint16_t fsm_port_id;
static T_QUEUE scn_queue;
static T_TIMER scn_timer;

static uint8_t error_status;
static uint8_t error_id;

static fsm_state_t state;
static struct nfc_msg_cb *pending_msg_cb;

static uint8_t *rx_data;
static uint16_t rx_len;

/** Internal NFC FSM event message id */
#define NFC_FSM_EVENT_MESSAGE 0xfe80

/**
 * Internal event structure.
 */
struct fsm_event_message {
	struct message m;
	int event;
	void *event_data;
	struct nfc_msg_cb *msg_cb;
};

typedef struct {
	fsm_state_t state;
	int event;
	fsm_state_t (*action)(struct fsm_event_message *evt); /* return next state */
} fsm_transition_t;

typedef struct {
	scn_exec_t exec;
	int timeout;
	uint8_t arg_len;
	uint8_t args[];
} nfc_scenario_t;

static void nfc_fsm_message_handler(struct message *msg, void *param);
static void nfc_fsm_run(struct fsm_event_message *evt);
static void nfc_fsm_rx_handler(void *priv);
static DRIVER_API_RC nfc_fsm_rx_reader(uint8_t *rx_data, uint16_t *rx_len);

static int set_msg_cb(struct nfc_msg_cb *msg_cb);
static void run_msg_cb(struct nfc_msg_cb *msg_cb, uint8_t status);
static void run_pending_msg_cb(uint8_t status);

static void scn_send_timer(nfc_scenario_t *scn, uint8_t timer_value);
static void scn_timer_cb(void *param);

static fsm_state_t act_sequencer(struct fsm_event_message *evt);
static fsm_state_t act_unhandled_event(struct fsm_event_message *evt);
static fsm_state_t act_scn_passed(struct fsm_event_message *evt);
static fsm_state_t act_scn_failed(struct fsm_event_message *evt);

static fsm_state_t act_init(struct fsm_event_message *evt);
static fsm_state_t act_config(struct fsm_event_message *evt);
static fsm_state_t act_start(struct fsm_event_message *evt);
static fsm_state_t act_stop(struct fsm_event_message *evt);
static fsm_state_t act_disable(struct fsm_event_message *evt);
static fsm_state_t act_hibernate_enter(struct fsm_event_message *evt);
static fsm_state_t act_hibernate_exit(struct fsm_event_message *evt);

static fsm_state_t act_goto_CONFIG(struct fsm_event_message *evt);
static fsm_state_t act_goto_READY(struct fsm_event_message *evt);
static fsm_state_t act_goto_ACTIVE(struct fsm_event_message *evt);
static fsm_state_t act_goto_OFF(struct fsm_event_message *evt);

static fsm_state_t act_err_cleanup(struct fsm_event_message *evt);
static fsm_state_t act_err_recovery(struct fsm_event_message *evt);
static fsm_state_t act_rf_tuning(struct fsm_event_message *evt);
#ifdef CONFIG_NFC_STN54_FW_UPDATE
static fsm_state_t act_fwu_start(struct fsm_event_message *evt);
static fsm_state_t act_fwu_pass(struct fsm_event_message *evt);
static fsm_state_t act_fwu_fail(struct fsm_event_message *evt);
#endif

static const fsm_transition_t fsm_table[] = {
	/* <state> , <event>, <action(event, event data)> */

#ifdef CONFIG_NFC_STN54_FW_UPDATE
	{ ST_ANY, EV_NFC_FW_UPDATE_START, act_fwu_start },      /* go to FW_UPDATE */
	{ ST_FW_UPDATE, EV_NFC_FW_UPDATE_DONE, act_fwu_pass },
	{ ST_FW_UPDATE, EV_NFC_FW_UPDATE_FAIL, act_fwu_fail },
#endif

	/* error sequencer handling */
	{ ST_ERROR, EV_NFC_ERR_START, act_err_cleanup },
	{ ST_ERROR, EV_NFC_ERR_DONE, act_err_recovery },
	{ ST_ERROR, EV_NFC_ANY, act_sequencer },

	/* normal scenario sequencer handling */
	{ ST_ANY, EV_NFC_RX_IRQ, act_sequencer },
	{ ST_ANY, EV_NFC_SCN_TIMER, act_sequencer },
	{ ST_ANY, EV_NFC_SCN_START, act_sequencer },
	{ ST_ANY, EV_NFC_SCN_DONE, act_sequencer },
	{ ST_ANY, EV_NFC_SCN_FAIL, act_sequencer },
	{ ST_ANY, EV_NFC_SCN_TIMEOUT, act_sequencer },
	{ ST_ANY, EV_NFC_SCN_IGNORE, act_sequencer },

	/* service commands handling, main FSM state changes */
	{ ST_ANY, EV_NFC_HIBERNATE, act_hibernate_enter },   /* go to HIBERNATE from any state */
	{ ST_HIBER, EV_NFC_INIT, act_hibernate_exit },      /* go to INIT */
	{ ST_OFF, EV_NFC_INIT, act_init },                /* go to INIT */
	{ ST_INIT, EV_NFC_SEQ_PASS, act_goto_CONFIG },       /* go to CONFIG */
	{ ST_CONFIG, EV_NFC_CONFIG_CE, act_config },      /* stay in CONFIG */
	{ ST_CONFIG, EV_NFC_SEQ_PASS, act_goto_READY },   /* go to READY */
	{ ST_READY, EV_NFC_RF_START, act_start },         /* stay in READY */
	{ ST_READY, EV_NFC_SEQ_PASS, act_goto_ACTIVE },   /* go to ACTIVE */
	{ ST_ACTIVE, EV_NFC_RF_STOP, act_stop },          /* stay in ACTIVE */
	{ ST_ACTIVE, EV_NFC_SEQ_PASS, act_goto_READY },   /* go to READY */
	{ ST_OFF, EV_NFC_DISABLE, act_unhandled_event },     /* exception: No DISABLE in OFF state */
	{ ST_ANY, EV_NFC_DISABLE, act_disable },          /* go to ST_DISABLE */
	{ ST_DISABLE, EV_NFC_SEQ_PASS, act_goto_OFF },    /* go to ST_OFF */

	{ ST_ANY, EV_NFC_RF_TUNING, act_rf_tuning },      /* keep state */

	/* generic handling of scenarios pass/fail; can be overriden above if necessary */
	{ ST_ANY, EV_NFC_SEQ_FAIL, act_scn_failed },
	{ ST_ANY, EV_NFC_SEQ_PASS, act_scn_passed },

	/* catch all - discards unhandled callbacks/messages */
	{ ST_ANY, EV_NFC_ANY, act_unhandled_event }
};

void nfc_fsm_init(void *queue)
{
	OS_ERR_TYPE err;

	fsm_port_id = port_alloc(queue);
	port_set_handler(fsm_port_id, nfc_fsm_message_handler, NULL);

	scn_queue = queue_create(5);
	scn_timer = timer_create(scn_timer_cb, "", 500, 0, 0, NULL);

	state = ST_OFF;
	pending_msg_cb = NULL;
	error_status = NFC_STATUS_ERROR;
	error_id = 0;

	rx_len = 0;
	rx_data = balloc(ST21NFC_NCI_FRAME_MAX_SIZE, &err);
	if (err != E_OS_OK) {
		pr_error(LOG_MODULE_NFC, "Failed to balloc rx data buf: %d",
			 err);
		return;
	}

	nfc_stn54_set_rx_handler(nfc_fsm_rx_handler);
}

static void nfc_fsm_message_handler(struct message *msg, void *param)
{
	struct fsm_event_message *evt = (struct fsm_event_message *)msg;

	if (evt != NULL) {
		nfc_fsm_run(evt);
		bfree(evt);
		evt = NULL;
	}
}

int nfc_fsm_event_post(int event, void *event_data, struct nfc_msg_cb *msg_cb)
{
	struct fsm_event_message *evt = (struct fsm_event_message *)
					message_alloc(sizeof(*evt), NULL);

	if (evt != NULL) {
		MESSAGE_ID(&evt->m) = NFC_FSM_EVENT_MESSAGE;
		MESSAGE_DST(&evt->m) = fsm_port_id;
		MESSAGE_SRC(&evt->m) = fsm_port_id;
		evt->event = event;
		evt->event_data = event_data;
		evt->msg_cb = msg_cb;
		return port_send_message(&evt->m);
	} else {
		pr_error(LOG_MODULE_NFC, "Failed to post event %d", event);
		return -1;
	}
}

static void nfc_fsm_run(struct fsm_event_message *evt)
{
	uint8_t i;
	uint8_t previous_state;

	for (i = 0; i < NUM_ELEMS(fsm_table); ++i) {
		if (fsm_table[i].state == state || fsm_table[i].state ==
		    ST_ANY) {
			if ((fsm_table[i].event == evt->event) ||
			    fsm_table[i].event == EV_NFC_ANY) {
				if (fsm_table[i].action != NULL) {
					previous_state = state;
					state = (fsm_table[i].action)(evt);
					if (previous_state != state) {
						pr_info(
							LOG_MODULE_NFC,
							"nfc: %s -> %s (event:%d)",
							state_name[
								previous_state],
							state_name[state],
							evt->event);
					}
				}
				break; /* we found the matching state/event */
			}
		}
	}
}

void nfc_fsm_rx_handler(void *priv)
{
	/* priv contains pointer to nfc device */
	nfc_fsm_event_post(EV_NFC_RX_IRQ, priv, NULL);
}

DRIVER_API_RC nfc_fsm_rx_reader(uint8_t *rx_data, uint16_t *rx_len)
{
	int ret_h = -1, ret_p = -1, len = 0, p_len = 0;

	ret_h = nfc_stn54_read(&rx_data[0], ST21NFC_NCI_FRAME_HEADER);
	if (ret_h == DRV_RC_OK) {
		len += ST21NFC_NCI_FRAME_HEADER;
		p_len = rx_data[ST21NFC_NCI_FRAME_HEADER - 1];
		if ((p_len >= 1) &&
		    (p_len <= ST21NFC_NCI_FRAME_MAX_SIZE -
		     ST21NFC_NCI_FRAME_HEADER)) {
			ret_p =
				nfc_stn54_read(
					&rx_data[
						ST21NFC_NCI_FRAME_HEADER
					], p_len);
			if (ret_p == DRV_RC_OK) {
				len += p_len;
			}
		}
	}
	*rx_len = len;

#ifdef CONFIG_NFC_COMPARATOR_IRQ
	nfc_stn54_config_irq_out();
#endif

	if (ret_h != DRV_RC_OK) {
		return ret_h;
	} else if (ret_p != DRV_RC_OK && len > ST21NFC_NCI_FRAME_HEADER) {
		return ret_p;
	}

	return DRV_RC_OK;
}

void run_msg_cb(struct nfc_msg_cb *msg_cb, uint8_t status)
{
	if (msg_cb) {
		if (msg_cb->msg_cb && msg_cb->msg) {
			msg_cb->msg_cb(msg_cb->msg, status);
		}

		if (msg_cb->msg) {
			cfw_msg_free(msg_cb->msg);
			msg_cb->msg = NULL;
		}

		msg_cb->msg_cb = NULL;
		bfree(msg_cb);
		msg_cb = NULL;
	}
}

void run_pending_msg_cb(uint8_t status)
{
	if (pending_msg_cb != NULL) {
		run_msg_cb(pending_msg_cb, status);
		pending_msg_cb = NULL;
	}
}

int set_msg_cb(struct nfc_msg_cb *msg_cb)
{
	if (msg_cb) {
		if (pending_msg_cb == NULL) { /* set pending req/callback */
			pending_msg_cb = msg_cb;
			return 0;
		} else {
			run_msg_cb(msg_cb, NFC_STATUS_NOT_ALLOWED);
			return 1;
		}
	} else {
		return -1;
	}
}

int nfc_scn_queue(scn_exec_t scn, int timeout_ms, uint8_t arg_len,
		  uint8_t *args)
{
	OS_ERR_TYPE err;

	if (scn == NULL)
		return -1;

	nfc_scenario_t *new_scn = (nfc_scenario_t *)balloc(
		sizeof(*new_scn) + arg_len, NULL);

	if (!new_scn) {
		pr_error(LOG_MODULE_NFC, "No more mem!");
		return -1;
	}

	new_scn->exec = scn;
	new_scn->timeout = timeout_ms;
	new_scn->arg_len = arg_len;
	if (arg_len > 0) {
		memcpy(new_scn->args, args, arg_len);
	}

	queue_send_message(scn_queue, (void *)new_scn, &err);

	if (err) {
		bfree(new_scn);
		/* TODO: send an EV_NFC_SCN_FAIL ? */
		return -1;
	}

	return 0;
}

int nfc_scn_start(void)
{
	nfc_fsm_event_post(EV_NFC_SCN_START, NULL, NULL);
	return 0;
}

void scn_timer_cb(void *param)
{
	nfc_fsm_event_post(EV_NFC_SCN_TIMEOUT, NULL, NULL);
}

void scn_send_timer(nfc_scenario_t *scn, uint8_t timer_value)
{
	if (scn != NULL) {
		uint8_t tmr_msg[2];
		tmr_msg[0] = TIMER_MESSAGE_TYPE;
		tmr_msg[1] = timer_value;
		scn->exec(false, tmr_msg, sizeof(tmr_msg));
	}
}

void nfc_error_evt(uint8_t status, uint8_t error)
{
	nfc_service_err_evt_t evt;

	memset(&evt, 0, sizeof(evt));

	CFW_MESSAGE_ID(&evt.header) = MSG_ID_NFC_SERVICE_ERROR_EVT;
	CFW_MESSAGE_TYPE(&evt.header) = TYPE_EVT;
	CFW_MESSAGE_LEN(&evt.header) = sizeof(evt);
	CFW_MESSAGE_SRC(&evt.header) = cfw_get_service_port(NFC_SERVICE_ID);
	evt.status = status;
	evt.err_id = error;
	cfw_send_event(&evt.header);
}

fsm_state_t act_unhandled_event(struct fsm_event_message *evt)
{
	pr_info(LOG_MODULE_NFC,
		"unhandled event state: %s, event: %d, event_data: %d",
		state_name[state], evt->event,
		evt->event_data);
	if (evt->msg_cb != NULL) {
		pr_warning(LOG_MODULE_NFC,
			   "unhandled cb function: %x; response WRONG_STATE.",
			   evt->msg_cb);
		run_msg_cb(evt->msg_cb, NFC_STATUS_WRONG_STATE);
	}
	return state;
}

static fsm_state_t act_scn_passed(struct fsm_event_message *evt)
{
	run_pending_msg_cb(NFC_STATUS_SUCCESS);
	return state;
}

static fsm_state_t act_scn_failed(struct fsm_event_message *evt)
{
	run_pending_msg_cb((int)evt->event_data); /* ERROR or TIMEOUT */
	nfc_fsm_event_post(EV_NFC_ERR_START, NULL, NULL);
	return ST_ERROR;
}

static fsm_state_t act_err_cleanup(struct fsm_event_message *evt)
{
	/* in case there's something pending */
	run_pending_msg_cb(NFC_STATUS_ERROR);

	/* queue up error handling and cleanup scenario's */
	/* all scenarios here have to have a timeout */
	nfc_scn_queue(nfc_scn_get_config, 100, 0, NULL);
	nfc_scn_start();

	return state;
}

static fsm_state_t act_err_recovery(struct fsm_event_message *evt)
{
	nfc_stn54_power_down();
	nfc_error_evt(error_status, error_id); /* triggers recovery from app side */
	return ST_OFF;
}

static fsm_state_t act_goto_CONFIG(struct fsm_event_message *evt)
{
	run_pending_msg_cb(NFC_STATUS_SUCCESS);
	return ST_CONFIG;
}

static fsm_state_t act_goto_READY(struct fsm_event_message *evt)
{
	run_pending_msg_cb(NFC_STATUS_SUCCESS);
	return ST_READY;
}

static fsm_state_t act_goto_ACTIVE(struct fsm_event_message *evt)
{
	run_pending_msg_cb(NFC_STATUS_SUCCESS);
	return ST_ACTIVE;
}

static fsm_state_t act_goto_OFF(struct fsm_event_message *evt)
{
	/* power down chip */
	nfc_stn54_power_down();
	run_pending_msg_cb(NFC_STATUS_SUCCESS);
	return ST_OFF;
}

fsm_state_t act_init(struct fsm_event_message *evt)
{
	/* refuse svc command if another one is pending */
	if (set_msg_cb(evt->msg_cb) > 0) {
		pr_warning(LOG_MODULE_NFC, "another command is pending.");
		return state;
	}

	nfc_stn54_power_up();
#ifdef CONFIG_NFC_COMPARATOR_IRQ
	nfc_stn54_config_irq_out();
#endif

#if CONFIG_NFC_STN54_HIBERNATE
	uint8_t cmd = 1; /* Hibernate exit */
	nfc_scn_queue(nfc_scn_set_mode, 1000, sizeof(cmd), &cmd);
#endif

	nfc_scn_queue(nfc_scn_reset_init, 200, 0, NULL);
	nfc_scn_start();
	return ST_INIT;
}

fsm_state_t act_config(struct fsm_event_message *evt)
{
	/* refuse svc command if another one is pending */
	if (set_msg_cb(evt->msg_cb) > 0) {
		pr_warning(LOG_MODULE_NFC, "another command is pending.");
		return state;
	}

	nfc_scn_queue(nfc_scn_get_config, 100, 0, NULL);
	nfc_scn_queue(nfc_scn_enable_se, 500, 0, NULL);
	nfc_scn_start();
	return state;
}

fsm_state_t act_start(struct fsm_event_message *evt)
{
	/* refuse svc command if another one is pending */
	if (set_msg_cb(evt->msg_cb) > 0) {
		pr_warning(LOG_MODULE_NFC, "another command is pending.");
		return state;
	}

	nfc_scn_queue(nfc_scn_start_rf, 50, 0, NULL);
	nfc_scn_start();
	return state;
}

fsm_state_t act_stop(struct fsm_event_message *evt)
{
	/* refuse svc command if another one is pending */
	if (set_msg_cb(evt->msg_cb) > 0) {
		pr_warning(LOG_MODULE_NFC, "another command is pending.");
		return state;
	}

	nfc_scn_queue(nfc_scn_stop_rf, 50, 0, NULL);
	nfc_scn_start();
	return state;
}

fsm_state_t act_disable(struct fsm_event_message *evt)
{
	run_pending_msg_cb(NFC_STATUS_ERROR);
	set_msg_cb(evt->msg_cb);

	/* simulate a scenario done!*/
	nfc_fsm_event_post(EV_NFC_SEQ_PASS, NULL, NULL);
	return ST_DISABLE;
}

static fsm_state_t act_hibernate_enter(struct fsm_event_message *evt)
{
	/* refuse svc command if another one is pending */
	if (set_msg_cb(evt->msg_cb) > 0) {
		pr_warning(LOG_MODULE_NFC, "another command is pending.");
		return state;
	}

	uint8_t cmd = 0; /* Hibernate start */
	nfc_scn_queue(nfc_scn_set_mode, 200, sizeof(cmd), &cmd);
	nfc_scn_start();

	/* send hibernate enter */
	return ST_HIBER;
}

static fsm_state_t act_hibernate_exit(struct fsm_event_message *evt)
{
	run_pending_msg_cb(NFC_STATUS_ERROR);
	set_msg_cb(evt->msg_cb);

	uint8_t cmd = 1; /* Hibernate exit */
	nfc_scn_queue(nfc_scn_set_mode, 2000, sizeof(cmd), &cmd);

#ifdef CONFIG_NFC_COMPARATOR_IRQ
	nfc_stn54_config_irq_out();
#endif
	nfc_scn_queue(nfc_scn_reset_init, 200, 0, NULL);
	nfc_scn_start();

	/* reset chip, send hibernate exit, send init reset */
	return ST_INIT;
}

fsm_state_t act_rf_tuning(struct fsm_event_message *evt)
{
	nfc_tuning_io();
	local_task_sleep_ms(10);
	nfc_tuning_card_a();
	local_task_sleep_ms(10);
	pr_warning(LOG_MODULE_NFC, "RF tuning done. Please reset the board.");
	return state;
}

fsm_state_t act_sequencer(struct fsm_event_message *evt)
{
	static nfc_scenario_t *current_scenario = NULL;

	switch (evt->event) {
	case EV_NFC_SCN_TIMER:
		pr_debug(LOG_MODULE_NFC, "scn: timer (%d)",
			 (int)(evt->event_data));
		scn_send_timer(current_scenario, (int)(evt->event_data));
		break;

	case EV_NFC_SCN_TIMEOUT:
		if (current_scenario == NULL) {
			/* timeout message invalid - scn has already finished,
			 * but timer was not stopped in time. */
			pr_warning(LOG_MODULE_NFC, "Discard timeout event.");
			break;
		}
	case EV_NFC_SCN_FAIL:
		if (state != ST_ERROR) {
			if (evt->event == EV_NFC_SCN_TIMEOUT) {
				pr_warning(LOG_MODULE_NFC, "scn: timeout");
				error_status = NFC_STATUS_TIMEOUT;
				error_id = 0;
			} else if (evt->event == EV_NFC_SCN_FAIL) {
				pr_warning(LOG_MODULE_NFC, "scn: fail");
				error_status = NFC_STATUS_ERROR;
				error_id = (int)evt->event_data;
				timer_stop(scn_timer);
			}

#ifdef CONFIG_NFC_STN54_AMS_DEBUG_TCMD
			/* workaround for manual run scenarios - ex: AMS tcmd+scenario */
			if (state == ST_OFF) {
				pr_info(LOG_MODULE_NFC,
					"Receiver is probably off, can't run.");
				if (current_scenario != NULL) {
					bfree(current_scenario);
					current_scenario = NULL;
				}
				return state;
			}
#endif

			/* cleanup remaining pending scenarios */
			do {
				if (current_scenario != NULL) {
					bfree(current_scenario);
					current_scenario = NULL;
				}
				queue_get_message(scn_queue,
						  (void *)&current_scenario,
						  OS_NO_WAIT,
						  NULL);
			} while (current_scenario != NULL);

			nfc_fsm_event_post(EV_NFC_SEQ_FAIL, (void *)evt->event,
					   NULL);
			break;
		} /* fallthrough if ST_ERROR, because we want to ignore TIMEOUT and FAIL */

	case EV_NFC_SCN_DONE:
	case EV_NFC_SCN_IGNORE:
		if (evt->event == EV_NFC_SCN_DONE) {
			pr_info(LOG_MODULE_NFC, "scn: done (%d)",
				(int)evt->event_data);
		} else if (evt->event == EV_NFC_SCN_FAIL) {
			pr_info(LOG_MODULE_NFC, "scn: ignore(%d)!",
				(int)evt->event_data);
		}
		if (current_scenario != NULL) {
			timer_stop(scn_timer);
			bfree(current_scenario);
			current_scenario = NULL;
		}

		/* send a SCN_START to check if other scenario is pending. */
		nfc_fsm_event_post(EV_NFC_SCN_START, NULL, NULL);
		break; /* NO fallthrough; we want to parse any pending queue messages*/

	case EV_NFC_SCN_START:
		if (current_scenario == NULL) {
			queue_get_message(scn_queue, (void *)&current_scenario,
					  OS_NO_WAIT,
					  NULL);
			if (current_scenario != NULL) {
				//pr_info(LOG_MODULE_NFC, "scn:start (timeout: %dms)!", current_scenario->timeout);
				if (current_scenario->timeout > 0) {
					timer_start(scn_timer,
						    current_scenario->timeout,
						    NULL);
				}
				current_scenario->exec(
					true, current_scenario->args,
					current_scenario->arg_len);
			} else {
				if (state != ST_ERROR) {
					pr_info(LOG_MODULE_NFC, "scn: all done");
					nfc_fsm_event_post(EV_NFC_SEQ_PASS,
							   NULL,
							   NULL);
				} else {
					nfc_fsm_event_post(EV_NFC_ERR_DONE,
							   NULL,
							   NULL);
				}
			}
		}
		break;

	case EV_NFC_RX_IRQ:
		while (nfc_fsm_rx_reader(rx_data, &rx_len) == DRV_RC_OK) {
			nfc_scn_default(false, rx_data, rx_len);
			if (current_scenario != NULL) {
				current_scenario->exec(false, rx_data, rx_len);
			}
		}
		break;

	default:
		/* refuse all incoming service commands while we're in error handling */
		if (state == ST_ERROR && evt->msg_cb) {
			run_msg_cb(evt->msg_cb, NFC_STATUS_ERROR);
		}
		break;
	} /* evt->event */

	return state;
}

#ifdef CONFIG_NFC_STN54_FW_UPDATE

/* definitions for CRC Computation */
typedef uint16_t crc;
#define POLYNOMIAL 0xA010  /* x^16 + x^12 + x^5 + 1, */
#define WIDTH  (8 * sizeof(crc))
#define TOPBIT (1 << (WIDTH - 1))

crc serial_flash_fw_compute_crc(struct td_device *spiflash, uint32_t start,
				uint32_t len,
				uint8_t *temp)
{
	crc shiftRegister = 0;
	unsigned int retlen;
	uint32_t offset = start;
	uint32_t remaining = len;
	uint8_t chunk_len;

	while (remaining > 0) {
		chunk_len = remaining >= 250 ? 250 : remaining; //250 -> less than temp size
		spi_flash_read_byte(spiflash, offset, chunk_len, &retlen,
				    (uint8_t *)temp);

		/*
		 * Perform modulo-2 division, a byte at a time.
		 */
		for (int pos = 0; pos < chunk_len; ++pos) {
			/*
			 * Bring the next byte into the shiftRegister.
			 */
			shiftRegister ^= ((uint8_t)temp[pos] << (WIDTH - 8));

			/*
			 * Perform modulo-2 division, a bit at a time.
			 */
			for (uint8_t bit = 8; bit > 0; --bit) {
				/*
				 * Try to divide the current data bit.
				 */
				if (shiftRegister & TOPBIT) {
					shiftRegister =
						(shiftRegister <<
						 1) ^ POLYNOMIAL;
				} else {
					shiftRegister = (shiftRegister << 1);
				}
			}
		}

		offset += chunk_len;
		remaining -= chunk_len;
	}

	/*
	 * The final shiftRegister is the CRC result.
	 */
	return shiftRegister;
}

fsm_state_t act_fwu_start(struct fsm_event_message *evt)
{
	uint8_t r_buff[253 + 2];
	uint32_t r_offset;
	uint32_t patch_size = 0;
	uint32_t len_to_do = 0;
	struct ota_header otah;
	unsigned int retlen;
	uint8_t i = 0;

	struct td_device *spidev =
		(struct td_device *)&pf_sba_device_flash_spi0;

	pr_info(LOG_MODULE_NFC, "Start fw update!");

	/* start read offset; ex CACHE PARTITION = 0x00 */
	r_offset = (uint32_t)evt->event_data;
	memset(&otah, sizeof(otah), 0);

	if (sizeof(otah) > 255) {
		pr_error(LOG_MODULE_NFC, "OTA header too big. Abort.");
		return state;
	}

	if (spidev == NULL ||
	    spi_flash_read_byte(spidev, r_offset, sizeof(otah), &retlen,
				(uint8_t *)&otah) != DRV_RC_OK) {
		pr_error(LOG_MODULE_NFC, "Can't read from SPI Flash. Abort.");
		return state;
	}

	if (memcmp(otah.magic, "OTA", sizeof(otah.magic)) != 0) {
		pr_error(LOG_MODULE_NFC, "OTA magic failed.");
		return state;
	}

	for (i = 0; i < ARRAY_SIZE(otah.binaries); i++) {
		struct ota_binary_header *p_bhdr = &otah.binaries[i];
		if (p_bhdr->length == 0)
			continue;

		if (!memcmp(p_bhdr->magic, "NFC", sizeof(p_bhdr->magic))) {
			if (p_bhdr->length > 11 && p_bhdr->type == 0x04) {
				r_offset += otah.hdr_length + p_bhdr->offset;
				patch_size = p_bhdr->length - 2; /* binary size - 2 len bytes */
				pr_info(
					LOG_MODULE_NFC,
					"i, r_offset, size, type: %d %02x %02x %02x",
					i,
					r_offset, p_bhdr->length, p_bhdr->type);
			}
			break;
		}
	}

	if (r_offset == 0) {
		pr_error(LOG_MODULE_NFC, "OTA magic failed.");
		return state;
	}

	spi_flash_read_byte(spidev, r_offset, 9, &retlen, (uint8_t *)r_buff);
	len_to_do = (((r_buff[0] & 0x00ff) << 8) | (r_buff[1] & 0x00ff));

	if (len_to_do != patch_size) {
		pr_error(LOG_MODULE_NFC, "Wrong patch size? (%d, %d)",
			 len_to_do,
			 patch_size);
		return state;
	}

	crc computed_crc =
		serial_flash_fw_compute_crc(spidev, r_offset + 2, len_to_do - 2,
					    r_buff);

	/* last two bytes = crc */
	spi_flash_read_byte(spidev, r_offset + 2 + len_to_do - 2, 2, &retlen,
			    (uint8_t *)r_buff);
	crc sample_crc = (((r_buff[0] & 0x00ff) << 8) | (r_buff[1] & 0x00ff));

	if (computed_crc != sample_crc) {
		pr_error(LOG_MODULE_NFC, "Bad CRC (%x, %x)!", computed_crc,
			 sample_crc);
		return state;
	}

	len_to_do -= (2 + 4 + 3); // 2 bytes of CRC + 4 bytes of version number + 3 bytes for the configuration number;
	r_offset += 2 + 4 + 3; // pointer on the beginning of the data to upload (2 bytes of length, 4 bytes of fw version)

	local_task_sleep_ms(10);

	/* start fwu scenario, send the scenario the r_offset and len_to_do */
	memcpy(&r_buff[0], &r_offset, sizeof(r_offset));
	memcpy(&r_buff[0 + sizeof(r_offset)], &len_to_do, sizeof(len_to_do));
	nfc_scn_queue(nfc_scn_fw_update, 0,
		      (sizeof(r_offset) + sizeof(len_to_do)), &r_buff[0]);
	nfc_scn_start();
	return ST_FW_UPDATE;
}

fsm_state_t act_fwu_pass(struct fsm_event_message *evt)
{
	pr_info(LOG_MODULE_NFC,
		"Firmware updated OK. Updating RF tuning registers.");
	local_task_sleep_ms(100);
	nfc_tuning_io();
	local_task_sleep_ms(10);
	nfc_tuning_card_a();
	local_task_sleep_ms(10);
	nfc_stn54_power_down();
	local_task_sleep_ms(50);
	nfc_stn54_power_up();
	pr_warning(LOG_MODULE_NFC, "Update successful! Please reset the board.");
	return ST_OFF;
}

fsm_state_t act_fwu_fail(struct fsm_event_message *evt)
{
	nfc_stn54_power_down();
	pr_error(LOG_MODULE_NFC, "Update failed! Please reset the board.");
	return ST_ERROR;
}

void nfc_fw_upd_tcmd(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	uint32_t val;
	char *pend;

	if (argc < 3)
		goto show_help;

	val = (uint32_t)strtoul(argv[2], &pend, 16);
	nfc_fsm_event_post(EV_NFC_FW_UPDATE_START, (void *)val, NULL);
	return;

show_help:
	TCMD_RSP_ERROR(
		ctx,
		"nfc fw_update <partition offset; ex: 0x00 for CACHE partition>");
}
DECLARE_TEST_COMMAND(nfc, fw_update, nfc_fw_upd_tcmd);
#endif
