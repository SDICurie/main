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
#ifndef __ADXL362_SUPPORT_H__
#define __ADXL362_SUPPORT_H__
#include "util/compiler.h"
#include "machine.h"
#include "os/os.h"
#include "infra/log.h"
#include "infra/time.h"
#include "drivers/data_type.h"
#include "string.h"
#include "drivers/sensor/sensor_bus_common.h"
#include "adxl362_regs.h"

/* GPIO_SS[5] */
#define ADXL362_INT2_PIN 5
/* GPIO_SS[6] */
#define ADXL362_INT1_PIN 6

#define ADXL362_WATERMARK 16
#define ADXL362_XYZ_SIZE 6
#define ADXL362_FIFO_READ_MAX 16
#define ADXL362_FIFO_READ_BUFFER_SIZE (ADXL362_XYZ_SIZE * ADXL362_FIFO_READ_MAX)

#define ADXL362_DEBUG 0

#define ADXL362_REQ_NUM 2
#define ADXL362_SLAVE_ADDR SPI_SE_4

/** ADXL362 runtime data structure */
struct adxl362_t {
	uint8_t id[2];
	struct td_device *int_pin_gpio;
};

typedef struct adxl362_xyz {
	int16_t x;
	int16_t y;
	int16_t z;
} adxl362_accel_data_t;

typedef enum {
	ADXL362_ASR_16_Hz = 0,
	ADXL362_ASR_32_Hz = 1,
	ADXL362_ASR_64_Hz = 2,
	ADXL362_ASR_128_Hz = 3,
	ADXL362_ASR_256_Hz = 4,
	ADXL362_ASR_512_Hz = 5,
	ADXL362_ASR_1024_Hz = 6,
	ADXL362_ASR_NUM_SAMPLE_RATES,
} adxl362_sample_rate_t;

int adxl362_init(void);
DRIVER_API_RC adxl362_softreset(void);
DRIVER_API_RC adxl362_sampling_register_setup(adxl362_sample_rate_t sample_rate,
					      uint16_t
					      watermark);
DRIVER_API_RC adxl362_standby(void);
void adxl362_sample_trigger(bool trigger);
uint16_t adxl362_get_fifo_len(void);
bool adxl362_read_pin1_level(void);
uint16_t adxl362_fifo_read(uint8_t *buffer, uint16_t num_samples);
struct adxl362_t *get_adxl362_ptr(void);
DRIVER_API_RC adxl362_bus_read(uint8_t reg_addr, uint8_t *reg_data, uint8_t cnt);
DRIVER_API_RC adxl362_bus_fifo_read(uint8_t *fifo_data, uint8_t cnt);
DRIVER_API_RC adxl362_bus_write(uint8_t reg_addr, uint8_t *reg_data,
				uint8_t cnt);

#if ADXL362_DEBUG
void adxl362_reg_dump(void);
#endif
#endif
