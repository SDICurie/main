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

#include <stdbool.h>
#include <string.h>

#include "os/os.h"
#include "drivers/data_type.h"
#include "cfw/cfw_service.h"
#include "infra/log.h"
#include "infra/system_events.h"
#include "util/assert.h"
#include "services/service_queue.h"
#include "services/services_ids.h"
#include "services/battery_service/battery_service.h"

#include "battery_service_private.h"
#include "charging_sm.h"
#include "fuel_gauge_api.h"
#include "battery_property.h"

#define BATT_S_SOC_MAX          100
#define BATT_GRANULARITY        5

/**********************************************************
************** Local definitions  ************************
**********************************************************/
static uint8_t batt_prev_level = 101;

void bs_client_connected(conn_handle_t *instance);
void bs_client_disconnected(conn_handle_t *instance);
static void bs_service_shutdown(service_t *svc, struct cfw_message *msg);
static void bs_properties_status(bp_status_t bp_status);

static service_t battery_service = {
	.service_id = BATTERY_SERVICE_ID,
	.client_connected = bs_client_connected,
	.client_disconnected = bs_client_disconnected,
	.shutdown_request = bs_service_shutdown,
};

DEFINE_LOG_MODULE(LOG_MODULE_BS, "BATS")

/*******************************************************************************
 *********************** SERVICE CONTROL IMPLEMENTATION ************************
 ******************************************************************************/

/*******************************************************************************
 *********************** SERVICE IMPLEMENTATION ********************************
 ******************************************************************************/

/**@brief Change the battery level for the granularity defined.
 * @param[in]  pointer of battery level value
 */
static void battery_fg_filter(uint8_t *batt_level)
{
	uint8_t bat_temp = BATT_S_SOC_MAX + 1;

	bat_temp = *batt_level % BATT_GRANULARITY;
	if (bat_temp > (BATT_GRANULARITY / 2))
		*batt_level = *batt_level + (BATT_GRANULARITY - (bat_temp));
	else
		*batt_level = *batt_level - (bat_temp);
}

/**@brief Callback to manage results of requests for getting the battery SOC.
 * @param[in]  msg  Request message.
 */
static void handle_bs_get_soc(battery_service_soc_rsp_msg_t *battery_soc)
{
	uint8_t bat_soc = BATT_S_SOC_MAX + 1;

	bat_soc = fg_get_battery_soc();
	battery_fg_filter(&bat_soc);

	if (BATT_S_SOC_MAX >= bat_soc) {
		((battery_service_soc_rsp_msg_t *)battery_soc)->bat_soc =
			bat_soc;
		((battery_service_soc_rsp_msg_t *)battery_soc)->status =
			BATTERY_STATUS_SUCCESS;
	} else {
		((battery_service_soc_rsp_msg_t *)battery_soc)->status =
			BATTERY_STATUS_ERROR_VALUE_OUT_OF_RANGE;
	}
	return;
}

/**
 * @brief Fuel Gauge get Battery Cell-Pack Voltage.
 * @param[in,out] batt_voltage Battery Cell-Pack Voltage [mV].
 */
static void handle_bs_get_voltage(
	battery_service_voltage_rsp_msg_t *battery_voltage)
{
	fg_status_t fg_status = FG_STATUS_ERROR_ADC_SERVICE;
	uint16_t batt_voltage = 0;

	fg_status = fg_get_battery_voltage(&batt_voltage);

	switch (fg_status) {
	case FG_STATUS_SUCCESS:
		battery_voltage->status = BATTERY_STATUS_SUCCESS;
		battery_voltage->bat_vol = batt_voltage;
		break;
	case FG_STATUS_ERROR_ADC_SERVICE:
		battery_voltage->status = BATTERY_STATUS_ERROR_ADC_SERVICE;
		break;
	default:
		battery_voltage->status = BATTERY_STATUS_ERROR;
		break;
	}
}

/**@brief Callback to manage results of requests of getting the battery low level alarm threshold.
 * @param[in]  msg  Request message.
 */
