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

#ifndef _BATTERY_SERVICE_H_
#define _BATTERY_SERVICE_H_

#include "cfw/cfw.h"

#include "services/services_ids.h"

#include <stdint.h>

/**
 * @defgroup battery_service Battery Service
 * Handle the multiple functionalities of Battery Service
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt>\#include "services/battery_service/battery_service.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>framework/src/services/battery_service</tt>
 * <tr><th><b>Config flag</b> <td><tt>SERVICES_QUARK_SE_BATTERY, SERVICES_QUARK_SE_BATTERY_IMPL, and more in Kconfig</tt>
 * <tr><th><b>Service Id</b>  <td><tt>BATTERY_SERVICE_ID</tt>
 * </table>
 *
 * @ingroup services
 */

/**
 * @defgroup battery_service_api  Battery Services API
 * Defines the interface for Battery Services
 * @ingroup battery_service
 * @{
 */

/*
 * @enum battery_status_t
 * Battery error list
 */
typedef enum {
	BATTERY_STATUS_SUCCESS = 0,             /*!< Operation succeed */
	BATTERY_STATUS_PENDING,                 /*!< Operation is pending */
	BATTERY_STATUS_IN_USE,                  /*!< Operation already in use */
	BATTERY_STATUS_NOT_IMPLEMENTED,         /*!< Functionality not implemented */
	BATTERY_STATUS_ERROR_ADC_SERVICE,       /*!< ADC service does not function properly */
	BATTERY_STATUS_ERROR_VALUE_OUT_OF_RANGE, /*!< Out of range value Error */
	BATTERY_STATUS_ERROR_IPC,               /*!< Internal Error */
	BATTERY_STATUS_ERROR_FUEL_GAUGE,        /*!< Fuel Gauging Error */
	BATTERY_STATUS_ERROR_NO_MEMORY,         /*!< Memory Error */
	BATTERY_STATUS_ERROR                    /*!< Generic Error */
} battery_status_t;

typedef enum {
	BATTERY_DATA_LEVEL = 0,                 /*!< Getting Battery State Of Charge */
	BATTERY_DATA_STATUS,                    /*!< Getting Battery status (CHARGING, DISCHARGING, MAINTENANCE) */
	BATTERY_DATA_VBATT,                     /*!< Getting Battery Voltage */
	BATTERY_DATA_TEMPERATURE,               /*!< Getting Battery Temperature */
	BATTERY_DATA_CHARGE_CYCLE,              /*!< Getting number of Charging cycle */
	BATTERY_DATA_CHARGER_STATUS,            /*!< Getting Charger status (CONNECTED, NOT CONNECTED)*/
	BATTERY_DATA_CHARGER_TYPE,              /*!< Getting Charger type (USB, QI)*/
	BATTERY_DATA_MEASURE_INTERVAL,          /*!< Setting the interval between 2 ADC measurement*/
	BATTERY_DATA_LOW_LEVEL_ALARM,           /*!< Getting Low level alarm threshold */
	BATTERY_DATA_CRITICAL_LEVEL_ALARM       /*!< Getting Critical level alarm threshold */
} battery_data_info_t;

/**
 * @enum battery_charger_src_t
 * Battery charging sources
 */
typedef enum {
	CHARGING_NONE = 0,
	CHARGING_USB,
	CHARGING_WIRELESS,
	CHARGING_DC,
	CHARGING_UNKNOWN
} battery_charger_src_t;

/**
 * @enum battery_cfg_type_t
 * Type of configurable measurement period
 */
typedef enum {
	CFG_VOLTAGE = 0,
	CFG_TEMPERATURE,
} battery_cfg_type_t;

/**
 * @enum battery_alarm_type_t
 * Type of configurable alarm
 */
typedef enum {
	BATTERY_LOW_LEVEL_ALARM = 0,
	BATTERY_CRITICAL_LEVEL_ALARM,
	BATTERY_SHUTDOWN_LEVEL_ALARM,
} battery_alarm_type_t;

/** Message ID Response offset */
#define MSG_ID_BATTERY_SERVICE_RSP                            (	\
		MSG_ID_BATT_SERVICE_BASE + 0x40)
/** Message ID Events offset */
#define MSG_ID_BATTERY_SERVICE_EVT                            (	\
		MSG_ID_BATT_SERVICE_BASE + 0x80)

