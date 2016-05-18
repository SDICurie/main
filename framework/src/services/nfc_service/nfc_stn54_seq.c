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
#include "infra/log.h"
#ifdef CONFIG_SYSTEM_EVENTS
#include "infra/system_events.h"
#include "infra/time.h"
#endif
#include "drivers/nfc_stn54.h"
#include "services/nfc_service/nfc_service.h"
#include "nfc_service_private.h"

#include "nfc_stn54_sm.h"
#include "nfc_stn54_seq.h"

#ifdef CONFIG_NFC_STN54_FW_UPDATE
#include <string.h>
#include <drivers/spi_flash.h>
static T_TIMER fwu_timer;
static ndlc_msg_t tx_msg;
#endif

static const uint8_t fdt_reg_tuned[] = { 0x84, 0x01, 0x00, 0x24, 0x82, 0x11,
					 0xFF, 0x80, 0x00, 0xE8, 0x11, 0x02,
					 0x05, 0x18, 0x78, 0x22, 0x58, 0x38,
					 0x00,
					 0x18, 0x18, 0x40, 0x01, 0x02, 0x01,
					 0x02, 0x00, 0x10, 0xF0, 0x00, 0x02,
					 0x06,
					 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					 0x00, 0x00 };

static const uint8_t card_a_tuned[] = { 0x84, 0x01, 0x00, 0xa4, 0x82, 0x11,
					0xFF, 0x82, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x0A, 0x00, 0x00, 0x10, 0x00,
					0xF7, 0x02, 0xCF, 0x04, 0x00, 0x00,
					0x34, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x0A, 0x00, 0x00, 0x10, 0x00,
					0x27, 0x02, 0xE8, 0x04, 0x00, 0x00,
					0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x0A, 0x00, 0x00, 0x10, 0x00,
					0xE7, 0x02, 0xF8, 0x04, 0x00, 0x00,
					0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x0A, 0x00, 0x00, 0x10, 0x00,
					0xE7, 0x02, 0xF8, 0x04, 0x00, 0x00,
					0x00, 0x00,
					0x00, 0x00, 0x2E, 0x00, 0x10, 0x01,
					0x70, 0x02, 0xD4, 0x03, 0x10, 0x04,
					0x48, 0x05, 0x00, 0x06, 0x0A, 0x07,
					0x13, 0x08,
					0x00, 0x09, 0xB8, 0x0A, 0x71, 0x0B,
					0x69, 0x0C, 0x63, 0x0D, 0x99, 0x0E,
					0x30, 0x0F, 0xB0, 0x10, 0x30, 0x11,
					0x90, 0x12,
					0x68, 0x13, 0x00, 0x18, 0x3C, 0x19,
					0xE3, 0x1A, 0xB1, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00,
					0x00, 0x00 };

static T_TIMER hotplug_timer;
static T_TIMER nfcee_ntf_timer;
static bool timers_created;

static uint16_t mHciState;
static uint8_t mAdpuPipeId;
static bool mHotPlugRx;

static void hotplug_timer_cb(void *param)
{
	nfc_fsm_event_post(EV_NFC_SCN_TIMER, (void *)1, NULL);
	return;
}

static void nfcee_timer_cb(void *param)
{
	nfc_fsm_event_post(EV_NFC_SCN_TIMER, (void *)2, NULL);
	return;
}

#ifdef CONFIG_NFC_STN54_FW_UPDATE
static void fwu_timer_cb(void *param)
{
	nfc_fsm_event_post(EV_NFC_SCN_TIMER, (void *)3, NULL);
	return;
}

int nfc_send_ndlc(uint8_t *buffer, int length)
{
	ndlc_msg_t *msg = &tx_msg;

	msg->pcb = PCB_TYPE_DATAFRAME |	\
		   PCB_DATAFRAME_RETRANSMIT_NO | \
		   PCB_FRAME_CRC_INFO_NOTPRESENT;
	memcpy(msg->buffer, buffer, length);
	return nfc_stn54_write((uint8_t *)msg, length + 1);
}
#endif

static void print_buff_by4(int log_level, uint8_t *data, int len)
{
#ifdef CONFIG_NFC_DEBUG_RX_MESSAGE
	char out[5 * 4 + 1]; /* output 4 bytes on one line */
	int i, j;
	for (i = 0; i < (len / 4); i++) {
		for (j = 0; j < 4; j++) {
			snprintf(&out[j * 5], 6, " 0x%02x", data[i * 4 + j]);
		}
		out[j * 5] = 0;
		log_printk(log_level, LOG_MODULE_NFC, "%s[%3d..%3d]:%s",
			   (i == 0 ? "h" : "p"), i * 4, i * 4 + j, out);
	}
	if (len % 4 > 0) {
		for (j = 0; j < len % 4; j++) {
			snprintf(&out[j * 5], 6, " 0x%02x", data[i * 4 + j]);
		}
		out[j * 5] = 0;
		log_printk(log_level, LOG_MODULE_NFC, "%s[%3d..%3d]:%s",
			   (i == 0 ? "h" : "p"), i * 4, i * 4 + j, out);
	}
#endif
}

void nfc_tuning_io(void)
{
	pr_info(LOG_MODULE_NFC, "tuning: set FDT = 0x%x", fdt_reg_tuned[31]);
	nfc_stn54_write(fdt_reg_tuned, sizeof(fdt_reg_tuned));
}

void nfc_tuning_card_a(void)
{
	pr_info(LOG_MODULE_NFC, "tuning: set CardA");
	nfc_stn54_write(card_a_tuned, sizeof(card_a_tuned));
}

/*
 *  Default scenario that is run whenever an RX event comes. It decodes incoming
 *  messages and prints their type, arguments, etc.
 *  It's not run as a standard scenario (but it could be).
 */
