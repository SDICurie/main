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

#ifndef INTEL_SBA_H_
#define INTEL_SBA_H_

#include "infra/device.h"
#include "drivers/common_spi.h"
#include "drivers/common_i2c.h"
#include "drivers/data_type.h"
#include "drivers/clk_system.h"

/**
 * @defgroup sba Serial Bus Access Driver
 * SOC Serial Bus Access driver API.
 * @ingroup soc_drivers
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "drivers/serial_bus_access.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/drivers/sba</tt>
 * <tr><th><b>Config flag</b> <td><tt>SBA</tt>
 * <tr><th><b>Dependencies</b> <td><tt>INTEL_QRK_SPI | INTEL_QRK_I2C | SS_SPI | SS_I2C</tt>
 * </table>
 *
 * Serial Bus Access Driver provides an abstracted access to underlying SPI or I2C serial bus.
 * It offers the possibility to queue transactions. All transactions are asynchronous and
 * a callback is called on transaction completion.
 *
 * In the device tree, there should be:
 * - one SBA bus device per used SPI/I2C bus with attached struct sba_master_cfg_data
 * - for each bus device, one or more SBA device with attached struct sba_device
 *
 * @{
 */

/**
 * List of all request types
 */
typedef enum {
	SBA_RX = 0,           /*!< Read  */
	SBA_TX,               /*!< Write */
	SBA_TRANSFER          /*!< Read and write */
} SBA_REQUEST_TYPE;

/**
 * List of all serial controllers in system
 */
typedef enum {
	SBA_SPI_MASTER_0 = 0, /*!< SOC SPI master controller 0, accessible by both processing entities */
	SBA_SPI_MASTER_1,     /*!< SOC SPI master controller 1, accessible by both processing entities */
	SBA_SPI_SLAVE_0,      /*!< SOC SPI slave controller */
	SBA_I2C_MASTER_0,     /*!< SOC I2C master controller 0, accessible by both processing entities */
	SBA_I2C_MASTER_1,     /*!< SOC I2C master controller 0, accessible by both processing entities */

	SBA_SS_SPI_MASTER_0,  /*!< SS  SPI master controller 0, accessible by ARC cpu only */
	SBA_SS_SPI_MASTER_1,  /*!< SS  SPI master controller 1, accessible by ARC cpu only */
	SBA_SS_I2C_MASTER_0,  /*!< SS  I2C master controller 0, accessible by ARC cpu only */
	SBA_SS_I2C_MASTER_1   /*!< SS  I2C master controller 1, accessible by ARC cpu only */
} SBA_BUSID;

/**
 *  Transfer request structure.
 */
struct sba_request;
typedef struct sba_request {
	list_t list;                                /*!< Pointer of the next request */
	SBA_REQUEST_TYPE request_type;              /*!< Request type */
	uint32_t tx_len;                            /*!< Size of data to write */
	uint8_t *tx_buff;                           /*!< Write buffer */
	uint32_t rx_len;                            /*!< Size of data to read */
	uint8_t *rx_buff;                           /*!< Read buffer */
	uint8_t full_duplex;                        /*!< Full duplex transfer request */
	union {
		SPI_SLAVE_ENABLE cs;                /*!< Chip select (SPI slave selection) */
		uint32_t slave_addr;                /*!< Address of the I2C slave */
	} addr;
	SBA_BUSID bus_id;                           /*!< Controller ID */
	SPI_BUS_MODE spi_bus_mode;              /*!< SPI Bus Mode selection*/
	int8_t status;                              /*!< 0 if ok, -1 if error */
	void *priv_data;                            /*!< User private data */
	void (*callback)(struct sba_request *);     /*!< Callback to notify transaction completion */
}sba_request_t;

/**
 *  SPI controller configuration.
 */
