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

#ifndef _LP5562_H_
#define _LP5562_H_

#include "drivers/serial_bus_access.h"
#include "infra/pm.h"

/**
 * @defgroup lp5562_driver LP5562 LED Driver
 * LP5562 LED driver API
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "drivers/lp5562.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/drivers/led</tt>
 * <tr><th><b>Config flag</b> <td><tt>LP5562_LED</tt>
 * </table>
 *
 * The LP5562 LED driver API provides low level access to the LP5562 component.
 * This API should not be directly used but through the LED driver API (see \ref led_drivers).
 *
 * @note
 *    The preferred way to play patterns is to use the UI service (see \ref ui_service).
 *
 * @{
 * @ingroup ext_drivers
 */

/* LED id (for the set PWM/current commands) */
#define LED_B 0
#define LED_G 1
#define LED_R 2
#define LED_W 3

/* Config register bits */
#define REG_CONFIG_PWM_HF   (1 << 6)
#define REG_CONFIG_PWR_SAVE (1 << 5)
#define REG_CONFIG_EXT_CLK  (0)
#define REG_CONFIG_INT_CLK  (1)
#define REG_CONFIG_AUTO_CLK (2)

/* Start command masks */
/* Start program and run it until the end */
#define LED_EN1_RUN_MASK (2 << 4)
#define LED_EN2_RUN_MASK (2 << 2)
#define LED_EN3_RUN_MASK (2 << 0)
/* Run one program command and increase PC */
#define LED_EN1_STEP_MASK (1 << 4)
#define LED_EN2_STEP_MASK (1 << 2)
#define LED_EN3_STEP_MASK (1 << 0)
/* Run one program command */
#define LED_EN1_EXEC_MASK (0 << 4)
#define LED_EN2_EXEC_MASK (0 << 2)
#define LED_EN3_EXEC_MASK (0 << 0)
/* Stop engine */
#define LED_EN1_HOLD_MASK (0 << 4)
#define LED_EN2_HOLD_MASK (0 << 2)
#define LED_EN3_HOLD_MASK (0 << 0)

/* Set mode command masks */
/* Set engine in RUN mode */
#define LED_EN1_RUN_MODE (2 << 4)
#define LED_EN2_RUN_MODE (2 << 2)
#define LED_EN3_RUN_MODE (2 << 0)
/* Set engine in Direct Control mode (PWM) */
#define LED_EN1_DC_MODE (3 << 4)
#define LED_EN2_DC_MODE (3 << 2)
#define LED_EN3_DC_MODE (3 << 0)
/* Disable engine */
#define LED_EN1_STOP_MODE (0 << 4)
#define LED_EN2_STOP_MODE (0 << 2)
#define LED_EN3_STOP_MODE (0 << 0)

/* Engine id for the LED_MAP macro */
#define LED_EN1 (1)
#define LED_EN2 (2)
#define LED_EN3 (3)
#define LED_PWM (0)
/* Led map register bits */
#define REG_LEDMAP_SHIFT_B (0)
#define REG_LEDMAP_SHIFT_G (2)
#define REG_LEDMAP_SHIFT_R (4)
#define REG_LEDMAP_SHIFT_W (6)
#define LED_MAP(engine, RGB) (engine << REG_LEDMAP_SHIFT_ ## RGB)

/**
 * LED driver structure
 */
extern struct driver led_lp5562_driver;

/**
 * Structure to handle a LP5562 device
 */
struct lp5562_info {
	uint8_t led_en_pin;              /*!< GPIO pin used to enable the device */
	uint8_t led_map;                 /*!< Default led map to use */
	uint8_t config;                  /*!< Default configuration to use */
	uint8_t status;                  /*!< Current status */
	struct pm_wakelock wakelock;     /*!< Wakelock */
	/* Internal driver fields */
	struct sba_request req;          /*!< SBA request object used to transfer i2c data */
	T_SEMAPHORE i2c_sync_sem;        /*!< Semaphore to wait for an i2c transfer to complete */
	struct td_device *led_en_dev;    /*!< GPIO device handler for led enable */
};

/**
 * Structure to program led patterns
 */
struct lp5562_pattern {
	uint16_t *pattern;
	uint8_t size;
};

/**
 * Enable/disable lp5562 device.
 *
 * @param dev    LED device to use
 * @param enable true to enable or false to disable
 */
void led_lp5562_enable(struct td_device *dev, bool enable);

/**
 * Reset lp5562 device.
 *
 * @param dev LED device to use
 */
int led_lp5562_reset(struct td_device *dev);

/**
 * Get lp5562 device status.
 *
 * @param dev LED device to use
 */
uint8_t led_lp5562_get_status(struct td_device *dev);

/**
 * Set lp5562 device configuration
 *
 * @param dev     LED device to use
 * @param config  New device configuration (default is internal clock, low power)
 * @param led_map New led_map. Default is
 *                LED_MAP(LED_EN3, R) | LED_MAP(LED_EN2, G) | LED_MAP(LED_EN1, B) | LED_MAP(PWM, W)
 */
void led_lp5562_config(struct td_device *dev, uint8_t config, uint8_t led_map);

/**
 * Set lp5562 device led current.
 *
 * @param dev     LED device to use
 * @param led     LED to configure (LED_B, lED_G, lED_R or LED_W)
 * @param current Current value (0->255, 0mA->25.5mA)
 */
void led_lp5562_set_current(struct td_device *dev, uint8_t led, uint8_t current);

/**
 * Set lp5562 device led PWM value.
 *
 * @param dev LED device to use
 * @param led LED to configure (LED_B, lED_G, lED_R or LED_W)
 * @param pwm PWM value (0->255)
 */
void led_lp5562_set_pwm(struct td_device *dev, uint8_t led, uint8_t pwm);

/**
 * Program lp5562 pattern engines.
 *
 * @param dev    LED device to use
 * @param p_eng1 Pattern to program for engine1
 * @param p_eng2 Pattern to program for engine2
 * @param p_eng3 Pattern to program for engine3
 */
void led_lp5562_program_pattern(struct td_device *	dev,
				struct lp5562_pattern * p_eng1,
				struct lp5562_pattern * p_eng2,
				struct lp5562_pattern * p_eng3);

/**
 * Start pattern engines on lp5562 device.
 *
 * @param dev      LED device to use
 * @param run_mask Engines to start
 *      (LED_EN1_RUN_MASK | LED_EN2_RUN_MASK | LED_EN3_RUN_MASK)
 */
void led_lp5562_start(struct td_device *dev, uint8_t run_mask);

/**
 * Set engine mode on lp5562 device.
 *
 * @param dev       LED device to use
 * @param mode_mask Mode to operate device engines
 *      (LED_EN1_RUN_MODE | LED_EN2_DC_MODE | LED_EN3_STOP_MODE)
 */
void led_lp5562_set_mode(struct td_device *dev, uint8_t mode_mask);

/** @} */

#endif /* _LP5562_H_ */
