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

#include "lib/ble/tcmd/ble_tcmd.h"

#include <string.h>
#include <errno.h>
#include "infra/tcmd/handler.h"
#include "cfw/cfw.h"
#include "cfw/cfw_service.h"
#include "services/ble_service/ble_service.h"
#include "lib/ble/bas/ble_bas.h"
#include "lib/ble/dis/ble_dis.h"
#include "lib/ble/hrs/ble_hrs.h"
#include "lib/ble/lns/ble_lns.h"
#include "lib/ble/rscs/ble_rscs.h"
#if defined(CONFIG_PACKAGE_ISPP)
#include "ble_ispp.h"
#endif
#include "cfw/cproxy.h"
#include "infra/log.h"

#include "ble_service_int.h"
#include "util/misc.h"

#include "lib/ble/ble_app.h"
#include "ble_service_internal.h"

/* NBLE specific */
#include "gap_internal.h"

#define ANS_LENGTH 120

/* #define BLE_TCMD */

#ifdef BLE_TCMD
struct _enable {
	uint8_t enable_flag;
	uint8_t name;
	uint8_t argc;
};
#endif

struct _args_index {
#ifdef BLE_TCMD
	struct _enable enable;
#endif
};

static const struct _args_index _args = {
#ifdef BLE_TCMD
	.enable = {
		.enable_flag = 2,
		.name = 3,
		.argc = 4
	},
#endif
};

struct _info_for_rsp {
	struct tcmd_handler_ctx *ctx;
	uint8_t enable_flag;
};

struct _ble_service_tcmd_cb {
	cfw_service_conn_t *ble_service_conn;
	struct bt_conn *conn;
	struct tcmd_handler_ctx *indicate_ctx;
	struct tcmd_handler_ctx *rssi_ctx;
	struct tcmd_handler_ctx *ver_ctx;
};

static struct _ble_service_tcmd_cb *_ble_service_tcmd_cb = NULL;

static void on_connected(struct bt_conn *conn, uint8_t err)
{
	struct bt_conn_info info;

	bt_conn_get_info(conn, &info);

	pr_info(LOG_MODULE_BLE, "TCMD connected conn:%p role:%d",
		conn, info.role);
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	pr_info(LOG_MODULE_BLE, "TCMD disconnected conn:%p hci_reason:0x%x",
		conn, reason);
}

#if defined(CONFIG_BLUETOOTH_SMP)
static void on_security_changed(struct bt_conn *conn, bt_security_t level)
{
	pr_info(LOG_MODULE_BLE, "TCMD security_changed level:%d", level);
}
#endif

static struct bt_conn_cb conn_callbacks = {
	.connected = on_connected,
	.disconnected = on_disconnected,
#if defined(CONFIG_BLUETOOTH_SMP)
	.security_changed = on_security_changed,
#endif
};


#if defined(CONFIG_BLUETOOTH_IO_KEYBOARD_DISPLAY) || \
	defined(CONFIG_BLUETOOTH_IO_DISPLAY)
void passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	pr_info(LOG_MODULE_MAIN, "TCMD passkey_display conn:%p passkey:%.6d",
		conn, passkey);
}
#else
#define passkey_display NULL
#endif

#if defined(CONFIG_BLUETOOTH_IO_KEYBOARD) || \
	defined(CONFIG_BLUETOOTH_IO_KEYBOARD_DISPLAY) || \
	defined(CONFIG_BLUETOOTH_IO_DISPLAY_YESNO)
void passkey_entry(struct bt_conn *conn)
{
	pr_info(LOG_MODULE_MAIN, "TCMD passkey_entry conn:%p",
		conn);
}
#else
#define passkey_entry NULL
#endif

#if defined(CONFIG_BLUETOOTH_IO_DISPLAY_YESNO)
void passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	pr_info(LOG_MODULE_MAIN, "TCMD passkey_confirm conn:%p passkey:%.6d",
		conn, passkey);
}
#else
#define passkey_confirm NULL
#endif

#if defined(CONFIG_BLUETOOTH_SMP)
/* pairing failed/cancelled/timeout */
static void pairing_cancel(struct bt_conn *conn)
{
	pr_warning(LOG_MODULE_BLE, "TCMD pairing_cancel conn:%p", conn);
}
#endif

#if defined(CONFIG_BLUETOOTH_SMP)
static const struct bt_conn_auth_cb auth_callbacks = {
	passkey_display,
	passkey_entry,
	passkey_confirm,
	pairing_cancel,
#if defined(CONFIG_BLUETOOTH_BREDR)
	NULL,
#endif
};
#endif

/* check status and release directly ctx */
static int __used _check_ret(int ret, struct tcmd_handler_ctx *ctx)
{
	static char answer[ANS_LENGTH];

	snprintf(answer, ANS_LENGTH, "KO %d", ret);
	if (ret)
		TCMD_RSP_ERROR(ctx, answer);
	return ret;
}

/*
 * Display message if status is KO.
 *
 * @param       status return value in RSP
 * @param[in]   ctx   The context to pass back to responses
 * @return
 */
static int32_t _check_status(int32_t status, struct tcmd_handler_ctx *ctx)
{
	if (status) {
		char answer[ANS_LENGTH];
		snprintf(answer, ANS_LENGTH, "KO %d", status);
		TCMD_RSP_ERROR(ctx, answer);
	}
	return status;
}

#if defined(BLE_TCMD) || \
	defined(CONFIG_TCMD_BLE_DEBUG)
/*
 * Checks api return.
 *
 * @param       status return value by api
 * @param[in]   ctx   The context to pass back to responses
 * @return      none
 */
static int _api_check_status(int32_t status, struct _info_for_rsp *info_for_rsp)
{
	_check_status(status, info_for_rsp->ctx);
	if (status != E_OS_OK) {
		bfree(info_for_rsp);
	}
	return status;
}
#endif

static void version_print(const struct nble_version *ver)
{
	char buf[ANS_LENGTH];

	if (!_ble_service_tcmd_cb->ver_ctx)
		return;

	snprintf(buf, ANS_LENGTH, "%d.%d.%d", ver->major,
		 ver->minor, ver->patch);
	TCMD_RSP_PROVISIONAL(_ble_service_tcmd_cb->ver_ctx, buf);

	snprintf(buf, ANS_LENGTH, "%.20s", ver->version_string);
	TCMD_RSP_PROVISIONAL(_ble_service_tcmd_cb->ver_ctx, buf);

	snprintf(buf, ANS_LENGTH, "Micro-sha1 : %02x%02x%02x%02x", ver->hash[0],
		 ver->hash[1], ver->hash[2], ver->hash[3]);
	TCMD_RSP_FINAL(_ble_service_tcmd_cb->ver_ctx, buf);
	_ble_service_tcmd_cb->ver_ctx = NULL;
}