typedef struct sba_spi_config {
	uint32_t speed;                             /*!< SPI bus speed in KHz   */
	SPI_TRANSFER_MODE txfr_mode;                /*!< Transfer mode          */
	SPI_DATA_FRAME_SIZE data_frame_size;        /*!< Data Frame Size ( 4 - 16 bits ) */
	SPI_SLAVE_ENABLE slave_enable;              /*!< Slave Enable ( 0 = none - possibly used for Slaves that are selected by GPIO ) */
	SPI_BUS_MODE bus_mode;                      /*!< See SPI_BUS_MODE above for description */
	SPI_MODE_TYPE spi_mode_type;                /*!< SPI Master or Slave mode */
	uint8_t loopback_enable;                    /*!< Loopback enable */
}sba_spi_config_t;

/**
 *  I2C controller configuration.
 */
typedef struct sba_i2c_config {
	I2C_SPEED speed;                            /*!< I2C bus speed - Slow or Fast */
	I2C_ADDR_MODE addressing_mode;              /*!< 7 bit / 10 bit addressing */
	I2C_MODE_TYPE mode_type;                    /*!< Master or Slave */
	uint32_t slave_adr;                         /*!< I2C address if configured as a Slave */
}sba_i2c_config_t;

/**
 *  SBA controller configuration.
 */
union sba_config {
	sba_spi_config_t spi_config;
	sba_i2c_config_t i2c_config;
};

/**
 *  Structure to handle sba bus devices configuration
 */
struct sba_master_cfg_data {
	SBA_BUSID bus_id;                       /*!< Controller ID */
	union sba_config config;                /*!< SBA config*/
	/* internal fields */
	list_head_t request_list;               /*!< List to pending requests */
	sba_request_t *current_request;         /*!< Current request pointer */
	uint8_t controller_initialised;         /*!< Controller initialized flag */
	struct pm_wakelock sba_wakelock;        /*!< Power manager wakelock */
	struct clk_gate_info_s *clk_gate_info;  /*!< Clock gate data */
};

/**
 *  Structure to handle sba slave devices
 */
struct sba_device {
	struct td_device dev;           /*!< Device base structure */
	struct td_device *parent;       /*!< Parent Device bus */
	union {
		SPI_SLAVE_ENABLE cs;    /*!< Chip select */
		uint32_t slave_addr;    /*!< Address of the slave */
	} addr;
};

/** Export sba driver to link it with devices in soc_config file */
extern struct driver serial_bus_access_driver;

/**
 *  Request a SBA transfer.
 *
 *  Configuration parameters must be valid or an error is returned - see return values below.
 *  The callback specified in the request will be called on transfer completion.
 *
 *  @param request  Pointer to the request structure
 *
 *  @return
 *           - DRV_RC_OK on success,
 *           - DRV_RC_INVALID_CONFIG        - if any configuration parameter is not valid
 *           - DRV_RC_INVALID_OPERATION     - if both Rx and Tx while it is not implemented
 *           - DRV_RC_CONTROLLER_IN_USE     - when device is busy
 *           - DRV_RC_FAIL                  otherwise
 */
DRIVER_API_RC sba_exec_request(sba_request_t *request);

/**
 *  Request a SBA transfer for a specific device.
 *
 *  Configuration parameters must be valid or an error is returned - see return values below.
 *  The callback specified in the request will be called on transfer completion.
 *
 *  @param dev      sba device used to send the request
 *  @param req      sba request to send
 *
 *  @return
 *           - DRV_RC_OK on success,
 *           - DRV_RC_INVALID_CONFIG        - if any configuration parameter is not valid
 *           - DRV_RC_INVALID_OPERATION     - if both Rx and Tx while it is not implemented
 *           - DRV_RC_CONTROLLER_IN_USE     - when device is busy
 *           - DRV_RC_FAIL                  otherwise
 */
DRIVER_API_RC sba_exec_dev_request(struct sba_device *	dev,
				   struct sba_request * req);

/**
 *  Get physical bus id from logical bus id.
 *
 *  @param sba_alias_id logical bus id
 *
 *  @return  physical bus id
 */
uint32_t get_bus_id_from_sba(uint32_t sba_alias_id);
/** @} */

#endif /* INTEL_SBA_H_ */
