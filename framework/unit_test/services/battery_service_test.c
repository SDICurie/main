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

#include "cfw/cfw.h"

#include "infra/log.h"

#include "services/service_tests.h"
#include "util/cunit_test.h"
#include "../src/services/battery_service/battery_service_private.h"
#include "services/battery_service/battery_service.h"
#include "drivers/data_type.h"

#define TIMEOUT 10000

#define REGISTRATION_TIMEOUT            200
#define REGISTRATION_POLLING_RETRY      0xFFFFFF


static bool batt_fail = false;
static uint8_t g_low_level_alarm;
static uint8_t g_critical_level_alarm;


cfw_service_conn_t *test_bs_client_conn = NULL;
static bool battery_init_done = false;

static battery_status_t battery_service_check_status(
	struct cfw_message *msg,
	void *
	text)
{
	battery_status_t bat_status = BATTERY_STATUS_ERROR;

	if ((NULL != msg) &&
	    (NULL != text)) {
		bat_status =
			((battery_service_info_request_rsp_msg_t *)msg)->
			battery_soc.
			status;

		switch (bat_status) {
		case BATTERY_STATUS_SUCCESS:
			break;
		case BATTERY_STATUS_PENDING:
			cu_print("BT_TEST INFO %s - BATTERY_STATUS_PENDING\n",
				 text);
			break;
		case BATTERY_STATUS_IN_USE:
			cu_print("BT_TEST INFO %s - BATTERY_STATUS_PENDING\n",
				 text);
			break;
		case BATTERY_STATUS_ERROR:
			cu_print("BT_TEST INFO %s - BATTERY_STATUS_ERROR\n",
				 text);
			break;
		case BATTERY_STATUS_NOT_IMPLEMENTED:
			cu_print(
				"BT_TEST INFO %s - BATTERY_STATUS_NOT_IMPLEMENTED\n",
				text);
			break;
		case BATTERY_STATUS_ERROR_ADC_SERVICE:
			cu_print(
				"BT_TEST INFO %s - BATTERY_STATUS_ERROR_ADC_SERVICE\n",
				text);
			break;
		case BATTERY_STATUS_ERROR_VALUE_OUT_OF_RANGE:
			cu_print(
				"BT_TEST INFO %s - BATTERY_STATUS_ERROR_VALUE_OUT_OF_RANGE\n",
				text);
			break;
		case BATTERY_STATUS_ERROR_IPC:
			cu_print("BT_TEST INFO %s - BATTERY_STATUS_ERROR_IPC\n",
				 text);
			break;
		case BATTERY_STATUS_ERROR_FUEL_GAUGE:
			cu_print(
				"BT_TEST INFO %s - BATTERY_STATUS_ERROR_FUEL_GAUGE\n",
				text);
			break;
		default:
			cu_print("BT_TEST INFO %s - UNKNOWN STATUS\n", text);
			break;
		}
	}

	return bat_status;
}
/**@brief Function to handle requests, responses and events
 *
 * @param[in]   msg  Event message.
 * @return      none
 */