static void _ble_tcmd_handle_msg(struct cfw_message *msg, void *data)
{
	char answer[ANS_LENGTH] = { 0 };
	struct _info_for_rsp *info_for_rsp = msg->priv;

	switch (CFW_MESSAGE_ID(msg)) {
	case MSG_ID_BLE_ENABLE_RSP:
		if (!_check_status(((struct ble_enable_rsp *)msg)->status,
				   info_for_rsp->ctx)) {
			if (0 == info_for_rsp->enable_flag &&
			    _ble_service_tcmd_cb) {
				if (_ble_service_tcmd_cb->ble_service_conn) {
					cproxy_disconnect(
						_ble_service_tcmd_cb->
						ble_service_conn);
					_ble_service_tcmd_cb->ble_service_conn
						= NULL;
				}
				bfree(_ble_service_tcmd_cb);
			}
			TCMD_RSP_FINAL(info_for_rsp->ctx, "0");
		}
		break;
	case MSG_ID_BLE_ADV_TO_EVT:
		goto _free_msg;
		break;
#ifdef CONFIG_TCMD_BLE_DEBUG
	case MSG_ID_BLE_DBG_RSP: {
		struct ble_dbg_req_rsp *rsp = (struct ble_dbg_req_rsp *)msg;
		snprintf(answer, ANS_LENGTH, "ble dbg: %d/0x%x  %d/%0x",
			 rsp->u0, rsp->u0, rsp->u1,
			 rsp->u1);
		TCMD_RSP_FINAL(info_for_rsp->ctx, answer);
		break;
	}
#endif // CONFIG_TCMD_BLE_DEBUG
	default:
		snprintf(answer, ANS_LENGTH, "Default cfw handler. ID = %d.\n",
			 CFW_MESSAGE_ID(msg));
		TCMD_RSP_FINAL(info_for_rsp->ctx, answer);
		break;
	}
	bfree(info_for_rsp);
_free_msg:
	cfw_msg_free(msg);
}

/*
 * Ble test command initialization.
 *
 */
void ble_tcmd_init(void)
{
	cfw_service_conn_t *ble_service_conn;

	/* if handle is NON-NULL, the service is already open */
	if (_ble_service_tcmd_cb && _ble_service_tcmd_cb->ble_service_conn)
		return;

	ble_service_conn = cproxy_connect(BLE_SERVICE_ID, _ble_tcmd_handle_msg,
					  NULL);
	if (!ble_service_conn) {
		pr_info(LOG_MODULE_BLE, "Cannot connect to BLE Service !");
		return;
	}

	if (!_ble_service_tcmd_cb) {
		_ble_service_tcmd_cb =
			balloc(sizeof(struct _ble_service_tcmd_cb), NULL);
		memset(_ble_service_tcmd_cb, 0,
		       sizeof(struct _ble_service_tcmd_cb));
	}
	_ble_service_tcmd_cb->ble_service_conn = ble_service_conn;

	int client_events[] = {
		MSG_ID_BLE_ADV_TO_EVT,
	};
	bt_conn_cb_register(&conn_callbacks);

#if defined(CONFIG_BLUETOOTH_SMP)
	bt_conn_auth_cb_register(&auth_callbacks);
#endif

	cfw_register_events(ble_service_conn, client_events,
			    sizeof(client_events) / sizeof(int), NULL);

	return;
}

/*
 * documentation should be maintained in: wearable_device_sw/doc/test_command_syntax.md
 */
void tcmd_ble_version(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	if (argc != 2)
		goto print_help;

	_ble_service_tcmd_cb->ver_ctx = ctx;
	ble_gap_get_version(version_print);
	return;

print_help:
	TCMD_RSP_ERROR(ctx, "Usage: ble version");
}
DECLARE_TEST_COMMAND(ble, version, tcmd_ble_version);

#ifdef BLE_TCMD
/*
 * documentation should be maintained in: wearable_device_sw/doc/test_command_syntax.md
 */
void tcmd_ble_enable(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	int8_t enable_flag;
	uint32_t ret;
	struct _info_for_rsp *info_for_rsp;
	struct ble_enable_config en_config = { 0 };
	char *p_name = NULL;

	if (argc < _args.enable.argc - 2)
		goto print_help;

	if ((enable_flag = atoi(argv[_args.enable.enable_flag])) != 0) {
		if (argc < _args.enable.argc - 1)
			goto print_help;

		if (argc == _args.enable.argc)
			p_name = argv[_args.enable.name];
	}

#ifdef CONFIG_BLE_APP
	ble_app_set_device_name((uint8_t *)p_name, true);
#endif

	info_for_rsp = balloc(sizeof(struct _info_for_rsp), NULL);
	info_for_rsp->ctx = ctx;
	info_for_rsp->enable_flag = enable_flag;

	ret =
		ble_service_enable(_ble_service_tcmd_cb->ble_service_conn,
				   enable_flag,
				   &en_config,
				   info_for_rsp);

	_api_check_status(ret, info_for_rsp);

	return;

print_help:
	TCMD_RSP_ERROR(ctx, "Usage: ble enable <enable_flag> [name]");
}

DECLARE_TEST_COMMAND_ENG(ble, enable, tcmd_ble_enable);

/*
 * documentation should be maintained in: wearable_device_sw/doc/test_command_syntax.md
 */
void tcmd_ble_set_name(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	int ret;
	char buf[ANS_LENGTH];

	if (argc != 3)
		goto print_help;

	uint8_t name[BLE_MAX_DEVICE_NAME + 1] = {};
	strncpy(name, argv[2], BLE_MAX_DEVICE_NAME);

	ret = ble_app_set_device_name(name, true);
	snprintf(buf, ANS_LENGTH, "status:%d", ret);

	TCMD_RSP_FINAL(ctx, buf);
	return;

print_help:
	TCMD_RSP_ERROR(ctx, "Usage: ble set_name <name>");
}

DECLARE_TEST_COMMAND_ENG(ble, set_name, tcmd_ble_set_name);
#endif

#ifdef CONFIG_TCMD_BLE_DEBUG
/*
 * Test command to add BLE service : ble add_service <uuid>.
 *
 * - <uuid> - (DIS) 180a, (BAS) 180f
 * - Example: ble add_service 180F
 * - Return: negative: failure, 0: success
 *
 * @param[in]	argc	Number of arguments in the Test Command (including group and name),
 * @param[in]	argv	Table of null-terminated buffers containing the arguments
 * @param[in]	ctx	The context to pass back to responses
 */
