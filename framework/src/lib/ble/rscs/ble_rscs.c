/*
 * Copyright (c) 2016, Intel Corporation. All rights reserved.
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

#include "lib/ble/rscs/ble_rscs.h"

#include <misc/byteorder.h>
#include <bluetooth/gatt.h>

#include "infra/log.h"

/* RSC measurement characteristic */
static struct bt_gatt_ccc_cfg rsc_measurement_ccc_cfg[1] = {};

static ssize_t read_stepcadence_feature(struct bt_conn *conn,
					const struct bt_gatt_attr *attr,
					void *buf, uint16_t len,
					uint16_t offset)
{
	uint16_t rsc_feature = 0;

	if (on_ble_rscs_get_stride_lenth() >= 0) {
		rsc_feature |= 1;
	}

	if (on_ble_rscs_get_total_distance() >= 0) {
		rsc_feature |= 2;
	}

	if (on_ble_rscs_get_walk_run() >= 0) {
		rsc_feature |= 4;
	}

#ifdef CONFIG_BLE_RSCS_MULTIPLE_SENSOR_LOCATION_SUPPORT
	rsc_feature |= 10;
#endif

	/* other optional elements not yet implemented
	 * - calibration procedure supported:
	 *   rsc_feature |= 8;
	 */

	rsc_feature = sys_cpu_to_le16(rsc_feature);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &rsc_feature,
				 sizeof(rsc_feature));
}

#ifdef CONFIG_BLE_RSCS_SENSOR_LOCATION_SUPPORT
static ssize_t read_sensor_location(struct bt_conn *conn,
				    const struct bt_gatt_attr *attr,
				    void *buf, uint16_t len,
				    uint16_t offset)
{
	uint8_t sensor_location = 0;

	sensor_location = on_ble_rscs_get_sensor_location();
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &sensor_location,
				 sizeof(sensor_location));
}
#endif

static void rsc_measurement_ccc_cfg_changed(uint16_t value)
{
	if (value & BT_GATT_CCC_NOTIFY) {
		/* Start running speed and cadence sensor */
		pr_debug(LOG_MODULE_BLE, "RSCS enabled");
		on_ble_rscs_enabled();
	} else {
		/* Stop running speed and cadence sensor */
		pr_debug(LOG_MODULE_BLE, "RSCS disabled");
		on_ble_rscs_disabled();
	}
}

/* RSC Service Declaration */
static const struct bt_gatt_attr rscs_attrs[] = {
	/* Running Speed and Cadence Service */
	BT_GATT_PRIMARY_SERVICE(BT_UUID_RSCS),
	/* RSC Measurement */
	BT_GATT_CHARACTERISTIC(BT_UUID_RSC_MEASUREMENT, BT_GATT_CHRC_NOTIFY),
	BT_GATT_DESCRIPTOR(BT_UUID_RSC_MEASUREMENT, 0, NULL, NULL, NULL),
	BT_GATT_CCC(rsc_measurement_ccc_cfg, rsc_measurement_ccc_cfg_changed),

	/* RSC Feature */
	BT_GATT_CHARACTERISTIC(BT_UUID_RSC_FEATURE, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_RSC_FEATURE, BT_GATT_PERM_READ,
			   read_stepcadence_feature, NULL, NULL),

#ifdef CONFIG_BLE_RSCS_SENSOR_LOCATION_SUPPORT
	/* Sensor Location*/
	BT_GATT_CHARACTERISTIC(BT_UUID_SENSOR_LOCATION, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_SENSOR_LOCATION, BT_GATT_PERM_READ,
			   read_sensor_location, NULL, NULL),
#endif
};

static struct bt_gatt_attr const *const measurement_value = &rscs_attrs[2];

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	int i;
	const bt_addr_le_t *dst;

	dst = bt_conn_get_dst(conn);
	if (!dst) {
		return;
	}

	/* This is a specific behaviour: we are discarding CCCD value for the
	 * measurement characteristic -> forces the peer to resubscribe
	 */
	for (i = 0; i < ARRAY_SIZE(rsc_measurement_ccc_cfg); i++) {
		if (!bt_addr_le_cmp(dst, &rsc_measurement_ccc_cfg[i].peer)) {
			rsc_measurement_ccc_cfg[i].value = 0;
		}
	}
}

static struct bt_conn_cb conn_callbacks = {
	.disconnected = on_disconnected,
};

int ble_rscs_init(void)
{
	int err;

	err = bt_gatt_register((struct bt_gatt_attr *)rscs_attrs,
			       ARRAY_SIZE(rscs_attrs));

	if (err < 0) {
		goto out;
	}
	bt_conn_cb_register(&conn_callbacks);
out:
	return err;
}

struct ble_rscs_measurement {
	/* bit 0: stride present, bit 1: distance present, bit 2: walk/run */
	uint8_t flags;
	/* mandatory */
	uint16_t speed;
	/* mandatory */
	uint8_t cadence;
	/* optional: stride or distance */
	union {
		uint16_t stride;
		uint32_t distance;
	};
	/* optional: distance if stride is present */
	uint32_t distance_after_stride;
} __packed;

int ble_rscs_update(uint16_t speed, uint8_t cadence)
{
	struct ble_rscs_measurement measurement;
	int rv;
	uint16_t length = offsetof(struct ble_rscs_measurement, stride);

	measurement.flags = 0;
	measurement.speed = sys_cpu_to_le16(speed);
	measurement.cadence = cadence;
	rv = on_ble_rscs_get_stride_lenth();
	if (rv >= 0) {
		measurement.flags |= 1;
		measurement.stride = sys_cpu_to_le16((uint16_t)rv);
		length += sizeof(uint16_t);
	}
	rv = on_ble_rscs_get_total_distance();
	if (rv >= 0) {
		uint32_t distance = sys_cpu_to_le32((uint32_t)rv);

		measurement.flags |= 2;
		if (measurement.flags & 1) {
			measurement.distance_after_stride = distance;
		} else {
			measurement.distance = distance;
		}
		length += sizeof(uint32_t);
	}
	rv = on_ble_rscs_get_walk_run();
	if (rv >= 0) {
		measurement.flags |= rv ? 4 : 0;
	}

	return bt_gatt_notify(NULL, measurement_value, &measurement, length,
			      NULL);
}

__weak void on_ble_rscs_enabled(void)
{
}

__weak void on_ble_rscs_disabled(void)
{
}

__weak int on_ble_rscs_get_stride_lenth(void)
{
	return -1;
}

__weak int on_ble_rscs_get_total_distance(void)
{
	return -1;
}

__weak int on_ble_rscs_get_walk_run(void)
{
	return -1;
}

#ifdef CONFIG_BLE_RSCS_SENSOR_LOCATION_SUPPORT
__weak uint8_t on_ble_rscs_get_sensor_location(void)
{
	return BLE_SENSOR_LOCATION_OTHER;
}
#endif
