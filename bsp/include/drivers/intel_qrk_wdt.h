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

#ifndef INTEL_QRK_WDT_H_
#define INTEL_QRK_WDT_H_

#include "drivers/data_type.h"
#include "drivers/clk_system.h"

/**
 * @defgroup wdt Watchdog Driver
 * WatchDog Timer driver API.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "drivers/intel_qrk_wdt.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/drivers/wdt</tt>
 * <tr><th><b>Config flag</b> <td><tt>INTEL_QRK_WDT</tt>
 * </table>
 *
 * The Watchdog interrupt is routed as a non maskable interrupt (NMI) and a
 * panic is triggered.
 * This ensures that in a case of stall the system goes into panic.
 *
 * For watchdog management see \ref watchdog.
 *
 * @ingroup soc_drivers
 * @{
 */

/**
 * Watchdog driver.
 */
extern struct driver watchdog_driver;

/*! Watchdog Power management structure */
struct wdt_pm_data {
	uint32_t timeout_range_register;        /*!< Watchdog enable/disable and reset configuration */
	uint32_t control_register;              /*!< Watchdog timeout duration */
};

/** @} */

#endif /* INTEL_QRK_WDT_H_ */