void tcmd_ble_add_service(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	int rv;
	struct bt_uuid_128 svc_uuid;
	char buf[ANS_LENGTH];

	if (argc != 3)
		goto print_help;

	svc_uuid.uuid.type =
		(2 == strlen(argv[2]) / 2) ? BT_UUID_TYPE_16 : BT_UUID_TYPE_128;
	switch (svc_uuid.uuid.type) {
	case BT_UUID_TYPE_16:
		BT_UUID_16(&svc_uuid.uuid)->val = strtol(argv[2], NULL, 16);
		break;
	case BT_UUID_TYPE_128: {
		int i;

		for (i = 1;
		     i <=
		     (sizeof(BT_UUID_128(&svc_uuid.uuid)->val) /
		      sizeof(uint32_t));
		     i++) {
			char *walk_ptr =
				(argv[2] + 2 * (16 - sizeof(uint32_t) * i));
			((uint32_t *)&(BT_UUID_128(&svc_uuid.uuid)->val))[i -
									  1] =
				strtoul(walk_ptr, NULL, 16);
			*walk_ptr = '\0';
		}
	}
	break;
	default:
		TCMD_RSP_ERROR(ctx, "Invalid UUID type.");
		return;
	}

	if (BT_UUID_TYPE_16 == svc_uuid.uuid.type) {
		if (BT_UUID_16(&svc_uuid.uuid)->val == BT_UUID_BAS_VAL) {
			rv = ble_bas_init();
		} else
		if (BT_UUID_16(&svc_uuid.uuid)->val == BT_UUID_DIS_VAL) {
			rv = ble_dis_init();
		} else
			rv = -EINVAL;
	} else {
		rv = -EINVAL;
#if defined(CONFIG_PACKAGE_ISPP)
		rv = ble_ispp_init();
#endif
	}
	snprintf(buf, ANS_LENGTH, "status:%d", rv);
	TCMD_RSP_FINAL(ctx, buf);

	return;

print_help:
	TCMD_RSP_ERROR(ctx, "Usage: ble add_service <uuid>");
}

DECLARE_TEST_COMMAND_ENG(ble, add_service, tcmd_ble_add_service);
#endif // CONFIG_TCMD_BLE_DEBUG

#if (defined(BLE_TCMD) && (CONFIG_BLUETOOTH_SMP)) && \
	(defined(CONFIG_BLUETOOTH_IO_DISPLAY_YESNO) || \
	defined(CONFIG_BLUETOOTH_IO_KEYBOARD_DISPLAY) || \
	defined(CONFIG_BLUETOOTH_IO_KEYBOARD))
/*
 * documentation should be maintained in: wearable_device_sw/doc/test_command_syntax.md
 */
void tcmd_ble_send_passkey(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	uint32_t ret;
	struct _info_for_rsp *info_for_rsp;
	uint8_t action;
	uint32_t passkey;

	if (argc != 4)
		goto print_help;

	struct bt_conn *conn = (void *)strtoul(argv[2], NULL, 0);

	info_for_rsp = balloc(sizeof(struct _info_for_rsp), NULL);
	info_for_rsp->ctx = ctx;

	action = strtoul(argv[3], NULL, 0);
	passkey = atoi(argv[4]);

	switch (action) {
	case BLE_GAP_SM_PK_NONE:
		ret = bt_conn_auth_cancel(conn);
		break;
#if defined(CONFIG_BLUETOOTH_IO_DISPLAY_YESNO)
	case BLE_GAP_SM_PK_PASSKEY:
		ret = bt_conn_auth_passkey_confirm(
			conn,
			(passkey ==
			 0) ? false : true);
		break;
#endif
#if defined(CONFIG_BLUETOOTH_IO_KEYBOARD_DISPLAY) || \
		defined(CONFIG_BLUETOOTH_IO_KEYBOARD)
	case BLE_GAP_SM_PK_PASSKEY:
		ret = bt_conn_auth_passkey_entry(conn, passkey);
		break;
#endif
	default:
		ret = -EINVAL;
	}

	if (!_check_ret(ret, ctx))
		TCMD_RSP_FINAL(ctx, NULL);

	bfree(info_for_rsp);

	return;

print_help:
	TCMD_RSP_ERROR(ctx, "Usage: ble key <conn_ref> <action> <pass_key>");
}

DECLARE_TEST_COMMAND_ENG(ble, key, tcmd_ble_send_passkey);
#endif

static void ble_get_bond_info_cb(const struct nble_gap_sm_bond_info *info,
				 const bt_addr_le_t *peer_addr, uint16_t len,
				 void *user_data)
{
	char buf[ANS_LENGTH];
	struct tcmd_handler_ctx *ctx = user_data;

	if (!ctx)
		return;

	if (info->err) {
		TCMD_RSP_ERROR(ctx, NULL);
		return;
	}

	snprintf(buf, ANS_LENGTH, "# of bonded identities addr:%d - irk:%d",
		 info->addr_count, info->irk_count);
#if CONFIG_TCMD_BLE_DEBUG
	if (info->addr_count > 0) {
		int i;

		TCMD_RSP_PROVISIONAL(ctx, buf);
		snprintf(buf, ANS_LENGTH, "Dev # - address/type:");
		for (i = 0; i < info->addr_count; i++) {
			TCMD_RSP_PROVISIONAL(ctx, buf);
			snprintf(buf, ANS_LENGTH,
				 "nr:%02d - %02x:%02x:%02x:%02x:%02x:%02x/%c",
				 i,
				 peer_addr[i].val[5], peer_addr[i].val[4],
				 peer_addr[i].val[3], peer_addr[i].val[2],
				 peer_addr[i].val[1], peer_addr[i].val[0],
				 (!peer_addr[i].type) ? 'P' : 'R');
		}
		;
	}
#endif
	TCMD_RSP_FINAL(ctx, buf);
}

/*
 * documentation should be maintained in: wearable_device_sw/doc/test_command_syntax.md
 */
void tcmd_ble_info(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	uint8_t info_type = 0;
	char buf[ANS_LENGTH];

	if (argc < 2)
		goto print_help;

	if (argc == 3) {
		info_type = atoi(argv[2]);
		if (info_type >= BLE_INFO_LAST)
			goto print_help;
	}

	if (info_type == 0) {
		const bt_addr_le_t *bda;
		bda = ble_app_get_device_bda();
		snprintf(buf, ANS_LENGTH,
			 "Address/Type:%.2x:%.2x:%.2x:%.2x:%.2x:%.2x/%d",
			 bda->val[5], bda->val[4], bda->val[3],
			 bda->val[2], bda->val[1], bda->val[0],
			 bda->type);
		TCMD_RSP_PROVISIONAL(ctx, buf);

		snprintf(buf, ANS_LENGTH, "GAP device name:%s",
			 (char *)ble_app_get_device_name());
		TCMD_RSP_FINAL(ctx, buf);
		return;
	}

	if (info_type == BLE_INFO_BONDING)
		ble_gap_get_bonding_info(ble_get_bond_info_cb, ctx, true);
	else
		_check_status(-EOPNOTSUPP, ctx);

	return;

print_help:
	TCMD_RSP_ERROR(ctx, "Usage: ble info [<info_type>]");
}
DECLARE_TEST_COMMAND(ble, info, tcmd_ble_info);


static uint8_t char_to_byte(char c)
{
	if (c >= 'a')
		return 10 + (c - 'a');
	else if (c >= 'A')
		return 10 + (c - 'A');
	else if (c >= '0')
		return c - '0';
	return 0;
}

static uint8_t str_to_byte(const char *s)
{
	return (char_to_byte(s[0]) << 4) + char_to_byte(s[1]);
}

