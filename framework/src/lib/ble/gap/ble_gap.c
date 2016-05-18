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

#include "lib/ble/gap/ble_gap.h"

#include <string.h>
#include <bluetooth/gatt.h>
#include <misc/byteorder.h>

static int read_string(struct bt_conn *conn,
		       const struct bt_gatt_attr *attr,
		       void *buf, uint16_t buf_len,
		       uint16_t offset, const char *value)
{
	size_t str_len = strlen(value);

	return bt_gatt_attr_read(conn, attr, buf, buf_len, offset, value,
				 str_len);
}

__attribute__((weak)) ssize_t on_gap_rd_device_name(
	struct bt_conn *conn,
	const struct bt_gatt_attr *
	attr,
	void *buf, uint16_t len,
	uint16_t offset)
{
	const char *value = "Curie default name";

	return read_string(conn, attr, buf, len, offset, value);
}

__attribute__((weak)) ssize_t on_gap_rd_appearance(
	struct bt_conn *conn,
	const struct bt_gatt_attr *
	attr,
	void *buf, uint16_t len,
	uint16_t offset)
{
	uint16_t appearance = 0;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &appearance,
				 sizeof(appearance));
}

__attribute__((weak)) ssize_t on_gap_rd_ppcp(struct bt_conn *conn,
					     const struct bt_gatt_attr *attr,
					     void *buf, uint16_t len,
					     uint16_t offset)
{
	/* Default values */
	struct gap_ppcp ppcp;

	ppcp.interval_min = sys_cpu_to_le16(10);
	ppcp.interval_max = sys_cpu_to_le16(50);
	ppcp.slave_latency = sys_cpu_to_le16(0);
	ppcp.link_sup_to = sys_cpu_to_le16(200);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &ppcp,
				 sizeof(ppcp));
}

static const struct bt_gatt_attr gap_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_GAP),
	BT_GATT_CHARACTERISTIC(BT_UUID_GAP_DEVICE_NAME, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_GAP_DEVICE_NAME, BT_GATT_PERM_READ,
			   on_gap_rd_device_name, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_GAP_APPEARANCE, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_GAP_APPEARANCE, BT_GATT_PERM_READ,
			   on_gap_rd_appearance, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_GAP_PPCP, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_GAP_PPCP, BT_GATT_PERM_READ,
			   on_gap_rd_ppcp, NULL, NULL),
};

int ble_gap_init(void)
{
	/* cast to discard the const qualifier */
	return bt_gatt_register((struct bt_gatt_attr *)gap_attrs,
				ARRAY_SIZE(gap_attrs));
}
