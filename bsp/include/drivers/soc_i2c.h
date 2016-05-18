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

#ifndef SOC_I2C_H_
#define SOC_I2C_H_

#include "drivers/common_i2c.h"
#include "drivers/serial_bus_access.h"

/**
 * @defgroup soc_i2c SOC I2C Driver
 * Quark SE SOC Inter-Integrated Circuit drivers API.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "drivers/soc_i2c.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/drivers/i2c</tt>
 * <tr><th><b>Config flag</b> <td><tt>INTEL_QRK_I2C</tt>
 * </table>
 *
 * \note SOC I2C driver should not be used as is. Prefer using \ref sba.
 *
 * @ingroup soc_drivers
 * @{
 */

#define SOC_I2C_CONTROLLER int

typedef enum {
	SLAVE_WRITE = 0,    /*!< SLAVE WRITE MODE */
	SLAVE_READ,         /*!< SLAVE READ MODE */
} SOC_I2C_SLAVE_MODE;


/**
 *  Configure the specified I2C controller.
 *
 *  Configuration parameters must be valid or an error is returned - see return values below.
 *
 *  @param   controller_id   I2C  controller identifier
 *  @param   config          Pointer to configuration structure
 *
 *  @return
 *           - DRV_RC_OK on success
 *           - DRV_RC_DEVICE_TYPE_NOT_SUPPORTED    - if device type is not supported by this controller
 *           - DRV_RC_INVALID_CONFIG               - if any configuration parameters are not valid
 *           - DRV_RC_CONTROLLER_IN_USE,           - if controller is in use
 *           - DRV_RC_CONTROLLER_NOT_ACCESSIBLE    - if controller is not accessible from this core
 *           - DRV_RC_FAIL otherwise
 */
DRIVER_API_RC soc_i2c_set_config(SOC_I2C_CONTROLLER	controller_id,
				 i2c_cfg_data_t *	config);


/**
 *  Retrieve configuration of the specified I2C controller
 *
 *  @param   controller_id   I2C controller identifier
 *  @param   config          Pointer to configuration structure to store current setup
 *
 *  @return
 *           - DRV_RC_OK on success
 *           - DRV_RC_FAIL otherwise
 */
DRIVER_API_RC soc_i2c_get_config(SOC_I2C_CONTROLLER	controller_id,
				 i2c_cfg_data_t *	config);


/**
 *  Place the I2C controller into a disabled and default state (as if hardware reset occurred).
 *
 *  This function assumes that there is no pending transaction on the I2C interface in question.
 *  It is the responsibility of the calling application to do so.
 *  Upon success, the specified controller configuration is reset to default.
 *
 *  @param   controller_id   I2C controller identifier
 *
 *  @return
 *           - DRV_RC_OK on success
 *           - DRV_RC_FAIL otherwise
 */
DRIVER_API_RC soc_i2c_deconfig(SOC_I2C_CONTROLLER controller_id);


/**
 *  Enable the clock for the specified I2C controller.
 *
 *  Upon success, the specified I2C interface is no longer clock gated in hardware, it is now
 *  capable of transmitting and receiving on the I2C bus, and generating interrupts.
 *
 *  @param sba_dev           Pointer to bus configuration data
 *
 *  @return
 *           - DRV_RC_OK on success
 *           - DRV_RC_FAIL otherwise
 */
DRIVER_API_RC soc_i2c_clock_enable(struct sba_master_cfg_data *sba_dev);


/**
 *  Disable the clock for the specified I2C controller.
 *
 *  This function assumes that there is no pending transaction on the I2C interface in question.
 *  It is the responsibility of the calling application to do so.
 *  Upon success, the specified I2C interface is clock gated in hardware,
 *  it is no longer capable of generating interrupts.
 *
 *  @param sba_dev           Pointer to bus configuration data
 *
 *  @return
 *           - DRV_RC_OK on success
 *           - DRV_RC_FAIL otherwise
 */
DRIVER_API_RC soc_i2c_clock_disable(struct sba_master_cfg_data *sba_dev);


/**
 *  Send a block of data to the specified I2C slave.
 *
 *  @param   controller_id   I2C controller identifier
 *  @param   data            Pointer to data to write
 *  @param   data_len        Length of data to write
 *  @param   slave_addr      I2C address of the slave to write data to
 *
 *  @return
 *           - DRV_RC_OK on success
 *           - DRV_RC_FAIL otherwise
 */
DRIVER_API_RC soc_i2c_write(SOC_I2C_CONTROLLER controller_id, uint8_t *data,
			    uint32_t data_len,
			    uint32_t slave_addr);

/**
 *  Receive a block of data from the specified I2C slave.
 *
 *  If set as a master, this will receive 'data_len' bytes transmitted from slave.
 *  If set as a slave, this will receive any data sent by a master addressed to the I2C address as
 *  configured in the "slave_adr" for this controller configuration, in which case 'data_len'
 *  indicates the amount of data received and 'data' holds the data.
 *
 *  @param   controller_id   I2C controller identifier
 *  @param   data            Pointer where to store read data
 *  @param   data_len        Length of data to read
 *  @param   slave_addr      I2C address of the slave
 *
 *  @return
 *           - DRV_RC_OK on success
 *           - DRV_RC_FAIL otherwise
 */
DRIVER_API_RC soc_i2c_read(SOC_I2C_CONTROLLER controller_id, uint8_t *data,
			   uint32_t data_len,
			   uint32_t slave_addr);

/**
 *  Send and receive a block of data to the specified I2C slave
 *  with repeated start
 *
 *  @param   controller_id   I2C controller identifier
 *  @param   data_write      Pointer to data to write
 *  @param   data_write_len  Length of data to write
 *  @param   data_read       Pointer where to store read data
 *  @param   data_read_len   Length of data to read
 *  @param   slave_addr      I2C address of the slave
 *
 *  @return
 *           - DRV_RC_OK on success
 *           - DRV_RC_FAIL otherwise
 */
DRIVER_API_RC soc_i2c_transfer(SOC_I2C_CONTROLLER controller_id,
			       uint8_t *data_write, uint32_t data_write_len,
			       uint8_t *data_read,
			       uint32_t data_read_len,
			       uint32_t slave_addr);

/**
 *  Get current state of I2C controller
 *
 *  @param   controller_id   I2C controller identifier
 *
 *  @return
 *           - I2C_OK   - controller ready
 *           - I2C_BUSY - controller busy
 */
DRIVER_I2C_STATUS_CODE soc_i2c_status(SOC_I2C_CONTROLLER controller_id);

/** @} */

#endif