static size_t unhexlify(char *in, uint8_t *out, size_t buf_size)
{
	size_t i;

	/* Convert ascii hex string into binary data */
	for (i = 0; i < buf_size && in[2 * i]; i++) {
		out[i] = str_to_byte(&in[2 * i]);
	}

	return i;
}

#ifdef CONFIG_SERVICES_BLE_GATTC
static void str_to_uuid(struct bt_uuid *uuid, const char *s)
{
	if (strlen(s) == 36) {
		uuid->type = BT_UUID_TYPE_128;
		BT_UUID_128(uuid)->val[0] = str_to_byte(&s[34]);
		BT_UUID_128(uuid)->val[1] = str_to_byte(&s[32]);
		BT_UUID_128(uuid)->val[2] = str_to_byte(&s[30]);
		BT_UUID_128(uuid)->val[3] = str_to_byte(&s[28]);
		BT_UUID_128(uuid)->val[4] = str_to_byte(&s[26]);
		BT_UUID_128(uuid)->val[5] = str_to_byte(&s[24]);
		BT_UUID_128(uuid)->val[6] = str_to_byte(&s[21]);
		BT_UUID_128(uuid)->val[7] = str_to_byte(&s[19]);
		BT_UUID_128(uuid)->val[8] = str_to_byte(&s[16]);
		BT_UUID_128(uuid)->val[9] = str_to_byte(&s[14]);
		BT_UUID_128(uuid)->val[10] = str_to_byte(&s[11]);
		BT_UUID_128(uuid)->val[11] = str_to_byte(&s[9]);
		BT_UUID_128(uuid)->val[12] = str_to_byte(&s[6]);
		BT_UUID_128(uuid)->val[13] = str_to_byte(&s[4]);
		BT_UUID_128(uuid)->val[14] = str_to_byte(&s[2]);
		BT_UUID_128(uuid)->val[15] = str_to_byte(&s[0]);
	} else {
		uuid->type = BT_UUID_TYPE_16;
		BT_UUID_16(uuid)->val = strtoul(s, NULL, 0);
	}
}

struct ble_discover_int_params {
	struct bt_gatt_discover_params params;
	struct _info_for_rsp *info_for_rsp;
	struct bt_uuid_128 u128;
	uint8_t attr_cnt;
};

int print_uuid(void *data, size_t length, const struct bt_uuid *uuid)
{
	void *p_data = data;
	int i;
	int rv;

	if (BT_UUID_TYPE_16 == uuid->type) {
		rv = snprintf(p_data, length, "uuid:0x%02X",
			      BT_UUID_16(uuid)->val);
		if (rv < 0) return rv;
		p_data += rv;
	} else if (BT_UUID_TYPE_128 == uuid->type) {
		rv = snprintf(p_data, length, "uuid:");
		if (rv < 0) return rv;
		p_data += rv;
		for (i = sizeof(BT_UUID_128(uuid)->val) - 1; i >= 0; i--) {
			rv = snprintf(p_data, (data + length - 1) - p_data,
				      "%02X", BT_UUID_128(uuid)->val[i]);
			if (rv < 0) return rv;
			p_data += rv;
		}
	} else
		return -EFAULT;

	return p_data - data;
}

static uint8_t ble_discover_cb(struct bt_conn *			conn,
			       const struct bt_gatt_attr *	attr,
			       struct bt_gatt_discover_params * params)
{
	struct ble_discover_int_params *disc_params =
		container_of(params, struct ble_discover_int_params, params);
	char answer[ANS_LENGTH];
	char *p_answer = answer;
	int rv;

	/* Check if this is the terminating call */
	if (attr) {
		struct bt_gatt_service *svc_value;
		struct bt_gatt_chrc *chr_value;
		struct bt_gatt_include *inc_value;

		if (BT_GATT_DISCOVER_PRIMARY == params->type) {
			svc_value = attr->user_data;
			rv = print_uuid(p_answer,
					&answer[ANS_LENGTH - 1] - p_answer,
					svc_value->uuid);
			if (rv < 0) goto err;
			p_answer += rv;
			rv = snprintf(p_answer,
				      &answer[ANS_LENGTH - 1] - p_answer,
				      " handle:%d end:%d - ", attr->handle,
				      svc_value->end_handle);
			if (rv < 0) goto err;
		} else if (BT_GATT_DISCOVER_INCLUDE == params->type) {
			inc_value = attr->user_data;
			if (inc_value->uuid) {
				rv = print_uuid(
					p_answer,
					&answer[ANS_LENGTH -
						1] - p_answer,
					inc_value->uuid);
			} else {
				struct bt_uuid_16 uuid_16 = BT_UUID_INIT_16(0);
				rv = print_uuid(
					p_answer,
					&answer[ANS_LENGTH -
						1] - p_answer,
					&uuid_16.uuid);
			}
			if (rv < 0) goto err;
			p_answer += rv;
			rv = snprintf(p_answer,
				      &answer[ANS_LENGTH - 1] - p_answer,
				      " handle:%d start:%d end:%d - ",
				      attr->handle, inc_value->start_handle,
				      inc_value->end_handle);
			if (rv < 0) goto err;
		} else if (BT_GATT_DISCOVER_CHARACTERISTIC == params->type) {
			chr_value = attr->user_data;
			rv = print_uuid(p_answer,
					&answer[ANS_LENGTH - 1] - p_answer,
					chr_value->uuid);
			if (rv < 0) goto err;
			p_answer += rv;
			rv = snprintf(p_answer,
				      &answer[ANS_LENGTH - 1] - p_answer,
				      " handle:%d prop:%d - ",
				      attr->handle,
				      chr_value->properties);
			if (rv < 0) goto err;
		} else if (BT_GATT_DISCOVER_DESCRIPTOR == params->type) {
			rv = print_uuid(p_answer,
					&answer[ANS_LENGTH - 1] - p_answer,
					attr->uuid);
			if (rv < 0) goto err;
			p_answer += rv;
			rv = snprintf(p_answer,
				      &answer[ANS_LENGTH - 1] - p_answer,
				      " handle:%d - ",
				      attr->handle);
			if (rv < 0) goto err;
		}
		TCMD_RSP_PROVISIONAL(disc_params->info_for_rsp->ctx, answer);
		disc_params->attr_cnt += 1;

		return BT_GATT_ITER_CONTINUE;
	} else {
		if (!conn) {
			goto err;
		} else {
			if (!disc_params->attr_cnt)
				TCMD_RSP_FINAL(disc_params->info_for_rsp->ctx,
					       "None");
			else
				TCMD_RSP_FINAL(disc_params->info_for_rsp->ctx,
					       NULL);
			goto out;
		}
	}
err:
	TCMD_RSP_ERROR(disc_params->info_for_rsp->ctx, "KO");

out:
	bfree(disc_params->info_for_rsp);
	bfree(disc_params);
	return BT_GATT_ITER_STOP;
}

/*
 * documentation should be maintained in: wearable_device_sw/doc/test_command_syntax.md
 */
