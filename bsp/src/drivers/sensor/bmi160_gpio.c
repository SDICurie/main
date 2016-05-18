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

#include "util/workqueue.h"
#include "bmi160_bus.h"
#include "bmi160_support.h"
#include "bmi160_gpio.h"
#include "drivers/gpio.h"
#include "infra/tcmd/handler.h"

uint32_t bmi160_any_motion_timestamp = 0;
#ifdef CONFIG_BMI160_DOUBLE_TAP
uint8_t bmi160_wait_double_tap = 0;
#endif
uint8_t bmi160_wait_first_fifo_read_after_anymotion = 0;
uint8_t bmi160_need_update_wm_interrupt = 0;

static void handle_sensor_reg(struct bmi160_rt_t *	bmi160_rt,
			      uint8_t			sensor_type)
{
	uint32_t buffer[3]; /** gyro/mag need 12 bytes, accle need 6 bytes */
	uint8_t sensor_int_en_mask = (1 << sensor_type);
	uint8_t data_size = BMI160_MAG_FRAME_SIZE;

	if (sensor_type >= BMI160_SENSOR_COUNT) { /** current sensor count is 2 */
		pr_debug(LOG_MODULE_BMI160, "Invalid sensor type: %u",
			 sensor_type);
		return;
	}
	if (sensor_type == BMI160_SENSOR_ACCEL)
		data_size = BMI160_ACCEL_FRAME_SIZE;
	else if (sensor_type == BMI160_SENSOR_GYRO)
		data_size = BMI160_GYRO_FRAME_SIZE;

	if ((bmi160_rt->int_en[BMI160_OFFSET_DRDY] & sensor_int_en_mask))
		bmi160_rt->reg_data_read_funs[sensor_type]((uint8_t *)buffer,
							   data_size);
	if (_Usually(bmi160_rt->read_callbacks[sensor_type])) {
		bmi160_rt->read_callbacks[sensor_type]
			((uint8_t *)buffer, data_size,
			bmi160_rt->cb_priv_data[sensor_type]);
	}
}

static void handle_reg_data(struct bmi160_rt_t *bmi160_rt)
{
	uint8_t user_status = 0;

	if (!bmi160_rt->int_en[BMI160_OFFSET_DRDY]) /* check if drdy is enabled */
		return;

	if (bmi160_read_reg(BMI160_USER_STAT_ADDR, &user_status))
		return;

	if (!(bmi160_rt->fifo_en & (1 << BMI160_SENSOR_ACCEL)) &&
	    BMI160_GET_BITSLICE(user_status, BMI160_USER_STAT_DATA_RDY_ACCEL))
		handle_sensor_reg(bmi160_rt, BMI160_SENSOR_ACCEL);

	if (!(bmi160_rt->fifo_en & (1 << BMI160_SENSOR_GYRO)) &&
	    BMI160_GET_BITSLICE(user_status, BMI160_USER_STAT_DATA_RDY_GYRO))
		handle_sensor_reg(bmi160_rt, BMI160_SENSOR_GYRO);

#if BMI160_ENABLE_MAG
	if (!(bmi160_rt->fifo_en & (1 << BMI160_SENSOR_MAG)) &&
	    BMI160_GET_BITSLICE(user_status, BMI160_USER_STAT_DATA_RDY_MAG))
		handle_sensor_reg(bmi160_rt, BMI160_SENSOR_MAG);
#endif
}

#if BMI160_SUPPORT_FIFO_INT_DATA_REPORT
static void report_fifo_data(uint8_t sensor_type, uint8_t *buffer,
			     uint16_t frame_cnt,
			     uint8_t frame_size)
{
	uint8_t sensor_int_en_mask = (1 << sensor_type);
	struct bmi160_rt_t *bmi160_rt = bmi160_get_ptr();

	if (bmi160_rt->int_en[BMI160_OFFSET_FIFO_WM] & sensor_int_en_mask) {
		if (_Usually(bmi160_rt->read_callbacks[sensor_type])) {
			bmi160_rt->read_callbacks[sensor_type]
				((uint8_t *)buffer, frame_cnt * frame_size,
				bmi160_rt->cb_priv_data[sensor_type]);
		}
	}
}
#endif