/** Message ID for State of Charge Response */
#define MSG_ID_BATTERY_SERVICE_GET_BATTERY_INFO_RSP            ( \
		MSG_ID_BATTERY_SERVICE_RSP + 0x02)
/** Message ID for Setting Low Level Alarm Response */
#define MSG_ID_BATTERY_SERVICE_SET_LOW_LEVEL_ALARM_RSP         ( \
		MSG_ID_BATTERY_SERVICE_RSP + 0x03)
/** Message ID for Setting Critical Level Alarm Response */
#define MSG_ID_BATTERY_SERVICE_SET_CRITICAL_LEVEL_ALARM_RSP    ( \
		MSG_ID_BATTERY_SERVICE_RSP + 0x04)
/** Message ID for Setting Critical Level Alarm Response */
#define MSG_ID_BATTERY_SERVICE_SET_SHUTDOWN_LEVEL_ALARM_RSP    ( \
		MSG_ID_BATTERY_SERVICE_RSP + 0x05)
/** Message ID for Battery Charging State Response */
#define MSG_ID_BATTERY_SERVICE_SET_MEASURE_INTERVAL_RSP        ( \
		MSG_ID_BATTERY_SERVICE_RSP + 0x06)

/** Message ID for Battery Fully Charged Event */
#define MSG_ID_BATTERY_SERVICE_FULLY_CHARGED_EVT               ( \
		MSG_ID_BATTERY_SERVICE_EVT + 0x01)
/** Message ID for Low level Reached Event */
#define MSG_ID_BATTERY_SERVICE_LEVEL_LOW_EVT                   ( \
		MSG_ID_BATTERY_SERVICE_EVT + 0x02)
/** Message ID for Critical level Reached Event */
#define MSG_ID_BATTERY_SERVICE_LEVEL_CRITICAL_EVT              ( \
		MSG_ID_BATTERY_SERVICE_EVT + 0x03)
/** Message ID for Battery level changed Event */
#define MSG_ID_BATTERY_SERVICE_LEVEL_UPDATED_EVT               ( \
		MSG_ID_BATTERY_SERVICE_EVT + 0x04)
/** Message ID for Charger connected Event */
#define MSG_ID_BATTERY_SERVICE_CHARGER_CONNECTED_EVT           ( \
		MSG_ID_BATTERY_SERVICE_EVT + 0x05)
/** Message ID for Charger disconnected Event */
#define MSG_ID_BATTERY_SERVICE_CHARGER_DISCONNECTED_EVT        ( \
		MSG_ID_BATTERY_SERVICE_EVT + 0x06)
/** Message ID for Shutdown Level Reached Event */
#define MSG_ID_BATTERY_SERVICE_LEVEL_SHUTDOWN_EVT              ( \
		MSG_ID_BATTERY_SERVICE_EVT + 0x07)
/** Message ID for critical temperature Reached Event */
#define MSG_ID_BATTERY_SERVICE_TEMPERATURE_CRITICAL_EVT        ( \
		MSG_ID_BATTERY_SERVICE_EVT + 0x08)
/** Message ID for Shutdown Temperature Reached Event */
#define MSG_ID_BATTERY_SERVICE_TEMPERATURE_SHUTDOWN_EVT        ( \
		MSG_ID_BATTERY_SERVICE_EVT + 0x09)

/**
 * Alarm message structure response
 * for battery_service_get_info function
 */
typedef struct {
	uint32_t status;
	uint8_t level_alarm;
} battery_service_level_alarm_rsp_msg_t;

/**
 * State Of Charge message structure response
 * for battery_service_get_info function
 */
typedef struct {
	uint32_t status;
	uint8_t bat_soc;
} battery_service_soc_rsp_msg_t;

/**
 * Charging source message structure response
 * for battery_service_get_info function
 */
typedef struct {
	uint32_t status;
	battery_charger_src_t charging_source;
} battery_service_charging_source_rsp_msg_t;

/**
 * Battery charging state message structure response
 * for battery_service_get_info function
 */
typedef struct {
	uint32_t status;
	bool is_charging;
} battery_service_charging_rsp_msg_t;

/**
 * Battery presence message structure response
 * for battery_service_get_info function
 */
typedef struct battery_service_present_rsp_msg {
	struct cfw_message header;
	bool is_battery_present;
} battery_service_present_rsp_msg_t;

