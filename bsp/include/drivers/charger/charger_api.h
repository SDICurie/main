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

#ifndef _CHARGER_API_H_
#define _CHARGER_API_H_

#include "infra/device.h"

/**
 * @defgroup charger Charger Driver
 * Charger driver API
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "drivers/charger/charger_api.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/drivers/charger</tt>
 * <tr><th><b>Config flag</b> <td><tt>CHARGER</tt>
 * </table>
 *
 * @ingroup ext_drivers
 * @{
 */

/**
 * @enum DRV_CHG_EVT
 * @brief Charger events
 */
typedef enum {
	DISCHARGING = 0,
	CHARGING,
	CHARGE_COMPLETE,
	CHARGER_FAULT
} DRV_CHG_EVT;

/**
 * Charger driver structure.
 */
extern struct driver charger_driver;

/**
 * Structure to handle a charger device
 */
struct charger_info {
	struct td_device *evt_dev;      /*!< Pin event source device */
	/* Internal driver fields */
	DRV_CHG_EVT state;              /*!< Current state */
	list_head_t cb_head;            /*!< List for user callback functions */
};

/**
 * Register a callback to handle charging event
 *
 * @param dev  Charger device used
 * @param cb   Callback to register
 *             - 1st parameter is the charger event
 *             - 2nd parameter is the private data provided on callback registration
 * @param priv User private data that will be passed to callback
 *
 */
void charger_register_callback(struct td_device *dev, void (*cb)(DRV_CHG_EVT,
								 void *),
			       void *priv);

/**
 * Unregister a callback function
 *
 * @param dev Charger device used
 * @param cb  Callback to unregister
 *
 * @return 0 if unregister succeeded, -1 otherwise.
 */
int charger_unregister_callback(struct td_device *dev, void (*cb)(DRV_CHG_EVT,
								  void *));

/**
 * Get current charge status
 *
 * @param dev Charger device used
 *
 * @return state of charge
 */
DRV_CHG_EVT charger_get_current_soc(struct td_device *dev);

/**
 * Enable charger
 */
void charger_enable(void);

/**
 * Disable charger
 */
void charger_disable(void);

/**
 * Config charger
 */
void charger_config(void);

/** @} */

#endif /* _CHARGER_API_H_ */
