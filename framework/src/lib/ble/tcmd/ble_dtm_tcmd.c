/* INTEL CONFIDENTIAL Copyright 2015 Intel Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to
 * the source code ("Material") are owned by Intel Corporation or its suppliers
 * or licensors.
 * Title to the Material remains with Intel Corporation or its suppliers and
 * licensors.
 * The Material contains trade secrets and proprietary and confidential information
 * of Intel or its suppliers and licensors. The Material is protected by worldwide
 * copyright and trade secret laws and treaty provisions.
 * No part of the Material may be used, copied, reproduced, modified, published,
 * uploaded, posted, transmitted, distributed, or disclosed in any way without
 * Intel's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise.
 *
 * Any license under such intellectual property rights must be express and
 * approved by Intel in writing
 *
 ******************************************************************************/

#include "infra/tcmd/handler.h"

#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

#ifdef CONFIG_UART_NS16550
#include "board.h"
#include "machine.h"
#include "drivers/ipc_uart_ns16550.h"
#endif

#include "nble_driver.h"

/* NBLE specific */
#include "gap_internal.h"

#include "util/misc.h"

#define ANS_LENGTH 120

struct _tx_test {
	uint8_t op;
	uint8_t freq;
	uint8_t len;
	uint8_t pattern;
	uint8_t argc;
};

struct _rx_test {
	uint8_t op;
	uint8_t freq;
	uint8_t argc;
};

struct _test_set_tx_pwr {
	uint8_t dbm;
	uint8_t argc;
};

struct _args_index {
	struct _tx_test tx_test;
	struct _rx_test rx_test;
	struct _test_set_tx_pwr test_set_tx_pwr;
};

static const struct _args_index _args = {
	.tx_test = {
		.op = 2,
		.freq = 3,
		.len = 4,
		.pattern = 5,
		.argc = 6
	},
	.rx_test = {
		.op = 2,
		.freq = 3,
		.argc = 4
	},
	.test_set_tx_pwr = {
		.dbm = 2,
		.argc = 3
	}
};

struct ble_dtm_rx_test {
	uint8_t freq;
};

struct ble_dtm_tx_test {
	uint8_t freq;
	uint8_t len;
	uint8_t pattern;
};

struct ble_set_txpower {
	int8_t dbm;
};

struct ble_dtm_test_result {
	uint16_t mode;
	uint16_t nb;
};

struct ble_test_cmd_param {
	uint8_t mode;
	union {
		struct ble_dtm_rx_test rx;
		struct ble_dtm_tx_test tx;
		struct ble_set_txpower tx_pwr;
	};
};
struct ble_test_cmd {
	struct ble_test_cmd_param param;
	struct tcmd_handler_ctx *ctx;
};

struct ble_dtm_test_rsp {
	int status;
	void *user_data;
	struct ble_dtm_test_result result;
};

static void ble_test_print_rsp(struct ble_dtm_test_rsp *rsp,
			       struct tcmd_handler_ctx *ctx,
			       enum NBLE_TEST_OPCODES	test_opcode)
{
	char answer[ANS_LENGTH];

	if (!rsp->status) {
		if (test_opcode == NBLE_TEST_END_DTM)
			snprintf(answer, ANS_LENGTH,
				 "RX results: Mode = %u. Nb = %u",
				 rsp->result.mode,
				 rsp->result.nb);
		else
			answer[0] = '\0';
		TCMD_RSP_FINAL(ctx, answer);
	} else {
		snprintf(answer, ANS_LENGTH, "KO %d", rsp->status);
		TCMD_RSP_ERROR(ctx, answer);
	}
}

