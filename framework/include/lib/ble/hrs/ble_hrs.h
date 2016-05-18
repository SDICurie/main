/*
 * Copyright (c) 2015-2016, Intel Corporation. All rights reserved.
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

#ifndef BLE_HRS_H_
#define BLE_HRS_H_

#include <stdint.h>

/**
 * @defgroup ble_hrs HSR BLE service
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "lib/ble/hrs/ble_hrs.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>framework/src/lib/ble/hrs</tt>
 * <tr><th><b>Config flag</b> <td><tt>BLE_HRS_LIB</tt>
 * </table>
 *
 * @ingroup ble_services
 * @{
 */

/* Forward declarations */
struct bt_gatt_attr;

/**
 * Initialize the HRS library and register HRS service.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int ble_hrs_init(void);

/**
 * Supported body sensor locations
 */
enum ble_hrs_location {
	BLE_HRS_OTHER,
	BLE_HRS_CHEST,
	BLE_HRS_WRIST,
	BLE_HRS_FINGER,
	BLE_HRS_HAND,
	BLE_HRS_EAR_LOBE,
	BLE_HRS_FOOT
};

/**
 * Body sensor location, to be defined by the user, defaults to wrist.
 */
extern const enum ble_hrs_location ble_hrs_sensor_location;

/**
 * Function to update the heart rate measure.
 *
 * This function will update the heart rate value and notify any device that
 * has subscribed to receive value updates.
 *
 * @param value New heart rate value
 *
 * @return 0 in case of success or negative value in case of error.
 */
int ble_hrs_update(uint8_t value);

/**
 * Retrieve the reference of the HRS measure characteristic value attribute.
 *
 * @note This function is for test purposes only.
 *
 * @return The reference of the HRS measure characteristic value attribute.
 */
const struct bt_gatt_attr *ble_hrs_attr(void);

/**
 * @}
 */
#endif /* BLE_HRS_H_ */