static void battery_service_test_handle(struct cfw_message *msg, void *param)
{
	battery_status_t bat_status = BATTERY_STATUS_ERROR;

	cu_print("BT_TEST INFO ID=%d\n", CFW_MESSAGE_ID(msg));


	switch (CFW_MESSAGE_ID(msg)) {
	case MSG_ID_CFW_OPEN_SERVICE_RSP:
		cu_print(
			"BT_TEST INFO CLIENT: MSG_ID_CFW_OPEN_SERVICE_RSP %s\n",
			(char *)msg->priv);

		test_bs_client_conn =
			(cfw_service_conn_t *)((cfw_open_conn_rsp_msg_t *)msg)
			->service_conn;

		battery_init_done = true;
		int events[6] = { MSG_ID_BATTERY_SERVICE_LEVEL_UPDATED_EVT,
				  MSG_ID_BATTERY_SERVICE_LEVEL_LOW_EVT,
				  MSG_ID_BATTERY_SERVICE_LEVEL_CRITICAL_EVT,
				  MSG_ID_BATTERY_SERVICE_LEVEL_SHUTDOWN_EVT,
				  MSG_ID_BATTERY_SERVICE_TEMPERATURE_CRITICAL_EVT,
				  MSG_ID_BATTERY_SERVICE_TEMPERATURE_SHUTDOWN_EVT };

		cfw_register_events(test_bs_client_conn, events, 6,
				    CFW_MESSAGE_PRIV(
					    msg));

		break;
	case MSG_ID_CFW_CLOSE_SERVICE_RSP:
	case MSG_ID_CFW_REGISTER_EVT_RSP:
		break;
	case MSG_ID_BATTERY_SERVICE_GET_BATTERY_INFO_RSP:
		switch (((battery_service_info_request_rsp_msg_t *)msg)->
			batt_info_id) {
		case BATTERY_DATA_LEVEL:
			if (BATTERY_STATUS_SUCCESS ==
			    battery_service_check_status(msg, "GET_SOC")) {
				cu_print(
					"%s: BT_TEST INFO FG_SOC:\t%d[%%]\n",
					(char *)msg->priv,
					((
						 battery_service_info_request_rsp_msg_t
						 *)msg)->
					battery_soc.bat_soc);
			}
			batt_fail = true;
			break;
		case BATTERY_DATA_LOW_LEVEL_ALARM:
			if (BATTERY_STATUS_SUCCESS ==
			    battery_service_check_status(msg,
							 "LOW_LEVEL_ALARM")) {
				g_low_level_alarm =
					((
						 battery_service_info_request_rsp_msg_t
						 *)msg)
					->battery_low_level_alarm.level_alarm;
				cu_print("%s :BT_TEST INFO LOW LEVEL:\t%d\n",
					 (char *)msg->priv, g_low_level_alarm);
			}
			batt_fail = true;
			break;
		case BATTERY_DATA_CRITICAL_LEVEL_ALARM:
			if (BATTERY_STATUS_SUCCESS ==
			    battery_service_check_status(msg,
							 "CRITICAL_LEVEL_ALARM"))
			{
				g_critical_level_alarm =
					((
						 battery_service_info_request_rsp_msg_t
						 *)msg)
					->battery_critical_level_alarm.
					level_alarm;
				cu_print(
					"%s :BT_TEST INFO CRITICAL LEVEL:\t%d\n",
					(char *)msg->priv,
					g_critical_level_alarm);
			}
			batt_fail = true;
			break;
		case BATTERY_DATA_VBATT:
			bat_status = battery_service_check_status(msg,
								  "GET_VOL");
			if (BATTERY_STATUS_SUCCESS == bat_status) {
				cu_print(
					"%s :BT_TEST INFO BATTERY_VOLTAGE:\t%d[mV]\n",
					(char *)msg->priv,
					((
						 battery_service_info_request_rsp_msg_t
						 *)msg)->
					battery_voltage.bat_vol);
			}
			if (BATTERY_STATUS_PENDING != bat_status) batt_fail =
					true;
			break;
		case BATTERY_DATA_TEMPERATURE:
			bat_status = battery_service_check_status(msg,
								  "GET_TEMP");
			if (BATTERY_STATUS_SUCCESS == bat_status) {
				cu_print(
					"%s :BT_TEST INFO GET_TEMP:\t%d[0.1C]\n",
					(char *)msg->priv,
					((
						 battery_service_info_request_rsp_msg_t
						 *)msg)->
					battery_temperature.bat_temp);
			}
			if (BATTERY_STATUS_PENDING != bat_status) batt_fail =
					true;
			break;
		default:
			break;
		}
		break;
	case MSG_ID_BATTERY_SERVICE_SET_LOW_LEVEL_ALARM_RSP:
		cu_print(
			"BT_TEST INFO MSG_ID_BATTERY_SERVICE_SET_LOW_LEVEL_ALARM_RSP\n");
		batt_fail = true;
		break;
	case MSG_ID_BATTERY_SERVICE_SET_CRITICAL_LEVEL_ALARM_RSP:
		batt_fail = true;
		cu_print(
			"BT_TEST INFO MSG_ID_BATTERY_SERVICE_SET_CRITICAL_LEVEL_ALARM_RSP\n");
		break;
	case MSG_ID_BATTERY_SERVICE_LEVEL_LOW_EVT:
		cu_print(
			"BT_TEST INFO CLIENT:  SOC LEVEL TOO LOW \t%d[%%]\n",
			((battery_service_listen_evt_rsp_msg_t *)msg)->
			battery_service_evt_content_rsp_msg.bat_soc);
		break;
	case MSG_ID_BATTERY_SERVICE_LEVEL_CRITICAL_EVT:
		cu_print(
			"BT_TEST INFO CLIENT:  SOC LEVEL CRITICAL \t%d[%%]\n",
			((battery_service_listen_evt_rsp_msg_t *)msg)->
			battery_service_evt_content_rsp_msg.bat_soc);
		break;
	case MSG_ID_BATTERY_SERVICE_CHARGER_CONNECTED_EVT:
		cu_print("BT_TEST INFO CLIENT:  CHARGER_CONNECTED");
		break;
	case MSG_ID_BATTERY_SERVICE_FULLY_CHARGED_EVT:
		cu_print("BT_TEST INFO CLIENT:  BATT_FULLY_CHARGED");
		break;
	case MSG_ID_BATTERY_SERVICE_LEVEL_UPDATED_EVT:
		cu_print(
			"BT_TEST INFO CLIENT:  EVENT SOC UPDATED \t%d[%%]\n",
			((battery_service_listen_evt_rsp_msg_t *)msg)->
			battery_service_evt_content_rsp_msg.bat_soc);
		break;
	case MSG_ID_BATTERY_SERVICE_LEVEL_SHUTDOWN_EVT:
		cu_print("BT_TEST INFO CLIENT::  SYSTEM NEED TO SHUTDOWN\n");
		break;
	case MSG_ID_BATTERY_SERVICE_CHARGER_DISCONNECTED_EVT:
		break;
	case MSG_ID_BATTERY_SERVICE_TEMPERATURE_CRITICAL_EVT:
		cu_print(
			"BT_TEST INFO CLIENT::  SOC TEMPERATURE CRITICAL \t%dC\n",
			((battery_service_listen_evt_rsp_msg_t *)msg)->
			battery_service_evt_content_rsp_msg.temp_soc);
		break;
	case MSG_ID_BATTERY_SERVICE_TEMPERATURE_SHUTDOWN_EVT:
		cu_print(
			"BT_TEST INFO CLIENT::  SYSTEM NEED TO SHUTDOWN OVER TEMPERATURE PARAMETER\n");
		break;
	default:
		break;
	}
	cfw_msg_free(msg);
}

