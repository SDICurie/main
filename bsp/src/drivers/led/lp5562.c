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

#include "drivers/lp5562.h"
#include <string.h>
#include "infra/log.h"
#include <microkernel.h>
#include "drivers/gpio.h"

#define SBA_TIMEOUT             1000

#define LED_ENGINE_MAX 3

#define REG_ENABLE    0x00
#define REG_OP_MODE   0x01
#define REG_B_PWM     0x02
#define REG_B_CURRENT 0x05
#define REG_W_PWM     0x0E
#define REG_W_CURRENT 0x0F
#define REG_CONFIG    0x08
#define REG_PC1       0x09
#define REG_PC2       0x0A
#define REG_PC3       0x0B
#define REG_STATUS    0x0C
#define REG_RESET     0x0D
#define REG_LEDMAP    0x70

#define REG_PRG_EN1 0x10
#define REG_PRG_EN2 0x30
#define REG_PRG_EN3 0x50

/* Op mode register bits */
#define REG_OP_MODE_PROG_ALL 0x15
#define REG_OP_MODE_RUN_ALL  0x2A

/* Enable register bits */
#define REG_ENABLE_LOG_EN  (1 << 7)
#define REG_ENABLE_CHIP_EN (1 << 6)

/* Reset register bits */
#define REG_RESET_RST  0xFF

/* Default intensity */

#define C113_RED_DEFAULT   13
#define C113_GREEN_DEFAULT 127
#define C113_BLUE_DEFAULT  5

#define SMLP34_RED_DEFAULT   15
#define SMLP34_GREEN_DEFAULT 37
#define SMLP34_BLUE_DEFAULT  10

#define C115_RED_DEFAULT   112
#define C115_GREEN_DEFAULT 32
#define C115_BLUE_DEFAULT  80

#define LX404_RED_DEFAULT   62
#define LX404_GREEN_DEFAULT 112
#define LX404_BLUE_DEFAULT  50

/* Device driver internal functions */
static void led_sba_completion_callback(struct sba_request *);
static DRIVER_API_RC i2c_sync(struct td_device *, struct sba_request *);
static void led_lp5562_limit_current(struct td_device *dev);

static int led_lp5562_init(struct td_device *device)
{
	int ret;
	struct sba_device *dev = (struct sba_device *)device;
	struct lp5562_info *led_dev = (struct lp5562_info *)device->priv;

	/* Init wakelock */
	pm_wakelock_init(&led_dev->wakelock);
	pr_debug(LOG_MODULE_DRV, "LP5562 %d init", device->id);

	/* Create a taken semaphore for sba transfers */
	if ((led_dev->i2c_sync_sem = semaphore_create(0)) == NULL) {
		return -1;
	}

	/* Init sba_request struct */
	led_dev->req.addr.slave_addr = dev->addr.slave_addr;
	led_dev->req.request_type = SBA_TX;
	led_dev->req.tx_len = 2;

	if (led_dev->led_en_dev) {
		gpio_cfg_data_t gpio_cfg = { .gpio_type = GPIO_OUTPUT };
		/* FIXME use a generic API for all gpio ports */
		ret = gpio_set_config(led_dev->led_en_dev,
				      led_dev->led_en_pin, &gpio_cfg);
		if (ret != DRV_RC_OK) {
			pr_error(LOG_MODULE_DRV,
				 "LP5562 %d gpio conf failed %d",
				 device->id,
				 ret);
			return ret;
		}
		/* Enable lp5562 device for probing */
		gpio_write(led_dev->led_en_dev, led_dev->led_en_pin, 1);
	} else {
		/* Device is always in power on state */
	}

	/* Reset device */
	ret = led_lp5562_reset(device);

	if (ret != 0) {
		pr_error(LOG_MODULE_DRV, "LP5562 %d probe %d", device->id, ret);
		return ret;
	}

	/* Configure device */
	led_lp5562_enable(device, true);

	/* Configure current */
	led_lp5562_limit_current(device);

	led_lp5562_enable(device, false);

	return 0;
}

struct driver led_lp5562_driver = { .init = led_lp5562_init,
				    .suspend = NULL,
				    .resume = NULL };

static void led_sba_completion_callback(struct sba_request *req)
{
	/* Give led sba semaphore to notify that the i2c transfer is complete */
	semaphore_give((T_SEMAPHORE)(req->priv_data), NULL);
}

