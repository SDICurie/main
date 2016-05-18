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
#include <microkernel.h>
#include "infra/log.h"
#include "drivers/gpio.h"
#include "drivers/nfc_stn54.h"

#ifdef CONFIG_NFC_COMPARATOR_IRQ
#include "machine.h"
#include "drivers/soc_comparator.h"
#endif

#define NFC_WAKELOCK_TIMEOUT    1000
#define NFC_SBA_TIMEOUT                 1000

#define NFC_ESE_CS_PIN                  13 /* SPI1_M_CS_B[2] */

static nfc_stn54_rx_handler_t nfc_rx_handler = NULL;
/* Device driver internal functions */
static void nfc_sba_completion_callback(struct sba_request *);
static DRIVER_API_RC i2c_sync(struct td_device *, struct sba_request *);

#ifdef CONFIG_NFC_COMPARATOR_IRQ
static void nfc_irq_req_callback(void *priv)
#else
static void nfc_irq_req_callback(bool state, void *priv)
#endif
{
	/* priv = pointer to NFC 'struct td_device *' */
	if (nfc_rx_handler != NULL) {
		nfc_rx_handler((void *)priv);
	}
}

void nfc_stn54_config_irq_out(void)
{
	int ret;
	struct td_device *dev = (struct td_device *)&pf_sba_device_nfc;
	struct nfc_stn54_info *nfc_dev = (struct nfc_stn54_info *)dev->priv;

#ifdef CONFIG_NFC_COMPARATOR_IRQ
	struct td_device *comp_dev =
		(struct td_device *)&pf_device_soc_comparator;
	ret = comp_configure(
		comp_dev,
		nfc_dev->stn_irq_pin,
		CMP_REF_POL_POS, /* polarity */
		CMP_REF_SEL_B, /* reference */
		nfc_irq_req_callback,
		dev);
#else
	gpio_cfg_data_t pin_cfg;
	pin_cfg.gpio_type = GPIO_INTERRUPT;
	pin_cfg.int_type = EDGE;
	pin_cfg.int_polarity = ACTIVE_HIGH;
	pin_cfg.int_debounce = DEBOUNCE_OFF;
	pin_cfg.gpio_cb = nfc_irq_req_callback;
	pin_cfg.gpio_cb_arg = dev;
	ret = gpio_set_config(nfc_dev->gpio_dev, nfc_dev->stn_irq_pin, &pin_cfg);
#endif

	if (ret != E_OS_OK) {
		pr_error(LOG_MODULE_DRV,
			 "Failed to configure IRQ_OUT interrupt.");
	}
}

static int nfc_stn54_init(struct td_device *device)
{
	struct sba_device *dev = (struct sba_device *)device;
	struct nfc_stn54_info *nfc_dev = (struct nfc_stn54_info *)device->priv;

	/* Init wakelock */
	pm_wakelock_init(&nfc_dev->wakelock);

	/* Configure GPIO pins */
	if (!nfc_dev->gpio_dev) {
		/* Cannot retrive gpio device */
		pr_error(LOG_MODULE_DRV, "STN54 %d gpio failed", device->id);
		return -1;
	}

	gpio_cfg_data_t pin_cfg = { .gpio_type = GPIO_OUTPUT };
#ifdef CONFIG_STN54_HAS_PWR_EN
	gpio_set_config(nfc_dev->gpio_dev, nfc_dev->stn_pwr_en_pin, &pin_cfg);
#endif
	gpio_set_config(nfc_dev->gpio_dev, nfc_dev->stn_reset_pin, &pin_cfg);
#ifdef CONFIG_STN54_HAS_BOOSTER
	gpio_set_config(nfc_dev->gpio_dev, nfc_dev->booster_reset_pin, &pin_cfg);
#endif

	pr_debug(LOG_MODULE_DRV, "HighZ eSE CS pin.");
	pin_cfg.gpio_type = GPIO_INPUT;
	gpio_set_config(nfc_dev->gpio_dev, NFC_ESE_CS_PIN, &pin_cfg); /* 47 = SPI1_M_CS_B[2] */

	nfc_stn54_power_down();

	/* Configure IRQ_REQ pin */
	nfc_stn54_config_irq_out();

	/* Init sba_request struct */
	nfc_dev->req.addr.slave_addr = dev->addr.slave_addr;

	/* Create a taken semaphore for sba transfers */
	if ((nfc_dev->i2c_sync_sem = semaphore_create(0)) == NULL) {
		pr_error(LOG_MODULE_DRV, "STN54 %d semaphore failed",
			 device->id);
		return -1;
	}

	pr_debug(LOG_MODULE_DRV, "STN54 %d init done.", device->id);
	return 0;
}

struct driver nfc_stn54_driver = {
	.init = nfc_stn54_init,
	.suspend = NULL,
	.resume = NULL
};

static void nfc_sba_completion_callback(struct sba_request *req)
{
	/* Give nfc sba semaphore to notify that the i2c transfer is complete */
	semaphore_give((T_SEMAPHORE)(req->priv_data), NULL);
}