void nfc_scn_default(bool start, uint8_t *rx_data, uint8_t len)
{
	ndlc_msg_t *rx_msg = (ndlc_msg_t *)rx_data;
	uint8_t *p_buffer = rx_msg->buffer;

#ifdef CONFIG_SYSTEM_EVENTS
	static uint32_t last_event_time;
#endif

	if (start) {
		/* not used for now */
	}

	if (rx_msg->pcb == 0x84) {
		int nci_type = p_buffer[NCI_TYPE_OFFSET] & NCI_FRAME_TYPE_MASK;
		int nci_oid = p_buffer[NCI_OID_OFFSET];

		print_buff_by4(LOG_LEVEL_DEBUG, rx_data, len);

		switch (nci_type) {
		case NCI_CORE_RSP:
			switch (nci_oid) {
			case NCI_CORE_RESET:
				pr_debug(LOG_MODULE_NFC, "RESET_RSP(%d)",
					 p_buffer[3]);
				break;

			case NCI_CORE_INIT:
				pr_debug(LOG_MODULE_NFC, "INIT_RSP(%d)",
					 p_buffer[3]);
				break;

			case NCI_CORE_CONN_CREATE:
				pr_debug(LOG_MODULE_NFC,
					 "CONN_CREATE_RSP(%d,id:%d)",
					 p_buffer[3],
					 p_buffer[6]);
				break;
			} /* end nci_oid */
			break;

		case NCI_CORE_NTF:
			pr_debug(LOG_MODULE_NFC, "NCI_CORE_NTF");
			break;

		case NCI_RF_RSP:
			switch (nci_oid) {
			case NCI_RF_DISCOVER_MAP:
				pr_debug(LOG_MODULE_NFC, "RF_DISCOVER_MAP(%d)",
					 p_buffer[3]);
				break;

			case NCI_RF_DISCOVER:
				pr_info(LOG_MODULE_NFC, "RF_DISCOVER(%d)",
					p_buffer[3]);
				break;

			case NFC_RF_DEACTIVATE:
				pr_info(LOG_MODULE_NFC, "RF_DEACTIVATE(%d)",
					p_buffer[3]);
				break;

			default:
				pr_debug(LOG_MODULE_NFC, "NCI_RF_RSP(%d)",
					 p_buffer[3]);
				break;
			}
			break;

		case NCI_RF_NTF:
			switch (nci_oid) {
			case NCI_RF_FIELD_INFO:
				pr_info(LOG_MODULE_NFC, "RF_FIELD_NTF %s",
					((p_buffer[3] == 0) ? "0" : "1"));
				break;

			case NCI_RF_INTF_ACTIVATED:
#ifdef CONFIG_SYSTEM_EVENTS
				if ((get_uptime_ms() - last_event_time) >
				    1000) {
					system_event_push_nfc_reader_detected(
						true);
					last_event_time = get_uptime_ms();
				}
#endif
				pr_info(
					LOG_MODULE_NFC,
					"RF_INTF_ACTIVATED_NTF id:0x%02x, intf:0x%x",
					p_buffer[3], p_buffer[4]);
				break;

			case NCI_RF_NFCEE_ACTION:
				pr_info(
					LOG_MODULE_NFC,
					"RF_NFCEE_ACTION_NTF id:0x%x action:0x%x",
					p_buffer[3], p_buffer[4]);
				break;

			case NFC_RF_DEACTIVATE:
#ifdef CONFIG_SYSTEM_EVENTS
				if ((get_uptime_ms() - last_event_time) >
				    1000) {
					system_event_push_nfc_reader_detected(
						false);
					last_event_time = get_uptime_ms();
				}
#endif
				pr_info(LOG_MODULE_NFC,
					"RF_DEACTIVATE_NTF (0x%02x)",
					p_buffer[3]);
				break;

			default:
				pr_debug(LOG_MODULE_NFC, "Unknown NCI_RF_NTF");
				break;
			}
			break;

		case NCI_NFCEE_RSP:
			switch (nci_oid) {
			case NCI_NFCEE_DISCOVER:
				pr_debug(LOG_MODULE_NFC,
					 "NFCEE_DISCOVER_RSP(%d)",
					 p_buffer[3]);
				break;

			case NCI_NFCEE_MODE_SET:
				pr_debug(LOG_MODULE_NFC,
					 "NFCEE_MODE_SET_RSP(%d)",
					 p_buffer[3]);
				break;

			default:
				pr_debug(LOG_MODULE_NFC, "NCI_NFCEE_RSP(%d)",
					 p_buffer[3]);
				break;
			}
			break;

		case NCI_NFCEE_NTF:
			switch (nci_oid) {
			case NCI_NFCEE_DISCOVER:
				pr_debug(LOG_MODULE_NFC,
					 "NCI_NFCEE_DISCOVER_NTF(id:%02x)",
					 p_buffer[3]);
				break;

			default:
				pr_debug(LOG_MODULE_NFC, "NCI_NFCEE_NTF");
				break;
			}
			break;

		case NCI_DATA_HCI: {
#ifdef CONFIG_NFC_DEBUG_RX_MESSAGE
			uint8_t pipe_id = p_buffer[HCI_1RST_BYTE_OFFSET] & 0x7f;
			uint8_t hci_type = p_buffer[HCI_2ND_BYTE_OFFSET] & 0xc0;
			uint8_t hci_inst = p_buffer[HCI_2ND_BYTE_OFFSET] & 0x3f;
			pr_info(LOG_MODULE_NFC,
				"HCI[%02x,%02x] p:%02x t:%02x i:%02x",
				p_buffer[HCI_1RST_BYTE_OFFSET],
				p_buffer[HCI_2ND_BYTE_OFFSET], pipe_id,
				hci_type,
				hci_inst);
#endif
		}
		break;

		default:
			pr_debug(LOG_MODULE_NFC, "Unknown NCI type!");
			break;
		} /* end nci_type */
	}
}

