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

#include <unistd.h>
#include <errno.h>

/* for container_of */
#include "util/misc.h"

#include "cfw/cfw_service.h"

#include "lib/ble/dis/ble_dis.h"
#include "service_queue.h"
#include "infra/log.h"
#include "ble_service_int.h"

static cfw_service_conn_t *ble_service_conn = NULL;
static void test_ble_enable_request(void);

struct _ble_register_svc {
	 /**< callback function to execute on MSG_ID_xxx_RSP
	  * @param reg this buffer */
	void (*func_cback)(struct _ble_register_svc *reg);
	int cback_param; /**< register parameter @ref BLE_APP_SVC_REG */
};


struct _ble_service_test_cb {
	uint32_t counter;
	struct bt_conn *conn;
	uint8_t battery_level;
};

static struct _ble_service_test_cb _ble_service_test_cb;
const uint8_t ble_dev_name_change[] = "TEST_CHANGE_NAME";

static void on_connected(struct bt_conn *conn, uint8_t err)
{
	_ble_service_test_cb.conn = conn;
	_ble_service_test_cb.counter = 1;

}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	pr_debug(LOG_MODULE_MAIN, "BLE disconnected(conn: %p, hci_reason: 0x%x)",
			conn, reason);

	_ble_service_test_cb.conn = NULL;
	_ble_service_test_cb.counter = 0;
}

static struct bt_conn_cb conn_callbacks = {
		.connected = on_connected,
		.disconnected = on_disconnected,
};

static void client_ble_service_handle_message(struct cfw_message *msg,
					      void *param)
{
	(void)param;

	switch (CFW_MESSAGE_ID(msg)) {
	case MSG_ID_CFW_OPEN_SERVICE_RSP:
	{
		ble_service_conn =
		    (cfw_service_conn_t *) ((cfw_open_conn_rsp_msg_t *)
					     msg)->service_conn;
		bt_conn_cb_register(&conn_callbacks);
	}
		break;
	case MSG_ID_CFW_REGISTER_EVT_RSP:
		break;
	case MSG_ID_BLE_ENABLE_RSP:
		handle_msg_id_ble_enable_rsp(msg);
		break;
	default:
		cfw_print_default_handle_error_msg(LOG_MODULE_BLE, CFW_MESSAGE_ID(msg));
		break;
	}
	cfw_msg_free(msg);
}

void test_ble_service_init(void)
{
	cfw_client_t *ble_client =
	    cfw_client_init(get_service_queue(), client_ble_service_handle_message,
		     "Client BLE");
	_ble_service_test_cb.conn = NULL;
	cfw_open_service_conn(ble_client, BLE_SERVICE_ID, "BLE Test Client");
}

const uint8_t ble_dev_name[] = "Curie 1.0";
static void test_ble_enable_request(void)
{
	struct ble_enable_config en_config;

	const struct bt_le_conn_param conn_params =
	    {MSEC_TO_1_25_MS_UNITS(80), MSEC_TO_1_25_MS_UNITS(150), 0, MSEC_TO_10_MS_UNITS(6000)};

	en_config.p_bda = NULL;
	en_config.central_conn_params = conn_params;

	ble_service_enable(ble_service_conn, 1, &en_config, NULL);
}

void test_ble_service_update_battery_level(uint8_t level)
{
	if (_ble_service_test_cb.counter) {
		if (((++_ble_service_test_cb.counter) % 10) == 0) {
			int ret;

			_ble_service_test_cb.battery_level = level;
			_ble_service_test_cb.counter = 0; /* restart it in response */
			if (ret)
				pr_warning(LOG_MODULE_BLE, "failed to update bat lvl");
		}
	}
}