static void handle_bs_get_low_level_alarm(
	battery_service_level_alarm_rsp_msg_t *battery_low_level_alarm)
{
	uint8_t low_level_alarm_threshold = 0;

	if (FG_STATUS_SUCCESS ==
	    fg_get_low_level_alarm_threshold(&low_level_alarm_threshold)) {
		battery_low_level_alarm->status = BATTERY_STATUS_SUCCESS;
		battery_low_level_alarm->level_alarm =
			low_level_alarm_threshold;
	} else {
		battery_low_level_alarm->status =
			BATTERY_STATUS_ERROR_FUEL_GAUGE;
	}
}

/**@brief Callback to manage results of requests of getting the battery critical level alarm threshold.
 * @param[in]  msg  Request message.
 */
static void handle_bs_get_critical_level_alarm(
	battery_service_level_alarm_rsp_msg_t *battery_critical_level_alarm)
{
	uint8_t critical_level_alarm_threshold = 0;

	if (FG_STATUS_SUCCESS ==
	    fg_get_critical_level_alarm_threshold(&
						  critical_level_alarm_threshold))
	{
		battery_critical_level_alarm->status = BATTERY_STATUS_SUCCESS;
		battery_critical_level_alarm->level_alarm =
			critical_level_alarm_threshold;
	} else {
		battery_critical_level_alarm->status =
			BATTERY_STATUS_ERROR_FUEL_GAUGE;
	}
	return;
}


/**@brief Callback to manage results of requests of getting the battery temperature.
 * @param[in]  msg  Request message.
 */
static void handle_bs_get_temperature(
	battery_service_temperature_rsp_msg_t *battery_temperature)
{
	fg_status_t fg_status = FG_STATUS_ERROR_ADC_SERVICE;
	int16_t temp = 0;

	fg_status = fg_get_battery_temperature(&temp);

	switch (fg_status) {
	case FG_STATUS_SUCCESS:
		battery_temperature->status = BATTERY_STATUS_SUCCESS;
		battery_temperature->bat_temp = temp;
		break;
	case FG_STATUS_ERROR_ADC_SERVICE:
		battery_temperature->status = BATTERY_STATUS_ERROR_ADC_SERVICE;
		break;
	default:
		battery_temperature->status = BATTERY_STATUS_ERROR;
		break;
	}
	return;
}

/**@brief Callback to manage results of requests of getting battery cycle.
 * @param[in]  msg  Request message.
 */
static void handle_bs_get_charge_cycle(
	battery_service_charge_cycle_rsp_msg_t *battery_charge_cycle)
{
	fg_status_t fg_status = FG_STATUS_ERROR_ADC_SERVICE;
	uint16_t bat_charge_cycle = 0;

	fg_status = fg_get_charge_cycle(&bat_charge_cycle);
	switch (fg_status) {
	case FG_STATUS_SUCCESS:
		battery_charge_cycle->status = BATTERY_STATUS_SUCCESS;
		battery_charge_cycle->bat_charge_cycle = bat_charge_cycle;
		break;
	default:
		battery_charge_cycle->status = BATTERY_STATUS_ERROR;
		break;
	}
	return;
}

/**@brief Callback to manage results of requests for getting the battery information.
 * @param[in]  msg  Request message.
 */
static void handle_battery_service_get_info(struct cfw_message *msg)
{
	battery_service_info_request_rsp_msg_t *resp =
		(battery_service_info_request_rsp_msg_t *)cfw_alloc_rsp_msg(
			msg,
			MSG_ID_BATTERY_SERVICE_GET_BATTERY_INFO_RSP,
			sizeof(
				*resp));

	switch (((battery_status_msg_t *)msg)->batt_info_id) {
	case BATTERY_DATA_LEVEL:
		handle_bs_get_soc(&resp->battery_soc);
		resp->status = resp->battery_soc.status;
		break;
	case BATTERY_DATA_STATUS:
		resp->battery_charging.is_charging = charging_sm_is_charging();
		resp->status = resp->battery_charging.status =
				       BATTERY_STATUS_SUCCESS;
		break;
	case BATTERY_DATA_VBATT:
		handle_bs_get_voltage(&resp->battery_voltage);
		resp->status = resp->battery_voltage.status;
		break;
	case BATTERY_DATA_TEMPERATURE:
		handle_bs_get_temperature(&resp->battery_temperature);
		resp->status = resp->battery_temperature.status;
		break;
	case BATTERY_DATA_CHARGE_CYCLE:
		handle_bs_get_charge_cycle(&resp->battery_charge_cycle);
		resp->status = resp->battery_charge_cycle.status;
		break;
	case BATTERY_DATA_CHARGER_STATUS:
		resp->battery_charger_connected.is_charger_connected =
			charging_sm_is_charger_connected();
		resp->status = resp->battery_charger_connected.status =
				       BATTERY_STATUS_SUCCESS;
		break;
	case BATTERY_DATA_CHARGER_TYPE:
		resp->battery_charging_source.charging_source =
			charging_sm_get_source();
		resp->status = resp->battery_charging_source.status =
				       BATTERY_STATUS_SUCCESS;
		break;
	case BATTERY_DATA_LOW_LEVEL_ALARM:
		handle_bs_get_low_level_alarm(&resp->battery_low_level_alarm);
		resp->status = resp->battery_low_level_alarm.status;
		break;
	case BATTERY_DATA_CRITICAL_LEVEL_ALARM:
		handle_bs_get_critical_level_alarm(
			&resp->battery_critical_level_alarm);
		resp->status = resp->battery_critical_level_alarm.status;
		break;
	default:
		break;
	}

	resp->batt_info_id = ((battery_status_msg_t *)msg)->batt_info_id;
	cfw_send_message(resp);
	return;
}

