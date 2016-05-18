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

#include "cfw/cfw_service.h"
#include "infra/log.h"
#include "services/nfc_service/nfc_service.h"
#include "nfc_service_private.h"

#ifdef CONFIG_NFC_STN54_SUPPORT
#include "nfc_stn54_sm.h"
#endif

static void nfc_generic_rsp_callback(struct cfw_message *msg, uint8_t result);

static void handle_nfc_init(struct cfw_message *msg, bool *free_msg)
{
	/* setup callback function for response */
	struct nfc_msg_cb *enable_clf_cb =
		(struct nfc_msg_cb *)balloc(sizeof(*enable_clf_cb), NULL);

	enable_clf_cb->msg = msg;
	enable_clf_cb->msg_cb = nfc_generic_rsp_callback;
	*free_msg = false;
	nfc_fsm_event_post(EV_NFC_INIT, NULL, enable_clf_cb);
}

static void handle_nfc_set_ce_mode(struct cfw_message *msg, bool *free_msg)
{
	/* setup callback function for response */
	struct nfc_msg_cb *set_ce_cb =
		(struct nfc_msg_cb *)balloc(sizeof(*set_ce_cb), NULL);

	set_ce_cb->msg = msg;
	set_ce_cb->msg_cb = nfc_generic_rsp_callback;
	*free_msg = false;
	nfc_fsm_event_post(EV_NFC_CONFIG_CE, NULL, set_ce_cb);
}

static void handle_nfc_disable(struct cfw_message *msg, bool *free_msg)
{
	/* setup callback function for response */
	struct nfc_msg_cb *disable_clf_cb =
		(struct nfc_msg_cb *)balloc(sizeof(*disable_clf_cb), NULL);

	disable_clf_cb->msg = msg;
	disable_clf_cb->msg_cb = nfc_generic_rsp_callback;
	*free_msg = false;
	nfc_fsm_event_post(EV_NFC_DISABLE, NULL, disable_clf_cb);
}

static void handle_nfc_start_rf(struct cfw_message *msg, bool *free_msg)
{
	/* setup callback function for response */
	struct nfc_msg_cb *start_cb =
		(struct nfc_msg_cb *)balloc(sizeof(*start_cb), NULL);

	start_cb->msg = msg;
	start_cb->msg_cb = nfc_generic_rsp_callback;
	*free_msg = false;
	nfc_fsm_event_post(EV_NFC_RF_START, NULL, start_cb);
}

static void handle_nfc_stop_rf(struct cfw_message *msg, bool *free_msg)
{
	/* setup callback function for response */
	struct nfc_msg_cb *stop_cb =
		(struct nfc_msg_cb *)balloc(sizeof(*stop_cb), NULL);

	stop_cb->msg = msg;
	stop_cb->msg_cb = nfc_generic_rsp_callback;
	*free_msg = false;
	nfc_fsm_event_post(EV_NFC_RF_STOP, NULL, stop_cb);
}

static void handle_nfc_hibernate(struct cfw_message *msg, bool *free_msg)
{
	/* setup callback function for response */
	struct nfc_msg_cb *hibernate_cb =
		(struct nfc_msg_cb *)balloc(sizeof(*hibernate_cb), NULL);

	hibernate_cb->msg = msg;
	hibernate_cb->msg_cb = nfc_generic_rsp_callback;
	*free_msg = false;
	nfc_fsm_event_post(EV_NFC_HIBERNATE, NULL, hibernate_cb);
}

static void nfc_generic_rsp_callback(struct cfw_message *msg, uint8_t result)
{
	int rsp_id = 0;

	switch (CFW_MESSAGE_ID(msg)) {
	case MSG_ID_NFC_SERVICE_INIT:
		rsp_id = MSG_ID_NFC_SERVICE_INIT_RSP;
		break;
	case MSG_ID_NFC_SERVICE_SET_CE_MODE:
		rsp_id = MSG_ID_NFC_SERVICE_SET_CE_MODE_RSP;
		break;
	case MSG_ID_NFC_SERVICE_START_RF:
		rsp_id = MSG_ID_NFC_SERVICE_START_RF_RSP;
		break;
	case MSG_ID_NFC_SERVICE_STOP_RF:
		rsp_id = MSG_ID_NFC_SERVICE_STOP_RF_RSP;
		break;
	case MSG_ID_NFC_SERVICE_DISABLE:
		rsp_id = MSG_ID_NFC_SERVICE_DISABLE_RSP;
		break;
	case MSG_ID_NFC_SERVICE_HIBERNATE:
		rsp_id = MSG_ID_NFC_SERVICE_HIBERNATE_RSP;
		break;
	default:
		pr_info(LOG_MODULE_NFC, "%s: unexpected cb message id: %x",
			__func__, CFW_MESSAGE_ID(
				msg));
		break;
	}

	nfc_service_rsp_msg_t *resp =
		(nfc_service_rsp_msg_t *)cfw_alloc_rsp_msg(msg,
							   rsp_id,
							   sizeof(*resp));
	resp->status = result;
	cfw_send_message(resp);
}

static void handle_message(struct cfw_message *msg, void *param)
{
	pr_debug(LOG_MODULE_NFC, "%s: handle_message: %d", __FILE__,
		 CFW_MESSAGE_ID(
			 msg));

	bool do_msg_free = true;

	switch (CFW_MESSAGE_ID(msg)) {
	case MSG_ID_NFC_SERVICE_INIT:
		handle_nfc_init(msg, &do_msg_free);
		break;
	case MSG_ID_NFC_SERVICE_SET_CE_MODE:
		handle_nfc_set_ce_mode(msg, &do_msg_free);
		break;
	case MSG_ID_NFC_SERVICE_START_RF:
		handle_nfc_start_rf(msg, &do_msg_free);
		break;
	case MSG_ID_NFC_SERVICE_STOP_RF:
		handle_nfc_stop_rf(msg, &do_msg_free);
		break;
	case MSG_ID_NFC_SERVICE_DISABLE:
		handle_nfc_disable(msg, &do_msg_free);
		break;
	case MSG_ID_NFC_SERVICE_HIBERNATE:
		handle_nfc_hibernate(msg, &do_msg_free);
		break;
	default:
		pr_info(LOG_MODULE_NFC, "%s: unexpected message id: %x",
			__func__, CFW_MESSAGE_ID(
				msg));
		break;
	}

	if (do_msg_free)
		cfw_msg_free(msg);
}

static service_t nfc_service;

static void nfc_service_init(int service_id, T_QUEUE queue)
{
	nfc_service.service_id = service_id;
	cfw_register_service(queue, &nfc_service, handle_message, NULL);
	nfc_fsm_init(queue);
}

CFW_DECLARE_SERVICE(NFC, NFC_SERVICE_ID, nfc_service_init);