void nfc_scn_reset_init(bool start, uint8_t *rx_data, uint8_t len)
{
	static const uint8_t nci_core_reset[] = { 0x84, 0x20, 0x00, 0x01, 0x01 };
	static const uint8_t nci_core_init[] = { 0x84, 0x20, 0x01, 0x00 };
	static const uint8_t nci_set_clf_config[] =
	{ 0x84, 0x01, 0x00, 0x0a, 0x82, 0x11, 0xd0, 0x00, 0x00, 0x00, 0x00,
	  0x00,
	  0x00, 0x7e };

	ndlc_msg_t *rx_msg = (ndlc_msg_t *)rx_data;
	uint8_t *p_buffer = rx_msg->buffer;

	if (start) {
		pr_info(LOG_MODULE_NFC, "scn: reset init");
		nfc_stn54_write(nci_core_reset, sizeof(nci_core_reset));
		return;
	}

	if (rx_msg->pcb == 0x84) {
		int nci_type = p_buffer[NCI_TYPE_OFFSET] & NCI_FRAME_TYPE_MASK;
		int nci_oid = p_buffer[NCI_OID_OFFSET];

		switch (nci_type) {
		case NCI_CORE_RSP:
			switch (nci_oid) {
			case NCI_CORE_RESET:
				nfc_stn54_write(nci_core_init,
						sizeof(nci_core_init));
				break;

			case NCI_CORE_INIT:
				local_task_sleep_ms(50);
				pr_info(LOG_MODULE_NFC,
					"stn fw ver(hex): %x.%x", p_buffer[22],
					p_buffer[23]);

				if ((p_buffer[22] >
				     CONFIG_NFC_FW_MAJOR_VERSION) ||
				    ((p_buffer[22] ==
				      CONFIG_NFC_FW_MAJOR_VERSION) &&
				     (p_buffer[23] >=
				      CONFIG_NFC_FW_MINOR_VERSION))
				    ) {
					/* we're good, carry on. if a firmware update is done, the config will be updated then */
					nfc_fsm_event_post(EV_NFC_SCN_DONE,
							   (void *)1, NULL);
					return;
				} else {
					pr_info(
						LOG_MODULE_NFC,
						"NFC requires firmware update. Set default config until then.");
					nfc_stn54_write(
						nci_set_clf_config,
						sizeof(nci_set_clf_config));
				}
				break;
			} /* end nci_oid */
			break;

		case NCI_DATA_HCI:
			if ((p_buffer[HCI_1RST_BYTE_OFFSET] == HCI_DM_GATE_DATA)
			    && (p_buffer[HCI_2ND_BYTE_OFFSET] ==
				(HCI_RSP_TYPE | HCI_ANY_OK))) {
				nfc_fsm_event_post(EV_NFC_SCN_DONE, (void *)2,
						   NULL);
				return;
			}
			/* workaround for some boards */
			if ((p_buffer[HCI_1RST_BYTE_OFFSET] ==
			     HCI_ADMIN_GATE_DATA)
			    && (p_buffer[HCI_2ND_BYTE_OFFSET] ==
				(HCI_EVT_TYPE | HCI_OPEN_PIPE))) {
				local_task_sleep_ms(50);
				nfc_fsm_event_post(EV_NFC_SCN_DONE, (void *)3,
						   NULL);
				return;
			}
			break;
		} /* end nci_type */
	}
}