/**@brief Callback to manage results of requests of setting the battery level alarm threshold.
 * @param[in]  msg    Request message.
 *             msg_id Type of level alarm threshold.
 */
static void handle_bs_set_level_alarm(struct cfw_message *msg, int msg_id)
{
	int rsp_id = 0;
	int level_alarm_threshold = 0;

	fg_status_t (*fg_set_alarm)(uint8_t) = NULL;
	battery_set_level_alarm_rsp_msg_t *resp;

	switch (msg_id) {
	case MSG_ID_BATTERY_SERVICE_SET_LOW_LEVEL_ALARM_REQ:
		rsp_id = MSG_ID_BATTERY_SERVICE_SET_LOW_LEVEL_ALARM_RSP;
		fg_set_alarm = &fg_set_low_level_alarm_threshold;
		break;
	case MSG_ID_BATTERY_SERVICE_SET_CRITICAL_LEVEL_ALARM_REQ:
		rsp_id = MSG_ID_BATTERY_SERVICE_SET_CRITICAL_LEVEL_ALARM_RSP;
		fg_set_alarm = &fg_set_critical_level_alarm_threshold;
		break;
	default:
		assert(0);
		break;
	}
	level_alarm_threshold =
		((battery_set_level_alarm_msg_t *)msg)->level_alarm;
	resp = (battery_set_level_alarm_rsp_msg_t *)cfw_alloc_rsp_msg(
		msg, rsp_id, sizeof(*resp));
	resp->status = BATTERY_STATUS_ERROR_FUEL_GAUGE;
	if (FG_STATUS_SUCCESS == fg_set_alarm(level_alarm_threshold))
		resp->status = BATTERY_STATUS_SUCCESS;
	cfw_send_message(resp);

	return;
}

/**@brief Callback to manage results of requests of setting ADC measure interval
 * @param[in]  msg  Request message.
 */
static void handle_battery_set_measure_interval(struct cfw_message *msg)
{
	uint16_t new_period_ms = 0;
	battery_cfg_type_t cfg_period;

	struct cfw_message *resp;

	resp = cfw_alloc_rsp_msg(
		msg,
		MSG_ID_BATTERY_SERVICE_SET_MEASURE_INTERVAL_RSP,
		sizeof(*resp));
	new_period_ms =
		((battery_set_measure_interval_msg_t *)msg)->period_cfg.
		new_period_ms;
	cfg_period =
		((battery_set_measure_interval_msg_t *)msg)->period_cfg.
		cfg_type;
	if (cfg_period == CFG_TEMPERATURE) {
		fg_set_temp_interval(new_period_ms);
	} else if (cfg_period == CFG_VOLTAGE) {
		fg_set_voltage_interval(new_period_ms);
	}
	cfw_send_message(resp);

	return;
}

/**@brief Sending an event
 * @param[in]  id  Message identifier
 * @param[in] battery_service_evt_content_rsp_msg Content of battery event
 */