static void handle_fifo(struct bmi160_rt_t *bmi160_rt, uint8_t status)
{
	if (!bmi160_rt->fifo_en) return;

	uint8_t fifo_wm = 0;
	if (bmi160_rt->int_en[BMI160_OFFSET_FIFO_WM])
		fifo_wm = BMI160_GET_BITSLICE(
			status, BMI160_USER_INTR_STAT_1_FIFO_WM_INTR);

	if (fifo_wm) {
#if BMI160_SUPPORT_FIFO_INT_DATA_REPORT
		bmi160_read_fifo_header_data(FIFO_FRAME);
		report_fifo_data(BMI160_SENSOR_ACCEL,
				 (uint8_t *)bmi160_accel_fifo,
				 bmi160_accel_index,
				 sizeof(phy_accel_data_t));
		report_fifo_data(BMI160_SENSOR_GYRO,
				 (uint8_t *)bmi160_gyro_fifo,
				 bmi160_gyro_index,
				 sizeof(phy_gyro_data_t));
#if BMI160_ENABLE_MAG
		report_fifo_data(BMI160_SENSOR_MAG, (uint8_t *)bmi160_mag_fifo,
				 bmi160_mag_index, sizeof(phy_mag_data_t));
#endif
#else
		bmi160_int_disable(BMI160_INT_SET_1, BMI160_OFFSET_FIFO_WM, 0);
		bmi160_need_update_wm_interrupt = 1;

		/*Just report the watermark event*/
		for (int i = 0; i < BMI160_SENSOR_COUNT; i++) {
			if (bmi160_rt->wm_callback[i] &&
			    (bmi160_rt->fifo_en & (1 << i))) {
				phy_sensor_event_t wm_event;
				wm_event.event_type = PHY_SENSOR_EVENT_WM;
				wm_event.data = NULL;
				bmi160_rt->wm_callback[i](&wm_event,
							  bmi160_rt->
							  wm_cb_priv_data[i]);
				break;
			}
		}
#endif
	}

	if (BMI160_GET_BITSLICE(status,
				BMI160_USER_INTR_STAT_1_FIFO_FULL_INTR)) {
		bmi160_int_disable(BMI160_INT_SET_1, BMI160_OFFSET_FIFO_FULL, 0);
		if (bmi160_rt->int_en[BMI160_OFFSET_FIFO_FULL] &&
		    bmi160_rt->motion_state)
			pr_warning(LOG_MODULE_BMI160, "FIFO full");
	}
}

#ifdef CONFIG_BMI160_ANY_MOTION
static void handle_any_motion(struct bmi160_rt_t *bmi160_rt, uint8_t status0)
{
	read_data callback;
	void *cb_data;
	uint32_t saved = irq_lock();

	callback = bmi160_rt->read_callbacks[BMI160_SENSOR_ANY_MOTION];
	cb_data = bmi160_rt->cb_priv_data[BMI160_SENSOR_ANY_MOTION];
	irq_unlock(saved);

	if (!callback)
		return;

	if (bmi160_rt->int_en[BMI160_OFFSET_ANYMOTION]) {
		uint8_t any_motion = 0;
		any_motion = BMI160_GET_BITSLICE(
			status0, BMI160_USER_INTR_STAT_0_ANY_MOTION);
		if (any_motion) {
			bmi160_any_motion_timestamp = get_uptime_ms();
			bmi160_wait_first_fifo_read_after_anymotion = 1;
			bmi160_rt->motion_state = 1;
#ifdef CONFIG_BMI160_ANY_MOTION_ONESHOT
			bmi160_int_disable(BMI160_INT_SET_0,
					   BMI160_OFFSET_ANYMOTION,
					   0);
#endif
#ifdef CONFIG_BMI160_ENABLE_NOMOTION_AFTER_ANYMOTION
			if (bmi160_rt->read_callbacks[BMI160_SENSOR_NO_MOTION])
				bmi160_int_enable(BMI160_INT_SET_2,
						  BMI160_OFFSET_NOMOTION,
						  1 << BMI160_SENSOR_NO_MOTION);
#endif

#ifdef CONFIG_BMI160_DOUBLE_TAP
			bmi160_wait_double_tap = 1;
#endif
			uint8_t event = PHY_SENSOR_EVENT_ANY_MOTION;
			callback(&event, 1, cb_data);
		}
	}
}
#endif