void nfc_scn_enable_se(bool start, uint8_t *rx_data, uint8_t len)
{
	ndlc_msg_t *rx_msg = (ndlc_msg_t *)rx_data;
	uint8_t *p_buffer = rx_msg->buffer;

	static bool ese_detected, apduPipeCreated;
	static uint8_t hciType, hciMsgId, inhibited, eseEvent, createdPipeId;
	static int status, whitelistEventsMap, notifyPipeCreatedEventMap,
		   nextEvtBit;

	static const uint8_t nci_nfcee_discover_enable[] =
	{ 0x84, 0x22, 0x00, 0x01, 0x01 };
	static const uint8_t nci_nfcee_discover_disable[] =
	{ 0x84, 0x22, 0x00, 0x01, 0x00 };
	static const uint8_t nci_core_conn_create[] =
	{ 0x84, 0x20, 0x04, 0x06, 0x03, 0x01, 0x01, 0x02, 0x80, 0x01 };
	static const uint8_t nci_nfcee_mode_set_hci[] =
	{ 0x84, 0x22, 0x01, 0x02, 0x80, 0x01 };
	static const uint8_t nci_nfcee_mode_set[] =
	{ 0x84, 0x22, 0x01, 0x02, 0xc0, 0x01 };
	//HCI command over NCI data
	static const uint8_t hci_open_admin[] =
	{ 0x84, 0x01, 0x00, 0x02, 0x81, 0x03 };
	static const uint8_t hci_set_session_id[] =
	{ 0x84, 0x01, 0x00, 0x0b, 0x81, 0x01, 0x01, 0xbb, 0xb9, 0x61, 0x02,
	  0xb9, 0xb9,
	  0x61, 0x02 };
	static const uint8_t hci_set_whitelist[] =
	{ 0x84, 0x01, 0x00, 0x05, 0x81, 0x01, 0x03, 0x02, 0xc0 };
	static const uint8_t hci_admin_ok[] =
	{ 0x84, 0x01, 0x00, 0x02, 0x81, 0x80 };
	static uint8_t hci_any_ok[] =
	{ 0x84, 0x01, 0x00, 0x02, 0x80, 0x80 };

	if (start) {
		pr_info(LOG_MODULE_NFC, "scn: enable eSE");
		if (!timers_created) {
			hotplug_timer =
				timer_create(hotplug_timer_cb, "", 250, 0, 0,
					     NULL);
			nfcee_ntf_timer =
				timer_create(nfcee_timer_cb, "", 50, 0, 0,
					     NULL);
			timers_created = true;
		}

		/* globals */
		mHciState = eOPEN_ADMIN;
		apduPipeCreated = 0;
		mHotPlugRx = 0;
		mAdpuPipeId = 0xff;

		/* locals */
		status = 0;
		ese_detected = 0;
		whitelistEventsMap = 0;

		hciType = 0;
		hciMsgId = 0;
		inhibited = 0;
		eseEvent = 0;
		apduPipeCreated = 0;
		notifyPipeCreatedEventMap = 0;
		nextEvtBit = 0;
		createdPipeId = 0;

		status =
			nfc_stn54_write(nci_nfcee_discover_enable,
					sizeof(nci_nfcee_discover_enable));
		return;
	}

	if (rx_msg->pcb == 0x84) {
		int nci_type = p_buffer[NCI_TYPE_OFFSET] & NCI_FRAME_TYPE_MASK;
		int nci_oid = p_buffer[NCI_OID_OFFSET];

		switch (nci_type) {
		case NCI_CORE_RSP: //CORE mgmt - rsp

			switch (nci_oid) {
			case NCI_CORE_RESET: //reset
				break;

			case NCI_CORE_INIT: //init
				break;

			case NCI_CORE_CONN_CREATE: //conn_create
				pr_debug(
					LOG_MODULE_NFC,
					"NCI_CORE_CONN_CREATE_RSP: open HCI admin gate");
				status =
					nfc_stn54_write(hci_open_admin,
							sizeof(hci_open_admin));
				mHciState = eSET_WHITELIST;
				break;
			}
			break;

		case NCI_CORE_NTF:
			switch (nci_oid) {
			case NCI_CORE_CONN_CREDITS: //conn_credits
				if (mHciState == eAPDU_PIPE_OK) {
					pr_debug(LOG_MODULE_NFC,
						 "hotplug tmr stop");
					timer_stop(hotplug_timer);
					nfc_fsm_event_post(EV_NFC_SCN_DONE,
							   (void *)4, NULL);
					return;
				}
				break;
			}
			break;

		case NCI_NFCEE_RSP: //NFCEE mgmt - rsp
			switch (nci_oid) {
			case NCI_NFCEE_DISCOVER:
				pr_debug(LOG_MODULE_NFC,
					 "NCI_NFCEE_DISCOVER_RSP");
				if (ese_detected == 1) {
					pr_debug(
						LOG_MODULE_NFC,
						"NCI_NFCEE_MODE_SET(wait for hot plug)");
					status = nfc_stn54_write(
						nci_nfcee_mode_set,
						sizeof(
							nci_nfcee_mode_set));
					//Start timer to wait for hot plug
					pr_debug(LOG_MODULE_NFC,
						 "hotplug tmr start");
					timer_start(hotplug_timer, 250, NULL);
				}
				break;

			case NCI_NFCEE_MODE_SET:
				pr_debug(LOG_MODULE_NFC,
					 "NCI_NFCEE_MODE_SET_RSP");
				if (mHciState == eOPEN_ADMIN) {
					status = nfc_stn54_write(
						nci_core_conn_create,
						sizeof(
							nci_core_conn_create));
				}
				break;
			}
			break;

		case NCI_NFCEE_NTF:
			pr_debug(LOG_MODULE_NFC, "NCI_NFCEE_NTF (0x%02X)",
				 p_buffer[3]);
			if (p_buffer[3] == 0x80) { //HCI network
				status = nfc_stn54_write(nci_nfcee_mode_set_hci,
							 sizeof(
								 nci_nfcee_mode_set_hci));
			} else if (p_buffer[3] == 0x02) { //UICC
				whitelistEventsMap |= 0x2;
			} else if (p_buffer[3] == 0xc0) { //eSe
				pr_debug(LOG_MODULE_NFC, "detected eSE");
				whitelistEventsMap |= 0x4;
				ese_detected = 1;

				//Stop timer to wait for NFCEE notifications
				timer_stop(nfcee_ntf_timer);
			}
			break;

		case NCI_DATA_HCI: //NCI data - HCI
			if (mHciState == eSET_SESSION_ID) {
				pr_debug(LOG_MODULE_NFC,
					 "mHciState=eSET_SESSION_ID");
				if ((p_buffer[HCI_1RST_BYTE_OFFSET] ==
				     HCI_ADMIN_GATE_DATA)
				    && (p_buffer[HCI_2ND_BYTE_OFFSET]
					== (HCI_RSP_TYPE | HCI_ANY_OK))) {
					pr_debug(LOG_MODULE_NFC,
						 "send hci session_id");
					status = nfc_stn54_write(
						hci_set_session_id,
						sizeof(
							hci_set_session_id));
					mHciState = eSET_WHITELIST;
				}
			} else if (mHciState == eSET_WHITELIST) {
				pr_debug(LOG_MODULE_NFC,
					 "mHciState=eSET_WHITELIST");
				if ((p_buffer[HCI_1RST_BYTE_OFFSET] ==
				     HCI_ADMIN_GATE_DATA)
				    && (p_buffer[HCI_2ND_BYTE_OFFSET]
					== (HCI_RSP_TYPE | HCI_ANY_OK))) {
					pr_debug(LOG_MODULE_NFC,
						 " send hci whitelist");
					status = nfc_stn54_write(
						hci_set_whitelist,
						sizeof(
							hci_set_whitelist));
					mHciState = eHCI_INIT_DONE;

					//Start timer to wait for NFCEE notifications
					timer_start(nfcee_ntf_timer, 50, NULL);
				}
			} else if (mHciState >= eHCI_INIT_DONE) {
				pr_debug(LOG_MODULE_NFC,
					 "mHciState >= eHCI_INIT_DONE");
				if (p_buffer[HCI_1RST_BYTE_OFFSET] ==
				    HCI_ADMIN_GATE_DATA) {               //admin pipe
					hciType =
						p_buffer[HCI_2ND_BYTE_OFFSET] &
						0xc0;
					hciMsgId =
						p_buffer[HCI_2ND_BYTE_OFFSET] &
						0x3f;
					pr_debug(
						LOG_MODULE_NFC,
						"ADMIN gate data (hciType=0x%02X, hciMsgId=0x%02X)",
						hciType, hciMsgId);

					if (hciType == HCI_CMD_TYPE) { //cmd
						if (mHciState ==
						    eWAIT_PIPE_CREATION_NTF) { //Only first run after FW update
							//notify_pipe_created rx
							if (p_buffer[
								    HCI_2ND_BYTE_OFFSET
							    ] ==
							    HCI_NOTIFY_PIPE_CREATED)
							{
								pr_debug(
									LOG_MODULE_NFC,
									"pipe created:%02x %02x %02x %02x %02x ",
									p_buffer
									[5], /* source Hid */
									p_buffer
									[6], /* source Gid */
									p_buffer
									[7], /* destination Hid */
									p_buffer
									[8], /* destination Gid */
									p_buffer
									[9]); /* pipe id */
							}

							if ((p_buffer[
								     HCI_2ND_BYTE_OFFSET
							     ]
							     ==
							     HCI_NOTIFY_PIPE_CREATED)
							    && (p_buffer[6] ==
								0x41)) {
								notifyPipeCreatedEventMap
									|= 0x01;
								nextEvtBit =
									0x2;
								createdPipeId =
									p_buffer
									[9];
								status =
									nfc_stn54_write(
										hci_admin_ok,
										sizeof(
											hci_admin_ok));
								mHciState =
									eWAIT_PIPE_OPEN_NTF;
							} else if ((p_buffer[
									    HCI_2ND_BYTE_OFFSET
								    ]
								    ==
								    HCI_NOTIFY_PIPE_CREATED)
								   && (p_buffer
								       [6] ==
								       APDU_GATE_ID))
							{
								notifyPipeCreatedEventMap
									|= 0x04;
								nextEvtBit =
									0x8;
								mAdpuPipeId =
									p_buffer
									[9];
								createdPipeId =
									p_buffer
									[9];
								apduPipeCreated
									= 1;
								status =
									nfc_stn54_write(
										hci_admin_ok,
										sizeof(
											hci_admin_ok));
								mHciState =
									eWAIT_PIPE_OPEN_NTF;
							} else if (p_buffer[
									   HCI_2ND_BYTE_OFFSET
								   ] ==
								   0x15) { //NOTIFY_ALL_PIPE_CLEARED
								status =
									nfc_stn54_write(
										hci_admin_ok,
										sizeof(
											hci_admin_ok));
							}
						} else if (p_buffer[
								   HCI_2ND_BYTE_OFFSET
							   ] == 0x15) { //NOTIFY_ALL_PIPE_CLEARED
							status =
								nfc_stn54_write(
									hci_admin_ok,
									sizeof(
										hci_admin_ok));
							mHciState =
								eWAIT_PIPE_CREATION_NTF;
						}
					} else if (hciType == HCI_EVT_TYPE) { //event
						switch (hciMsgId) {
						case 0x03: //hot plug
							   //check add param
							inhibited =
								p_buffer[5] &
								0x80;
							eseEvent =
								p_buffer[5] &
								0x1;

							mHotPlugRx = 1;

							//If eSe event and not inhibited, more msg to come
							if ((eseEvent == 1) &&
							    (inhibited ==
							     0x80)) {
								pr_debug(
									LOG_MODULE_NFC,
									"eWAIT_PIPE_CR_NTF");
								mHciState =
									eWAIT_PIPE_CREATION_NTF;
							}
							//subsequent activation, need to check apdu pipe status
							break;
						}
					} else if (hciType == HCI_RSP_TYPE) { //rsp
						if (p_buffer[
							    HCI_2ND_BYTE_OFFSET
						    ]
						    != (HCI_RSP_TYPE |
							HCI_ANY_OK)) {
							pr_debug(
								LOG_MODULE_NFC,
								"unexpected HCI_RSP_TYPE");
							nfc_fsm_event_post(
								EV_NFC_SCN_FAIL,
								(void *)2, NULL);
							return;
						} else if (mHciState ==
							   eHCI_INIT_DONE) {
							whitelistEventsMap |=
								0x1;
						}
					}
				} else if (mHciState == eWAIT_PIPE_OPEN_NTF) { //hot plug - inhibited
					//open pipe on apdu pipe
					if ((p_buffer[HCI_1RST_BYTE_OFFSET]
					     == (HCI_NO_FRAG_MASK |
						 createdPipeId))
					    && (p_buffer[HCI_2ND_BYTE_OFFSET]
						== HCI_OPEN_PIPE)) {
						notifyPipeCreatedEventMap |=
							nextEvtBit;
						hci_any_ok[4] =
							(createdPipeId |
							 HCI_RSP_TYPE); /* Attention at the index! +1 because we have the NDLC added in all hardcoded commands */
						status = nfc_stn54_write(
							hci_any_ok,
							sizeof(hci_any_ok));

						if ((apduPipeCreated == 1)
						    && (
							    notifyPipeCreatedEventMap
							    ==
							    0xf)) {
							mHciState =
								eAPDU_PIPE_OK;
						} else {
							mHciState =
								eWAIT_PIPE_CREATION_NTF;
						}
					}
				}
			}
			break;
		}              //switch

		if (status != DRV_RC_OK) {
			nfc_fsm_event_post(EV_NFC_SCN_FAIL, (void *)3, NULL);
			return;
		}

		if (whitelistEventsMap == 0x7) {
			status = nfc_stn54_write(nci_nfcee_discover_disable,
						 sizeof(
							 nci_nfcee_discover_disable));
			whitelistEventsMap = 0;
		}
	}                    //if
	else if (rx_msg->pcb == TIMER_MESSAGE_TYPE) {
		if (p_buffer[0] == 0x01) {
			//pr_debug(LOG_MODULE_NFC,"Hotplug timer elapsed");
			//Hot plug timer has elapsed
			//Was the HOT PLUG received?
			if (mHotPlugRx == 1) {
				if ((inhibited != 0x80) &&
				    (mHciState == eHCI_INIT_DONE)) {
					timer_stop(nfcee_ntf_timer);
					nfc_fsm_event_post(EV_NFC_SCN_DONE,
							   (void *)5, NULL);
					return;
				}
			}
			//not received, no ese, exit
			else {
				//pr_error(LOG_MODULE_NFC,"did not detect eSE");
				nfc_fsm_event_post(EV_NFC_SCN_FAIL, (void *)5,
						   NULL);
				return;
			}
		} else if (p_buffer[0] == 0x02) {
			//no NFCEE NTF wre received, exit
			nfc_fsm_event_post(EV_NFC_SCN_FAIL, (void *)6, NULL);
			return;
		}
	}
}

