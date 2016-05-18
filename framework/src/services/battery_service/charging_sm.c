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
#include "infra/system_events.h"

#include "charging_sm.h"
#include "battery_service_private.h"
#include "em_board_config.h"
#include "hal_charger.h"

static struct power_supply_fct *source0;
static struct power_supply_fct *source1;
static struct power_supply_fct *source2;
static struct power_supply_fct *current_source = NULL;

static const char *str_state[] =
{ "INIT", "DISCHARGE", "CHARGE", "COMPLETE", "FAULT" };
#ifdef DEBUG_BATTERY_SERVICE
static const char *str_source[] = { "NONE", "USB", "WIRELESS", "DC", "UNKNOWN" };
#endif

static enum e_state state;
static enum charging_sm_source power_supply;

static bool is_charging;
/* Define call back handler for power supply signal event */
static void handler_ps_event(enum charging_sm_event	event,
			     enum charging_sm_source	src);

/* Define call back handler for soc_charger_driver signal event */
static void handler_charger_event(void *event);

/* Define call back function for Battery_Service signal event */
typedef void (*sm_event_fct)(enum e_bs_ch_event event);
static sm_event_fct sm_call_back_event;

/****************************************************************************************
******************** STATE (LOCAL) FUNCTION IMPLEMENTATION *****************************
****************************************************************************************/
static void state_init(void)
{
	is_charging = false;
	state = INIT;
}

static void state_discharge(void)
{
	is_charging = false;
#ifdef CONFIG_SYSTEM_EVENTS
	system_event_push_battery(SYSTEM_EVENT_BATT_CHARGE,
				  SYSTEM_EVENT_BATT_DISCHARGING);
#endif
	state = DISCHARGE;
}

static void state_charge(void)
{
	is_charging = true;
	state = CHARGE;
#ifdef CONFIG_SYSTEM_EVENTS
	system_event_push_battery(SYSTEM_EVENT_BATT_CHARGE,
				  SYSTEM_EVENT_BATT_CHARGING);
#endif
}

static void state_complete(void)
{
	is_charging = true;
#ifdef CONFIG_SYSTEM_EVENTS
	system_event_push_battery(SYSTEM_EVENT_BATT_CHARGE,
				  SYSTEM_EVENT_BATT_CH_COMPLETE);
#endif
	state = COMPLETE;
	sm_call_back_event(BS_CH_EVENT_CHARGE_COMPLETE);
	if ((current_source != NULL) &&
	    (current_source->maintenance_enable != NULL))
		current_source->maintenance_enable();
}

static void state_fault(int fault_state)
{
	state = FAULT;
	if (fault_state == CHARGING_CHARGE_FAULT) {
		is_charging = true;
#ifdef CONFIG_SYSTEM_EVENTS
		system_event_push_battery(SYSTEM_EVENT_BATT_CHARGE,
					  SYSTEM_EVENT_BATT_CHARGING);
#endif
	} else {
		is_charging = false;
#ifdef CONFIG_SYSTEM_EVENTS
		system_event_push_battery(SYSTEM_EVENT_BATT_CHARGE,
					  SYSTEM_EVENT_BATT_DISCHARGING);
#endif
	}
}

/****************************************************************************************
*********************** LOCAL FUNCTON IMPLEMENTATION ***********************************
****************************************************************************************/

/**@brief Function for update the power supply pointer.
 */
static void update_source(void)
{
	switch (power_supply) {
	case NONE: current_source = source0; break;
	case SRC1: current_source = source1; break;
	case SRC2: current_source = source2; break;
	/* Warning: check hardware priority to match with it */
	case BOTH: current_source = source1; break;
	default: break;
	}
#ifdef DEBUG_BATTERY_SERVICE
	pr_info(LOG_MODULE_CH, "Power source: %s",
		str_source[current_source->source_type]);
#endif
}


/**@brief Callback handler function for soc charger.
 * @param[in]  name of event (enum charging_sm_event)
 */