#ifdef CONFIG_BMI160_NO_MOTION
static void handle_no_motion(struct bmi160_rt_t *bmi160_rt, uint8_t status1)
{
	read_data callback;
	void *cb_data;
	uint32_t saved = irq_lock();

	callback = bmi160_rt->read_callbacks[BMI160_SENSOR_NO_MOTION];
	cb_data = bmi160_rt->cb_priv_data[BMI160_SENSOR_NO_MOTION];
	irq_unlock(saved);

	if (!callback)
		return;

	if (bmi160_rt->int_en[BMI160_OFFSET_NOMOTION]) {
		uint8_t no_motion = 0;
		no_motion = BMI160_GET_BITSLICE(
			status1, BMI160_USER_INTR_STAT_1_NOMOTION_INTR);
		if (no_motion) {
			/* when no motion detected, data ready and fifo watermark interrupt is NOT disabled automatically.
			 * The interrupts are disabled by physical sensor api user via phy_sensor_data_unregister_callback.
			 * So that the interrupts can be delivered if the sensor data is needed after no motion.
			 */
#ifdef CONFIG_BMI160_NO_MOTION_ONESHOT
			bmi160_int_disable(BMI160_INT_SET_2,
					   BMI160_OFFSET_NOMOTION,
					   0);
#endif
#ifdef CONFIG_BMI160_ENABLE_ANYMOTION_AFTER_NOMOTION
			if (bmi160_rt->read_callbacks[BMI160_SENSOR_ANY_MOTION])
				bmi160_int_enable(BMI160_INT_SET_0,
						  BMI160_OFFSET_ANYMOTION,
						  1 << BMI160_SENSOR_ANY_MOTION);
#endif
#ifdef CONFIG_BMI160_ENABLE_DOUBLE_TAP_AFTER_NOMOTION
			if (bmi160_rt->read_callbacks[
				    BMI160_SENSOR_DOUBLE_TAPPING])
				bmi160_int_enable(
					BMI160_INT_SET_0,
					BMI160_OFFSET_DOUBLE_TAP, 1 <<
					BMI160_SENSOR_DOUBLE_TAPPING);
#endif
			pr_debug(LOG_MODULE_BMI160, "no motion");
			uint8_t event = PHY_SENSOR_EVENT_NO_MOTION;
			callback(&event, 1, cb_data);
		}
	}
}
#endif

#ifdef CONFIG_BMI160_DOUBLE_TAP
static void handle_double_tapping(struct bmi160_rt_t *	bmi160_rt,
				  uint8_t		status0)
{
	read_data callback;
	void *cb_data;
	uint32_t saved = irq_lock();

	callback = bmi160_rt->read_callbacks[BMI160_SENSOR_DOUBLE_TAPPING];
	cb_data = bmi160_rt->cb_priv_data[BMI160_SENSOR_DOUBLE_TAPPING];
	irq_unlock(saved);

	if (!callback)
		return;

	if (bmi160_rt->int_en[BMI160_OFFSET_DOUBLE_TAP]) {
		uint8_t double_tap = 0;
		double_tap = BMI160_GET_BITSLICE(
			status0, BMI160_USER_INTR_STAT_0_DOUBLE_TAP_INTR);
		if (double_tap) {
#ifdef CONFIG_BMI160_DOUBLE_TAP_ONESHOT
			/* This should be the case using double tap as wakeup source */
			bmi160_flush_fifo();
			bmi160_int_disable(BMI160_INT_SET_0,
					   BMI160_OFFSET_DOUBLE_TAP,
					   0);
#endif
#ifdef CONFIG_BMI160_ENABLE_NOMOTION_AFTER_DOUBLE_TAP
			if (bmi160_rt->read_callbacks[BMI160_SENSOR_NO_MOTION]
			    &&
			    !bmi160_rt->read_callbacks[BMI160_SENSOR_ANY_MOTION
			    ])
				bmi160_int_enable(BMI160_INT_SET_2,
						  BMI160_OFFSET_NOMOTION,
						  1 << BMI160_SENSOR_NO_MOTION);
#endif
			bmi160_rt->motion_state = 1;
			pr_debug(LOG_MODULE_BMI160, "Double tap");
			uint8_t event = PHY_SENSOR_EVENT_DOUBLE_TAP;
			callback(&event, 1, cb_data);
		}
	}
}
#endif

