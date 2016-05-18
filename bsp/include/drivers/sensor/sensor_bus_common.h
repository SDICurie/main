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

#ifndef __SENSOR_BUS_COMMON_H__
#define __SENSOR_BUS_COMMON_H__

#include "drivers/serial_bus_access.h"


#define BMI160_SPI_ADDR     SPI_SE_1
#define BMI160_I2C_ADDR1    0x68 /**< SDO=0 */
#define BMI160_I2C_ADDR2    0x69 /**< SDO=1 */

#ifdef CONFIG_BMI160
#ifdef CONFIG_BMI160_SPI
extern struct driver spi_bmi160_driver;
#define BMI160_PRIMARY_BUS_ADDR BMI160_SPI_ADDR
#else
extern struct driver i2c_bmi160_driver;
#define BMI160_PRIMARY_BUS_ADDR BMI160_I2C_ADDR1
#endif
#endif

#ifdef CONFIG_BME280
#ifdef CONFIG_BME280_SPI
#define BME280_SBA_ADDR SPI_SE_1
#else
#define BME280_SBA_ADDR 0x76
#endif
extern struct driver sba_bme280_driver;
#endif

#define SPI_READ_CMD        (1 << 7)

/**
 * @defgroup sensor_bus Sensor Bus Access Driver
 * Sensor Bus Access driver API.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "drivers/sensor_bus_common.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/drivers/sensor</tt>
 * <tr><th><b>Config flag</b> <td><tt>SENSOR_BUS_COMMON</tt>
 * </table>
 *
 * The Sensor Bus Access Driver provides an API to configure the sensor buses. It uses \ref sba.
 *
 * @ingroup arc_drivers
 * @{
 */

/**
 * BUS type of sensor
 */
typedef enum _sensor_bus_type {
	SENSOR_BUS_TYPE_I2C = 0,
	SENSOR_BUS_TYPE_SPI,
} SENSOR_BUS_TYPE;

/**
 * SBA Block type of sensor
 */
typedef enum {
	SLEEP = 0,
	SPIN,
} BLOCK_TYPE;

/**
 * SBA request for sensor
 */
struct sensor_sba_req {
	sba_request_t req;
	T_SEMAPHORE sem;
	volatile int complete_flag;
};

/**
 * SBA information for sensor
 */
struct sensor_sba_info {
	struct sensor_sba_req *reqs;
	uint8_t bitmap;
	uint8_t req_cnt;
	BLOCK_TYPE block_type;
	SENSOR_BUS_TYPE bus_type;
	uint8_t dev_id;
};

/**
 *  Access on sensor bus.
 *
 *  @param info       Configuration information as returned by \ref sensor_config_bus
 *  @param tx_buffer  Pointer to transmit buffer
 *  @param tx_len     Length of transmit buffer
 *  @param rx_buffer  Pointer where to store received data
 *  @param rx_len     Length of receive buffer
 *  @param req_read   `true`-->SBA_TRANSFER, `false`-->SBA_TX, see @ref SBA_REQUEST_TYPE
 *  @param slave_addr Usually, this field is -1 unless multiple devices related to the struct sensor_sba_info
 *  @return see @ref DRIVER_API_RC\n
 */
DRIVER_API_RC sensor_bus_access(struct sensor_sba_info *info,
				uint8_t *tx_buffer, uint32_t tx_len,
				uint8_t *rx_buffer, uint32_t rx_len,
				bool req_read,
				int slave_addr);

/**
 *  Configure a sensor bus before any access.
 *
 *  @param dev_addr    Device address
 *  @param dev_id      Device id
 *  @param bus_id      Bus id, see @ref SBA_BUSID
 *  @param bus_type    Bus type, can be spi or i2c see @ref SENSOR_BUS_TYPE
 *  @param req_num     Number of requests needed
 *  @param block_type  See @ref BLOCK_TYPE, busy wait if the type is SPIN, otherwise, sleep
 *  @return Pointer to struct @ref sensor_sba_info
 */
struct sensor_sba_info *sensor_config_bus(int dev_addr, uint8_t dev_id,
					  SBA_BUSID bus_id,
					  SENSOR_BUS_TYPE bus_type, int req_num,
					  BLOCK_TYPE block_type);

/**
 *  Delay specific milliseconds
 *
 *  @param msecs       Delay in milliseconds
 */
void sensor_delay_ms(uint32_t msecs);
#endif
/** @} */
