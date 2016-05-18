/*
 * Copyright (c) 2015, Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdlib.h>
#include <string.h>

#include "infra/log.h"
#include "infra/tcmd/handler.h"

#include "services/services_ids.h"
#include "services/service_queue.h"
#include "cfw/cproxy.h"

#include "services/nfc_service/nfc_service.h"
#include "nfc_stn54_sm.h"
#include "nfc_stn54_seq.h"

static cfw_service_conn_t *nfc_service_conn;

/*
 * @defgroup cfw_tcmd_nfc NFC Test Commands
 * Interfaces to support NFC Test Commands.
 * @ingroup infra_tcmd
 * @{
 */
static void _tcmd_nfc_handle_msg(struct cfw_message *msg, void *data)
{
	switch (CFW_MESSAGE_ID(msg)) {
	case MSG_ID_NFC_SERVICE_INIT_RSP:
		pr_info(LOG_MODULE_MAIN, "Enable rsp: %d!",
			((nfc_service_rsp_msg_t *)msg)->status);
		break;

	case MSG_ID_NFC_SERVICE_SET_CE_MODE_RSP:
		pr_info(LOG_MODULE_MAIN, "Set CE rsp: %d!",
			((nfc_service_rsp_msg_t *)msg)->status);
		break;

	case MSG_ID_NFC_SERVICE_DISABLE_RSP:
		pr_info(LOG_MODULE_MAIN, "Disable rsp: %d!",
			((nfc_service_rsp_msg_t *)msg)->status);
		break;

	case MSG_ID_NFC_SERVICE_START_RF_RSP:
		pr_info(LOG_MODULE_MAIN, "Start rsp: %d!",
			((nfc_service_rsp_msg_t *)msg)->status);
		break;

	case MSG_ID_NFC_SERVICE_STOP_RF_RSP:
		pr_info(LOG_MODULE_MAIN, "Stop rsp: %d!",
			((nfc_service_rsp_msg_t *)msg)->status);
		break;

	case MSG_ID_NFC_SERVICE_HIBERNATE_RSP:
		pr_info(LOG_MODULE_MAIN, "Hibernate rsp: %d!",
			((nfc_service_rsp_msg_t *)msg)->status);
		break;

	default:
		pr_info(LOG_MODULE_MAIN, "Other NFC RSP/EVT!");
		break;
	}

	cproxy_disconnect(nfc_service_conn);
	cfw_msg_free(msg);
}

/*
 * Test commands to control the NFC service: nfc svc <init|set_ce|start|stop|disable|tune_rf>
 *
 * - nfc svc init : power up controller and run the init/reset scenarios
 * - nfc svc set_ce: configure the controller in card emulation mode (but rf not started)
 * - nfc svc start  : start the RF loop (card emulation is active)
 * - nfc svc stop   : stop the RF loop (card emulation is inactive)
 * - nfc svc disable: power down controller.
 * - nfc svc tune_rf: update the RF tunning values.
 *
 * @param[in]   argc    Number of arguments in the Test Command (including group and name),
 * @param[in]   argv    Table of null-terminated buffers containing the arguments
 * @param[in]   ctx The context to pass back to responses
 */
void nfc_svc_tcmd(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	if (argc < 3)
		goto show_help;

	nfc_service_conn = cproxy_connect(NFC_SERVICE_ID, _tcmd_nfc_handle_msg,
					  NULL);
	if (nfc_service_conn == NULL) {
		TCMD_RSP_ERROR(ctx, "Cannot connect to NFC service");
		return;
	}

	if (!strncmp("init", argv[2], 4)) {
		nfc_init(nfc_service_conn, NULL);
	} else if (!strncmp("set_ce", argv[2], 6)) {
		nfc_set_ce_mode(nfc_service_conn, NULL);
	} else if (!strncmp("disable", argv[2], 7)) {
		nfc_disable(nfc_service_conn, NULL);
	} else if (!strncmp("start", argv[2], 5)) {
		nfc_start_rf(nfc_service_conn, NULL);
	} else if (!strncmp("stop", argv[2], 4)) {
		nfc_stop_rf(nfc_service_conn, NULL);
	} else if (!strncmp("hibernate", argv[2], 4)) {
		nfc_hibernate(nfc_service_conn, NULL);
	} else if (!strncmp("tune_rf", argv[2], 7)) {
		nfc_fsm_event_post(EV_NFC_RF_TUNING, NULL, NULL);
	} else {
		goto show_help;
	}

	TCMD_RSP_FINAL(ctx, NULL);
	return;

show_help:
	TCMD_RSP_ERROR(
		ctx,
		"nfc svc <init|set_ce|start|stop|disable|hibernate|tune_rf>");
	return;
}
DECLARE_TEST_COMMAND_ENG(nfc, svc, nfc_svc_tcmd);