#ifdef CONFIG_UART_NS16550
static int uart_raw_ble_core_tx_rx(uint8_t *send_data, uint8_t send_no,
				   uint8_t *rcv_data, uint8_t rcv_no)
{
	struct td_device *dev = &pf_device_uart_ns16550;
	struct ipc_uart_info *info = dev->priv;
	int i;
	uint8_t rx_byte;
	int res;

	/* send command */
	for (i = 0; i < send_no; i++)
		uart_poll_out(info->uart_dev, send_data[i]);
	/* answer */
	i = 0;
	do {
		res = uart_poll_in(info->uart_dev, &rx_byte);
		if (res == 0) {
			rcv_data[i++] = rx_byte;
		}
	} while (i < rcv_no);
	return i;
}
#endif

static uint8_t ble_dtm_init_done = 0;

static void _ble_test_exec(struct ble_test_cmd *test_cmd)
{
	uint8_t send_data[7];
	uint8_t rcv_data[9] = {};
	int send_no = 0;
	int rcv_no = 0;
	struct ble_dtm_test_rsp resp = {};

	resp.status = -EOPNOTSUPP;

	send_data[0] = H4_CMD;
	send_data[1] = test_cmd->param.mode;
	send_data[2] = HCI_OGF_LE_CMD;

	switch (test_cmd->param.mode) {
	case NBLE_TEST_START_DTM_RX:
		send_data[3] = 1;       /* length */
		send_data[4] = test_cmd->param.rx.freq;
		send_no = 5;
		rcv_no = 7;
		break;
	case NBLE_TEST_START_DTM_TX:
		send_data[3] = 3;       /* length */
		send_data[4] = test_cmd->param.tx.freq;
		send_data[5] = test_cmd->param.tx.len;
		send_data[6] = test_cmd->param.tx.pattern;
		send_no = 7;
		rcv_no = 7;
		break;
	case NBLE_TEST_SET_TXPOWER:
		send_data[3] = 1;       /* length */
		send_data[4] = test_cmd->param.tx_pwr.dbm;
		send_no = 5;
		rcv_no = 7;
		break;
	case NBLE_TEST_START_TX_CARRIER:
		send_data[3] = 1;       /* length */
		send_data[4] = test_cmd->param.tx.freq;
		send_no = 5;
		rcv_no = 7;
		break;
	case NBLE_TEST_END_DTM:
		send_data[3] = 0;       /* length */
		send_no = 4;
		rcv_no = 9;
		break;
	}

#ifdef CONFIG_UART_NS16550
	uart_raw_ble_core_tx_rx(send_data, send_no, rcv_data, rcv_no);
	resp.status = rcv_data[DTM_HCI_STATUS_IDX];
#endif
	uint8_t *p;
	switch (test_cmd->param.mode) {
	case NBLE_TEST_END_DTM:
		p = &rcv_data[DTM_HCI_LE_END_IDX];
		resp.result.nb = ((uint16_t)p[0] << 8) | (uint16_t)p[1];
		break;
	}

	ble_test_print_rsp(&resp, test_cmd->ctx, test_cmd->param.mode);
	bfree(test_cmd);
}

/*
 * documentation should be maintained in: wearable_device_sw/doc/test_command_syntax.md
 */
void ble_tx_test(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	struct ble_test_cmd *test_cmd = balloc(sizeof(*test_cmd), NULL);

	if (argc < _args.tx_test.op + 1)
		goto print_help;

	if (!strcmp(argv[_args.tx_test.op], "stop")) {
		test_cmd->param.mode = NBLE_TEST_END_DTM;
	} else if (!strcmp(argv[_args.tx_test.op], "start")
		   && _args.tx_test.argc == argc) {
		test_cmd->param.mode = NBLE_TEST_START_DTM_TX;
		test_cmd->param.tx.freq = atoi(argv[_args.tx_test.freq]);
		test_cmd->param.tx.len = atoi(argv[_args.tx_test.len]);
		test_cmd->param.tx.pattern = atoi(argv[_args.tx_test.pattern]);
	} else
		goto print_help;

	test_cmd->ctx = ctx;

	if (!ble_dtm_init_done) {
		nble_gap_dtm_init_req(test_cmd);
		ble_dtm_init_done++;
	} else
		_ble_test_exec(test_cmd);
	return;

print_help:
	TCMD_RSP_ERROR(ctx,
		       "Usage: ble tx_test start|stop <freq> <len> <pattern>");
	bfree(test_cmd);
}