static void bmi160_gpio_generic_callback(struct bmi160_rt_t *	bmi160_rt,
					 uint8_t		pin)
{
	uint8_t status0 = 0;
	uint8_t status1 = 0;

	handle_reg_data(bmi160_rt);

	if (bmi160_read_reg(BMI160_USER_INTR_STAT_0_ADDR, &status0))
		goto OUT;
	if (bmi160_read_reg(BMI160_USER_INTR_STAT_1_ADDR, &status1))
		goto OUT;

	uint8_t reset_int = RESET_INT_ENGINE;
	bmi160_write_reg(BMI160_CMD_COMMANDS_ADDR, &reset_int);

#if DEBUG_BMI160
	uint8_t status0_reset = 0;
	uint8_t status1_reset = 0;
	bmi160_read_reg(BMI160_USER_INTR_STAT_0_ADDR, &status0_reset);
	bmi160_read_reg(BMI160_USER_INTR_STAT_1_ADDR, &status1_reset);
	pr_debug(LOG_MODULE_BMI160, "%x, %x --> %x, %x", status0, status1,
		 status0_reset,
		 status1_reset);
#endif

#ifdef CONFIG_BMI160_ANY_MOTION
	handle_any_motion(bmi160_rt, status0);
#endif
#ifdef CONFIG_BMI160_NO_MOTION
	handle_no_motion(bmi160_rt, status1);
#endif
#ifdef CONFIG_BMI160_DOUBLE_TAP
	handle_double_tapping(bmi160_rt, status0);
#endif
	handle_fifo(bmi160_rt, status1);

OUT:
	bmi160_unmask_int_pin(pin);
}

#ifdef CONFIG_BMI160_TCMD
static uint32_t bmi160_debug_isr_count = 0;
static uint32_t bmi160_debug_work_queue_ok_count = 0;
static uint32_t bmi160_debug_work_queue_err_count = 0;
static uint32_t bmi160_debug_work_callback_count = 0;
#endif

#if BMI160_USE_INT_PIN1
static void bmi160_gpio_pin1_callback(void *arg)
{
#ifdef CONFIG_BMI160_TCMD
	bmi160_debug_work_callback_count++;
#endif
	bmi160_gpio_generic_callback((struct bmi160_rt_t *)arg, BMI160_INT_PIN1);
}
/* BMI160 INT1 ISR callback */
static void bmi160_pin1_isr(bool state, void *arg)
{
	OS_ERR_TYPE err = 0;

	/* Mask this interrup */
	bmi160_mask_int_pin(BMI160_INT_PIN1);
	err = workqueue_queue_work(bmi160_gpio_pin1_callback, arg);
	if (err != E_OS_OK) {
		bmi160_unmask_int_pin(BMI160_INT_PIN1);
		pr_warning(LOG_MODULE_BMI160, "Fail to queue work");
#ifdef CONFIG_BMI160_TCMD
		bmi160_debug_work_queue_err_count++;
#endif
	}
#ifdef CONFIG_BMI160_TCMD
	else
		bmi160_debug_work_queue_ok_count++;


	bmi160_debug_isr_count++;
#endif
}
#endif