static void bs_send_evt_msg(
	uint16_t id,
	battery_service_evt_content_rsp_msg_t *
	battery_service_evt_content_rsp_msg)
{
	battery_service_listen_evt_rsp_msg_t *evt = NULL;

	evt = (battery_service_listen_evt_rsp_msg_t *)cfw_alloc_evt_msg(
		&battery_service, id,
		sizeof(
			battery_service_listen_evt_rsp_msg_t));

	if ((NULL != evt) &&
	    (NULL != battery_service_evt_content_rsp_msg)) {
		memcpy(&evt->battery_service_evt_content_rsp_msg,
		       battery_service_evt_content_rsp_msg,
		       sizeof(battery_service_evt_content_rsp_msg_t));
	}
	cfw_send_event(&evt->header);
	bfree(evt); /* message has been cloned by cfw_send_event */
}

/*@brief Function called by FG to advertises of any event
 * @param fg_event Event type
 * @param[in] battery_service_evt_content_rsp_msg Content of battery  temperature event
 */
static void bs_evt_from_fg(
	fg_event_t fg_event,
	battery_service_evt_content_rsp_msg_t *
	battery_service_evt_content_rsp_msg)
{
	uint16_t batt_voltage = 0;
	int16_t batt_temp = 0;

	switch (fg_event) {
	case FG_EVT_LOW_LEVEL:
		pr_warning(LOG_MODULE_BS, "[%d%%] BELOW LOW LEVEL",
			   battery_service_evt_content_rsp_msg->bat_soc);
		bs_send_evt_msg(MSG_ID_BATTERY_SERVICE_LEVEL_LOW_EVT,
				battery_service_evt_content_rsp_msg);
		break;
	case FG_EVT_CRITICAL_LEVEL:
		pr_warning(LOG_MODULE_BS, "[%d%%] BELOW CRITICAL LEVEL",
			   battery_service_evt_content_rsp_msg->bat_soc);
		bs_send_evt_msg(MSG_ID_BATTERY_SERVICE_LEVEL_CRITICAL_EVT,
				battery_service_evt_content_rsp_msg);
		break;
	case FG_EVT_SOC_UPDATED:
		fg_get_battery_temperature(&batt_temp);
		fg_get_battery_voltage(&batt_voltage);
		pr_info(LOG_MODULE_BS,
			"Batt Voltage [%dmV] -> SOC[%d%%] Temp [%dC]",
			batt_voltage,
			battery_service_evt_content_rsp_msg->bat_soc,
			batt_temp);
		battery_fg_filter(&battery_service_evt_content_rsp_msg->bat_soc);
		if (battery_service_evt_content_rsp_msg->bat_soc !=
		    batt_prev_level) {
			batt_prev_level =
				battery_service_evt_content_rsp_msg->bat_soc;
			bs_send_evt_msg(
				MSG_ID_BATTERY_SERVICE_LEVEL_UPDATED_EVT,
				battery_service_evt_content_rsp_msg);
#ifdef CONFIG_SYSTEM_EVENTS
			system_event_push_battery(
				SYSTEM_EVENT_BATT_LEVEL,
				battery_service_evt_content_rsp_msg->bat_soc);
#endif
		}
		break;
	case FG_EVT_SHDN_MAND_LEVEL:
		pr_warning(LOG_MODULE_BS, "BELOW SHUTDOWN LEVEL");
		bs_send_evt_msg(MSG_ID_BATTERY_SERVICE_LEVEL_SHUTDOWN_EVT,
				battery_service_evt_content_rsp_msg);
		break;
	case FG_EVT_CRITICAL_TEMP:
		pr_warning(LOG_MODULE_BS, "BEYOND CRITICAL TEMP");
		bs_send_evt_msg(MSG_ID_BATTERY_SERVICE_TEMPERATURE_CRITICAL_EVT,
				battery_service_evt_content_rsp_msg);
		break;
	case FG_EVT_SHDN_MAND_TEMP:
		pr_warning(LOG_MODULE_BS, "BEYOND SHUTDOWN TEMP");
		bs_send_evt_msg(MSG_ID_BATTERY_SERVICE_TEMPERATURE_SHUTDOWN_EVT,
				battery_service_evt_content_rsp_msg);
		break;
	default:
		break;
	}
}
/*@brief Function called by Charger to advertises of any event
 * @param fg_event Event type
 * @param[in] battery_service_evt_content_rsp_msg Content of battery event
 */
