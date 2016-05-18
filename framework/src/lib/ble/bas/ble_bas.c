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

#include "lib/ble/bas/ble_bas.h"

#include <bluetooth/gatt.h>

static uint8_t battery_level = 33; /* 33 % as init value */

/* Battery Service Variables */

static struct bt_gatt_ccc_cfg blvl_ccc_cfg[1] = {};

#define blvl_ccc_cfg_changed NULL

static ssize_t read_blvl(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(*value));
}

/* Example presentation format */
static const struct bt_gatt_cpf blvl_cpf = {
	.unit = 0x27AD, /* percentage */
	.description = 0x0001, /* first */
	.format = 0x04, /* unsigned 8 bit */
	.exponent = 0,
	.name_space = 1,
};

/* Battery Service Declaration */
static const struct bt_gatt_attr bas_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
	BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY),
	BT_GATT_DESCRIPTOR(BT_UUID_BAS_BATTERY_LEVEL, BT_GATT_PERM_READ,
			   read_blvl, NULL, &battery_level),
	BT_GATT_CCC(blvl_ccc_cfg, blvl_ccc_cfg_changed),
	BT_GATT_CPF((void *)&blvl_cpf),
};

/* Pointer to the HRS value attribute in the above table */
static struct bt_gatt_attr const *const bas_value = &bas_attrs[2];

int ble_bas_init(void)
{
	/* cast to discard the const qualifier */
	return bt_gatt_register((struct bt_gatt_attr *)bas_attrs,
				ARRAY_SIZE(bas_attrs));
}

int ble_bas_update(uint8_t level)
{
	battery_level = level;
	return bt_gatt_notify(NULL, bas_value, &level, sizeof(level), NULL);
}

const struct bt_gatt_attr *ble_bas_attr(void)
{
	return bas_value;
}
