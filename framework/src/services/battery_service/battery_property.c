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

#include <string.h>
#include "cfw/cfw.h"
#include "cfw/cfw_service.h"
#include "machine.h"
#include "infra/port.h"
#include "infra/log.h"
#include "infra/features.h"
#include "features_soc.h"
#include "util/assert.h"

#include "services/service_queue.h"
#include "services/properties_service/properties_service.h"

#include "battery_LUT/battery_LUT.h"
#include "battery_property.h"


static struct battery_properties_t g_battery_properties = {};


static cfw_service_conn_t *storage_service_conn = NULL; /**< ADC service handler */
static bool storage_init_done = false;                  /**< equal to 'true' once adc init done */

static cfw_service_conn_t *prop_service_conn = NULL;    /**< GPIO service handler */
static bool prop_init_done = false;                     /**< equal to 'true' once gpio init done */

static bool is_nvm_lookup_table_read = false;                   /**< equal to 'true' once gpio init done */
static uint16_t nvm_lookup_tables[BATTPROP_LOOKUP_TABLE_COUNT *
				  BATTPROP_LOOKUP_TABLE_SIZE] = {};

/**< pointer of function to be call when battery properties are initialised*/
static battery_propeties_cb_t g_battery_propeties_cb = NULL;

static cfw_client_t *g_client = NULL;

DEFINE_LOG_MODULE(LOG_MODULE_BP, "BATP")

#ifdef DEBUG_BATTERY_SERVICE
static void bp_print_property(void)
{
	pr_info(LOG_MODULE_BP, "battery_id = %d\n",
		g_battery_properties.battery_id);
	pr_info(LOG_MODULE_BP, "battery_cycles = %d\n",
		g_battery_properties.battery_charge_cyles);
	pr_info(LOG_MODULE_BP, "battery_overall_time_charging = %d\n",
		g_battery_properties.battery_overall_time_charging);
	pr_info(LOG_MODULE_BP, "battery_overall_time_discharging = %d\n",
		g_battery_properties.battery_overall_time_discharging);
	pr_info(LOG_MODULE_BP, "battery_soc = %d\n",
		g_battery_properties.battery_soc);
	pr_info(LOG_MODULE_BP, "battery_shutdown_voltage = %d\n",
		g_battery_properties.battery_shutdown_voltage);
	pr_info(LOG_MODULE_BP, "battery_charge_delta_soc = %d\n",
		g_battery_properties.battery_charge_delta_soc);
}
#endif

static void service_connection_cb(cfw_service_conn_t *conn, void *param)
{
	if ((void *)PROPERTIES_SERVICE_ID == param) {
#ifdef DEBUG_BATTERY_SERVICE
		pr_info(LOG_MODULE_BP, "property storage service open\n");
#endif
		prop_service_conn = conn;
		prop_init_done = true;
		properties_service_read(
			prop_service_conn,
			BATTERY_SERVICE_ID,
			BATTERY_PROPERTY_ID,
			NULL);
	} else if ((void *)LL_STOR_SERVICE_ID == param) {
#ifdef DEBUG_BATTERY_SERVICE
		pr_info(LOG_MODULE_BP, "ll storage service open\n");
#endif
		storage_service_conn = conn;
		storage_init_done = true;
		/*
		 * TODO
		 * Need to defined lookup table location
		 * partition_id: ?
		 * start_offset: ?
		 * ll_storage_service_read(storage_service_conn,partition_id,start_offset,lookup_table_size,NULL);
		 */
	}
}

