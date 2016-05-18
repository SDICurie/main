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

#ifndef _NFC_STN54_H_
#define _NFC_STN54_H_

#include "drivers/gpio.h"
#include "drivers/serial_bus_access.h"

/**
 * @defgroup common_driver_nfc STN54 NFC Driver
 * This driver implements NFC STN54D/E chip support.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "drivers/nfc_stn54.h.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/drivers/nfc</tt>
 * <tr><th><b>Config flag</b> <td><tt>NFC_STN54</tt>
 * </table>
 *
 * @ingroup ext_drivers
 * @{
 */

/**
 * NFC driver structure.
 */
extern struct driver nfc_stn54_driver;

/**
 * Internal variables used by the STN54E device driver
 */
struct nfc_stn54_info {
	struct pm_wakelock wakelock;                   /*!< Wakelock */
	uint8_t stn_reset_pin;                         /*!< STN reset pin number */
	uint8_t stn_pwr_en_pin;                        /*!< STN power enable pin number */
	uint8_t stn_irq_pin;                           /*!< STN IRQ_OUT pin number */
	uint8_t booster_reset_pin;                     /*!< Optional RF Booster reset pin */
	/* Internal driver fields */
	struct td_device *gpio_dev;                    /*!< GPIO device to use */
	struct sba_request req;                        /*!< SBA request object used to transfer i2c data */
	T_SEMAPHORE i2c_sync_sem;                      /*!< Semaphore to wait for an i2c transfer to complete */
};

/**
 * Configure the IRQ_OUT interrupt
 *
 * If the normal GPIO interrup is used, this only has to be done once at driver
 * initialization - but the interrupt will not be raised if SoC is in deep sleep.
 *
 * If the comparator mode is used, then the function will have to be called when
 * the rx handler is set by the NFC SM, and the interrupt will have to be re-enabled
 * after each i2c read from the NFC controller.
 *
 * @return none
 */
void nfc_stn54_config_irq_out(void);

/**
 *  RX handler callback function signature
 *
 *  @param dev Pointer to the NFC driver 'struct device *'
 *  @return none
 */
typedef void (*nfc_stn54_rx_handler_t)(void *dev);

/**
 *  Set the callback to handle rx events triggered on the IRQ_OUT pin by the
 *  NFC controller.
 *
 *  @param   handler Pointer to the callback function
 *  @return  none
 */
void nfc_stn54_set_rx_handler(nfc_stn54_rx_handler_t handler);

/**
 *  Clear the rx data callback.
 *
 */
void nfc_stn54_clear_rx_handler();

/**
 *  Power up and reset the NFC controller and booster (if available).
 *  Same function can be used as a reset only.
 *
 *  @return  none
 */
void nfc_stn54_power_up(void);

/**
 *  Power down the NFC controller and booster (if available).
 *
 *  @return  none
 */
void nfc_stn54_power_down(void);

/**
 *  Reset the NFC controller and booster (if available).
 *
 *  @return  none
 */
void nfc_stn54_reset(void);

/**
 *  Read data from the NFC controller over the I2C bus.
 *
 *  @param   buf    Pointer where to return the read data
 *  @param   size   Number of bytes to be read
 *  @return Result of bus communication function
 *           - DRV_RC_OK on success,
 *           - DRV_RC_TIMEOUT               - Could not take semaphore before timeout expiration
 *           - DRV_RC_INVALID_CONFIG        - If any configuration parameter is not valid
 *           - DRV_RC_INVALID_OPERATION     - If both Rx and Tx while it is not implemented
 *           - DRV_RC_CONTROLLER_IN_USE     - When device is busy
 *           - DRV_RC_FAIL                  Otherwise
 */
DRIVER_API_RC nfc_stn54_read(uint8_t *buf, uint16_t size);

/**
 *  Write data to the NFC controller over the I2C bus.
 *
 *  @param   buf    Pointer to data source buffer
 *  @param   size   Number of bytes to be written
 *  @return Result of bus communication function
 *           - DRV_RC_OK on success,
 *           - DRV_RC_TIMEOUT               - Could not take semaphore before timeout expiration
 *           - DRV_RC_INVALID_CONFIG        - If any configuration parameter is not valid
 *           - DRV_RC_INVALID_OPERATION     - If both Rx and Tx while it is not implemented
 *           - DRV_RC_CONTROLLER_IN_USE     - When device is busy
 *           - DRV_RC_FAIL                  Otherwise
 */
DRIVER_API_RC nfc_stn54_write(const uint8_t *buf, uint16_t size);

/** @} */

#endif /* _NFC_STN54_H_ */
