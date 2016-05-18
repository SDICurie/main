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

#ifndef _BLE_CORE_PM_H_
#define _BLE_CORE_PM_H_

#include "drivers/data_type.h"
#include "infra/device.h"

/**
 * @defgroup ble_core_pm BLE Core PM Driver
 * Handle PM communication from ble_core to Intel&reg; Curie&trade; platform
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "drivers/ble_core_pm.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/drivers/pm</tt>
 * <tr><th><b>Config flag</b> <td><tt>BLE_CORE_SUSPEND_BLOCKER_PM</tt>
 * </table>
 *
 * This driver prevents deep sleep mode when BLE Core is active
 * and configures a AON gpio to wakeup the platform from deepsleep
 * on BLE Core event.
 *
 * @ingroup ext_drivers
 * @{
 */

/**
 * BLE Core power management driver.
 */
extern struct driver ble_core_pm_driver;

/**
 * Structure to handle a ble_core_pm device
 */
struct ble_core_pm_info {
	uint8_t wakeup_pin;           /*!< AON GPIO used by BLE core to wakeup the SOC */
	struct td_device *gpio_dev;   /*!< GPIO device to use */
	bool gpio_pin_polarity;       /*!< GPIO pin polarity for triggering ISR */
	struct pm_wakelock ble_core_pm_wakelock; /*!< Wakelock to prevent suspend */
};

/**
 * Get current ble_core activity status
 *
 * @param   dev ble_core_pm device to use
 *
 * @return  TRUE if ble_core is active else false
 */
bool ble_core_pm_is_active(struct td_device *dev);

/** @} */

#endif //  _BLE_CORE_PM_H_
