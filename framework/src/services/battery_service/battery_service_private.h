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

#ifndef _BATTERY_SERVICE_PRIVATE_H_
#define _BATTERY_SERVICE_PRIVATE_H_

#include "services/battery_service/battery_service.h"
#include "infra/log.h"

DEFINE_LOG_MODULE(LOG_MODULE_CH, "CHGR")

/* Define for Charger event call back function */
enum e_bs_ch_event {
	BS_CH_EVENT_INIT_DONE = 0,
	BS_CH_EVENT_CHARGE_COMPLETE = 1,
	BS_CH_EVENT_CHARGER_CONNECTED = 2,
	BS_CH_EVENT_CHARGER_DISCONNECTED = 3
};

/** message ID for Setting Low Level Alarm */
#define MSG_ID_BATTERY_SERVICE_GET_BATTERY_INFO_REQ                     (~0x40 \
									 & \
									 MSG_ID_BATTERY_SERVICE_GET_BATTERY_INFO_RSP)
/** message ID for Setting Low Level Alarm */
#define MSG_ID_BATTERY_SERVICE_SET_LOW_LEVEL_ALARM_REQ                  (~0x40 \
									 & \
									 MSG_ID_BATTERY_SERVICE_SET_LOW_LEVEL_ALARM_RSP)
/** message ID for Charging source Request */
#define MSG_ID_BATTERY_SERVICE_SET_CRITICAL_LEVEL_ALARM_REQ     (~0x40 & \
								 MSG_ID_BATTERY_SERVICE_SET_CRITICAL_LEVEL_ALARM_RSP)
/** message ID for Battery Charging State Request */
#define MSG_ID_BATTERY_SERVICE_SET_SHUTDOWN_LEVEL_ALARM_REQ     (~0x40 & \
								 MSG_ID_BATTERY_SERVICE_SET_SHUTDOWN_LEVEL_ALARM_RSP)
/** message ID for Charge Cycle Request */
#define MSG_ID_BATTERY_SERVICE_SET_MEASURE_INTERVAL_REQ         (~0x40 & \
								 MSG_ID_BATTERY_SERVICE_SET_MEASURE_INTERVAL_RSP)

/**
 * @brief Setting alarm message structure request
 */
typedef struct battery_set_level_alarm_msg {
	struct cfw_message header;
	uint8_t level_alarm;
} battery_set_level_alarm_msg_t;

/**
 * @brief Setting alarm message structure response
 */
typedef struct battery_set_level_alarm_rsp_msg {
	struct cfw_message header;
	int status;
} battery_set_level_alarm_rsp_msg_t;

/**
 * ADC measure interval configuration message structure
 */
typedef struct battery_set_measure_interval_msg {
	struct cfw_message header;
	struct battery_service_period_cfg_msg period_cfg;
} battery_set_measure_interval_msg_t;


/**
 * battery status request structure message
 */
typedef struct battery_status_msg {
	struct cfw_message header;
	battery_data_info_t batt_info_id;
} battery_status_msg_t;

#endif  /* _BATTERY_SERVICE_PRIVATE_H_ */
