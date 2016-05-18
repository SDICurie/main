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

#include "drivers/serial_bus_access.h"
#include "drivers/sensor/sensor_bus_common.h"
#include "bme280_support.h"

/************************* Use Serial Bus Access API *******************/
#define BME280_REQ_NUM 2
#define BME280_BUS_WRITE_BUFFER_SIZE 2

struct sensor_sba_info *bme280_sba_info;

DRIVER_API_RC bme280_bus_read(uint8_t reg_addr, uint8_t *reg_data, uint8_t cnt)
{
	return bme280_bus_burst_read(reg_addr, reg_data, cnt);
}

DRIVER_API_RC bme280_bus_burst_read(uint8_t reg_addr, uint8_t *reg_data,
				    uint32_t cnt)
{
	if (bme280_sba_info->bus_type == SENSOR_BUS_TYPE_SPI)
		reg_addr |= SPI_READ_CMD;

	return sensor_bus_access(bme280_sba_info, &reg_addr, 1, reg_data, cnt,
				 true,
				 -1);
}

DRIVER_API_RC bme280_bus_write(uint8_t reg_addr, uint8_t *reg_data, uint8_t cnt)
{
	uint8_t buffer[BME280_BUS_WRITE_BUFFER_SIZE];

	if (cnt > 1)
		return DRV_RC_INVALID_CONFIG;

	if (bme280_sba_info->bus_type == SENSOR_BUS_TYPE_SPI)
		reg_addr &= 0x7F;

	buffer[0] = reg_addr;
	buffer[1] = reg_data[0];

	return sensor_bus_access(bme280_sba_info, buffer,
				 BME280_BUS_WRITE_BUFFER_SIZE, NULL, 0, false,
				 -1);
}

DRIVER_API_RC bme280_config_bus(struct td_device *dev)
{
	struct sba_device *sba_dev = (struct sba_device *)dev;
	struct sba_master_cfg_data *sba_priv =
		(struct sba_master_cfg_data *)sba_dev->parent->priv;

#ifdef CONFIG_BME280_SPI
	bme280_sba_info =
		sensor_config_bus(sba_dev->addr.cs, sba_dev->dev.id,
				  sba_priv->bus_id,
				  SENSOR_BUS_TYPE_SPI, BME280_REQ_NUM,
				  SLEEP);
#else
	bme280_sba_info =
		sensor_config_bus(sba_dev->addr.slave_addr, sba_dev->dev.id,
				  sba_priv->bus_id,
				  SENSOR_BUS_TYPE_I2C, BME280_REQ_NUM,
				  SLEEP);
#endif
	if (bme280_sba_info)
		return DRV_RC_OK;
	return DRV_RC_FAIL;
}


extern int bme280_sensor_register(void);
static int sba_bme280_init(struct td_device *dev)
{
	int ret;

	ret = bme280_sensor_register();
	ret += bme280_config_bus(dev);
	bme280_set_softreset();
	sensor_delay_ms(3);
	return ret;
}

struct driver sba_bme280_driver = {
	.init = sba_bme280_init,
	.suspend = NULL,
	.resume = NULL
};