void nfc_scn_start_rf(bool start, uint8_t *rx_data, uint8_t len)
{
	static const uint8_t nci_rf_discover_map[] =
	{ 0x84, 0x21, 0x00, 0x07, 0x02, 0x04, 0x03, 0x02, 0x05, 0x03, 0x03 };
	static const uint8_t nci_rf_discover[] =
	{ 0x84, 0x21, 0x03, 0x01, 0x00 };

	ndlc_msg_t *rx_msg = (ndlc_msg_t *)rx_data;
	uint8_t *p_buffer = rx_msg->buffer;

	if (start) {
		pr_info(LOG_MODULE_NFC, "scn: start RF");
		nfc_stn54_write(nci_rf_discover_map, sizeof(nci_rf_discover_map));
		return;
	}

	if (rx_msg->pcb == 0x84) {
		int nci_type = p_buffer[NCI_TYPE_OFFSET] & NCI_FRAME_TYPE_MASK;
		int nci_oid = p_buffer[NCI_OID_OFFSET];

		switch (nci_type) {
		case NCI_RF_RSP:
			switch (nci_oid) {
			case NCI_RF_DISCOVER_MAP:
				nfc_stn54_write(nci_rf_discover,
						sizeof(nci_rf_discover));
				break;

			case NCI_RF_DISCOVER:
				if (p_buffer[3] == 0x00) {
					nfc_fsm_event_post(EV_NFC_SCN_DONE,
							   (void *)6, NULL);
				} else {
					nfc_fsm_event_post(EV_NFC_SCN_FAIL,
							   (void *)2, NULL);
				}
				return;
				break;
			}
			break;
		} /* end nci_type */
	}
}

