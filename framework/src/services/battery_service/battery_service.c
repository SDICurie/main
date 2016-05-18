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

#include <infra/log.h>
#include "cfw/cfw_service.h"
#include "util/assert.h"
#include "services/battery_service/battery_service.h"

#include "battery_service_private.h"

/****************************************************************************************
*********************** SERVICE API IMPLEMENTATION **************************************
****************************************************************************************/
/**
 * @brief Battery Service: get the latest battery Information.
 * @param[in] c: service connection.
 * @param[in] batt_info_id: info id
 * @param[in] priv: additional data. NULL if not used.
 */
void battery_service_get_info(cfw_service_conn_t *	c,
			      battery_data_info_t	batt_info_id,
			      void *			priv)
{
	struct cfw_message *p_msg = cfw_alloc_message_for_service(
		c,
		MSG_ID_BATTERY_SERVICE_GET_BATTERY_INFO_REQ,
		sizeof
		(
			battery_status_msg_t),
		priv);

	assert(p_msg);
	((battery_status_msg_t *)p_msg)->batt_info_id = batt_info_id;
	cfw_send_message(p_msg);
}

/**
 * @brief Battery Service: set the battery level of alarm.
 * @param[in] c: service connection.
 * @param[in] low_level_alarm: level of the alarm.
 * @param[in] alarm_type: type of alarm needed
 * @param[in] priv: additional data. NULL if not used.
 */
void battery_service_set_level_alarm_thr(cfw_service_conn_t *	c,
					 uint8_t		level_alarm,
					 battery_alarm_type_t	alarm_type,
					 void *			priv)
{
	uint32_t message_id = 0;

	switch (alarm_type) {
	case BATTERY_LOW_LEVEL_ALARM: message_id =
		MSG_ID_BATTERY_SERVICE_SET_LOW_LEVEL_ALARM_REQ; break;
	case BATTERY_CRITICAL_LEVEL_ALARM: message_id =
		MSG_ID_BATTERY_SERVICE_SET_CRITICAL_LEVEL_ALARM_REQ; break;
	case BATTERY_SHUTDOWN_LEVEL_ALARM: message_id =
		MSG_ID_BATTERY_SERVICE_SET_SHUTDOWN_LEVEL_ALARM_REQ; break;
	default: assert(alarm_type);
	}
	if (level_alarm <= 100) {
		struct cfw_message *p_msg = cfw_alloc_message_for_service(
			c,
			message_id,
			sizeof
			(
				battery_set_level_alarm_msg_t),
			priv);
		assert(p_msg);
		((battery_set_level_alarm_msg_t *)p_msg)->level_alarm =
			level_alarm;
		cfw_send_message(p_msg);
	}
}

/**
 * @brief Battery Service: set ADC measure interval.
 * @param[in] c: service connection.
 * @param[in] period_cfg: contains configuration.
 * @param[in] priv: additional data. NULL if not used.
 */
void battery_service_set_measure_interval(
	cfw_service_conn_t *			c,
	struct battery_service_period_cfg_msg * period_cfg,
	void *					priv)
{
	struct cfw_message *p_msg = cfw_alloc_message_for_service(
		c,
		MSG_ID_BATTERY_SERVICE_SET_MEASURE_INTERVAL_REQ,
		sizeof
		(
			battery_set_measure_interval_msg_t),
		priv);

	assert(p_msg && period_cfg);
	((battery_set_measure_interval_msg_t *)p_msg)->period_cfg.new_period_ms
		= period_cfg->new_period_ms;
	((battery_set_measure_interval_msg_t *)p_msg)->period_cfg.cfg_type =
		period_cfg->cfg_type;
	cfw_send_message(p_msg);
}
