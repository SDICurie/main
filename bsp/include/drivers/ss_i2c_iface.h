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

#ifndef SS_I2C_IFACE_H_
#define SS_I2C_IFACE_H_

/**
 * @defgroup i2c_arc_driver I2C Driver
 * Inter-Integrated Circuit ARC driver API.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "drivers/ss_i2c_iface.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/drivers/i2c</tt>
 * <tr><th><b>Config flag</b> <td><tt>SS_I2C</tt>
 * </table>
 *
 * \note I2C driver should not be used as is. Prefer using \ref sba.
 *
 * @ingroup arc_drivers
 * @{
 */

#include "drivers/common_i2c.h"
#include "drivers/serial_bus_access.h"

/*!
 * List of all controllers
 */

typedef enum {
	I2C_SENSING_0 = 0,      /*!< Sensing I2C controller 0, accessible by Sensor Subsystem Core only */
	I2C_SENSING_1           /*!< Sensing I2C controller 1, accessible by Sensor Subsystem Core only */
} I2C_CONTROLLER;

/**
 *  Configure the specified I2C controller.
 *
 *  Configuration parameters must be valid or an error is returned - see return values below.
 *
 *  @param   controller_id   I2C controller identifier
 *  @param   config          Pointer to configuration structure
 *
 *  @return
 *           - DRV_RC_OK on success
 *           - DRV_RC_DEVICE_TYPE_NOT_SUPPORTED    - If device type is not supported by this controller
 *           - DRV_RC_INVALID_CONFIG               - If any configuration parameter is not valid
 *           - DRV_RC_CONTROLLER_IN_USE,           - If controller is in use
 *           - DRV_RC_CONTROLLER_NOT_ACCESSIBLE    - If controller is not accessible from this core
 *           - DRV_RC_FAIL Otherwise
 */
DRIVER_API_RC ss_i2c_set_config(I2C_CONTROLLER	controller_id,
				i2c_cfg_data_t *config);


/**
 *  Retrieve configuration of the specified I2C controller.
 *
 *  @param   controller_id   I2C controller identifier
 *  @param   config          Pointer to configuration structure where to store current setup
 *
 *  @return
 *           - DRV_RC_OK   - On success
 *           - DRV_RC_FAIL - Otherwise
 */
DRIVER_API_RC ss_i2c_get_config(I2C_CONTROLLER	controller_id,
				i2c_cfg_data_t *config);


/**
 *  Place I2C controller into a disabled and default state (as if hardware reset occurred).
 *
 *  This function assumes that there is no pending transaction on the I2C interface in question.
 *  It is the responsibility of the calling application to do so.
 *  Upon success, the specified I2C interface is clock gated in hardware,
 *  it is no longer capable to generating interrupts, it is also configured into a default state.
 *
 *  @param   sba_dev   Pointer to bus configuration data
 *
 *  @return
 *           - DRV_RC_OK   - On success
 *           - DRV_RC_FAIL - Otherwise
 */
DRIVER_API_RC ss_i2c_deconfig(struct sba_master_cfg_data *sba_dev);


/**
 *  Enable the clock for the specified I2C controller.
 *
 *  Upon success, the specified I2C interface is no longer clock gated in hardware, it is now
 *  capable of transmitting and receiving on the I2C bus, and generating interrupts.
 *
 *  @param   sba_dev   Pointer to bus configuration data
 *
 *  @return
 *           - DRV_RC_OK   - On success
 *           - DRV_RC_FAIL - Otherwise
 */
DRIVER_API_RC ss_i2c_clock_enable(struct sba_master_cfg_data *sba_dev);


/**
 *  Disable the clock for the specified I2C controller.
 *
 *  This function assumes that there is no pending transaction on the I2C interface in question.
 *  It is the responsibility of the calling application to do so.
 *  Upon success, the specified I2C interface is clock gated in hardware,
 *  it is no longer capable of generating interrupts.
 *
 *  @param   sba_dev   Pointer to bus configuration data
 *
 *  @return
 *           - DRV_RC_OK   - On success
 *           - DRV_RC_FAIL - Otherwise
 */
DRIVER_API_RC ss_i2c_clock_disable(struct sba_master_cfg_data *sba_dev);


/**
 *  Send a block of data to the specified I2C slave.
 *
 *  @param   controller_id   I2C controller identifier
 *  @param   data            Pointer to data to write
 *  @param   data_len        Length of data to write
 *  @param   slave_addr      I2C address of the slave
 *
 *  @return
 *           - DRV_RC_OK   - On success
 *           - DRV_RC_FAIL - Otherwise
 */
DRIVER_API_RC ss_i2c_write(I2C_CONTROLLER controller_id, uint8_t *data,
			   uint32_t data_len,
			   uint32_t slave_addr);


/**
 *  Receive a block of data from the specified I2C slave.
 *
 *  If set as a master, this will receive 'data_len' bytes transmitted from slave.
 *  If set as a slave, this will receive any data sent by a master addressed to the this I2C address as
 *  configured in the "slave_adr" for this controller configuration, in which case 'data_len'
 *  indicates the amount of data received and 'data' holds the data.
 *
 *  @param   controller_id   I2C controller identifier
 *  @param   data            Pointer where to store read data
 *  @param   data_len        Length of data to read
 *  @param   slave_addr      I2C address of the slave
 *
 *  @return
 *           - DRV_RC_OK   - On success
 *           - DRV_RC_FAIL - Otherwise
 */
DRIVER_API_RC ss_i2c_read(I2C_CONTROLLER controller_id, uint8_t *data,
			  uint32_t data_len,
			  uint32_t slave_addr);

/**
 *  Send and receive a block of data to the specified I2C slave
 *  with repeated start.
 *
 *  @param   controller_id   I2C controllerd identifier
 *  @param   data_write      Pointer to data to write
 *  @param   data_write_len  Length of data to write
 *  @param   data_read       Pointer where to store read data
 *  @param   data_read_len   Length of data to read
 *  @param   slave_addr      I2C address of the slave
 *
 *  @return
 *           - DRV_RC_OK   - On success
 *           - DRV_RC_FAIL - Otherwise
 */
DRIVER_API_RC ss_i2c_transfer(I2C_CONTROLLER controller_id, uint8_t *data_write,
			      uint32_t data_write_len, uint8_t *data_read,
			      uint32_t data_read_len,
			      uint32_t slave_addr);

/**
 *  Get current state of I2C controller.
 *
 *  @param   controller_id   I2C controller identifier
 *
 *  @return
 *           - I2C_OK   - Controller ready
 *           - I2C_BUSY - Controller busy
 */
DRIVER_I2C_STATUS_CODE ss_i2c_status(I2C_CONTROLLER controller_id);

/** @} */

#endif  /* SS_I2C_IFACE_H_ */