void tcmd_ble_discover(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	uint32_t ret;
	struct ble_discover_int_params *disc_params;
	struct bt_conn *conn;
	struct _info_for_rsp *info_for_rsp;

	if (argc != 5 && argc != 7)
		goto print_help;

	info_for_rsp = balloc(sizeof(struct _info_for_rsp), NULL);
	info_for_rsp->ctx = ctx;

	disc_params = balloc(sizeof(*disc_params), NULL);

	str_to_uuid(&disc_params->u128.uuid, argv[4]);
	/* Copy the entire 128b UUID (includes 16b) */
	if (disc_params->u128.uuid.type == BT_UUID_TYPE_16 &&
	    BT_UUID_16(&disc_params->u128.uuid)->val == 0)
		disc_params->params.uuid = NULL;
	else
		disc_params->params.uuid = &disc_params->u128.uuid;

	if (argc == 7) {
		disc_params->params.start_handle = strtoul(argv[5], NULL, 0);
		disc_params->params.end_handle = strtoul(argv[6], NULL, 0);
	} else {
		disc_params->params.start_handle = 1;
		disc_params->params.end_handle = 0xFFFF;
	}
	conn = (void *)strtoul(argv[2], NULL, 0);

	disc_params->params.func = ble_discover_cb;
	disc_params->params.type = strtoul(argv[3], NULL, 0);

	disc_params->info_for_rsp = info_for_rsp;

	disc_params->attr_cnt = 0;

	ret = bt_gatt_discover(conn, &disc_params->params);
	if (ret < 0) {
		ble_discover_cb(NULL, NULL, &disc_params->params);
	}
	return;

print_help:
	TCMD_RSP_ERROR(
		ctx,
		"Usage: ble discover <conn_ref> <type> <uuid> [start_handle] [end_handle]");
}
DECLARE_TEST_COMMAND(ble, discover, tcmd_ble_discover);

struct ble_write_int_params {
	struct _info_for_rsp *info_for_rsp;
	uint16_t data_length;
	uint8_t data[0];
};

static void ble_write_cb(struct bt_conn *conn, uint8_t err, const void *data)
{
	struct ble_conn_rsp resp;
	char answer[ANS_LENGTH] = { 0 };
	struct ble_write_int_params *p_params =
		container_of(data, struct ble_write_int_params, data);

	resp.status = err;
	resp.conn = conn;

	if (!_check_status(resp.status, p_params->info_for_rsp->ctx)) {
		snprintf(answer, ANS_LENGTH, "conn:%p", resp.conn);
		TCMD_RSP_FINAL(p_params->info_for_rsp->ctx, answer);
	}
	bfree(p_params);
}

/*
 * documentation should be maintained in: wearable_device_sw/doc/test_command_syntax.md
 */
void tcmd_ble_write(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	uint32_t ret;
	struct bt_conn *conn;
	uint16_t char_handle;
	uint16_t offset;
	bool with_resp;
	uint16_t data_idx;
	const uint16_t len = strlen(argv[6]) / 2;
	uint8_t data[len];
	struct _info_for_rsp *info_for_rsp;
	struct ble_write_int_params *params;

	if (argc != 7)
		goto print_help;

	info_for_rsp = balloc(sizeof(struct _info_for_rsp), NULL);
	info_for_rsp->ctx = ctx;

	conn = (void *)strtoul(argv[2], NULL, 0);
	with_resp = (1 == strtoul(argv[3], NULL, 0)) ? true : false;
	char_handle = strtoul(argv[4], NULL, 0);
	offset = strtoul(argv[5], NULL, 0);

	data_idx = unhexlify(argv[6], data, len);

	params = balloc(sizeof(*params) + data_idx, NULL);

	params->info_for_rsp = info_for_rsp;
	params->data_length = data_idx;
	memcpy(&params->data[0], data, params->data_length);

	if (with_resp) {
		ret = bt_gatt_write(conn, char_handle,
				    offset, &params->data[0],
				    params->data_length,
				    ble_write_cb);
		if (ret)
			ble_write_cb(conn, ret, &params->data[0]);
	} else {
		ret = bt_gatt_write_without_response(conn, char_handle,
						     &params->data[0],
						     params->data_length,
						     false);
		/* since write without response does not have callback,
		 * call directly
		 */
		ble_write_cb(conn, ret, &params->data[0]);
	}

	return;

print_help:
	TCMD_RSP_ERROR(
		ctx,
		"Usage: ble write <conn_ref> <withResponse> <handle> <offset> <value>");
}
DECLARE_TEST_COMMAND(ble, write, tcmd_ble_write);

struct ble_read_rsp {
	struct bt_conn *conn;
	int status;
	uint16_t data_length;
	uint8_t data[0];
};

struct ble_read_int_params {
	struct bt_gatt_read_params params;
	struct ble_read_rsp *rsp;
	struct _info_for_rsp *info_for_rsp;
};

static void read_rsp_print(struct ble_read_rsp *	resp,
			   struct _info_for_rsp *	info_for_rsp)
{
	char answer[ANS_LENGTH];
	size_t i;
	char *p_answer = answer;

	if (!_check_status(resp->status, info_for_rsp->ctx)) {
		p_answer += snprintf(p_answer,
				     (&answer[ANS_LENGTH - 1] - p_answer),
				     "conn:%p data:", resp->conn);
		for (i = 0;
		     i < resp->data_length && p_answer <
		     &answer[ANS_LENGTH - 1];
		     i++) {
			p_answer += snprintf(p_answer,
					     (&answer[ANS_LENGTH -
						      1] - p_answer),
					     "%02x", resp->data[i]);
		}
		TCMD_RSP_FINAL(info_for_rsp->ctx, answer);
	}
}

static uint8_t ble_read_cb(struct bt_conn *conn, int err,
			   struct bt_gatt_read_params *params,
			   const void *data, uint16_t length)
{
	struct ble_read_int_params *p_params =
		container_of(params, struct ble_read_int_params, params);
	struct ble_read_rsp *resp;
	uint16_t data_length;

	if (p_params->rsp)
		data_length = length + p_params->rsp->data_length;
	else
		data_length = length;

	resp = (void *)balloc(sizeof(*resp) + data_length, NULL);

	resp->status = err;
	resp->conn = conn;
	resp->data_length = data_length;

	if (data) {
		uint8_t *p_data;

		/* Append the new data to the previous one */
		p_data = resp->data;
		if (p_params->rsp) {
			memcpy(p_data, p_params->rsp->data,
			       p_params->rsp->data_length);
			p_data += p_params->rsp->data_length;
			bfree(p_params->rsp);
		}
		memcpy(p_data, data, length);

		p_params->rsp = resp;

		/* Always continue, until end is reached */
		return BT_GATT_ITER_CONTINUE;
	} else {
		if (p_params->rsp)
			resp = p_params->rsp;
		read_rsp_print(resp, p_params->info_for_rsp);
		bfree(resp);
		bfree(p_params);
	}
	return BT_GATT_ITER_STOP;
}
/*
 * documentation should be maintained in: wearable_device_sw/doc/test_command_syntax.md
 */
