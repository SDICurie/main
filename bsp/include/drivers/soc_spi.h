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

/*
 * Intel SOC SPI driver
 *
 */

#ifndef SOC_SPI_H_
#define SOC_SPI_H_

#include "drivers/common_spi.h"
#include "drivers/serial_bus_access.h"

/**
 * @defgroup soc_spi SPI Driver
 * SOC Serial Peripheral Interface driver API.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "drivers/soc_spi.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/drivers/spi</tt>
 * <tr><th><b>Config flag</b> <td><tt>INTEL_QRK_SPI</tt>
 * </table>
 *
 * \note SOC SPI driver should not be used as is. Prefer using \ref sba.
 *
 * @ingroup soc_drivers
 * @{
 */

#define SOC_SPI_CONTROLLER int

/**
 *  Configure the specified SPI controller.
 *
 *  Configuration parameters must be valid or an error is returned - see return values below.
 *
 *  @param  controller_id   SPI controller identifier
 *  @param  config          Pointer to configuration structure
 *
 *  @return
 *          - RC_OK                           - on success,
 *          - RC_DEVICE_TYPE_NOT_SUPPORTED    - if device type is not supported by this controller
 *          - RC_INVALID_CONFIG               - if any configuration parameters are not valid
 *          - RC_CONTROLLER_IN_USE,           - if controller is in use
 *          - RC_CONTROLLER_NOT_ACCESSIBLE    - if controller is not accessible from this core
 *          - RC_FAIL                         - otherwise
 */
DRIVER_API_RC soc_spi_set_config(SOC_SPI_CONTROLLER	controller_id,
				 spi_cfg_data_t *	config);

/**
 *  Retrieve configuration of specified SPI controller.
 *
 *  @param  controller_id   SPI controller identifier
 *  @param  config          Pointer to configuration structure where to store current setup
 *
 *  @return
 *          - RC_OK   - on success
 *          - RC_FAIL - otherwise
 */
DRIVER_API_RC soc_spi_get_config(SOC_SPI_CONTROLLER	controller_id,
				 spi_cfg_data_t *	config);

/**
 *  Place SPI controller into a disabled and default state (as if hardware reset occurred).
 *
 *  This function assumes that there is no pending transaction on the SPI interface.
 *  It is the responsibility of the calling application to do so.
 *  Upon success, the specified SPI interface is clock gated in hardware,
 *  it is no longer capable to generating interrupts, it is also configured into a default state.
 *
 *  @param sba_dev          Pointer to bus configuration data
 *
 *  @return
 *          - RC_OK on success
 *          - RC_FAIL otherwise
 */
DRIVER_API_RC soc_spi_deconfig(struct sba_master_cfg_data *sba_dev);

/**
 *  Enable the clock for the specified SPI controller.
 *
 *  Upon success, the specified SPI interface is no longer clock gated in hardware, it is now
 *  capable of transmitting and receiving on the SPI bus, and generating interrupts.
 *
 *  @param sba_dev          Pointer to bus configuration data
 *
 *  @return
 *           - RC_OK on success
 *           - RC_FAIL otherwise
 */
DRIVER_API_RC soc_spi_clock_enable(struct sba_master_cfg_data *sba_dev);

/**
 *  Disable the clock for the specified SPI controller.
 *
 *  This function assumes that there is no pending transaction on the SPI interface.
 *  It is the responsibility of the calling application to do so.
 *  Upon success, the specified SPI interface is clock gated in hardware,
 *  it is no longer capable of generating interrupts.
 *
 *  @param sba_dev          Pointer to bus configuration data
 *
 *  @return
 *          - RC_OK on success
 *          - RC_FAIL otherwise
 */
DRIVER_API_RC soc_spi_clock_disable(struct sba_master_cfg_data *sba_dev);

/**
 *  Send a command and receive a result from the specified SPI slave
 *
 *  @param  controller_id   SPI controller identifier
 *  @param  tx_data         Pointer to cmd to transmit
 *  @param  tx_data_len     Length of cmd to transmit
 *  @param  rx_data         Pointer where to return received data
 *  @param  rx_data_len     Length of data to receive
 *  @param  full_duplex     Set to 1 if data received between tx_phase should be put in rx_data buffer
 *                          rx_data_len should be >= tx_data_len !
 *  @param  slave           Slave device
 *
 *  @return
 *          - RC_OK                   -   on success
 *          - RC_CONTROLLER_IN_USE    -   when device is busy
 *          - RC_FAIL                 -   otherwise
 */
DRIVER_API_RC soc_spi_transfer(SOC_SPI_CONTROLLER controller_id,
			       uint8_t *tx_data, uint32_t tx_data_len,
			       uint8_t *rx_data, uint32_t rx_data_len,
			       int full_duplex,
			       SPI_SLAVE_ENABLE slave);

/**
 *  Get current state of the SPI controller
 *
 *  @param  controller_id   SPI controller identifier
 *
 *  @return
 *          - SOC_SPI_OK       - controller ready
 *          - SOC_SPI_BUSY     - controller busy
 *          - SOC_SPI_TFE      - TX FIFO Empty
 *          - SOC_SPI_RFNE     - RX FIFO Not Empty
 */
DRIVER_SPI_STATUS_CODE soc_spi_status(SOC_SPI_CONTROLLER controller_id);

/** @} */

#endif /* INTEL_QRK_SPI_H_ */
