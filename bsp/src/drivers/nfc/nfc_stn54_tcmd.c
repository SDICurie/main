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
#include <stdio.h>
#include <stdlib.h>

#include "os/os.h"
#include "util/workqueue.h"
#include "infra/tcmd/handler.h"
#include "infra/log.h"

#include "drivers/gpio.h"
#include "drivers/nfc_stn54.h"

static struct tx_msg {
	uint8_t len;
	uint8_t *msg;
} tx_msg;

static uint8_t nci_reset[] = { 0x84, 0x20, 0x00, 0x01, 0x00 };
static uint8_t nci_init[] = { 0x84, 0x20, 0x01, 0x00 };
static bool is_powered;

/*
 * @addtogroup infra_tcmd
 * @{
 */

/*
 * @defgroup infra_tcmd_nfc NFC Driver Test Commands
 * Interfaces to support NFC Driver Test Commands.
 * @{
 */
void nfc_read_worker(void *priv)
{
	uint8_t rx_data[25];
	int ret_h = -1, ret_p = -1;
	struct td_device *dev = (struct td_device *)priv;
	struct nfc_stn54_info *nfc_dev = (struct nfc_stn54_info *)dev->priv;

	gpio_mask_interrupt(nfc_dev->gpio_dev, nfc_dev->stn_irq_pin);
	ret_h = nfc_stn54_read(&rx_data[0], 4);

	if (ret_h == DRV_RC_OK) {
		pr_info(LOG_MODULE_DRV,
			"rx header:[0x%02x 0x%02x 0x%02x 0x%02x]",
			rx_data[0],
			rx_data[1],
			rx_data[2],
			rx_data[3]);

		uint8_t payload_len = rx_data[3];

		if (payload_len > 0 && payload_len <= (25 - 4)) {
			ret_p = nfc_stn54_read(&rx_data[4], payload_len);
			if (ret_p == DRV_RC_OK) {
				pr_info(LOG_MODULE_DRV,
					"payload len:%d status: %d",
					payload_len,
					ret_p);
				if (payload_len == 0x15) { //INIT RSP
					pr_info(LOG_MODULE_DRV,
						"nfc fw version (hex): %x.%x .",
						rx_data[23],
						rx_data[24]);
				}
			} else {
				pr_error(LOG_MODULE_DRV,
					 "Failed reading payload, status: %d",
					 ret_p);
			}
		}
	} else {
		pr_error(LOG_MODULE_DRV, "Failed reading header, status: %d",
			 ret_h);
	}

	gpio_unmask_interrupt(nfc_dev->gpio_dev, nfc_dev->stn_irq_pin);

#ifdef CONFIG_NFC_COMPARATOR_IRQ
	nfc_stn54e_config_irq_out(NULL);
#endif
}

void nfc_write_worker(void *priv)
{
	struct tx_msg *txm = (struct tx_msg *)priv;

	nfc_stn54_write(txm->msg, txm->len);
}

void simple_nfc_rx_handler(void *priv)
{
	pr_debug(LOG_MODULE_DRV, "Schedule NFC rx workqueue task.");
	workqueue_queue_work(nfc_read_worker, (void *)priv);
}

/*@brief Power up and/or reset the NFC controller.
 *
 * @param[in]   argc       Number of arguments in the Test Command (including group and name),
 * @param[in]   argv       Table of null-terminated buffers containing the arguments
 * @param[in]   ctx        The context to pass back to responses
 */
void nfc_reset(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	nfc_stn54_power_down();
	nfc_stn54_power_up();
	is_powered = 1;
}
DECLARE_TEST_COMMAND(nfc, reset, nfc_reset);
DECLARE_TEST_COMMAND(nfc, pwr_up, nfc_reset);

/*@brief Power down the NFC controller.
 *
 * @param[in]   argc       Number of arguments in the Test Command (including group and name),
 * @param[in]   argv       Table of null-terminated buffers containing the arguments
 * @param[in]   ctx        The context to pass back to responses
 */
void nfc_pwr_down(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	nfc_stn54_power_down();
	is_powered = 0;
}
DECLARE_TEST_COMMAND(nfc, pwr_down, nfc_pwr_down);


/*@brief Send basic commands to NFC controller.
 *
 * The supported commands are 'reset' and 'init'. The purpose is to test the
 * communication with the NFC controller even if no service/sequencer is
 * implemented. After each command, two messages are expected from the controller.
 *
 * @param[in]   argc       Number of arguments in the Test Command (including group and name),
 * @param[in]   argv       Table of null-terminated buffers containing the arguments
 * @param[in]   ctx        The context to pass back to responses
 */
void nfc_cmd(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	if (!is_powered)
		goto print_pwrup;

	if (argc < 3)
		goto print_help;

	nfc_stn54_set_rx_handler(simple_nfc_rx_handler);

	if (!strncmp("init", argv[2], 4)) {
		tx_msg.msg = nci_init;
		tx_msg.len = sizeof(nci_init);
	} else if (!strncmp("reset", argv[2], 5)) {
		tx_msg.msg = nci_reset;
		tx_msg.len = sizeof(nci_reset);
	} else {
		goto print_help;
	}
	workqueue_queue_work(nfc_write_worker, &tx_msg);

	return;

print_pwrup:
	TCMD_RSP_ERROR(ctx, "Controller not powered up. Use: nfc reset");
	return;
print_help:
	TCMD_RSP_ERROR(ctx, "Usage: nfc cmd <reset|init>");
	return;
}
DECLARE_TEST_COMMAND(nfc, cmd, nfc_cmd);

/*
 * @}
 *
 * @}
 */