/**
 * Charger connection message structure response
 * for battery_service_get_info function
 */
typedef struct {
	uint32_t status;
	bool is_charger_connected;
} battery_service_charger_connected_rsp_msg_t;

/**
 * Battery voltage message structure response
 * for battery_service_get_info function
 */
typedef struct {
	uint32_t status;
	uint16_t bat_vol;
} battery_service_voltage_rsp_msg_t;

/**
 * Battery temperature message structure response
 * for battery_service_get_info function
 */
typedef struct {
	uint32_t status;
	int16_t bat_temp;
} battery_service_temperature_rsp_msg_t;

/**
 * Charge Cycle message structure response
 * for battery_service_get_info function
 */
typedef struct {
	uint32_t status;
	uint16_t bat_charge_cycle;
} battery_service_charge_cycle_rsp_msg_t;

/**
 * Battery status response structure message
 * for \ref battery_service_get_info function
 */
typedef struct battery_service_info_request_rsp_msg {
	struct cfw_message header;
	battery_data_info_t batt_info_id;
	union {
		battery_service_level_alarm_rsp_msg_t battery_low_level_alarm;
		battery_service_level_alarm_rsp_msg_t
			battery_critical_level_alarm;
		battery_service_soc_rsp_msg_t battery_soc;
		battery_service_charging_source_rsp_msg_t
			battery_charging_source;
		battery_service_charging_rsp_msg_t battery_charging;
		battery_service_present_rsp_msg_t battery_present;
		battery_service_charger_connected_rsp_msg_t
			battery_charger_connected;
		battery_service_voltage_rsp_msg_t battery_voltage;
		battery_service_temperature_rsp_msg_t battery_temperature;
		battery_service_charge_cycle_rsp_msg_t battery_charge_cycle;
	};
	int status;
} battery_service_info_request_rsp_msg_t;

/**
 * Event data message structure
 */
typedef struct battery_service_evt_content_rsp_msg {
	uint8_t bat_soc;        /* Battery_state_of_charge */
	int8_t temp_soc;        /* Temperature value */
} battery_service_evt_content_rsp_msg_t;

/**
 * Event message structure
 */
typedef struct {
	struct cfw_message header;
	battery_service_evt_content_rsp_msg_t
		battery_service_evt_content_rsp_msg;
} battery_service_listen_evt_rsp_msg_t;

/**
 * Configuration of measurement period
 */
struct battery_service_period_cfg_msg {
	battery_cfg_type_t cfg_type;
	uint16_t new_period_ms;
};

/**
 * Get the latest battery Information.
 * @param[in] conn Service connection.
 * @param[in] batt_info_id Info id
 * @param[in] priv Private data pointer that will be passed in the response. NULL if not used.
 *
 * @b Response: _MSG_ID_BATTERY_SERVICE_GET_BATTERY_INFO_RSP_ with attached \ref battery_service_info_request_rsp_msg_t
 */
void battery_service_get_info(cfw_service_conn_t *	conn,
			      battery_data_info_t	batt_info_id,
			      void *			priv);

/**
 * Set the battery level of alarm.
 * @param[in] conn Service connection.
 * @param[in] level_alarm Level of the alarm.
 * @param[in] alarm_type: Type of alarm needed
 * @param[in] priv Private data pointer that will be passed in the response. NULL if not used.
 *
 * @b Response: _MSG_ID_BATTERY_SERVICE_SET_xxxx_LEVEL_ALARM_RSP_
 */
void battery_service_set_level_alarm_thr(cfw_service_conn_t *	conn,
					 uint8_t		level_alarm,
					 battery_alarm_type_t	alarm_type,
					 void *			priv);
/**
 * Set ADC measure interval.
 * @param[in] conn Service connection.
 * @param[in] period_cfg Period configuration.
 * @param[in] priv Private data pointer that will be passed in the response. NULL if not used.
 *
 * @b Response: _MSG_ID_BATTERY_SERVICE_SET_MEASURE_INTERVAL_RSP_
 */
void battery_service_set_measure_interval(
	cfw_service_conn_t *			conn,
	struct battery_service_period_cfg_msg * period_cfg,
	void *					priv);
/** @} */

#endif /* _BATTERY_SERVICE_H_ */