void nfc_scn_stop_rf(bool start, uint8_t *rx_data, uint8_t len)
{
	static const uint8_t nci_rf_deactivate[] =
	{ 0x84, 0x21, 0x06, 0x01, 0x00 };

	ndlc_msg_t *rx_msg = (ndlc_msg_t *)rx_data;
	uint8_t *p_buffer = rx_msg->buffer;

	if (start) {
		pr_info(LOG_MODULE_NFC, "scn: stop RF");
		nfc_stn54_write(nci_rf_deactivate, sizeof(nci_rf_deactivate));
		return;
	}

	if (rx_msg->pcb == 0x84) {
		int nci_type = p_buffer[NCI_TYPE_OFFSET] & NCI_FRAME_TYPE_MASK;
		int nci_oid = p_buffer[NCI_OID_OFFSET];

		switch (nci_type) {
		case NCI_RF_RSP:
			switch (nci_oid) {
			case NFC_RF_DEACTIVATE:
				nfc_fsm_event_post(EV_NFC_SCN_DONE, (void *)7,
						   NULL);
				return;
				break;
			}
			break;
		} /* end nci_type */
	}
}

void nfc_scn_enable_rf_field_ntf(bool start, uint8_t *rx_data, uint8_t len)
{
	static const uint8_t nci_set_param_rf_field[] =
	{ 0x84, 0x20, 0x02, 0x04, 0x01, 0x80, 0x01, 0x01 };

	ndlc_msg_t *rx_msg = (ndlc_msg_t *)rx_data;
	uint8_t *p_buffer = rx_msg->buffer;

	if (start) {
		pr_info(LOG_MODULE_NFC, "scn: set RF_FIELD_INFO active");
		nfc_stn54_write(nci_set_param_rf_field,
				sizeof(nci_set_param_rf_field));
		return;
	}

	if (rx_msg->pcb == 0x84) {
		int nci_type = p_buffer[NCI_TYPE_OFFSET] & NCI_FRAME_TYPE_MASK;
		int nci_oid = p_buffer[NCI_OID_OFFSET];

		switch (nci_type) {
		case NCI_CORE_RSP:
			switch (nci_oid) {
			case NCI_CORE_SET_CONFIG:
				nfc_fsm_event_post(EV_NFC_SCN_DONE, (void *)8,
						   NULL);
				return;
			}
			break;
			break;
		} /* end nci_type */
	}
}

void nfc_scn_get_config(bool start, uint8_t *rx_data, uint8_t len)
{
	static const uint8_t nci_get_clf_config[] =
	{ 0x84, 0x01, 0x00, 0x03, 0x82, 0x10, 0xd0 };

	ndlc_msg_t *rx_msg = (ndlc_msg_t *)rx_data;
	uint8_t *p_buffer = rx_msg->buffer;

	if (start) {
		pr_info(LOG_MODULE_NFC, "scn: get config");
		nfc_stn54_write(nci_get_clf_config, sizeof(nci_get_clf_config));
		return;
	}

	if (rx_msg->pcb == 0x84) {
		int nci_type = p_buffer[NCI_TYPE_OFFSET] & NCI_FRAME_TYPE_MASK;

		switch (nci_type) {
		case NCI_DATA_HCI:
			if ((p_buffer[HCI_1RST_BYTE_OFFSET] == HCI_DM_GATE_DATA)
			    && (p_buffer[HCI_2ND_BYTE_OFFSET] ==
				(HCI_RSP_TYPE | HCI_ANY_OK))) {
				pr_info(
					LOG_MODULE_NFC,
					"cfg: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x",
					rx_data[6],
					rx_data[7],
					rx_data[8],
					rx_data[9],
					rx_data[10],
					rx_data[11],
					rx_data[12]);

				nfc_fsm_event_post(EV_NFC_SCN_DONE, (void *)9,
						   NULL);
				return;
			}
			break;
		} /* end nci_type */
	}
}