static void battery_properties_handle_msg(struct cfw_message *msg, void *data)
{
	int8_t status = 0;
	struct cfw_message *msg_rsp = NULL;

	switch (CFW_MESSAGE_ID(msg)) {
	case MSG_ID_CFW_CLOSE_SERVICE_RSP:
		prop_init_done = false;
		storage_init_done = false;
		break;
	case MSG_ID_PROP_SERVICE_READ_RSP:
		status = ((properties_service_read_rsp_msg_t *)msg)->status;
		if (DRV_RC_INVALID_OPERATION == status) {
			properties_service_add(prop_service_conn,
					       BATTERY_SERVICE_ID,
					       BATTERY_PROPERTY_ID,
					       true,
					       &g_battery_properties,
					       sizeof(struct
						      battery_properties_t),
					       NULL);
		} else if (DRV_RC_OK == status) {
			/*
			 * TODO
			 * de-serialize data structure
			 */
			memcpy(
				&g_battery_properties,
				&((properties_service_read_rsp_msg_t *)msg)->
				start_of_values,
				((properties_service_read_rsp_msg_t *)msg)->
				property_size);
			/* initialization is done, advertise throw a callback*/
			if (NULL != g_battery_propeties_cb)
				g_battery_propeties_cb(BP_STATUS_SUCCESS);
#ifdef DEBUG_BATTERY_SERVICE
			bp_print_property();
#endif
		} else {
			/* initialization is done, advertise throw a callback*/
			if (NULL != g_battery_propeties_cb)
				g_battery_propeties_cb(
					BP_STATUS_ERROR_PROP_SERVICE);
		}

		break;
	case MSG_ID_PROP_SERVICE_WRITE_RSP:
		status = ((properties_service_write_rsp_msg_t *)msg)->status;
		if (DRV_RC_OK != status)
			pr_error(LOG_MODULE_BP, "Could not %s", "save");
		msg_rsp = (struct cfw_message *)(CFW_MESSAGE_PRIV(msg));
		if ((NULL != msg_rsp) &&
		    (CFW_MESSAGE_ID(msg_rsp) ==
		     MSG_ID_CFW_SERVICE_SHUTDOWN_RSP)) {
			/*  If the property save resulted from a shutdown_req send by the service manager send back the rply_msg */
			cfw_send_message(msg_rsp);
		}
#ifdef DEBUG_BATTERY_SERVICE
		pr_info(LOG_MODULE_BP,
			" MSG_ID_PROP_SERVICE_WRITE_RSP code %d\n",
			status);
#endif
		break;
	case MSG_ID_PROP_SERVICE_ADD_RSP:
		status = ((properties_service_add_rsp_msg_t *)msg)->status;
		if (DRV_RC_OK != status)
			pr_error(LOG_MODULE_BP, "Could not %s", "add");
		/* initialization is done, advertise throw a callback*/
		if (NULL != g_battery_propeties_cb)
			g_battery_propeties_cb(BP_STATUS_SUCCESS);
#ifdef DEBUG_BATTERY_SERVICE
		pr_info(LOG_MODULE_BP, " MSG_ID_PROP_SERVICE_ADD_RSP code %d\n",
			status);
#endif
		break;
	default:
		break;
	}
	cfw_msg_free(msg);
}

/*
 * @brief Get current lookup table ID
 * @param[in] battprop_fuelgauge current system state
 * @param[in,out] battprop_lookuptab_id Current lookup table id
 * @return BP_STATUS_SUCCESS if Ok
 * @remark This ID is compute from current temperature and charging state
 */
bp_status_t battery_properties_get_lookupTable_id(
	struct battprop_fuelgauge_t *	battprop_fuelgauge,
	battprop_lookuptab_id_t *	battprop_lookuptab_id)
{
	bp_status_t bp_status = BP_STATUS_ERROR_PARAMETER;

	assert(battprop_fuelgauge && battprop_lookuptab_id);

	*battprop_lookuptab_id = BATTPROP_LOOKUPTAB_ID_UNKNOWN;

	if (false != battprop_fuelgauge->is_charging) {
		*battprop_lookuptab_id = BATTPROP_LOOKUPTAB_ID_CHARGE;
	} else {
		if (BP_TH_DISCHARGE_0 > battprop_fuelgauge->temperature) {
			*battprop_lookuptab_id = BATTPROP_LOOKUPTAB_DISCHARGE_0;
		} else
		if ((BP_TH_DISCHARGE_0 <= battprop_fuelgauge->temperature) &&
		    (BP_TH_DISCHARGE_12 > battprop_fuelgauge->temperature)) {
			*battprop_lookuptab_id =
				BATTPROP_LOOKUPTAB_DISCHARGE_12;
		} else if (BP_TH_DISCHARGE_12 <=
			   battprop_fuelgauge->temperature) {
			*battprop_lookuptab_id =
				BATTPROP_LOOKUPTAB_DISCHARGE_25;
		}
	}

	if (BATTPROP_LOOKUPTAB_ID_UNKNOWN != *battprop_lookuptab_id) {
		bp_status = BP_STATUS_SUCCESS;
	}

	return bp_status;
}

/*
 * @brief Get default battery lookup table
 * @param[in] battprop_lookuptab_id Current lookup table id
 * @param[in,out] lookuptable lookup table
 * @return BP_STATUS_SUCCESS if ok
 */
bp_status_t battprop_get_dflt_lookupTable(
	battprop_lookuptab_id_t battprop_lookuptab_id,
	uint16_t **		lookuptable)
{
	bp_status_t bp_status = BP_STATUS_ERROR_PARAMETER;

	if ((BATTPROP_LOOKUPTAB_ID_CHARGE <= battprop_lookuptab_id) &&
	    (BATTPROP_LOOKUPTAB_ID_UNKNOWN > battprop_lookuptab_id)) {
		if (board_feature_has(HW_BATT_1K3_RESISTOR))
			*lookuptable =
				(uint16_t *)dflt_lookup_tables2[
					battprop_lookuptab_id];
		else
			*lookuptable =
				(uint16_t *)dflt_lookup_tables[
					battprop_lookuptab_id];
		bp_status = BP_STATUS_SUCCESS;
	}
	return bp_status;
}