static DRIVER_API_RC i2c_sync(struct td_device *dev, struct sba_request *req)
{
	DRIVER_API_RC ret;
	OS_ERR_TYPE ret_os;
	struct lp5562_info *led_dev = (struct lp5562_info *)dev->priv;

	req->priv_data = led_dev->i2c_sync_sem;
	req->callback = led_sba_completion_callback;

	if ((ret =
		     sba_exec_dev_request((struct sba_device *)dev,
					  req)) == DRV_RC_OK) {
		/* Wait for transfer to complete (timeout = 100ms) */
		if ((ret_os =
			     semaphore_take(led_dev->i2c_sync_sem,
					    SBA_TIMEOUT)) != E_OS_OK) {
			if (ret_os == E_OS_ERR_BUSY) {
				ret = DRV_RC_TIMEOUT;
			} else {
				ret = DRV_RC_FAIL;
			}
		} else {
			ret = req->status;
		}
	}

	return ret;
}

void led_lp5562_set_pwm(struct td_device *dev, uint8_t led, uint8_t pwm)
{
	uint8_t command[2];
	struct lp5562_info *led_dev = (struct lp5562_info *)dev->priv;

	if (led >= 3) {
		/* White LED */
		command[0] = REG_W_PWM;
	} else {
		command[0] = REG_B_PWM + led;
	}

	command[1] = pwm;
	led_dev->req.tx_buff = command;
	i2c_sync(dev, &led_dev->req);
}

void led_lp5562_set_current(struct td_device *dev, uint8_t led, uint8_t current)
{
	uint8_t command[2];
	struct lp5562_info *led_dev = (struct lp5562_info *)dev->priv;

	if (led >= 3) {
		/* White LED */
		led = REG_W_CURRENT - REG_B_CURRENT;
	}

	command[0] = REG_B_CURRENT + led;
	command[1] = current;
	led_dev->req.tx_buff = command;
	i2c_sync(dev, &led_dev->req);
}

void led_lp5562_limit_current(struct td_device *dev)
{
#ifdef CONFIG_LP_5562_LED_SMLP34
	led_lp5562_set_current(dev, 0, SMLP34_BLUE_DEFAULT);
	led_lp5562_set_current(dev, 1, SMLP34_GREEN_DEFAULT);
	led_lp5562_set_current(dev, 2, SMLP34_RED_DEFAULT);
#endif
#ifdef CONFIG_LP_5562_LED_C113
	led_lp5562_set_current(dev, 0, C113_BLUE_DEFAULT);
	led_lp5562_set_current(dev, 1, C113_GREEN_DEFAULT);
	led_lp5562_set_current(dev, 2, C113_RED_DEFAULT);
#endif
#ifdef CONFIG_LP_5562_LED_C115
	led_lp5562_set_current(dev, 0, C115_BLUE_DEFAULT);
	led_lp5562_set_current(dev, 1, C115_GREEN_DEFAULT);
	led_lp5562_set_current(dev, 2, C115_RED_DEFAULT);
#endif
#ifdef CONFIG_LP_5562_LED_LX404
	led_lp5562_set_current(dev, 0, LX404_BLUE_DEFAULT);
	led_lp5562_set_current(dev, 1, LX404_GREEN_DEFAULT);
	led_lp5562_set_current(dev, 2, LX404_RED_DEFAULT);
#endif
}

static uint8_t led_lp5562_get_reg(struct td_device *dev, uint8_t index)
{
	struct lp5562_info *led_dev = (struct lp5562_info *)dev->priv;
	uint8_t val = 0;
	uint8_t reg = index;

	led_dev->req.tx_len = 1;
	led_dev->req.rx_len = 1;
	led_dev->req.tx_buff = &reg;
	led_dev->req.rx_buff = &val;

	led_dev->req.request_type = SBA_TRANSFER;
	i2c_sync(dev, &led_dev->req);
	led_dev->req.request_type = SBA_TX;
	led_dev->req.tx_len = 2;
	led_dev->req.rx_len = 0;

	return val;
}

uint8_t led_lp5562_get_status(struct td_device *dev)
{
	struct lp5562_info *led_dev = (struct lp5562_info *)dev->priv;
	uint8_t eng_en = led_lp5562_get_reg(dev, REG_ENABLE) & 0x3F;

	/* Note: reading the status resets the bits */
	led_dev->status |= led_lp5562_get_reg(dev, REG_STATUS) & 0x07;

	/* If partial end of pattern, check engines status */
	if (led_dev->status != 0x07 && eng_en != 0 &&
	    led_lp5562_get_reg(dev, REG_PC1) == 0 &&
	    led_lp5562_get_reg(dev, REG_PC2) == 0 &&
	    led_lp5562_get_reg(dev, REG_PC3) == 0)
		/* Force status to 'all is completed' */
		led_dev->status = 0x07;

	return led_dev->status;
}