void tcmd_ble_read(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	uint32_t ret;
	struct ble_read_int_params *p_params;
	struct bt_conn *conn;
	struct _info_for_rsp *info_for_rsp;

	if (argc != 5)
		goto print_help;

	info_for_rsp = balloc(sizeof(struct _info_for_rsp), NULL);
	info_for_rsp->ctx = ctx;

	p_params = balloc(sizeof(*p_params), NULL);

	conn = (void *)strtoul(argv[2], NULL, 0);
	p_params->params.handle_count = 1;
	p_params->params.single.handle = strtoul(argv[3], NULL, 0);
	p_params->params.single.offset = strtoul(argv[4], NULL, 0);
	p_params->params.func = ble_read_cb;

	p_params->rsp = NULL;
	p_params->info_for_rsp = info_for_rsp;

	ret = bt_gatt_read(conn, &p_params->params);
	if (ret < 0) {
		ble_read_cb(NULL, ret, &p_params->params, NULL, 0);
	}

	return;

print_help:
	TCMD_RSP_ERROR(ctx, "Usage: ble read <conn_ref> <handle> <offset>");
}
DECLARE_TEST_COMMAND(ble, read, tcmd_ble_read);

/*
 * documentation should be maintained in: wearable_device_sw/doc/test_command_syntax.md
 */
void tcmd_ble_read_multiple(int argc, char *argv[],
			    struct tcmd_handler_ctx *ctx)
{
	uint32_t ret;
	struct ble_read_int_params *p_params;
	struct bt_conn *conn;
	struct _info_for_rsp *info_for_rsp;
	const uint16_t handle_count = strtoul(argv[3], NULL, 0);
	uint16_t handles[handle_count];
	size_t idx;

	if (argc < 5 || argc != (handle_count + 4))
		goto print_help;

	info_for_rsp = balloc(sizeof(struct _info_for_rsp), NULL);
	info_for_rsp->ctx = ctx;

	p_params = balloc(sizeof(*p_params), NULL);

	for (idx = 0; idx < handle_count; idx++) {
		handles[idx] = strtoul(argv[4 + idx], NULL, 0);
	}

	conn = (void *)strtoul(argv[2], NULL, 0);
	p_params->params.handle_count = handle_count;
	p_params->params.handles = &handles[0];
	p_params->params.func = ble_read_cb;

	p_params->rsp = NULL;
	p_params->info_for_rsp = info_for_rsp;

	ret = bt_gatt_read(conn, &p_params->params);
	if (ret < 0) {
		ble_read_cb(NULL, ret, &p_params->params, NULL, 0);
	}

	return;

print_help:
	TCMD_RSP_ERROR(
		ctx,
		"Usage: ble read_multiple <conn_ref> <handle count> <handle list>");
}
DECLARE_TEST_COMMAND(ble, read_multiple, tcmd_ble_read_multiple);

static uint8_t ble_notify_cb(struct bt_conn *conn,
			     struct bt_gatt_subscribe_params *params,
			     const void *data, uint16_t length)
{
	char answer[ANS_LENGTH] = { 0 };
	char *p_answer = answer;
	const uint8_t *notif_data = data;
	size_t i;

	if (NULL == data) {
		/* this is the case when subscription write failed */
		bfree(params);
	} else {
		/* notification or indication received */
		p_answer +=
			sprintf(p_answer, "TCMD notif conn:%p handle:%d data:",
				conn,
				params->value_handle);

		for (i = 0;
		     i < (length && p_answer < &answer[ANS_LENGTH - 1]);
		     i++)
			p_answer +=
				snprintf(p_answer,
					 &answer[ANS_LENGTH - 1] - p_answer,
					 "%02x",
					 notif_data[i]);
		pr_info(LOG_MODULE_MAIN, "%s", &answer);
		return BT_GATT_ITER_CONTINUE;
	}
	return BT_GATT_ITER_STOP;
}

/*
 * documentation should be maintained in: wearable_device_sw/doc/test_command_syntax.md
 */
void tcmd_ble_subscribe(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	uint32_t ret;
	struct bt_gatt_subscribe_params *params = balloc(sizeof(*params), NULL);
	struct bt_conn *conn;
	uint8_t data[ANS_LENGTH];

	if (argc != 6)
		goto print_help;

	conn = (void *)strtoul(argv[2], NULL, 0);
	params->ccc_handle = atoi(argv[3]);
	params->notify = ble_notify_cb;
	params->value = strtoul(argv[4], NULL, 0);
	params->value_handle = atoi(argv[5]);

	ret = bt_gatt_subscribe(conn, params);

	snprintf(data, ANS_LENGTH, "status:%d sub:%p", ret, params);
	TCMD_RSP_FINAL(ctx, data);

	if (ret < 0) {
		bfree(params);
	}

	return;

print_help:
	TCMD_RSP_ERROR(
		ctx,
		"Usage: ble subscribe <conn_ref> <ccc_handle> <value> <value_handle>");
}

DECLARE_TEST_COMMAND(ble, subscribe, tcmd_ble_subscribe);

/*
 * documentation should be maintained in: wearable_device_sw/doc/test_command_syntax.md
 */
void tcmd_ble_unsubscribe(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	uint32_t ret;
	uint8_t data[ANS_LENGTH];
	struct bt_gatt_subscribe_params *params;
	struct bt_conn *conn;

	if (argc != 4)
		goto print_help;

	conn = (void *)strtoul(argv[2], NULL, 0);
	params = (void *)strtoul(argv[3], NULL, 0);

	ret = bt_gatt_unsubscribe(conn, params);

	if (!ret)
		bfree(params);

	snprintf(data, ANS_LENGTH, "status:%d", ret);
	TCMD_RSP_FINAL(ctx, data);

	return;

print_help:
	TCMD_RSP_ERROR(ctx, "Usage: ble unsubscribe <conn_ref> <subscribe_ref>");
}

DECLARE_TEST_COMMAND(ble, unsubscribe, tcmd_ble_unsubscribe);

#endif

#ifdef CONFIG_TCMD_BLE_DEBUG
#define BLE_RSSI_EVT_SIZE       32
#define RSSI_SAMPLES_PER_LINE (BLE_RSSI_EVT_SIZE / 2)

static void on_rssi_evt(const int8_t *rssi_data)
{
	BUILD_BUG_ON(BLE_GAP_RSSI_EVT_SIZE != BLE_RSSI_EVT_SIZE);

	/* Prefix: 4 chars & each sample: space + 2 digit & NUL */
	char buf[5 + RSSI_SAMPLES_PER_LINE * 3] = "RSSI";
	int i = 0;
	do {
		char *p = buf + 4;
		int max_len = RSSI_SAMPLES_PER_LINE * 3 + 1;
		do {
			int len = snprintf(p, max_len, " %d", -rssi_data[i++]);
			p += len;
			max_len -= len;
		} while ((i % (RSSI_SAMPLES_PER_LINE)) && (0 < max_len));
		pr_info(LOG_MODULE_MAIN, buf);
	} while (i < BLE_RSSI_EVT_SIZE);
}