void bs_test_threshold(void)
{
	uint8_t low_level_alarm;
	uint8_t critical_level_alarm;

	batt_fail = false;
	battery_service_get_info(test_bs_client_conn,
				 BATTERY_DATA_LOW_LEVEL_ALARM,
				 "GET_LOW_LVL");
	SRV_WAIT(batt_fail == false, TIMEOUT);
	CU_ASSERT("Test for get low level failed", batt_fail == true);

	batt_fail = false;
	low_level_alarm = g_low_level_alarm + 1;
	battery_service_set_level_alarm_thr(test_bs_client_conn,
					    low_level_alarm,
					    BATTERY_LOW_LEVEL_ALARM,
					    "SET_LOW_LVL");
	SRV_WAIT(batt_fail == false, TIMEOUT);
	CU_ASSERT("Test for set low level failed", batt_fail == true);

	batt_fail = false;
	battery_service_get_info(test_bs_client_conn,
				 BATTERY_DATA_CRITICAL_LEVEL_ALARM,
				 "GET_CRITICAL_LVL");
	SRV_WAIT(batt_fail == false, TIMEOUT);
	CU_ASSERT("Test for get critical level failed", batt_fail == true);

	batt_fail = false;
	critical_level_alarm = g_critical_level_alarm + 1;
	battery_service_set_level_alarm_thr(test_bs_client_conn,
					    critical_level_alarm,
					    BATTERY_CRITICAL_LEVEL_ALARM,
					    "SET_CRITICAL_LVL");
	SRV_WAIT(batt_fail == false, TIMEOUT);
	CU_ASSERT("Test for set critical level failed", batt_fail == true);

	batt_fail = false;
	battery_service_get_info(test_bs_client_conn,
				 BATTERY_DATA_LOW_LEVEL_ALARM,
				 "GET_LOW_LVL");
	SRV_WAIT(batt_fail == false, TIMEOUT);
	CU_ASSERT("Test for get low level failed",
		  g_low_level_alarm == low_level_alarm);

	batt_fail = false;
	battery_service_get_info(test_bs_client_conn,
				 BATTERY_DATA_CRITICAL_LEVEL_ALARM,
				 "GET_CRITICAL_LVL");

	SRV_WAIT(batt_fail == false, TIMEOUT);
	CU_ASSERT("Test for get critical level failed",
		  g_critical_level_alarm == critical_level_alarm);
}

