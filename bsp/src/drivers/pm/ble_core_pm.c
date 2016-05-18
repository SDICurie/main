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

#include <errno.h>
#include <stdbool.h>
#include "infra/device.h"
#include "infra/log.h"
#include "drivers/ble_core_pm.h"
#include "drivers/gpio.h"

#define DRV_NAME "ble_core_pm"

static int ble_core_pm_setup(struct td_device *dev, INT_POLARITY polarity);

static void ble_core_pm_callback(bool state, void *priv_data)
{
	struct td_device *dev = (struct td_device *)priv_data;
	struct ble_core_pm_info *priv = (struct ble_core_pm_info *)dev->priv;

	if (ACTIVE_HIGH == priv->gpio_pin_polarity) {
		pm_wakelock_acquire(&priv->ble_core_pm_wakelock);
		ble_core_pm_setup(dev, ACTIVE_LOW);
	} else {
		pm_wakelock_release(&priv->ble_core_pm_wakelock);
		ble_core_pm_setup(dev, ACTIVE_HIGH);
	}

	pr_debug(LOG_MODULE_DRV, "%s %d: connected aon %d", DRV_NAME, dev->id,
		 priv->wakeup_pin);
}

static int ble_core_pm_setup(struct td_device *dev, INT_POLARITY polarity)
{
	int ret = 0;
	struct ble_core_pm_info *priv = (struct ble_core_pm_info *)dev->priv;
	gpio_cfg_data_t config;

	/* Configure GPIO AON as wake up source based on polarity */
	config.gpio_type = GPIO_INTERRUPT;
	config.int_type = LEVEL;
	config.int_polarity = polarity;
	config.int_debounce = DEBOUNCE_ON;
	config.int_ls_sync = LS_SYNC_OFF;
	config.gpio_cb = ble_core_pm_callback;
	config.gpio_cb_arg = dev;

	priv->gpio_pin_polarity = polarity;

	gpio_deconfig(priv->gpio_dev, priv->wakeup_pin);

	ret = gpio_set_config(priv->gpio_dev, priv->wakeup_pin, &config);

	pr_debug(LOG_MODULE_DRV, "%s %d: use aon - %d (%d)", DRV_NAME, dev->id,
		 priv->wakeup_pin, ret);

	return ret;
}

bool ble_core_pm_is_active(struct td_device *dev)
{
	struct ble_core_pm_info *priv = (struct ble_core_pm_info *)dev->priv;
	bool active = false;

	gpio_read(priv->gpio_dev, priv->wakeup_pin, &active);
	return active;
}

static int ble_core_pm_init(struct td_device *dev)
{
	struct ble_core_pm_info *priv = (struct ble_core_pm_info *)dev->priv;

	if (!priv->gpio_dev)
		return -EINVAL;

	pm_wakelock_init(&priv->ble_core_pm_wakelock);

	return ble_core_pm_setup(dev, ACTIVE_HIGH);
}

static int ble_core_pm_suspend(struct td_device *dev, PM_POWERSTATE state)
{
	if (ble_core_pm_is_active(dev)) {
		return -1;
	}
	return 0;
}

static int ble_core_pm_resume(struct td_device *dev)
{
	return 0;
}

struct driver ble_core_pm_driver = {
	.init = ble_core_pm_init,
	.suspend = ble_core_pm_suspend,
	.resume = ble_core_pm_resume
};
