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

#ifndef BLE_LNS_H_
#define BLE_LNS_H_

#include <stdint.h>

/**
 * @defgroup ble_lns LNS BLE service
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "lib/ble/lns/ble_lns.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>framework/src/lib/ble/lns</tt>
 * <tr><th><b>Config flag</b> <td><tt>BLE_LNS_LIB</tt>
 * </table>
 *
 * @ingroup ble_services
 * @{
 */

/* Forward declarations */
struct bt_conn;

/**
 * LNS response codes:
 * 1 Response for successful operation.
 * 2 Response if unsupported Op Code is received.
 * 3 Response if Parameter received does not meet the requirements
 *   of the service or is outside of the supported range of the Sensor.
 * 4 Response if the requested procedure failed.
 */
enum ble_lns_response {
	BLE_LNS_SUCCESS = 1,
	BLE_LNS_NOT_SUPPORTED = 2,
	BLE_LNS_INVALID_PARAM = 3,
	BLE_LNS_OPERATION_FAILED = 4,
};

/**
 * Register LNS service.
 *
 * The functions on_ble_lns_xxx must be implemented by the user of this
 * library to handle events.  If these event handlers are not defined, the
 * related feature will be unsupported.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int ble_lns_init(void);

/**
 * Function invoked when any peer has subscribed to notifications.
 */
void on_ble_lns_enabled(void);

/**
 * Function invoked when all peers have unsubscribed from notifications.
 */
void on_ble_lns_disabled(void);

/**
 * Function to update the elevation value.
 *
 * The remote peers will automatically be notified of this change if they
 * subscribed.
 *
 * @param elevation The new elevation value
 *
 * @return 0 in case of success or negative value in case of error.
 */
int ble_lns_elevation_update(int32_t elevation);

/**
 * Function invoked when a connected peer write the elevation.
 *
 * This procedure shall be completed by calling the corresponding reply
 * function.
 *
 * @param conn Connection object
 * @param elevation The elevation written by the peer
 */
void on_ble_lns_elevation_set(struct bt_conn *conn, int32_t elevation);

/**
 * Function to return a response upon elevation set event.
 *
 * @param conn Connection object
 * @param response Elevation set response
 */
void ble_lns_elevation_set_reply(struct bt_conn *	conn,
				 enum ble_lns_response	response);

/**
 * @}
 */
#endif /* BLE_LNS_H_ */