void nfc_scn_set_config(bool start, uint8_t *rx_data, uint8_t len)
{
	static const uint8_t nci_set_clf_config[] =
	{ 0x84, 0x01, 0x00, 0x0a, 0x82, 0x11, 0xd0, 0x00, 0x00, 0x00, 0x00,
	  0x00,
	  0x00, 0x7e };

	ndlc_msg_t *rx_msg = (ndlc_msg_t *)rx_data;
	uint8_t *p_buffer = rx_msg->buffer;

	if (start) {
		pr_info(LOG_MODULE_NFC, "scn: set config");
		nfc_stn54_write(nci_set_clf_config, sizeof(nci_set_clf_config));
		return;
	}

	if (rx_msg->pcb == 0x84) {
		int nci_type = p_buffer[NCI_TYPE_OFFSET] & NCI_FRAME_TYPE_MASK;

		switch (nci_type) {
		case NCI_DATA_HCI:
			if ((p_buffer[HCI_1RST_BYTE_OFFSET] == HCI_DM_GATE_DATA)
			    && (p_buffer[HCI_2ND_BYTE_OFFSET] ==
				(HCI_RSP_TYPE | HCI_ANY_OK))) {
				nfc_fsm_event_post(EV_NFC_SCN_DONE, (void *)10,
						   NULL);
				return;
			}
			break;
		} /* end nci_type */
	}
}

void nfc_scn_set_mode(bool start, uint8_t *rx_data, uint8_t len)
{
	static uint8_t nci_set_mode[] = { 0x84, 0x2f, 0x01, 0x02, 0x02, 0x01 };
	uint8_t mode = 0x00;

	ndlc_msg_t *rx_msg = (ndlc_msg_t *)rx_data;
	uint8_t *p_buffer = rx_msg->buffer;

	if (start) {
		if (rx_data)
			mode = rx_data[0];
		nci_set_mode[5] = mode; /* 0x00 = enter hibernate, 0x01 = wakeup */

		nfc_stn54_reset();

		pr_info(LOG_MODULE_NFC, "scn: set mode (%d)", mode);
		nfc_stn54_write(nci_set_mode, sizeof(nci_set_mode));
		return;
	}

	if (rx_msg->pcb == 0x84) {
		int nci_type = p_buffer[NCI_TYPE_OFFSET] & NCI_FRAME_TYPE_MASK;
		switch (nci_type) {
		case 0x4f: { /* mode set notif; if nothing received in two seconds => scenario timeout */
			nfc_fsm_event_post(EV_NFC_SCN_DONE, NULL, NULL);
			return;
		}
		break;
		}
	}
}

void nfc_scn_fw_update(bool start, uint8_t *rx_data, uint8_t len)
{
#ifndef CONFIG_NFC_STN54_FW_UPDATE
	pr_warning(LOG_MODULE_NFC, "Firmware update disabled.");
	nfc_fsm_event_post(EV_NFC_SCN_IGNORE, (void *)1, NULL);
	return;
#else
	ndlc_msg_t *rx_msg = (ndlc_msg_t *)rx_data;
	uint8_t *p_buffer = rx_msg->buffer;

	static uint32_t r_offset;
	static uint32_t len_to_do;
	int status = DRV_RC_OK;

	if (start) {
		pr_info(LOG_MODULE_NFC, "scn: fw update start");
		/* first = uint32_t = start of read address from flash */
		memcpy(&r_offset, &rx_data[0], sizeof(r_offset));
		/* second = uint32_t = length to read until the end of imgage */
		memcpy(&len_to_do, &rx_data[0 + sizeof(r_offset)],
		       sizeof(len_to_do));

		fwu_timer = timer_create(fwu_timer_cb, "", 250, 0, 0, NULL);

		/* simulate a timer event, to start the update */
		nfc_fsm_event_post(EV_NFC_SCN_TIMER, (void *)3, NULL);

		return;
	}

	if (rx_msg->pcb == 0x84) {
		/* do nothing with incoming data; we don't care for now */
	} else if (rx_msg->pcb == TIMER_MESSAGE_TYPE) {
		if (p_buffer[0] == 0x03) {
			if (len_to_do > 0) {
				uint8_t r_buff[253 + 2];
				uint16_t cmd_len;
				uint8_t cmd_delay;
				unsigned int retlen;
				struct td_device *spidev =
					(struct td_device *)&
					pf_sba_device_flash_spi0;

				spi_flash_read_byte(spidev, r_offset, 1,
						    &retlen,
						    (uint8_t *)r_buff);
				spi_flash_read_byte(spidev, r_offset + 1,
						    r_buff[0] + 1, &retlen,
						    (uint8_t *)(r_buff + 1));

				cmd_len = r_buff[0];
				cmd_delay = r_buff[cmd_len + 1];
				status = nfc_send_ndlc(&r_buff[1], cmd_len);
				if (status == DRV_RC_OK) {
					cmd_delay = r_buff[cmd_len + 1];
					r_offset += (cmd_len + 1 + 1); /*len + cmd + delay */
					len_to_do -= (cmd_len + 1 + 1); // The complete command (length command) + byte of length_command (1) + byte of Delay (1)
					timer_start(fwu_timer, 8 * cmd_delay,
						    NULL);
					pr_info(LOG_MODULE_NFC,
						"c:%3db, d:%3dms, r:%5db",
						cmd_len, 8 *
						cmd_delay,
						len_to_do);
				}
			}

			if (len_to_do > 0 && status != DRV_RC_OK) {
				timer_stop(fwu_timer);
				pr_error(LOG_MODULE_NFC,
					 "Error - o:%d, r:%d, s:%d)", r_offset,
					 len_to_do,
					 status);
				nfc_fsm_event_post(EV_NFC_FW_UPDATE_FAIL, NULL,
						   NULL);
			}

			if (len_to_do <= 0) {
				/* update done OK; do tuning */
				nfc_fsm_event_post(EV_NFC_FW_UPDATE_DONE, NULL,
						   NULL);
			}
		}
	}
#endif
}

