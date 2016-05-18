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

#ifndef _APDS9190_H_
#define _APDS9190_H_

#include "drivers/serial_bus_access.h"
#include "infra/pm.h"

/**
 * @defgroup apds9190 APDS9190 Proximity Sensor Driver
 * This driver implements APDS9190 chip support.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "drivers/apds9190.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/drivers/sensor</tt>
 * <tr><th><b>Config flag</b> <td><tt>APDS9190</tt>
 * </table>
 *
 * @ingroup ext_drivers
 * @{
 */

/**
 * IR Proximity driver structure
 */
extern struct driver apds9190_driver;

/**
 * Structure to handle and configure an apds9190 device
 */
struct apds9190_info {
	/* Configuration fields */
	uint8_t ptime;                  /*!< Proximity ADC time */
	uint8_t wtime;                  /*!< Wait time */
	uint8_t pers;                   /*!< Interrupt persistence filter */
	uint8_t config;                 /*!< Configuration */
	uint8_t ppcount;                /*!< Proximity pulse count */
	int8_t comp_pin;                /*!< Comparator pin for interrupt mode */
	uint16_t pilt;                  /*!< Proximity low threshold */
	uint16_t piht;                  /*!< Proximity high threshold */
	/* Internal driver fields */
	struct sba_request req;         /*!< SBA request object used to transfer i2c data */
	struct sba_request irq_req;     /*!< SBA request object used to transfer i2c data */
	uint8_t irq_tx_buff[8];
	bool prox;
	T_SEMAPHORE i2c_sync_sem;       /*!< Semaphore to wait for an i2c transfer to complete */
	struct td_device *comparator_device;
	void (*cb)(bool near, void *param);
	void *cb_data;
};


/**
 * Return the current proximity value
 *
 * \param dev Device structure
 * \return positive value of proximity
 *         - if > 0, the value is the raw adc value read between 0 and 1023
 *         - negative value means error
 */
int apds9190_read_prox(struct td_device *dev);

/**
 * Set the proximity callback
 *
 * \param device Device structure
 * \param callback Callback to be called on proximity change
 *                 - 1rst parameter is true if proximity above high threshold
 *                 - 2nd parameter is the param as passed on callback registration
 * \param param Data to pass back to the callback
 *
 * \return 0 if success, other if error
 */
int apds9190_set_callback(struct td_device *device, void (*callback)(bool,
								     void *),
			  void *param);

/**
 * Get proximity status
 *
 * \param device Device structure
 *
 * \return true if attached, false if detached
 */
bool apds9190_get_status(struct td_device *device);

/** @} */
#endif /* _LED_apds9190_H_ */
