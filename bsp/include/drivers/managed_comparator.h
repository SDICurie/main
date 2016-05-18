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

#ifndef _MANAGED_COMPARATOR_H_
#define _MANAGED_COMPARATOR_H_

#include "os/os.h"
#include "util/list.h"

/**
 * @defgroup managed_comparator Managed Comparator Driver
 * Manage Comparator Driver API
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "drivers/managed_comparator.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/drivers/aio</tt>
 * <tr><th><b>Config flag</b> <td><tt>MANAGED_COMPARATOR</tt>
 * </table>
 *
 * This driver uses SOC comparator driver to catch toggling event on a AIN pin
 * and notify changes to a registered callback.
 * At initialization time, the comparator is configured to notify a rising edge.
 * At each notified event (rising or falling), the comparator is configured
 * to notify the other edge (falling or rising).
 *
 * @ingroup soc_drivers
 * @{
 */

/**
 * managed_comparator driver struct.
 */
extern struct driver managed_comparator_driver;

/**
 * Structure to handle a managed_comparator device
 */
struct managed_comparator_info {
	struct td_device *evt_dev;      /*!< Pin event source device (typically the SOC comparator device) */
	uint8_t source_pin;             /*!< Pin to use for the selected source (typically the comparator index) */
	uint8_t debounce_delay;         /*!< Time in millisecond to debounce source_pin */
	/* Internal driver fields */
	T_TIMER pin_debounce_timer;     /*!< Timer used to debounce source_pin */
	uint8_t pin_deb_counter;        /*!< Debounce counter */
	bool pin_status;                /*!< Last pin status */
	list_head_t cb_head;            /*!< List of user callback functions */
};

/**
 * Register a callback to handle pin event
 *
 * @param   dev  managed_comparator device to use
 * @param   cb   Callback to register
 *               - First arg is the pin state
 *               - 2nd arg is the private data specified on registration
 * @param   priv User private data (passed to callback)
 *
 */
void managed_comparator_register_callback(struct td_device *dev, void (*cb)(
						  bool, void *), void *priv);

/**
 * Unregister a callback function
 *
 * @param   dev  managed_comparator device to use
 * @param   cb   Callback to unregister
 *
 * @return  (int) 0 if unregister succeeded, -1 otherwise.
 */
int managed_comparator_unregister_callback(struct td_device *dev, void (*cb)(
						   bool, void *));

/**
 * Get current pin status
 *
 * @param   dev managed_comparator device to use
 *
 * @return  TRUE if pin is high else false
 */
bool managed_comparator_get_state(struct td_device *dev);

/** @} */

#endif /* _MANAGED_COMPARATOR_H_ */
