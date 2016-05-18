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

#ifndef BLE_RSCS_H_
#define BLE_RSCS_H_

#include <stdint.h>

/**
 * @defgroup ble_rscs RSCS BLE service
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "lib/ble/rscs/ble_rscs.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>framework/src/lib/ble/rscs</tt>
 * <tr><th><b>Config flag</b> <td><tt>BLE_RSCS_LIB</tt>
 * </table>
 *
 * @ingroup ble_services
 * @{
 */

enum ble_rscs_sensor_location {
	BLE_SENSOR_LOCATION_OTHER,      /**<-- Other        */
	BLE_SENSOR_LOCATION_TOP_OF_SHOE, /**<-- Top of shoe  */
	BLE_SENSOR_LOCATION_IN_SHOE,    /**<-- In shoe      */
	BLE_SENSOR_LOCATION_HIP,        /**<-- Hip          */
	BLE_SENSOR_LOCATION_FRONT_WHEEL, /**<-- Front Wheel  */
	BLE_SENSOR_LOCATION_LEFT_CRANK, /**<-- Left Crank   */
	BLE_SENSOR_LOCATION_RIGHT_CRANK, /**<-- Right Crank  */
	BLE_SENSOR_LOCATION_LEFT_PEDAL, /**<-- Left Pedal   */
	BLE_SENSOR_LOCATION_RIGHT_PEDAL, /**<-- Right Pedal  */
	BLE_SENSOR_LOCATION_FRONT_HUB,  /**<-- Front Hub    */
	BLE_SENSOR_LOCATION_REAR_DROPOUT, /**<-- Rear Dropout */
	BLE_SENSOR_LOCATION_CHAINSTAY,  /**<-- Chainstay    */
	BLE_SENSOR_LOCATION_REAR_WHEEL, /**<-- Rear Wheel   */
	BLE_SENSOR_LOCATION_REAR_HUB,   /**<-- Rear Hub     */
	BLE_SENSOR_LOCATION_CHEST,      /**<-- Chest        */
	BLE_SENSOR_LOCATION_SPIDER,     /**<-- Spider       */
	BLE_SENSOR_LOCATION_CHAIN_RING, /**<-- Chain Ring   */
};

/**
 * Register RSCS service.
 *
 * The functions on_ble_rscs_xxx must be implemented by the user of this
 * library to handle events.  If these event handlers are not defined, the
 * related feature will be unsupported.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int ble_rscs_init(void);

/**
 * Function to update the measurement value.
 *
 * The remote peers will automatically be notified of this change if they
 * subscribed.
 *
 * @param speed The new instantaneous speed value
 * @param cadence The new instantaneous cadence value
 *
 * @return 0 in case of success or negative value in case of error.
 *
 * @note This function will invoke the corresponding functions to retrieve the
 * value of the optional measurements: stride length, total distance and
 * walk/run status.
 */
int ble_rscs_update(uint16_t speed, uint8_t cadence);

/**
 * Function invoked when any peer has subscribed to notifications.
 */
void on_ble_rscs_enabled(void);

/**
 * Function invoked when all peers have unsubscribed from notifications.
 */
void on_ble_rscs_disabled(void);

/**
 * Function invoked to get the current stride length.
 *
 * @return Stride length value when positive or negative value if not supported.
 */
int on_ble_rscs_get_stride_lenth(void);

/**
 * Function invoked to get the total distance.
 *
 * @return Distance value when positive or negative value if not supported.
 */
int on_ble_rscs_get_total_distance(void);

/**
 * Function invoked to get the walking or running status.
 *
 * @return 0 for walking, 1 for running or negative value if not supported.
 */
int on_ble_rscs_get_walk_run(void);

#ifdef CONFIG_BLE_RSCS_SENSOR_LOCATION_SUPPORT
/**
 * Function invoked to get the sensor location.
 *
 * @return Sensor location value (@ref ble_rscs_sensor_location)
 */
uint8_t on_ble_rscs_get_sensor_location(void);
#endif

/**
 * @}
 */
#endif /* BLE_RSCS_H_ */
