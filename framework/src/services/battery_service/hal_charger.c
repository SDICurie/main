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

#include "infra/log.h"
#include "cfw/cfw.h"
#include "os/os.h"
#include "util/assert.h"
#include "machine.h"
#include "drivers/charger/charger_api.h"
#include "util/workqueue.h"
#include "services/battery_service/battery_service.h"

#include "hal_charger.h"
#include "charging_sm.h"
#include "fuel_gauge_api.h"

/* Define call back function for charging_sm_event */
typedef void (*ch_event_fct)(void *charging_sm_event);

#if (CONFIG_CH_SW_EOC != 0)     /* TODO: have to make regression test for this feature */
#define CH_SW_EOC (uint32_t)(CONFIG_CH_SW_EOC * 60000)
#define CH_TM_DELAY_1MIN (60000)        /* Read the SOC every minute */
static T_TIMER eoc_timer;
static void ch_eoc_timer_handler(void *timer_event);
static bool ch_eoc_timer_started = false;
static void ch_start_sw_eoc(void);
static void ch_stop_sw_eoc(void);
#endif

/****************************************************************************************
*********************** LOCAL FUNCTON IMPLEMENTATION ***********************************
****************************************************************************************/

/**@brief Function to send callback function to event received.
 * @param[in]  State of pin
 * @param[in]  Pointer of user callback
 * @return   none.
 */
static void hal_charger_cb(DRV_CHG_EVT charger_status, void *param)
{
	static uint8_t ch_sm_event;
	ch_event_fct ch_call_back_event = param;

	ch_sm_event = charger_status;
	if (charger_status == CHARGING) {
#if (CONFIG_CH_SW_EOC != 0)
		ch_start_sw_eoc();
#endif
		ch_sm_event = CHARGING_START;
	} else {
#if (CONFIG_CH_SW_EOC != 0)
		ch_stop_sw_eoc();
#endif

#ifndef CONFIG_DRV_FAULT_NOTIF_CAPA
		if (charging_sm_is_charger_connected() &&
		    (charger_status != CHARGE_COMPLETE))
			ch_sm_event = CHARGING_DISCHARGE_FAULT;
#endif

#ifndef CONFIG_DRV_EOC_NOTIF_CAPA
		if ((fg_get_battery_soc() >= 95) &&
		    (charger_status != CHARGER_FAULT)
		    && (charging_sm_get_state() == CHARGE))
			ch_sm_event = CHARGING_COMPLETE;
#endif

#ifdef CONFIG_BQ25101H
		/* BQ25101H isn't reset with system,
		 * then we determine if the charger is in charge complete state
		 */
		if ((charging_sm_get_state() == INIT) &&
		    charging_sm_is_charger_connected())
			ch_sm_event = CHARGING_COMPLETE;
#endif
	}
	workqueue_queue_work(ch_call_back_event, &ch_sm_event);
}

#if (CONFIG_CH_SW_EOC != 0)

/**@brief Timer Handler function for End Of Charge detection.
 * @param priv_data private data of the timer, passed at creation
 */
static void ch_eoc_timer_handler(void *priv_data)
{
	static uint8_t event;
	ch_event_fct ch_call_back_event = priv_data;

	if (ch_eoc_timer_started == false) {
		event = CHARGING_COMPLETE;
		ch_call_back_event(&event);
	} else {
		if (fg_get_battery_soc() == 100) {
			ch_stop_sw_eoc();
			timer_start(eoc_timer, CH_SW_EOC, NULL);
		} else
			ch_start_sw_eoc();
	}
}

/**@brief Function to start timer to check SOC at 100%
 */
static void ch_start_sw_eoc(void)
{
	if (ch_eoc_timer_started == true) {
		timer_stop(eoc_timer);
	}
	ch_eoc_timer_started = true;
	timer_start(eoc_timer, CH_TM_DELAY_1MIN, NULL);
}

/**@brief Function to stop timer to check SOC at 100%
 */
static void ch_stop_sw_eoc(void)
{
	timer_stop(eoc_timer);
	ch_eoc_timer_started = false;
}

#endif

/****************************************************************************************
*********************** ACCESS FUNCTION IMPLEMENTATION *********************************
****************************************************************************************/
void hal_charger_init(T_QUEUE parent_queue, void *call_back)
{
	/* Attach callback to PIN event */
	struct td_device *ps_dev = &pf_device_charger;
	static uint8_t ch_sm_event;

	assert(call_back);
	ch_event_fct ch_call_back_event = call_back;
#if (CONFIG_CH_SW_EOC != 0)
	eoc_timer =
		timer_create(ch_eoc_timer_handler, call_back, CH_TM_DELAY_1MIN,
			     false, false,
			     NULL);
#endif
	/* Attach callback to managed_comparator event */
	charger_register_callback(ps_dev, hal_charger_cb, ch_call_back_event);
	/* Get the current state */
	if (charger_get_current_soc(ps_dev) == DISCHARGING)
		ch_sm_event = CHARGING_STOP;
	else
		ch_sm_event = CHARGING_START;
	ch_call_back_event(&ch_sm_event);
	charger_config();
}

void hal_charger_enable()
{
	charger_enable();
}

void hal_charger_disable()
{
	charger_disable();
}