/*
 * @brief Get battery lookup table saved in NVM
 * @param[in] battprop_lookuptab_id Current lookup table id
 * @param[in,out] lookuptable lookup table
 * @return BP_STATUS_SUCCESS if ok
 */
bp_status_t battery_properties_get_nvm_lookupTable(
	battprop_lookuptab_id_t battprop_lookuptab_id,
	uint16_t **		lookuptable)
{
	bp_status_t bp_status = BP_STATUS_ERROR_PARAMETER;

	if ((BATTPROP_LOOKUPTAB_ID_CHARGE <= battprop_lookuptab_id) &&
	    (BATTPROP_LOOKUPTAB_ID_UNKNOWN > battprop_lookuptab_id) &&
	    (false != is_nvm_lookup_table_read)) {
		*lookuptable =
			(uint16_t *)&nvm_lookup_tables[battprop_lookuptab_id *
						       BATTPROP_LOOKUP_TABLE_SIZE
			];
		bp_status = BP_STATUS_SUCCESS;
	}

	return bp_status;
}

/*
 * @brief Get battery lookup table
 * @param[in] battprop_fuelgauge current system state
 * @param[in,out] lookuptable lookup table
 * @return BP_STATUS_SUCCESS if ok
 */
bp_status_t battery_properties_get_lookupTable(
	struct battprop_fuelgauge_t *	battprop_fuelgauge,
	uint16_t **			lookuptable)
{
	bp_status_t bp_status = BP_STATUS_ERROR;
	battprop_lookuptab_id_t battprop_lookuptab_id =
		BATTPROP_LOOKUPTAB_ID_UNKNOWN;
	uint16_t *current_lookuptable = NULL;

	assert(battprop_fuelgauge);

	bp_status = battery_properties_get_lookupTable_id(
		battprop_fuelgauge,
		&battprop_lookuptab_id);

	if (BP_STATUS_SUCCESS != bp_status)
		return BP_STATUS_ERROR;

	if (BP_STATUS_SUCCESS != battery_properties_get_nvm_lookupTable(
		    battprop_lookuptab_id,
		    &current_lookuptable)) {
		bp_status =
			battprop_get_dflt_lookupTable
				(battprop_lookuptab_id,
				&current_lookuptable);
		if (BP_STATUS_SUCCESS == bp_status)
			*lookuptable = (uint16_t *)current_lookuptable;
	}

	return bp_status;
}

/**
 * @brief Init battery properties
 * Read battery properties from the storage
 * @param battery_propeties_cb function to be call when properties are retrieved
 * @remark shall call properties_service_add with BATTERY_SERVICE_ID and BATTERY_PROPERTY_ID
 */
void battery_properties_init(battery_propeties_cb_t battery_propeties_cb)
{
	assert(battery_propeties_cb);

	g_battery_propeties_cb = battery_propeties_cb;

	g_battery_properties.battery_id = 0;
	g_battery_properties.battery_charge_cyles = 0;
	g_battery_properties.battery_overall_time_charging = 0;
	g_battery_properties.battery_overall_time_discharging = 0;
	g_battery_properties.battery_soc = 0;
	g_battery_properties.battery_charge_delta_soc = 0;
	g_battery_properties.battery_shutdown_voltage = 0;

	g_client = cfw_client_init(
		get_service_queue(), battery_properties_handle_msg, NULL);
	cfw_open_service_helper(g_client, LL_STOR_SERVICE_ID,
				service_connection_cb,
				(void *)LL_STOR_SERVICE_ID);
	cfw_open_service_helper(g_client, PROPERTIES_SERVICE_ID,
				service_connection_cb,
				(void *)PROPERTIES_SERVICE_ID);
}
/**
 * @brief Get battery properties
 * @param battery_properties battery_properties buffer where to write data
 * @remark shall call properties_service_read with BATTERY_SERVICE_ID and BATTERY_PROPERTY_ID
 */
struct battery_properties_t *battery_properties_get(void)
{
	return &g_battery_properties;
}
/**
 * @brief Save battery properties
 * @param pointer to store the private field of the shutdown request message
 * @retval BP_STATUS_SUCCESS if success, error otherwise. List of errors to be specified
 * @remark shall call properties_service_read with BATTERY_SERVICE_ID and BATTERY_PROPERTY_ID
 */
bp_status_t battery_properties_save(struct cfw_message *msg)
{
	bp_status_t bp_status = BP_STATUS_ERROR_PROP_SERVICE;

	if (false != prop_init_done) {
#ifdef  DEBUG_BATTERY_SERVICE
		bp_print_property();
#endif
		/*
		 * TODO
		 * Serialize data structure
		 */
		properties_service_write(
			prop_service_conn,
			BATTERY_SERVICE_ID,
			BATTERY_PROPERTY_ID,
			(struct battery_properties_t *)&
			g_battery_properties,
			sizeof(struct battery_properties_t),
			msg);
		bp_status = BP_STATUS_SUCCESS;
	}
	return bp_status;
}