void nfc_scn_raw_write(bool start, uint8_t *rx_data, uint8_t len)
{
	if (start) {
		pr_info(LOG_MODULE_NFC, "RAW data write!");
		if (len > 0) {
			nfc_stn54_write(rx_data, len);
		}
		nfc_fsm_event_post(EV_NFC_SCN_IGNORE, NULL, NULL);
		return;
	}
}

#ifdef CONFIG_NFC_STN54_AMS_DEBUG_TCMD

void nfc_scn_ams_read_reg(bool start, uint8_t *rx_data, uint8_t len)
{
	/* last byte is the register to be read */
	static uint8_t ams_read_reg[] = { 0x84, 0x2F, 0x01, 0x02, 0x04, 0x16 };
	static uint8_t reg;

	ndlc_msg_t *rx_msg = (ndlc_msg_t *)rx_data;
	uint8_t *p_buffer = rx_msg->buffer;

	if (start) {
		reg = rx_data[0];
		ams_read_reg[5] = rx_data[0];
		pr_info(LOG_MODULE_NFC, "Read AMS register 0x%02x.", reg);
		nfc_stn54_write(ams_read_reg, sizeof(ams_read_reg));
		return;
	}

	if (rx_msg->pcb == 0x84) {
		int nci_type = p_buffer[NCI_TYPE_OFFSET] & NCI_FRAME_TYPE_MASK;

		switch (nci_type) {
		case NCI_PROP_RSP:
		{
			pr_info(LOG_MODULE_NFC, "AMS read reg 0x%02x rsp:", reg);
			print_buff_by4(LOG_LEVEL_INFO, rx_data, len);
			nfc_fsm_event_post(EV_NFC_SCN_IGNORE, NULL, NULL);
			return;
		}
		break;
		} /* end nci_type */
	}
}

void nfc_scn_ams_write_reg(bool start, uint8_t *rx_data, uint8_t len)
{
	static uint8_t ams_write_reg[] =
	{ 0x84, 0x2F, 0x01, 0x03, 0x03, 0x00, 0x00 };
	static uint8_t ams_read_reg[] = { 0x84, 0x2F, 0x01, 0x02, 0x04, 0x00 };
	static uint8_t reg;
	static uint8_t val;
	static bool wflag;

	ndlc_msg_t *rx_msg = (ndlc_msg_t *)rx_data;
	uint8_t *p_buffer = rx_msg->buffer;

	if (start) {
		wflag = 0;
		reg = rx_data[0];
		val = rx_data[1];
		ams_write_reg[5] = reg;
		ams_write_reg[6] = val;
		pr_info(LOG_MODULE_NFC, "Write AMS reg 0x%02x, val 0x%02x.",
			reg,
			val);
		nfc_stn54_write(ams_write_reg, sizeof(ams_write_reg));
		return;
	}

	if (rx_msg->pcb == 0x84) {
		int nci_type = p_buffer[NCI_TYPE_OFFSET] & NCI_FRAME_TYPE_MASK;

		switch (nci_type) {
		case NCI_PROP_RSP:
			if (!wflag) {
				if (rx_data[len - 1] == 0x00) {
					wflag = 1;
					pr_debug(
						LOG_MODULE_NFC,
						"write ok, check value of reg %02x:",
						reg);
					ams_read_reg[5] = reg;
					nfc_stn54_write(ams_read_reg,
							sizeof(ams_read_reg));
				} else {
					pr_info(LOG_MODULE_NFC,
						"write failed, status:",
						rx_data[len - 1]);
					nfc_fsm_event_post(EV_NFC_SCN_FAIL,
							   (void *)1, NULL);
					return;
				}
			} else {
				pr_info(LOG_MODULE_NFC,
					"read back reg 0x%02x rsp:",
					reg);
				print_buff_by4(LOG_LEVEL_INFO, rx_data, len);
				nfc_fsm_event_post(EV_NFC_SCN_IGNORE, NULL,
						   NULL);
				return;
			}
			break;
		} /* end nci_type */
	}
}

void nfc_scn_ams_status_dump(bool start, uint8_t *rx_data, uint8_t len)
{
	static const uint8_t ams_status_dump[] =
	{ 0x84, 0x01, 0x00, 0x04, 0x82, 0x10, 0xFF, 0x82 };

	ndlc_msg_t *rx_msg = (ndlc_msg_t *)rx_data;
	uint8_t *p_buffer = rx_msg->buffer;

	if (start) {
		pr_info(LOG_MODULE_NFC, "AMS status dump:");
		nfc_stn54_write(ams_status_dump, sizeof(ams_status_dump));
		return;
	}

	if (rx_msg->pcb == 0x84) {
		int nci_type = p_buffer[NCI_TYPE_OFFSET] & NCI_FRAME_TYPE_MASK;

		switch (nci_type) {
		case NCI_DATA_HCI: {
			print_buff_by4(LOG_LEVEL_INFO, rx_data, len);
			nfc_fsm_event_post(EV_NFC_SCN_IGNORE, NULL, NULL);
			return;
		}
		break;
		} /* end nci_type */
	}
}

#endif
