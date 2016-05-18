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

#include "util/assert.h"
#include "cfw/cfw.h"
#include "cfw/cproxy.h"
#include "infra/port.h"
#include "infra/log.h"
#include "infra/tcmd/handler.h"
#include "services/battery_service/battery_service.h"

#include "battery_service_private.h"
#include "battery_service_tcmd.h"

#define ANS_LENGTH                      80
#define STRING_VALUE                    "value"

struct _info_for_rsp {
	struct tcmd_handler_ctx *ctx;
	cfw_service_conn_t *battery_service_conn;
};


static void batt_tcmd_handle_message_batt_info(
	struct cfw_message *	msg,
	char *			answer,
	struct _info_for_rsp *	info_for_rsp)
{
	switch (((battery_service_info_request_rsp_msg_t *)msg)->batt_info_id)
	{
	case BATTERY_DATA_LEVEL:
		snprintf(
			answer, ANS_LENGTH, "%s :\t%d[%%]",
			STRING_VALUE,
			((battery_service_info_request_rsp_msg_t *)msg)->
			battery_soc.bat_soc);
		break;
	case BATTERY_DATA_STATUS:
		if (((battery_service_info_request_rsp_msg_t *)msg)->
		    battery_charging.is_charging)
			snprintf(answer, ANS_LENGTH, "%s : CHARGING",
				 STRING_VALUE);
		else
			snprintf(answer, ANS_LENGTH, "%s : NOT CHARGING",
				 STRING_VALUE);
		break;
	case BATTERY_DATA_VBATT:
		snprintf(
			answer, ANS_LENGTH, "%s :\t%d[mV]", STRING_VALUE,
			((battery_service_info_request_rsp_msg_t *)msg)->
			battery_voltage.bat_vol);
		break;
	case BATTERY_DATA_TEMPERATURE:
		snprintf(
			answer, ANS_LENGTH, "%s :\t%d[°C]", STRING_VALUE,
			((battery_service_info_request_rsp_msg_t *)msg)->
			battery_temperature.bat_temp);
		break;
	case BATTERY_DATA_CHARGE_CYCLE:
		snprintf(
			answer, ANS_LENGTH, "%s :\t%d", STRING_VALUE,
			((battery_service_info_request_rsp_msg_t *)msg)->
			battery_charge_cycle.bat_charge_cycle);
		break;
	case BATTERY_DATA_CHARGER_STATUS:
		if (((battery_service_info_request_rsp_msg_t *)msg)->
		    battery_charger_connected.is_charger_connected)
			snprintf(answer, ANS_LENGTH, "%s : CONNECTED",
				 STRING_VALUE);
		else
			snprintf(answer, ANS_LENGTH, "%s : NOT CONNECTED",
				 STRING_VALUE);
		break;
	case BATTERY_DATA_CHARGER_TYPE:
		switch (((battery_service_info_request_rsp_msg_t *)msg)->
			battery_charging_source.charging_source) {
		case CHARGING_USB:
			snprintf(answer, ANS_LENGTH, "%s : USB", STRING_VALUE);
			break;
		case CHARGING_WIRELESS:
			snprintf(answer, ANS_LENGTH, "%s : Qi", STRING_VALUE);
			break;
		case CHARGING_NONE:
			snprintf(answer, ANS_LENGTH, "%s : NONE", STRING_VALUE);
			break;
		case CHARGING_DC:
			snprintf(answer, ANS_LENGTH, "%s : DC", STRING_VALUE);
			break;
		case CHARGING_UNKNOWN:
		default:
			snprintf(answer, ANS_LENGTH, "Unknown");
			break;
		}
		break;
	default:
		break;
	}
	TCMD_RSP_FINAL(info_for_rsp->ctx, answer);
}

/**@brief Function to handle responses for level alarm messages
 *
 * @param[in]   msg  message.
 */