#ifdef CONFIG_NFC_FSM_TCMD
/*
 * Test commands to test the NFC state machine:
 *  nfc fsm <enable|disable|start|stop|scn_all_done|fsm_auto_test>
 *
 * Used to send state machine events directly, bypassing the service interface.
 *
 * @param[in]   argc    Number of arguments in the Test Command (including group and name),
 * @param[in]   argv    Table of null-terminated buffers containing the arguments
 * @param[in]   ctx The context to pass back to responses
 */
static void nfc_fsm_tcmd(int argc, char **argv, struct tcmd_handler_ctx *ctx)
{
	if (argc < 3)
		goto show_help;

	pr_info(LOG_MODULE_MAIN, "fsm event: %s", argv[2]);
	if (!strncmp("enable", argv[2], 6)) {
		nfc_fsm_event_post(EV_NFC_INIT, NULL, NULL);
	} else if (!strncmp("disable", argv[2], 7)) {
		nfc_fsm_event_post(EV_NFC_DISABLE, NULL, NULL);
	} else if (!strncmp("start", argv[2], 5)) {
		nfc_fsm_event_post(EV_NFC_RF_START, NULL, NULL);
	} else if (!strncmp("stop", argv[2], 4)) {
		nfc_fsm_event_post(EV_NFC_RF_STOP, NULL, NULL);
	} else if (!strncmp("error", argv[2], 5)) {
		nfc_fsm_event_post(EV_NFC_SEQ_FAIL, NULL, NULL);
	} else if (!strncmp("scn_all_done", argv[2], 12)) {
		nfc_fsm_event_post(EV_NFC_SEQ_PASS, NULL, NULL);
	} else if (!strncmp("fsm_auto_test", argv[2], 9)) {
		nfc_fsm_event_post(EV_NFC_INIT, NULL, NULL);
		nfc_fsm_event_post(EV_NFC_SEQ_PASS, NULL, NULL);
		nfc_fsm_event_post(EV_NFC_RF_START, NULL, NULL);
		nfc_fsm_event_post(EV_NFC_SEQ_PASS, NULL, NULL);
		nfc_fsm_event_post(EV_NFC_RF_STOP, NULL, NULL);
		nfc_fsm_event_post(EV_NFC_SEQ_PASS, NULL, NULL);
		nfc_fsm_event_post(EV_NFC_DISABLE, NULL, NULL);
		nfc_fsm_event_post(EV_NFC_SEQ_PASS, NULL, NULL);
	}

	TCMD_RSP_FINAL(ctx, NULL);
	return;

show_help:
	TCMD_RSP_ERROR(
		ctx,
		"nfc fsm <enable|disable|start|stop|scn_all_done|fsm_auto_test>");
	return;
}
DECLARE_TEST_COMMAND(nfc, fsm, nfc_fsm_tcmd);
#endif

#ifdef CONFIG_NFC_STN54_AMS_DEBUG_TCMD
/*
 * Test commands to run AMS calibration functions.
 *
 * Calls scenarios used to read/write AMS registers.
 *
 * @param[in]   argc    Number of arguments in the Test Command (including group and name),
 * @param[in]   argv    Table of null-terminated buffers containing the arguments
 * @param[in]   ctx The context to pass back to responses
 */
