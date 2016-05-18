/*
 * Copyright (c) 2015-2016, Intel Corporation. All rights reserved.
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

#include "lib/ble/hrs/ble_hrs.h"

#include <bluetooth/gatt.h>

// for __weak
#include "util/compiler.h"

/* Heart rate Service Variables */
static struct bt_gatt_ccc_cfg hrs_measurement_ccc_cfg[1] = {};
#define hrs_measurement_ccc_cfg_changed NULL
/* HRS Control Point characteristic */
#define hrs_control_point_ccc_cfg NULL
#define hrs_control_point_ccc_cfg_changed NULL

__weak const enum ble_hrs_location ble_hrs_sensor_location = BLE_HRS_WRIST;

static ssize_t read_body_sensor_location(struct bt_conn *conn,
					 const struct bt_gatt_attr *attr,
					 void *buf, uint16_t len,
					 uint16_t offset)
{
	uint8_t value = ble_hrs_sensor_location;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
				 sizeof(value));
}

/* HRS Service Declaration */
static const struct bt_gatt_attr hrs_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HRS),
	/* Heart Rate Measurement */
	BT_GATT_CHARACTERISTIC(BT_UUID_HRS_MEASUREMENT, BT_GATT_CHRC_NOTIFY),
	BT_GATT_DESCRIPTOR(BT_UUID_HRS_MEASUREMENT, 0, NULL, NULL, NULL),
	BT_GATT_CCC(hrs_measurement_ccc_cfg, hrs_measurement_ccc_cfg_changed),

	/* Body Sensor Location */
	BT_GATT_CHARACTERISTIC(BT_UUID_HRS_BODY_SENSOR, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_HRS_BODY_SENSOR, BT_GATT_PERM_READ,
			   read_body_sensor_location, NULL, NULL),
	BT_GATT_CCC(hrs_measurement_ccc_cfg, hrs_measurement_ccc_cfg_changed),
};

/* Pointer to the HRS value attribute in the above table */
static struct bt_gatt_attr const *const hrs_value = &hrs_attrs[2];

int ble_hrs_init(void)
{
	/* cast to discard the const qualifier */
	return bt_gatt_register((struct bt_gatt_attr *)hrs_attrs,
				ARRAY_SIZE(hrs_attrs));
}

struct ble_hrs_measurement {
	uint8_t flags;
	uint8_t value;
};

int ble_hrs_update(uint8_t value)
{
	struct ble_hrs_measurement measurement;

	measurement.flags = 0;
	measurement.value = value;
	return bt_gatt_notify(NULL, hrs_value, &measurement,
			      sizeof(measurement), NULL);
}

const struct bt_gatt_attr *ble_hrs_attr(void)
{
	return hrs_value;
}
