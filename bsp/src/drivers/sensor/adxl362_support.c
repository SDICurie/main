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

#include "adxl362_support.h"
#include "drivers/gpio.h"
#include "util/cunit_test.h"
#include "drivers/serial_bus_access.h"
#define ADXL362_BUS_WRITE_BUFFER_SIZE 4

static struct adxl362_t adxl362_s;

/* filter configurations for basis sample rates */
const uint8_t adxl362_filter_config[ADXL362_ASR_NUM_SAMPLE_RATES] = {
	ADXL362_RATE_12_5, /* ASR_16_Hz */
	ADXL362_RATE_25, /* ASR_32_Hz */
	ADXL362_RATE_50, /* ASR_64_Hz */
	ADXL362_RATE_100, /* ASR_128_Hz */
	ADXL362_RATE_200, /* ASR_256_Hz */
	ADXL362_RATE_400, /* ASR_512_Hz */
	ADXL362_RATE_400, /* ASR_1024Hz */
};

static uint8_t measurement_range = ADXL362_RANGE_8G;
static uint8_t resolution = 4;        /* 4 mg/LSB */

static int16_t s12_2_s16(uint16_t val)
{
	int16_t tmp;

	tmp = (int16_t)(val << 4);
	tmp >>= 4;
	return tmp;
}

static bool adxl362_reg_write_and_test(uint8_t reg, uint8_t reg_val)
{
	uint8_t value = 0;

	adxl362_bus_write(reg, &reg_val, 1);
	adxl362_bus_read(reg, &value, 1);
	return reg_val == value;
}

#if ADXL362_DEBUG
void adxl362_reg_dump(void)
{
	uint8_t reg_b0[4] = { 0 }; /* 0x00~0x03 */
	uint8_t reg_b1[14] = { 0 }; /* 0x08~0x15 */
	uint8_t reg_b2[16] = { 0 }; /* 0x1F~0x2E */
	int i;

	adxl362_bus_read(ADXL362_REG_DEVID_AD, reg_b0, 4);
	adxl362_bus_read(ADXL362_REG_XDATA8, reg_b1, 14);
	adxl362_bus_read(ADXL362_REG_SOFT_RESET, reg_b2, 16);

	for (i = 0; i < 3; i++)
		pr_info(LOG_MODULE_ADXL362, "REG[%x]=0x%x", i, reg_b0[i]);

	for (i = 8; i < 22; i++)
		pr_info(LOG_MODULE_ADXL362, "REG[%x]=0x%x", i, reg_b1[i - 8]);

	for (i = 31; i < 47; i++)
		pr_info(LOG_MODULE_ADXL362, "REG[%x]=0x%x", i, reg_b2[i - 31]);
}
#endif

DRIVER_API_RC adxl362_softreset(void)
{
	uint8_t tx_buf = 'R';

	return adxl362_bus_write(ADXL362_REG_SOFT_RESET, &tx_buf, 1);
}

DRIVER_API_RC adxl362_standby(void)
{
	uint8_t tx_buf = ADXL362_STANDBY;

	return adxl362_bus_write(ADXL362_REG_POWER_CTL, &tx_buf, 1);
}

DRIVER_API_RC adxl362_sampling_register_setup(
	adxl362_sample_rate_t sample_rate,
	uint16_t
	watermark)
{
	/* Set up the filter control */
	if (!adxl362_reg_write_and_test(ADXL362_REG_FILTER_CTL,
					adxl362_filter_config[sample_rate] |
					ADXL362_EXT_TRIGGER | measurement_range))
		return DRV_RC_FAIL;

	if (!adxl362_reg_write_and_test(ADXL362_REG_FIFO_CONTROL,
					ADXL362_FIFO_MODE_STREAM))
		return DRV_RC_FAIL;

	if (!adxl362_reg_write_and_test(ADXL362_REG_FIFO_SAMPLES,
					(watermark * 3) - 1))
		return DRV_RC_FAIL;

	if (!adxl362_reg_write_and_test(ADXL362_REG_INTMAP1,
					ADXL362_INT_FIFO_WATERMARK))
		return DRV_RC_FAIL;

	if (!adxl362_reg_write_and_test(ADXL362_REG_POWER_CTL, ADXL362_MEASURE))
		return DRV_RC_FAIL;

	sensor_delay_ms(50);

	return DRV_RC_OK;
}

