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

#include "lib/ble/lns/ble_lns.h"

#include <misc/byteorder.h>
#include <bluetooth/gatt.h>

#include "infra/log.h"

/* LNS Location and Speed characteristic */
static struct bt_gatt_ccc_cfg lns_location_speed_ccc_cfg[1] = {};

/* LNS Feature characteristic */
static struct bt_gatt_ccc_cfg lns_feature_ccc_cfg[1] = {};

/* LNS Control point characteristic */
static struct bt_gatt_ccc_cfg lns_control_point_ccc_cfg[1] = {};

/* Flags: Elevation present and source is Barometric Air Pressure */
#define FLAGS                   ((1 << 3) | (1 << 10))

/* Set_Elevation codes */
#define SET_ELEVATION_OPCODE            8
#define SET_ELEVATION_LENGTH            4 /* 1 (Op Code) + 3 (sint24) */

#define RESPONSE_OPCODE                 0x20 /* Index = 0 */

/* forward declarations */
static int indicate_control_point_result(struct bt_conn *conn, uint8_t op_code,
					 enum ble_lns_response response);
static ssize_t write_control_point(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   const void *buf, uint16_t len,
				   uint16_t offset);
static ssize_t read_lns_feature(struct bt_conn *conn,
				const struct bt_gatt_attr *attr, void *buf,
				uint16_t len,
				uint16_t offset);


static void lns_location_speed_ccc_cfg_changed(uint16_t value)
{
	if (value & BT_GATT_CCC_NOTIFY) {
		pr_debug(LOG_MODULE_BLE, "LNS enabled");
		on_ble_lns_enabled();
	} else {
		pr_debug(LOG_MODULE_BLE, "LNS disabled");
		on_ble_lns_disabled();
	}
}

/* LNS Service Declaration */
static const struct bt_gatt_attr lns_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_LNS),
	/* Location and Speed */
	BT_GATT_CHARACTERISTIC(BT_UUID_LNS_LOCATION_SPEED, BT_GATT_CHRC_NOTIFY),
	BT_GATT_DESCRIPTOR(BT_UUID_LNS_LOCATION_SPEED, 0, NULL, NULL, NULL),
	BT_GATT_CCC(lns_location_speed_ccc_cfg,
		    lns_location_speed_ccc_cfg_changed),

	/* LN Control point */
	BT_GATT_CHARACTERISTIC(BT_UUID_LNS_CONTROL_POINT,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE),
	BT_GATT_DESCRIPTOR(BT_UUID_LNS_CONTROL_POINT, BT_GATT_PERM_WRITE,
			   NULL, write_control_point, NULL),
	BT_GATT_CCC(lns_control_point_ccc_cfg, NULL),

	/* LNS Feature */
	BT_GATT_CHARACTERISTIC(BT_UUID_LNS_FEATURE, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_LNS_FEATURE, BT_GATT_PERM_READ,
			   read_lns_feature, NULL, NULL),
	BT_GATT_CCC(lns_feature_ccc_cfg, NULL),
};

static struct bt_gatt_attr const *const location_speed_value = &lns_attrs[2];
static struct bt_gatt_attr const *const control_point_value = &lns_attrs[5];