void led_lp5562_config(struct td_device *dev, uint8_t config, uint8_t led_map)
{
	struct lp5562_info *led_dev = (struct lp5562_info *)dev->priv;
	uint8_t command[2];

	led_dev->req.tx_buff = command;

	led_dev->config = config;
	led_dev->led_map = led_map;

	/* Write config */
	command[0] = REG_CONFIG;
	command[1] = config;
	i2c_sync(dev, &led_dev->req);

	/* Write led_map */
	command[0] = REG_LEDMAP;
	command[1] = led_map;
	i2c_sync(dev, &led_dev->req);
}

int led_lp5562_reset(struct td_device *dev)
{
	struct lp5562_info *led_dev = (struct lp5562_info *)dev->priv;
	/* Reset device */
	uint8_t cmd[2] = { REG_RESET, REG_RESET_RST };

	led_dev->req.tx_buff = cmd;
	return i2c_sync(dev, &led_dev->req);
}

void led_lp5562_program_pattern(struct td_device *	dev,
				struct lp5562_pattern * p_eng1,
				struct lp5562_pattern * p_eng2,
				struct lp5562_pattern * p_eng3)
{
	struct lp5562_info *led_dev = (struct lp5562_info *)dev->priv;

	uint8_t command[33];

	led_dev->req.tx_buff = command;
	/* Config all engines in load mode */
	command[1] = REG_OP_MODE_PROG_ALL;
	command[0] = REG_OP_MODE;
	i2c_sync(dev, &led_dev->req);

	if (p_eng1) {
		command[0] = REG_PRG_EN1;
		memcpy(&command[1], p_eng1->pattern, p_eng1->size);
		led_dev->req.tx_len = p_eng1->size + 1;
		i2c_sync(dev, &led_dev->req);
	}
	if (p_eng2) {
		command[0] = REG_PRG_EN2;
		memcpy(&command[1], p_eng2->pattern, p_eng2->size);
		led_dev->req.tx_len = p_eng2->size + 1;
		i2c_sync(dev, &led_dev->req);
	}
	if (p_eng3) {
		command[0] = REG_PRG_EN3;
		memcpy(&command[1], p_eng3->pattern, p_eng3->size);
		led_dev->req.tx_len = p_eng3->size + 1;
		i2c_sync(dev, &led_dev->req);
	}

	/* Config engines in run mode */
	command[1] = REG_OP_MODE_RUN_ALL;
	command[0] = REG_OP_MODE;
	led_dev->req.tx_len = 2;
	i2c_sync(dev, &led_dev->req);
}

void led_lp5562_start(struct td_device *dev, uint8_t run_mask)
{
	struct lp5562_info *led_dev = (struct lp5562_info *)dev->priv;
	uint8_t command[2];

	led_dev->req.tx_buff = command;

	/* Initialize status */
	led_dev->status = 0;

	/* Start engines */
	command[0] = REG_ENABLE;
	command[1] = REG_ENABLE_CHIP_EN | (run_mask & 0x3F);
	i2c_sync(dev, &led_dev->req);
}

void led_lp5562_set_mode(struct td_device *dev, uint8_t mode_mask)
{
	struct lp5562_info *led_dev = (struct lp5562_info *)dev->priv;
	uint8_t command[2];

	led_dev->req.tx_buff = command;

	/* Set engines mode */
	command[0] = REG_OP_MODE;
	command[1] = mode_mask & 0x3F;
	i2c_sync(dev, &led_dev->req);
}

void led_lp5562_enable(struct td_device *dev, bool enable)
{
	struct lp5562_info *led_dev = (struct lp5562_info *)dev->priv;

	if (led_dev->led_en_dev) {
		gpio_write(led_dev->led_en_dev, led_dev->led_en_pin, enable);
		if (!enable) {
			/* Device power off */
			return;
		}
	}
	/* 'Setting EN pin low resets the CHIP_EN state to 0. Allow 500 µs delay
	 * after setting chip_en bit to '1''
	 * local_task_sleep_ms(1) is based on os tick, so it's possible that we'll
	 * wait less than 500us so ask 2ms to be sure */
	local_task_sleep_ms(2);
	uint8_t command[2];
	command[0] = REG_ENABLE;
	command[1] = enable ? REG_ENABLE_CHIP_EN : 0;
	led_dev->req.tx_buff = command;
	i2c_sync(dev, &led_dev->req);
	if (enable) {
		/* enforce: 'Delay between consecutive I2C writes to ENABLE register (00h)
		 * need to be longer than 488 μs'  */
		local_task_sleep_ms(2);

		/* Configure the device */
		led_lp5562_config(dev, led_dev->config, led_dev->led_map);
	}
}
