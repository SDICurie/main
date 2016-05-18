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

#ifndef INTEL_QRK_AONPT_H_
#define INTEL_QRK_AONPT_H_

#include "infra/device.h"

/**
 * @defgroup aonpt_driver AON Periodic Timer Driver
 * AON Periodic Timer management.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "drivers/intel_qrk_aonpt.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/drivers/rtc</tt>
 * <tr><th><b>Config flag</b> <td><tt>INTEL_QRK_AON_PT</tt>
 * </table>
 *
 * AON Periodic Timer is based on a 32768 Hz clock.
 * The timer is loaded with a value and constinuously decrements until the counter reaches 0.
 * When the timer expires, it reloads the configured value or stops if the value is 0.
 *
 * - \ref qrk_aonpt_configure to configure the timer
 * - \ref qrk_aonpt_start to start the timer
 * - \ref qrk_aonpt_stop to stop the timer
 *
 * @ingroup soc_drivers
 * @{
 */

/**
 *  AONPT driver.
 */
extern struct driver aonpt_driver;

/**
 *  Start or restart the periodic timer. This will decrement the initial
 *  value set via \ref qrk_aonpt_configure.
 *
 *  If the timer is already running, it will be stopped and restarted.
 *  If timer was configured with _one_shot_ set to true, the callback will be called only once.
 */
void qrk_aonpt_start(void);

/**
 *  Read the AON counter value.
 *
 *  The unit is (1/32768) secs.
 *
 *  @return  AON counter current value.
 */
uint32_t qrk_aonpt_read(void);

/**
 *  Stop the periodic timer.
 */
void qrk_aonpt_stop(void);

/**
 *  Configure the periodic timer. Call to \ref qrk_aonpt_start is required to
 *  actually start the timer.
 *
 *  When one shot feature is requested, the timer is automatically stopped after expiration.
 *
 *  @param   period          Initial value to trigger the alarm, unit is in (1/32768) secs.
 *  @param   on_timeout_cb   Callback function pointer, this cb function is called in ISR context.
 *  @param   one_shot        If true, the callback function is called once.
 *
 */
void qrk_aonpt_configure(uint32_t period,
			 void (*on_timeout_cb)(), bool one_shot);

/**
 * @}
 */
#endif //INTEL_QRK_AONPT_H_