static void bs_evt_from_ch(enum e_bs_ch_event ch_event)
{
	switch (ch_event) {
	case BS_CH_EVENT_INIT_DONE:
		battery_properties_init(&bs_properties_status);
		break;
	case BS_CH_EVENT_CHARGE_COMPLETE:
		bs_send_evt_msg(MSG_ID_BATTERY_SERVICE_FULLY_CHARGED_EVT, NULL);
		break;
	case BS_CH_EVENT_CHARGER_CONNECTED:
		bs_send_evt_msg(MSG_ID_BATTERY_SERVICE_CHARGER_CONNECTED_EVT,
				NULL);
		break;
	case BS_CH_EVENT_CHARGER_DISCONNECTED:
		bs_send_evt_msg(MSG_ID_BATTERY_SERVICE_CHARGER_DISCONNECTED_EVT,
				NULL);
		break;
	default:
		break;
	}
}

/**@brief Function to handle requests, responses and events
 * @param[in]  msg  Event message.
 */
static void bs_handle_message(struct cfw_message *msg, void *param)
{
	switch (CFW_MESSAGE_ID(msg)) {
	case MSG_ID_BATTERY_SERVICE_GET_BATTERY_INFO_REQ:
		handle_battery_service_get_info(msg);
		break;
	case MSG_ID_BATTERY_SERVICE_SET_LOW_LEVEL_ALARM_REQ:
	case MSG_ID_BATTERY_SERVICE_SET_CRITICAL_LEVEL_ALARM_REQ:
		handle_bs_set_level_alarm(msg, CFW_MESSAGE_ID(msg));
		break;
	case MSG_ID_BATTERY_SERVICE_SET_MEASURE_INTERVAL_REQ:
		handle_battery_set_measure_interval(msg);
		break;
	default:
		pr_error(LOG_MODULE_BS, "Unexpected message id: %x",
			 CFW_MESSAGE_ID(
				 msg));
		break;
	}
	cfw_msg_free(msg);
}

void bs_client_connected(conn_handle_t *instance)
{
#ifdef DEBUG_BATTERY_SERVICE
	pr_info(LOG_MODULE_BS, "SERVICE: %s ", __func__);
#endif
}

void bs_client_disconnected(conn_handle_t *instance)
{
#ifdef DEBUG_BATTERY_SERVICE
	pr_info(LOG_MODULE_BS, "SERVICE: %s ", __func__);
#endif
}

void bs_service_shutdown(service_t *svc, struct cfw_message *msg)
{
	battery_properties_save(msg);
}

/****************************************************************************************
************************** SERVICE INITIALIZATION **************************************
****************************************************************************************/

static void *queue;

/*
 * @brief Register battery service
 * @remark Called by fuel gauge layer to advertise its end of initialization
 */
static void bs_fuel_gauge_init_done(void)
{
	if (cfw_register_service(queue, &battery_service,
				 bs_handle_message, NULL) == -1)
		pr_error(LOG_MODULE_BS, "Cannot register Battery service");
}

/*
 * @brief Launch fuel gauge initialization
 * @param[in] bp_status status
 * @remark Called by battery properties layer to advertise battery properties status
 */
static void bs_properties_status(bp_status_t bp_status)
{
	fg_event_callback_t fg_event_callback = {};

	fg_event_callback.fg_callback = (fg_callback_t)bs_evt_from_fg;

	if (BP_STATUS_SUCCESS == bp_status) {
		/* Initialize interface with Fuel Gauge */
		fg_init(
			get_service_queue(), &fg_event_callback,
			bs_fuel_gauge_init_done);
	}
}

/**@brief Function to initialize Battery Service.
 *
 * @details Internal variable and structures are initialized, service and
 *          port handles allocated by the framework. Battery service registers
 *          to the framework.
 * @param[in]  batt_svc_queue Queue of messages to be exchanged.
 * @param[in]  service_id Battery service ID.
 */
static void bs_init(int service_id, T_QUEUE batt_svc_queue)
{
	battery_service.service_id = service_id;

	queue = batt_svc_queue;

	if (!charging_sm_init(batt_svc_queue, bs_evt_from_ch))
		pr_error(LOG_MODULE_BS, "Battery service initialization failed");
}

CFW_DECLARE_SERVICE(battery, BATTERY_SERVICE_ID, bs_init);