static void on_rssi_set_rsp(int status)
{
	if (status == E_OS_OK && _ble_service_tcmd_cb->rssi_ctx) {
		uint8_t data[ANS_LENGTH];

		snprintf(data, ANS_LENGTH, "conn:%p",
			 _ble_service_tcmd_cb->conn);
		TCMD_RSP_FINAL(_ble_service_tcmd_cb->rssi_ctx, data);
		_ble_service_tcmd_cb->rssi_ctx = NULL;
	}
}

/** Test command to start ble rssi reporting: ble rssi <start|stop> [<delta_dBm> <min_count>].
 *
 * - Example: ble rssi start 5 3
 * - The default option is delta_dBm 5 and min_count 3
 * - Return: negative: failure, 0: success
 *
 * @param[in]	argc	Number of arguments in the Test Command (including group and name),
 * @param[in]	argv	Table of null-terminated buffers containing the arguments
 * @param[in]	ctx	The context to pass back to responses
 */
void tcmd_ble_rssi(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	uint32_t delta_dBm = 5;
	uint32_t min_count = 3;
	struct nble_rssi_report_params params = { 0 };

	if (argc < 3 || argc > 5)
		goto print_help;

	if (argc > 3)
		delta_dBm = atoi(argv[3]);

	if (argc > 4)
		min_count = atoi(argv[4]);

	if (!strcmp(argv[2], "start")) {
		params.op = NBLE_GAP_RSSI_ENABLE_REPORT;
		params.channel = 1;
		params.delta_dBm = delta_dBm;
		params.min_count = min_count;
	} else if (!strcmp(argv[2], "stop")) {
		params.op = NBLE_GAP_RSSI_DISABLE_REPORT;
		params.channel = 1;
	} else
		goto print_help;

	_ble_service_tcmd_cb->rssi_ctx = ctx;

	ble_gap_set_rssi_report(&params, _ble_service_tcmd_cb->conn,
				on_rssi_set_rsp, on_rssi_evt);

	pr_debug(LOG_MODULE_BLE, "tcmd_ble_rssi, conn: %p svc_hdl %d",
		 _ble_service_tcmd_cb->conn,
		 _ble_service_tcmd_cb->ble_service_conn);
	return;

print_help:
	TCMD_RSP_ERROR(ctx,
		       "Usage: ble rssi <start|stop> [<delta_dBm> <min_count>]");
}

DECLARE_TEST_COMMAND_ENG(ble, rssi, tcmd_ble_rssi);
#endif // CONFIG_TCMD_BLE_DEBUG

static void indicate_cb(struct bt_conn *conn,
			const struct bt_gatt_attr *attr, int err)
{
	char answer[ANS_LENGTH];

	if (_ble_service_tcmd_cb->indicate_ctx) {
		snprintf(answer, ANS_LENGTH, "status:%d", err);
		TCMD_RSP_FINAL(_ble_service_tcmd_cb->indicate_ctx, answer);
		_ble_service_tcmd_cb->indicate_ctx = NULL;
	}
}

static const struct bt_gatt_attr *get_attr(uint16_t service_uuid)
{
	switch (service_uuid) {
#ifdef CONFIG_BLE_BAS_LIB
	case BT_UUID_BAS_VAL:
		return ble_bas_attr();
#endif
#ifdef CONFIG_BLE_HRS_LIB
	case BT_UUID_HRS_VAL:
		return ble_hrs_attr();
#endif
	default:
		return NULL;
	}
}

void tcmd_ble_notify(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	int ret = 0;
	char data[ANS_LENGTH];
	const struct bt_gatt_attr *attr;

	struct bt_conn *conn;
	uint16_t service_uuid;
	size_t data_len;

	if (argc != 5)
		goto print_help;

	conn = (void *)strtoul(argv[2], NULL, 0);
	service_uuid = strtoul(argv[3], NULL, 0);
	data_len = unhexlify(argv[4], data, sizeof(data));

	attr = get_attr(service_uuid);

	ret = bt_gatt_notify(conn, attr, data, data_len, NULL);
	snprintf(data, sizeof(data), "status:%d", ret);
	TCMD_RSP_FINAL(ctx, data);
	return;

print_help:
	TCMD_RSP_ERROR(ctx,
		       "Usage: ble notify <conn_ref> <service_uuid> <value>");
}

DECLARE_TEST_COMMAND_ENG(ble, notify, tcmd_ble_notify);

void tcmd_ble_indicate(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	int ret = 0;
	uint8_t data[ANS_LENGTH];
	struct bt_gatt_indicate_params ind_params;

	struct bt_conn *conn;
	uint16_t service_uuid;
	size_t data_len;

	if (argc != 5)
		goto print_help;

	conn = (void *)strtoul(argv[2], NULL, 0);
	service_uuid = strtoul(argv[3], NULL, 0);
	data_len = unhexlify(argv[4], data, sizeof(data));

	_ble_service_tcmd_cb->indicate_ctx = ctx;

	ind_params.attr = get_attr(service_uuid);
	ind_params.data = data;
	ind_params.len = data_len;
	ind_params.func = indicate_cb;

	ret = bt_gatt_indicate(conn, &ind_params);

	if (ret) {
		snprintf(data, sizeof(data), "status:%d", ret);
		TCMD_RSP_FINAL(ctx, data);
	}
	return;

print_help:
	TCMD_RSP_ERROR(ctx,
		       "Usage: ble indicate <conn_ref> <service_uuid> <value>");
}

DECLARE_TEST_COMMAND_ENG(ble, indicate, tcmd_ble_indicate);

void tcmd_ble_update(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	int status = -EOPNOTSUPP;
	char answer[ANS_LENGTH];
	uint16_t service_uuid;
	uint32_t value;

	if (argc < 4)
		goto print_help;

	service_uuid = strtoul(argv[2], NULL, 0);
	value = strtoul(argv[3], NULL, 0);

	switch (service_uuid) {
#ifdef CONFIG_BLE_BAS_LIB
	case BT_UUID_BAS_VAL:
		status = ble_bas_update(value);
		break;
#endif
#ifdef CONFIG_BLE_HRS_LIB
	case BT_UUID_HRS_VAL:
		status = ble_hrs_update(value);
		break;
#endif
#ifdef CONFIG_BLE_LNS_LIB
	case BT_UUID_LNS_VAL:
		status = ble_lns_elevation_update(value);
		break;
#endif
#ifdef CONFIG_BLE_RSCS_LIB
	case BT_UUID_RSCS_VAL:
		status = ble_rscs_update(0, value);
#endif
	default:
		break;
	}

	snprintf(answer, sizeof(answer), "status:%d", status);
	TCMD_RSP_FINAL(ctx, answer);

	return;

print_help:
	TCMD_RSP_ERROR(ctx, "Usage: ble update <service_uuid> <values> ... ");
}

DECLARE_TEST_COMMAND_ENG(ble, update, tcmd_ble_update);

