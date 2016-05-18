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

#ifndef __REBOOT_H__
#define __REBOOT_H__

#include "boot.h"

/**
 * @defgroup infra_pm_api Power Management API
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "infra/reboot.h"</tt>
 * </table>
 *
 * @ingroup infra_pm
 * @{
 */

/**
 * List of all shutdown types.
 */
enum shutdown_reason {
	SHUTDOWN_DEFAULT, /* default shutdown */
	SHUTDOWN_THERMAL, /* shutdown because of thermal issue */
	SHUTDOWN_LOW_BAT, /* shutdown because of low battery */
	SHUTDOWN_USER_DEFINED = 0x1F, /* User defined shutdown reason, others
	                               * user defined reasons shall stay after
	                               * this one. */
	MAX_SHUTDOWN_REASONS = 0xFF
};

/**
 * Request a reboot of the platform.
 *
 * This triggers a platform reboot in the selected functional mode.
 *
 * Before rebooting, the platform is gracefully shutdown, as in @ref shutdown.
 *
 * @param boot_target Requested functional mode
 */
void reboot(enum boot_targets boot_target);

/** Shutdown
 *
 * Platform will be gracefully shutdown. System and Services are gracefully
 * stopped.
 *
 * The battery is not put into ship mode.
 *
 * Platform can be started upon external or internal event depending on the
 * hardware (power button, clock alarm, ...).
 *
 * @param reason Reason of the shutdown
 * */
void shutdown(enum shutdown_reason reason);

/** Emergency shutdown
 *
 * Platform will be shutdown. System and Services are not gracefully stopped.
 *
 * Data loss can be expected, as no data are flushed to non-volatile memory.
 *
 * The battery is not put into ship mode.
 *
 * Platform can be started upon external or internal event depending on the
 * hardware (power button, clock alarm, ...).
 *
 * @param reason Reason of the shutdown
 * */
void emergency_shutdown(enum shutdown_reason reason);

/**
 * Power off the platform.
 *
 * Platform is gracefully shutdown, and then put in a minimal consumption state.
 * The battery is latched or put in ship mode.
 *
 * Note that the latched off mode needs specific hardware support to physically
 * disconnect the battery and re-connect it with a power button press.
 *
 * @param reason Reason of the power off
 */
void power_off(enum shutdown_reason reason);

/** @} */

#endif