static void nfc_ams_tcmd(int argc, char **argv, struct tcmd_handler_ctx *ctx)
{
	char *pend;
	uint8_t val;

	static uint8_t clf_cnf[] =
	{ 0x84, 0x01, 0x00, 0x0a, 0x82, 0x011, 0xd0, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x7e };

	static uint8_t card_a_tuned[] = { 0x84, 0x01, 0x00, 0xa4, 0x82, 0x11,
					  0xFF, 0x82, 0x00, 0x00, 0x00, 0x00,
					  0x00, 0x0A, 0x00, 0x00, 0x10, 0x00,
					  0xF7, 0x02, 0xCF, 0x04, 0x00,
					  0x00, 0x34, 0x00,
					  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					  0x00, 0x0A, 0x00, 0x00, 0x10, 0x00,
					  0x27, 0x02, 0xE8, 0x04, 0x00,
					  0x00, 0x00, 0x00,
					  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					  0x00, 0x0A, 0x00, 0x00, 0x10, 0x00,
					  0xE7, 0x02, 0xF8, 0x04, 0x00,
					  0x00, 0x00, 0x00,
					  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					  0x00, 0x0A, 0x00, 0x00, 0x10, 0x00,
					  0xE7, 0x02, 0xF8, 0x04, 0x00,
					  0x00, 0x00, 0x00,
					  0x00, 0x00, 0x2E, 0x00, 0x10, 0x01,
					  0x70, 0x02, 0xD4, 0x03, 0x10, 0x04,
					  0x48, 0x05, 0x00, 0x06, 0x0A,
					  0x07, 0x13, 0x08,
					  0x00, 0x09, 0xB8, 0x0A, 0x71, 0x0B,
					  0x69, 0x0C, 0x63, 0x0D, 0x99, 0x0E,
					  0x30, 0x0F, 0xB0, 0x10, 0x30,
					  0x11, 0x90, 0x12,
					  0x68, 0x13, 0x00, 0x18, 0x3C, 0x19,
					  0xE3, 0x1A, 0xB1, 0x00, 0x00, 0x00,
					  0x00, 0x00, 0x00, 0x00, 0x00,
					  0x00, 0x00, 0x00,
					  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					  0x00, 0x00, 0x00, 0x00, 0x00,
					  0x00, 0x00, 0x00,
					  0x00, 0x00 };

	static uint8_t card_a_small_tuned[] =
	{ 0x84, 0x01, 0x00, 0xa4, 0x82, 0x11,
	  0xFF, 0x82, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x0A, 0x00, 0x00, 0x10, 0x00, 0xF7, 0x02, 0xCF, 0x04,
	  0x00, 0x00, 0x34, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x0A, 0x00, 0x00, 0x10, 0x00, 0x27, 0x02, 0xE8, 0x04,
	  0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x0A, 0x00, 0x00, 0x10, 0x00, 0xE7, 0x02, 0xF8, 0x04,
	  0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x0A, 0x00, 0x00, 0x10, 0x00, 0xE7, 0x02, 0xF8, 0x04,
	  0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x2E, 0x00, 0x10,
	  0x01, 0x70, 0x02, 0xD4, 0x03, 0x10, 0x04, 0x48, 0x05, 0x00, 0x06,
	  0x0A, 0x07, 0x0B, 0x08,
	  0x00, 0x09, 0x38, 0x0A, 0x71,
	  0x0B, 0x69, 0x0C, 0x63, 0x0D, 0x99, 0x0E, 0x30, 0x0F, 0xB0, 0x10,
	  0x30, 0x11, 0x90, 0x12,
	  0x68, 0x13, 0x00, 0x18, 0x3C,
	  0x19, 0xE3, 0x1A, 0xB1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00 };

	static uint8_t fdt_reg[] = { 0x84, 0x01, 0x00, 0x24, 0x82, 0x11,
				     0xFF, 0x80, 0x00, 0xE8, 0x11, 0x02, 0x05,
				     0x18, 0x78, 0x22, 0x58, 0x38, 0x00,
				     0x18, 0x18, 0x40, 0x01, 0x02, 0x01, 0x02,
				     0x00, 0x10, 0xF0, 0x00, 0x02, 0x0A,
				     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				     0x00 };

	if (argc < 3)
		goto show_help;

	if (!strncmp("read", argv[2], 4)) {
		if (argc < 4)
			goto show_help;

		uint8_t reg = (uint8_t)strtoul(argv[3], &pend, 16);
		if (*pend != 0)
			goto show_help;

		nfc_scn_queue(nfc_scn_ams_read_reg, 100, sizeof(reg), &reg);
	} else if (!strncmp("write", argv[2], 5)) {
		if (argc < 5)
			goto show_help;

		uint8_t reg = (uint8_t)strtoul(argv[3], &pend, 16);
		if (*pend != 0)
			goto show_help;

		uint8_t val = (uint8_t)strtoul(argv[4], &pend, 16);
		if (*pend != 0)
			goto show_help;

		uint8_t data[] = { reg, val };
		nfc_scn_queue(nfc_scn_ams_write_reg, 100, sizeof(data), data);
	} else if (!strncmp("dump", argv[2], 4)) {
		nfc_scn_queue(nfc_scn_ams_status_dump, 100, 0, NULL);
	} else if (!strncmp("set_normal_ant", argv[2], 7)) {
		nfc_scn_queue(nfc_scn_raw_write, 100, sizeof(card_a_tuned),
			      card_a_tuned);
	} else if (!strncmp("set_small_ant", argv[2], 7)) {
		nfc_scn_queue(nfc_scn_raw_write, 100, sizeof(card_a_small_tuned),
			      card_a_small_tuned);
	} else if (!strncmp("set_config", argv[2], 7)) {
		if (argc < 4)
			goto show_help;
		val = (uint8_t)strtoul(argv[3], &pend, 16);
		if (*pend != 0)
			goto show_help;
		clf_cnf[13] = val;
		nfc_scn_queue(nfc_scn_raw_write, 100, sizeof(clf_cnf), clf_cnf);
	} else if (!strncmp("set_fdt", argv[2], 7)) {
		if (argc < 4)
			goto show_help;
		val = (uint8_t)strtoul(argv[3], &pend, 16);
		if (*pend != 0)
			goto show_help;
		fdt_reg[31] = val;
		nfc_scn_queue(nfc_scn_raw_write, 100, sizeof(fdt_reg), fdt_reg);
	}

	nfc_scn_start();

	TCMD_RSP_FINAL(ctx, NULL);
	return;

show_help:
	TCMD_RSP_ERROR(
		ctx,
		"nfc ams <read [reg]|write [reg] [value]|dump>|set_fdt <delay>|set_normal_ant|set_small_ant|set_config <last byte>");
	return;
}
DECLARE_TEST_COMMAND(nfc, ams, nfc_ams_tcmd);
#endif

/* @} */