/**@brief Battery Service tests.
 *
 * @details The function upon battery service open,
 *              tests one by one all battery service apis and events
 * @param[in]   batt_svc_queue: queue of messages.
 * @return      none.
 */
void battery_service_test(void)
{
	cfw_client_t *battery_client =
		cfw_client_init(get_test_queue(), battery_service_test_handle,
				"Client 1");

	SRV_WAIT(!cfw_service_registered(
			 BATTERY_SERVICE_ID), REGISTRATION_TIMEOUT);
	CU_ASSERT("batterie_service not registered",
		  cfw_service_registered(BATTERY_SERVICE_ID));

	cfw_open_service_conn(battery_client, BATTERY_SERVICE_ID,
			      "Battery Test Client");
	SRV_WAIT((battery_init_done == false), OS_WAIT_FOREVER);
	CU_ASSERT("Unable to open batterie_service", battery_init_done == true);

	cu_print("##################################################\n");
	cu_print("# Purpose of battery service tests :             #\n");
	cu_print("#            Launch voltage request              #\n");
	cu_print("#            Launch temperature request          #\n");
	cu_print("#            Launch update soc request           #\n");
	cu_print("#            Launch get soc request              #\n");
	cu_print("#            Set alarm threshold                 #\n");
	cu_print("#            Set critical threshold              #\n");
	cu_print("##################################################\n");

	batt_fail = false;
	battery_service_get_info(test_bs_client_conn, BATTERY_DATA_VBATT,
				 "GET_VOL");
	SRV_WAIT(batt_fail == false, TIMEOUT);
	CU_ASSERT("Test for voltage request failed", batt_fail == true);

	batt_fail = false;
	battery_service_get_info(test_bs_client_conn, BATTERY_DATA_TEMPERATURE,
				 "GET_TEMP");
	SRV_WAIT(batt_fail == false, TIMEOUT);
	CU_ASSERT("Test for temperature request failed", batt_fail == true);

	batt_fail = false;
	battery_service_get_info(test_bs_client_conn, BATTERY_DATA_LEVEL,
				 "GET_SOC");
	SRV_WAIT(batt_fail == false, TIMEOUT);
	CU_ASSERT("Test for get soc request failed", batt_fail == true);

	bs_test_threshold();
}