uint16_t adxl362_get_fifo_len(void)
{
	uint8_t rx_value[2] = { 0 };

	/* Retrieve valid FIFO samples entries. */
	adxl362_bus_read(ADXL362_REG_FIFO_ENTRIES_L, rx_value, 2);
	return (rx_value[0] + (rx_value[1] & 0x3) * 256) * 2;
}

uint16_t adxl362_fifo_read(uint8_t *buffer, uint16_t num_samples)
{
	uint16_t *ac_data;
	uint16_t samples_in_fifo;
	uint16_t read_len_in_byte;
	uint16_t valid_data_in_byte;
	uint16_t sample_read;
	adxl362_accel_data_t *fifo_data_parsed;

	if (num_samples > ADXL362_FIFO_READ_MAX)
		num_samples = ADXL362_FIFO_READ_MAX;

	samples_in_fifo = adxl362_get_fifo_len() / ADXL362_XYZ_SIZE;

	if (num_samples > samples_in_fifo)
		num_samples = samples_in_fifo;

	read_len_in_byte = num_samples * ADXL362_XYZ_SIZE;

	adxl362_bus_fifo_read(buffer, read_len_in_byte);

	valid_data_in_byte = read_len_in_byte;

	ac_data = (uint16_t *)buffer;
	fifo_data_parsed = (adxl362_accel_data_t *)buffer;

	/* look for first X element (in case of misalignment, buffer full etc...) */
	while ((*ac_data & 0xC000) &&
	       (valid_data_in_byte > sizeof(*ac_data))) {
		ac_data++;
		valid_data_in_byte -= sizeof(*ac_data);
	}

	sample_read = valid_data_in_byte / ADXL362_XYZ_SIZE;

	for (int i = 0; i < sample_read; i++) {
		fifo_data_parsed[i].x = s12_2_s16(ac_data[0]) * resolution;
		fifo_data_parsed[i].y = s12_2_s16(ac_data[1]) * resolution;
		fifo_data_parsed[i].z = s12_2_s16(ac_data[2]) * resolution;
		ac_data += 3;
	}
	return sample_read;
}

void adxl362_sample_trigger(bool trigger)
{
	gpio_write(adxl362_s.int_pin_gpio, ADXL362_INT2_PIN, (trigger ? 1 : 0));
}

bool adxl362_read_pin1_level(void)
{
	bool value = 0;

	gpio_read(adxl362_s.int_pin_gpio, ADXL362_INT1_PIN, &value);
	return value;
}

int adxl362_init(void)
{
	gpio_cfg_data_t out_cfg = { 0 };
	gpio_cfg_data_t in_cfg = { 0 };

	out_cfg.gpio_type = GPIO_OUTPUT;
	in_cfg.gpio_type = GPIO_INPUT;
	adxl362_s.int_pin_gpio = &pf_device_ss_gpio_8b0;
	gpio_set_config(adxl362_s.int_pin_gpio, ADXL362_INT2_PIN, &out_cfg);
	gpio_set_config(adxl362_s.int_pin_gpio, ADXL362_INT1_PIN, &in_cfg);

	adxl362_s.id[0] = adxl362_s.id[1] = 0;
	adxl362_bus_read(ADXL362_REG_DEVID_AD, adxl362_s.id, 2);
	if ((adxl362_s.id[0] == 0xff && adxl362_s.id[1] == 0xff)
	    || (adxl362_s.id[0] == 0 && adxl362_s.id[1] == 0))
		return -1;

	/* SW reset the sensor */
	adxl362_softreset();
	sensor_delay_ms(100);

	adxl362_standby();
	return 0;
}

struct adxl362_t *get_adxl362_ptr(void)
{
	return &adxl362_s;
}