static ssize_t write_control_point(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   const void *buf, uint16_t len,
				   uint16_t offset)
{
	const uint8_t *bytes = buf;
	/* Initialize indic_result with operation failed */
	enum ble_lns_response response = BLE_LNS_OPERATION_FAILED;
	const struct _bt_gatt_ccc *ccc = lns_attrs[6].user_data;

	/* TODO: multiple peripheral connections: check ccc->cfg[conn].value */
	if (ccc->value != BT_GATT_CCC_INDICATE)
		return BT_GATT_ERR(BT_ATT_ERR_CCC_IMPROPER_CONF);

	/* Check Op code */
	if (bytes[0] == SET_ELEVATION_OPCODE) {
		if (len == SET_ELEVATION_LENGTH) {
			int32_t value = (bytes[3] << 16) | (bytes[2] << 8) |
					bytes[1];
			/* Manage negative value (from sint24 to sint32) */
			if (value > 0x7FFFFF)
				value |= (0xFF << 24);
			pr_info(LOG_MODULE_BLE, "New elevation value: %d",
				value);
			/* Notify user of new elevation value, user is
			 * responsible of invoking
			 * ble_lns_elevation_set_reply to send the
			 * indicate response value */
			on_ble_lns_elevation_set(conn, value);
			return len;
		} else {
			pr_debug(LOG_MODULE_BLE, "Wrong length: %d != %d",
				 len, SET_ELEVATION_LENGTH);
			response = BLE_LNS_INVALID_PARAM;
		}
	} else {
		pr_debug(LOG_MODULE_BLE, "Not Supported OpCode [%d]", bytes[0]);
		response = BLE_LNS_NOT_SUPPORTED;
	}
	indicate_control_point_result(conn, bytes[0], response);
	return len;
}

static ssize_t read_lns_feature(struct bt_conn *conn,
				const struct bt_gatt_attr *attr, void *buf,
				uint16_t len, uint16_t offset)
{
	/* Elevation supported (1<<3) and Elevation setting supported (1<<19) */
	uint32_t value = (1 << 3) | (1 << 19);

	value = sys_cpu_to_le32(value);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
				 sizeof(value));
}

static void indicate_response(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, int err)
{
	if (err)
		pr_error(LOG_MODULE_BLE, "LNS indic failed [%d]", err);
}

static int indicate_control_point_result(struct bt_conn *conn, uint8_t op_code,
					 enum ble_lns_response response)
{
	uint8_t r[3] = { RESPONSE_OPCODE, op_code, response };
	struct bt_gatt_indicate_params ind_params;

	ind_params.attr = control_point_value;
	ind_params.data = r;
	ind_params.len = sizeof(r);
	ind_params.func = indicate_response;
	return bt_gatt_indicate(conn, &ind_params);
}

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
	for (i = 0; i < ARRAY_SIZE(lns_location_speed_ccc_cfg); i++) {
		if (!bt_addr_le_cmp(dst,
				    &lns_location_speed_ccc_cfg[i].peer)) {
			lns_location_speed_ccc_cfg[i].value = 0;
		}
	}
}

static struct bt_conn_cb conn_callbacks = {
	.disconnected = on_disconnected,
};

int ble_lns_init(void)
{
	int err;

	err = bt_gatt_register((struct bt_gatt_attr *)lns_attrs,
			       ARRAY_SIZE(lns_attrs));
	if (err < 0) {
		goto out;
	}
	bt_conn_cb_register(&conn_callbacks);
out:
	return err;
}

int ble_lns_elevation_update(int32_t elevation)
{
	/* Characteristic: Flags (16 bits) and elevation (sint24) */
	uint8_t notif_value[5];

	notif_value[0] = FLAGS & 0xFF;
	notif_value[1] = (FLAGS >> 8) & 0xFF;
	notif_value[2] = elevation & 0xFF;
	notif_value[3] = (elevation >> 8) & 0xFF;
	notif_value[4] = (elevation >> 16) & 0xFF;
	return bt_gatt_notify(NULL, location_speed_value, &notif_value,
			      sizeof(notif_value), NULL);
}

void ble_lns_elevation_set_reply(struct bt_conn *	conn,
				 enum ble_lns_response	response)
{
	/* Indicate set_altitude result */
	indicate_control_point_result(conn, SET_ELEVATION_OPCODE, response);
}

__weak void on_ble_lns_enabled(void)
{
}

__weak void on_ble_lns_disabled(void)
{
}

__weak void on_ble_lns_elevation_set(struct bt_conn *conn, int32_t elevation)
{
	ble_lns_elevation_set_reply(conn, false);
}