#ifdef CONFIG_TCMD_BLE_DEBUG
void tcmd_ble_dbg(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	uint32_t ret;
	struct _info_for_rsp *info_for_rsp;

	info_for_rsp = balloc(sizeof(struct _info_for_rsp), NULL);
	info_for_rsp->ctx = ctx;

	struct ble_dbg_req_rsp *msg = (struct ble_dbg_req_rsp *)
				      cfw_alloc_message_for_service(
		_ble_service_tcmd_cb->ble_service_conn,
		MSG_ID_BLE_DBG_REQ,
		sizeof(*msg), info_for_rsp);
	msg->u0 = (argc >= 3) ? strtoul(argv[2], NULL, 0) : 0;
	msg->u1 = (argc >= 4) ? strtoul(argv[3], NULL, 0) : 0;

	ret = cfw_send_message(msg);
	_api_check_status(ret, info_for_rsp);
}
DECLARE_TEST_COMMAND_ENG(ble, dbg, tcmd_ble_dbg);
#endif // CONFIG_TCMD_BLE_DEBUG

/*
 * documentation should be maintained in: wearable_device_sw/doc/test_command_syntax.md
 */
void tcmd_ble_clear_bonds(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	uint8_t data[ANS_LENGTH];

	snprintf(data, ANS_LENGTH, "status:%d",
		 bt_conn_remove_info(BT_ADDR_LE_ANY));
	TCMD_RSP_FINAL(ctx, data);
}
DECLARE_TEST_COMMAND_ENG(ble, clear, tcmd_ble_clear_bonds);

#if defined(CONFIG_SERVICES_BLE_CENTRAL)
/*
 * documentation should be maintained in: wearable_device_sw/doc/test_command_syntax.md
 */
void tcmd_ble_connect(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	bt_addr_le_t bd_addr;
	struct bt_le_conn_param conn_params;
	char *tail;
	int i;
	struct bt_conn *conn;
	char answer[ANS_LENGTH];

	if (argc != 3)
		goto print_help;

	conn_params.interval_min = 0x0018;  /* 30 ms */
	conn_params.interval_max = 0x0028;  /* 50 ms */
	conn_params.timeout = 100;
	conn_params.latency = 8;

	tail = argv[2];
	for (i = 5; i >= 0; i--) {
		bd_addr.val[i] = strtol(tail, &tail, 16);
		tail++;
	}
	bd_addr.type = strtol(tail, &tail, 16);

	if (bd_addr.type > 1) {
		TCMD_RSP_ERROR(ctx, "Invalid BDA Type");
		return;
	}

	conn = bt_conn_create_le(&bd_addr, &conn_params);
	if (conn) {
		snprintf(answer, ANS_LENGTH, "conn:%p", conn);
		TCMD_RSP_FINAL(ctx, answer);
		/* Since we do not save the connection pointer, we unref it */
		bt_conn_unref(conn);
	} else
		TCMD_RSP_ERROR(ctx, NULL);
	return;

print_help:
	TCMD_RSP_ERROR(ctx, "Usage: ble connect addr/type.");
}

DECLARE_TEST_COMMAND(ble, connect, tcmd_ble_connect);

/*
 * documentation should be maintained in: wearable_device_sw/doc/test_command_syntax.md
 */
void tcmd_ble_disconnect(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	uint32_t ret;

	if (argc != 3)
		goto print_help;

	struct bt_conn *conn = (void *)strtoul(argv[2], NULL, 0);

	ret = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (!_check_ret(ret, ctx)) {
		_ble_service_tcmd_cb->conn = NULL;
		TCMD_RSP_FINAL(ctx, NULL);
	}
	return;

print_help:
	TCMD_RSP_ERROR(ctx, "Usage: ble disconnect <conn_ref>");
}

DECLARE_TEST_COMMAND(ble, disconnect, tcmd_ble_disconnect);

static void ble_scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
			const uint8_t *adv_data, uint8_t len)
{
	pr_info(
		LOG_MODULE_MAIN,
		"TCMD adv_report addr:%02x:%02x:%02x:%02x:%02x:%02x/%d - rssi:%d - type:%02x - len:%d",
		addr->val[5], addr->val[4],
		addr->val[3], addr->val[2],
		addr->val[1], addr->val[0],
		addr->type, rssi,
		adv_type, len);
#ifdef BLE_TCMD
	if (len) {
		char answer[ANS_LENGTH];
		int i;
		char *p_answer = NULL;
		p_answer = answer;
		for (i = 0; i < len && p_answer < &answer[ANS_LENGTH - 1]; i++)
			p_answer +=
				snprintf(p_answer,
					 &answer[ANS_LENGTH - 1] - p_answer,
					 "%02x",
					 adv_data[i]);
		pr_info(LOG_MODULE_MAIN, "TCMD data:%s", &answer);
	}
#endif /* BLE_TCMD */
}

/*
 * documentation should be maintained in: wearable_device_sw/doc/test_command_syntax.md
 */
void tcmd_ble_start_stop_scan(int argc, char *argv[],
			      struct tcmd_handler_ctx *ctx)
{
	int ret;

	if (argc < 3)
		goto print_help;

	if (!strcmp(argv[2], "stop")) {
		ret = bt_le_scan_stop();
	} else {
		struct bt_le_scan_param params;
		if (argc != 6)
			goto print_help;

		params.type = strtol(argv[3], NULL, 0);
		params.interval = strtol(argv[4], NULL, 0);
		params.window = strtol(argv[5], NULL, 0);
		params.filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_ENABLE;
		ret = bt_le_scan_start(&params, ble_scan_cb);
	}
	if (!_check_ret(ret, ctx))
		TCMD_RSP_FINAL(ctx, NULL);

	return;

print_help:
	TCMD_RSP_ERROR(ctx, "Usage: ble scan <op> <type> <interval> <window>");
}
DECLARE_TEST_COMMAND(ble, scan, tcmd_ble_start_stop_scan);

#ifdef CONFIG_BLUETOOTH_SMP
/*
 * documentation should be maintained in: wearable_device_sw/doc/test_command_syntax.md
 */
void tcmd_ble_conn_security(int argc, char *argv[],
			    struct tcmd_handler_ctx *ctx)
{
	struct _info_for_rsp *info_for_rsp;
	int ret;
	struct bt_conn *conn;
	uint8_t sec;

	if (argc != 4)
		goto print_help;

	info_for_rsp = balloc(sizeof(struct _info_for_rsp), NULL);
	info_for_rsp->ctx = ctx;

	conn = (void *)strtoul(argv[2], NULL, 0);
	sec = strtoul(argv[3], NULL, 0);

	ret = bt_conn_security(conn, sec);
	if (!_check_ret(ret, ctx))
		TCMD_RSP_FINAL(ctx, NULL);

	return;

print_help:
	TCMD_RSP_ERROR(ctx, "Usage: ble security <conn_ref> <sec>");
}
DECLARE_TEST_COMMAND(ble, security, tcmd_ble_conn_security);
#endif /* CONFIG_BLUETOOTH_SMP */
#endif

/* @} */