#if BMI160_USE_INT_PIN2
static void bmi160_gpio_pin2_callback(void *arg)
{
	bmi160_gpio_generic_callback((struct bmi160_rt_t *)arg, BMI160_INT_PIN2);
}
/* BMI160 INT2 ISR callback */
static void bmi160_pin2_isr(bool state, void *arg)
{
	OS_ERR_TYPE err = 0;

	/* Mask this interrup */
	bmi160_mask_int_pin(BMI160_INT_PIN2);
	err = workqueue_queue_work(bmi160_gpio_pin2_callback, arg);
	if (err != E_OS_OK) {
		bmi160_unmask_int_pin(BMI160_INT_PIN2);
		pr_debug(LOG_MODULE_BMI160, "%s: fail to queue work", __func__);
	}
}
#endif

void bmi160_mask_int_pin(uint8_t pin)
{
	if (pin == BMI160_INT_PIN1) {
		gpio_mask_interrupt(&pf_device_soc_gpio_aon,
				    BMI160_GPIN_AON_PIN);
	}
#if BMI160_USE_INT_PIN2
	else if (pin == BMI160_INT_PIN2)
		gpio_mask_interrupt(&pf_device_ss_gpio_8b0, BMI160_GPIO_SS_PIN);
#endif
}

void bmi160_unmask_int_pin(uint8_t pin)
{
	if (pin == BMI160_INT_PIN1) {
		gpio_unmask_interrupt(&pf_device_soc_gpio_aon,
				      BMI160_GPIN_AON_PIN);
	}
#if BMI160_USE_INT_PIN2
	else if (pin == BMI160_INT_PIN2)
		gpio_unmask_interrupt(&pf_device_ss_gpio_8b0,
				      BMI160_GPIO_SS_PIN);
#endif
}

DRIVER_API_RC bmi160_config_gpio(void)
{
	struct td_device *dev;

	gpio_cfg_data_t cfg = { 0 };

	cfg.gpio_type = GPIO_INTERRUPT;
	cfg.int_type = LEVEL;
	cfg.int_polarity = ACTIVE_HIGH;
	cfg.int_debounce = DEBOUNCE_ON;
	cfg.gpio_cb_arg = (void *)bmi160_get_ptr();

#if BMI160_USE_INT_PIN1
	cfg.gpio_cb = bmi160_pin1_isr;
	dev = &pf_device_soc_gpio_aon;
	if (gpio_set_config(dev, BMI160_GPIN_AON_PIN, &cfg)) {
		pr_debug(LOG_MODULE_BMI160, "Config GPIO[%d-%d] failed",
			 SOC_GPIO_AON_ID, BMI160_GPIN_AON_PIN);
		return DRV_RC_FAIL;
	}
#endif

#if BMI160_USE_INT_PIN2
	/* GPIO_SS4 connected to interrupt PIN2 of BMI160 */
	cfg.gpio_cb = bmi160_pin2_isr;
	dev = &pf_device_ss_gpio_8b0;
	if (gpio_set_config(dev, BMI160_GPIO_SS_PIN, &cfg)) {
		pr_debug(LOG_MODULE_BMI160, "Config GPIO[%d-%d] failed",
			 SOC_GPIO_AON_ID, BMI160_GPIN_AON_PIN);
		return DRV_RC_FAIL;
	}
#endif
	pr_debug(LOG_MODULE_BMI160, "GPIO config done");
	return DRV_RC_OK;
}

#ifdef CONFIG_BMI160_TCMD
void show_int_status(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	pr_info(LOG_MODULE_BMI160,
		"isr(%d), work queue(Success:%d, Fail:%d), callback(%d)",
		bmi160_debug_isr_count, bmi160_debug_work_queue_ok_count,
		bmi160_debug_work_queue_err_count,
		bmi160_debug_work_callback_count);
	TCMD_RSP_FINAL(ctx, NULL);
}
DECLARE_TEST_COMMAND(bmi160, status, show_int_status);
#endif