DECLARE_TEST_COMMAND(ble, tx_test, ble_tx_test);

/*
 * documentation should be maintained in: wearable_device_sw/doc/test_command_syntax.md
 */
void ble_rx_test(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	struct ble_test_cmd *test_cmd = balloc(sizeof(*test_cmd), NULL);

	if (argc < _args.rx_test.op + 1)
		goto print_help;

	if (!strcmp(argv[_args.tx_test.op], "stop")) {
		test_cmd->param.mode = NBLE_TEST_END_DTM;
	} else if (!strcmp(argv[_args.rx_test.op], "start")
		   && _args.rx_test.argc == argc) {
		test_cmd->param.mode = NBLE_TEST_START_DTM_RX;
		test_cmd->param.rx.freq = atoi(argv[_args.rx_test.freq]);
	} else
		goto print_help;

	test_cmd->ctx = ctx;

	if (!ble_dtm_init_done) {
		nble_gap_dtm_init_req(test_cmd);
		ble_dtm_init_done++;
	} else
		_ble_test_exec(test_cmd);
	return;

print_help:
	TCMD_RSP_ERROR(ctx, "Usage: ble rx_test start|stop <freq>");
	bfree(test_cmd);
}

DECLARE_TEST_COMMAND(ble, rx_test, ble_rx_test);

/*
 * documentation should be maintained in: wearable_device_sw/doc/test_command_syntax.md
 */
void ble_test_set_tx_pwr(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	struct ble_test_cmd *test_cmd = balloc(sizeof(*test_cmd), NULL);

	if (argc != _args.test_set_tx_pwr.argc)
		goto print_help;

	test_cmd->param.mode = NBLE_TEST_SET_TXPOWER;
	test_cmd->param.tx_pwr.dbm = atoi(argv[_args.test_set_tx_pwr.dbm]);

	test_cmd->ctx = ctx;

	if (!ble_dtm_init_done) {
		nble_gap_dtm_init_req(test_cmd);
		ble_dtm_init_done++;
	} else
		_ble_test_exec(test_cmd);
	return;

print_help:
	TCMD_RSP_ERROR(ctx, "Usage: ble test_set_tx_pwr <dbm>");
	bfree(test_cmd);
}

DECLARE_TEST_COMMAND_ENG(ble, test_set_tx_pwr, ble_test_set_tx_pwr);

/*
 * documentation should be maintained in: wearable_device_sw/doc/test_command_syntax.md
 */
void ble_test_carrier(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	struct ble_test_cmd *test_cmd = balloc(sizeof(*test_cmd), NULL);

	if (argc < 3)
		goto print_help;

	if (!strcmp(argv[2], "stop")) {
		test_cmd->param.mode = NBLE_TEST_END_DTM;
	} else if (!strcmp(argv[2], "start")
		   && 4 == argc) {
		test_cmd->param.mode = NBLE_TEST_START_TX_CARRIER;
		test_cmd->param.tx.freq = atoi(argv[3]);
	} else
		goto print_help;

	test_cmd->ctx = ctx;

	if (!ble_dtm_init_done) {
		nble_gap_dtm_init_req(test_cmd);
		ble_dtm_init_done++;
	} else
		_ble_test_exec(test_cmd);
	return;

print_help:
	TCMD_RSP_ERROR(ctx, "Usage: ble tx_carrier start|stop <freq>");
	bfree(test_cmd);
}
DECLARE_TEST_COMMAND_ENG(ble, tx_carrier, ble_test_carrier);


void on_nble_gap_dtm_init_rsp(void *user_data)
{
	struct ble_test_cmd *test_cmd =
		container_of(user_data, struct ble_test_cmd,
			     param);

#ifdef CONFIG_UART_NS16550
	uart_ipc_disable();
#endif

	_ble_test_exec(test_cmd);
}