static DRIVER_API_RC i2c_sync(struct td_device *dev, struct sba_request *req)
{
	DRIVER_API_RC ret;
	OS_ERR_TYPE ret_os;
	struct nfc_stn54_info *nfc_dev = (struct nfc_stn54_info *)dev->priv;

#ifndef CONFIG_NFC_COMPARATOR_IRQ
	soc_gpio_mask_interrupt(nfc_dev->gpio_dev, nfc_dev->stn_irq_pin);
#endif

	req->priv_data = nfc_dev->i2c_sync_sem;
	req->callback = nfc_sba_completion_callback;

	if ((ret =
		     sba_exec_dev_request((struct sba_device *)dev,
					  req)) == DRV_RC_OK) {
		/* Wait for transfer to complete (timeout = 100ms) */
		if ((ret_os =
			     semaphore_take(nfc_dev->i2c_sync_sem,
					    NFC_SBA_TIMEOUT)) != E_OS_OK) {
			if (ret_os == E_OS_ERR_BUSY) {
				ret = DRV_RC_TIMEOUT;
			} else {
				ret = DRV_RC_FAIL;
			}
		} else {
			ret = req->status;
		}
	}

#ifndef CONFIG_NFC_COMPARATOR_IRQ
	soc_gpio_unmask_interrupt(nfc_dev->gpio_dev, nfc_dev->stn_irq_pin);
#endif

	return ret;
}

void nfc_stn54_power_down(void)
{
	pr_info(LOG_MODULE_DRV, "NFC power down.");

	struct td_device *dev = (struct td_device *)&pf_sba_device_nfc;
	struct nfc_stn54_info *nfc_dev = (struct nfc_stn54_info *)dev->priv;

	/* Set reset low */
	gpio_write(nfc_dev->gpio_dev, nfc_dev->stn_reset_pin, 0);

#ifdef CONFIG_STN54_HAS_PWR_EN
	/* Cut off power */
	gpio_write(nfc_dev->gpio_dev, nfc_dev->stn_pwr_en_pin, 0);
#endif

#ifdef CONFIG_STN54_HAS_BOOSTER
	/* Cut off booster */
	gpio_write(nfc_dev->gpio_dev, nfc_dev->booster_reset_pin, 0);
#endif
}

void nfc_stn54_power_up(void)
{
	pr_info(LOG_MODULE_DRV, "NFC power-up.");

#ifdef CONFIG_STN54_HAS_PWR_EN
	struct td_device *dev = (struct td_device *)&pf_sba_device_nfc;
	struct nfc_stn54_info *nfc_dev = (struct nfc_stn54_info *)dev->priv;

	/* Power up STN54 */
	gpio_write(nfc_dev->gpio_dev, nfc_dev->stn_pwr_en_pin, 1);
	local_task_sleep_ms(2);
#endif

	nfc_stn54_reset();
}

void nfc_stn54_reset(void)
{
	pr_info(LOG_MODULE_DRV, "NFC reset.");

	struct td_device *dev = (struct td_device *)&pf_sba_device_nfc;
	struct nfc_stn54_info *nfc_dev = (struct nfc_stn54_info *)dev->priv;

	gpio_write(nfc_dev->gpio_dev, nfc_dev->stn_reset_pin, 0);
#ifdef CONFIG_STN54_HAS_BOOSTER
	gpio_write(nfc_dev->gpio_dev, nfc_dev->booster_reset_pin, 0);
#endif

	local_task_sleep_ms(2);

#ifdef CONFIG_STN54_HAS_BOOSTER
	gpio_write(nfc_dev->gpio_dev, nfc_dev->booster_reset_pin, 1);
#endif

	gpio_write(nfc_dev->gpio_dev, nfc_dev->stn_reset_pin, 1);
	local_task_sleep_ms(80);
}

void nfc_stn54_set_rx_handler(nfc_stn54_rx_handler_t handler)
{
	nfc_rx_handler = handler;
}

void nfc_stn54_clear_rx_handler()
{
	nfc_rx_handler = NULL;
}

DRIVER_API_RC nfc_stn54_write(const uint8_t *buf, uint16_t size)
{
	DRIVER_API_RC ret;
	uint8_t retry_cnt = 2;

	struct td_device *dev = (struct td_device *)&pf_sba_device_nfc;
	struct nfc_stn54_info *nfc_dev = (struct nfc_stn54_info *)dev->priv;

	if (!buf) {
		pr_error(LOG_MODULE_DRV, "Wrong arguments.");
		return DRV_RC_INVALID_CONFIG;
	}

	nfc_dev->req.request_type = SBA_TX;
	nfc_dev->req.tx_len = size;
	nfc_dev->req.tx_buff = (uint8_t *)buf;

	do {
		ret = i2c_sync(dev, &nfc_dev->req);
		if (ret == DRV_RC_OK)
			break;

		pr_debug(LOG_MODULE_DRV, "NFC: tx retry, prev status: %d", ret);

		retry_cnt--;
		local_task_sleep_ms(5);
	} while (retry_cnt > 0);

	if (ret != DRV_RC_OK) {
		pr_error(LOG_MODULE_DRV, "NFC: Failed tx data(%d)!", ret);
	}

	return ret;
}

DRIVER_API_RC nfc_stn54_read(uint8_t *buf, uint16_t size)
{
	if (!buf) {
		pr_error(LOG_MODULE_DRV, "Wrong arguments.");
		return DRV_RC_INVALID_CONFIG;
	}

	struct td_device *dev = (struct td_device *)&pf_sba_device_nfc;
	struct nfc_stn54_info *nfc_dev = (struct nfc_stn54_info *)dev->priv;

	nfc_dev->req.request_type = SBA_RX;
	nfc_dev->req.rx_len = size;
	nfc_dev->req.rx_buff = buf;

	return i2c_sync(dev, &nfc_dev->req);
}