static void batt_tcmd_handle_message_rsp(struct cfw_message *	msg,
					 char *			answer,
					 struct _info_for_rsp * info_for_rsp)
{
	assert(msg && answer && info_for_rsp);
	if (((battery_set_level_alarm_rsp_msg_t *)msg)->status
	    == BATTERY_STATUS_SUCCESS) {
		snprintf(answer, ANS_LENGTH, "%s : OK", STRING_VALUE);
		TCMD_RSP_FINAL(info_for_rsp->ctx, answer);
	} else
		TCMD_RSP_ERROR(info_for_rsp->ctx, NULL);
}

/**@brief Function to handle requests, responses
 *
 * @param[in]   msg  message.
 */
static void batt_tcmd_handle_message(struct cfw_message *msg, void *param)
{
	struct _info_for_rsp *info_for_rsp = (struct _info_for_rsp *)msg->priv;
	char *answer = balloc(ANS_LENGTH, NULL);

	switch (CFW_MESSAGE_ID(msg)) {
	case MSG_ID_BATTERY_SERVICE_GET_BATTERY_INFO_RSP:
		if (((battery_service_info_request_rsp_msg_t *)msg)->status
		    != BATTERY_STATUS_SUCCESS) {
			TCMD_RSP_ERROR(info_for_rsp->ctx, NULL);
			break;
		} else
			batt_tcmd_handle_message_batt_info(msg, answer,
							   info_for_rsp);
		break;
	case MSG_ID_BATTERY_SERVICE_SET_MEASURE_INTERVAL_RSP:
		snprintf(answer, ANS_LENGTH, "%s : OK", STRING_VALUE);
		TCMD_RSP_FINAL(info_for_rsp->ctx, answer);
		break;
	case MSG_ID_BATTERY_SERVICE_SET_LOW_LEVEL_ALARM_RSP:
	case MSG_ID_BATTERY_SERVICE_SET_CRITICAL_LEVEL_ALARM_RSP:
		batt_tcmd_handle_message_rsp(msg, answer, info_for_rsp);
		break;
	default:
		break;
	}
	cproxy_disconnect(info_for_rsp->battery_service_conn);
	bfree(info_for_rsp);
	bfree(answer);
	cfw_msg_free(msg);
}


static cfw_service_conn_t *_get_service_conn(struct tcmd_handler_ctx *ctx)
{
	cfw_service_conn_t *batt_tcmd_client_conn = NULL;

	if ((batt_tcmd_client_conn =
		     cproxy_connect(BATTERY_SERVICE_ID,
				    batt_tcmd_handle_message,
				    NULL)) == NULL) {
		TCMD_RSP_ERROR(ctx, "Cannot connect to Battery Service!");
	}

	return batt_tcmd_client_conn;
}

void battery_service_cmd_handler(struct tcmd_handler_ctx *	ctx,
				 battery_data_info_t		bs_cmd,
				 void *				param)
{
	cfw_service_conn_t *batt_tcmd_client_conn = _get_service_conn(ctx);

	assert(batt_tcmd_client_conn);

	struct _info_for_rsp *info_for_rsp = balloc(
		sizeof(struct _info_for_rsp), NULL);
	info_for_rsp->ctx = ctx;
	info_for_rsp->battery_service_conn = batt_tcmd_client_conn;

	switch (bs_cmd) {
	case BATTERY_DATA_LEVEL:
	case BATTERY_DATA_STATUS:
	case BATTERY_DATA_VBATT:
	case BATTERY_DATA_TEMPERATURE:
	case BATTERY_DATA_CHARGE_CYCLE:
	case BATTERY_DATA_CHARGER_STATUS:
	case BATTERY_DATA_CHARGER_TYPE:
		battery_service_get_info(batt_tcmd_client_conn, bs_cmd,
					 info_for_rsp);
		break;
	case BATTERY_DATA_MEASURE_INTERVAL:
		battery_service_set_measure_interval(batt_tcmd_client_conn,
						     (struct
						      battery_service_period_cfg_msg
						      *)param, info_for_rsp);
		break;
	default:
		break;
	}
}