static void handler_charger_event(void *event)
{
	switch (state) {
	case INIT:
		sm_call_back_event(BS_CH_EVENT_INIT_DONE);
		switch (*(enum charging_sm_event *)event) {
		case CHARGING_STOP:
			state_discharge(); break;
		case CHARGING_START:
			state_charge(); break;
		case CHARGING_COMPLETE:
			state_complete(); break;
		case CHARGING_CHARGE_FAULT:
			state_fault(CHARGING_CHARGE_FAULT); break;
		case CHARGING_DISCHARGE_FAULT:
			state_fault(CHARGING_DISCHARGE_FAULT); break;
		default: break;
		}
		break;

	case DISCHARGE:
		switch (*(enum charging_sm_event *)event) {
		case CHARGING_STOP:
			break;
		case CHARGING_START:
			state_charge(); break;
		case CHARGING_COMPLETE:
			break;
		case CHARGING_CHARGE_FAULT:
			state_fault(CHARGING_CHARGE_FAULT); break;
		case CHARGING_DISCHARGE_FAULT:
			state_fault(CHARGING_DISCHARGE_FAULT); break;
		default: break;
		}
		break;

	case CHARGE:
		switch (*(enum charging_sm_event *)event) {
		case CHARGING_STOP:
			state_discharge(); break;
		case CHARGING_START:
			break;
		case CHARGING_COMPLETE:
			state_complete(); break;
		case CHARGING_CHARGE_FAULT:
			state_fault(CHARGING_CHARGE_FAULT); break;
		case CHARGING_DISCHARGE_FAULT:
			state_fault(CHARGING_DISCHARGE_FAULT); break;
		default: break;
		}
		break;

	case COMPLETE:
		switch (*(enum charging_sm_event *)event) {
		case CHARGING_STOP:
			break;
		case CHARGING_START:
			state_charge(); break;
		case CHARGING_COMPLETE:
			break;
		case CHARGING_CHARGE_FAULT:
			state_fault(CHARGING_CHARGE_FAULT); break;
		case CHARGING_DISCHARGE_FAULT:
			state_fault(CHARGING_DISCHARGE_FAULT); break;
		default: break;
		}
		break;

	case FAULT:
		switch (*(enum charging_sm_event *)event) {
		case CHARGING_STOP:
			state_discharge(); break;
		case CHARGING_START:
			state_charge(); break;
		case CHARGING_COMPLETE:
			break;
		case CHARGING_CHARGE_FAULT:
			state_fault(CHARGING_CHARGE_FAULT); break;
		case CHARGING_DISCHARGE_FAULT:
			state_fault(CHARGING_DISCHARGE_FAULT); break;
		default: break;
		}
		break;

	default: break;
	}

	/* Print updated state */
	pr_info(LOG_MODULE_CH, "Charging state: %s", str_state[state]);
}

/**@brief Callback handler function for power supply.
 * @param[in]  name of event (enum charging_sm_event)
 * @param[in]  name of source (enum charging_sm_source)
 */
static void handler_ps_event(enum charging_sm_event	event,
			     enum charging_sm_source	src)
{
	switch (event) {
	case CHARGING_PLUGGED_IN:
		if (src == NONE) {
			pr_error(
				LOG_MODULE_CH,
				"CHARGER CB recv: ERROR => NONE isn't a plugged source");
			break;
		}
		/* Power supply update */
		power_supply = power_supply | src;
		update_source();
		sm_call_back_event(BS_CH_EVENT_CHARGER_CONNECTED);
		break;
	case CHARGING_PLUGGED_OUT:
		if (src == NONE) {
			pr_error(
				LOG_MODULE_CH,
				"CHARGER CB recv: ERROR => NONE isn't an unplugged source");
			break;
		}
		/* Power supply update */
		power_supply = power_supply & (~src);
		update_source();
		/* In case of all power supply are disconnected,
		 * force charging state to "DISCHARGE" */
		if (power_supply == NONE) {
			sm_call_back_event(BS_CH_EVENT_CHARGER_DISCONNECTED);
			if (state != INIT) {
				if (state != DISCHARGE)
					state_discharge();
			}
		}
		break;
	default:
		break;
	}
}

/****************************************************************************************
*********************** ACCESS FUNCTION IMPLEMENTATION *********************************
****************************************************************************************/
bool charging_sm_init(T_QUEUE parent_queue, void *call_back)
{
	sm_call_back_event = call_back;
	power_supply = NONE;
	get_source(&source0, &source1, &source2);
	state_init();
	if (source1->init != NULL)
		if (!source1->init(parent_queue, handler_ps_event))
			return false;
	if (source2->init != NULL)
		if (!source2->init(parent_queue, handler_ps_event))
			return false;
	hal_charger_init(parent_queue, handler_charger_event);

	return true;
}

bool charging_sm_is_charging()
{
	return is_charging;
}

bool charging_sm_is_charger_connected()
{
	if (power_supply == NONE)
		return false;
	else
		return true;
}

battery_charger_src_t charging_sm_get_source()
{
	return current_source->source_type;
}

enum e_state charging_sm_get_state()
{
	return state;
}
