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

#ifndef __PM_H__
#define __PM_H__

#include "util/list.h"
#include "os/os.h"
#include "infra/reboot.h"

/**
 * @defgroup infra_pm Power Management
 * @ingroup infra
 * @{
 */

/**
 * @defgroup infra_pm_wl Wakelocks API
 * @ingroup infra_pm
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "infra/pm.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/drivers/pm</tt>
 * </table>
 *
 * - A wakelock is a lock used to prevent the platform to enter suspend/shutdown.
 * - When a component acquires a wakelock, it needs to release it before platform
 *   can enter in suspend/resume mode.
 * - The platform is notified that all wakelocks have been released through the
 *   callback function configured using the function
 *   pm_wakelock_set_list_empty_cb.
 *
 * A component using wakelocks should allocate a struct pm_wakelock.
 *  - pm_wakelock_init to initialize the wakelock structure
 *  - pm_wakelock_acquire to acquire the wakelock
 *  - pm_wakelock_release to release the wakelock
 *
 * @{
 */

/**
 * Wakelock managing structure.
 *
 */
struct pm_wakelock {
	list_t list;     /*!< Internal list management member */
	unsigned int lock; /*!< Lock to avoid acquiring a lock several times */
};

/**
 * Initialize wakelock management structure.
 *
 * This is automatically done by @ref bsp_init.
 */
void pm_wakelock_init_mgr();

/**
 * Initialize a wakelock structure.
 *
 * @param wli Address of the wakelock structure to initialize.
 */
void pm_wakelock_init(struct pm_wakelock *wli);

/**
 * Acquire a wakelock.
 *
 * This prevents the platform from sleeping.
 *
 * @param wl Wakelock to acquire.
 *
 * @return 0 on success, -EINVAL if already locked
 */
int pm_wakelock_acquire(struct pm_wakelock *wl);

/**
 * Release a wakelock.
 *
 * @param wl Wakelock to release
 *
 * @return 0 on success, -EINVAL if already released, -ENOENT if list clear
 */
int pm_wakelock_release(struct pm_wakelock *wl);

/**
 * Check if wakelock list is empty.
 *
 * @return true if at least one wakelock is acquired else false
 */
bool pm_wakelock_is_list_empty();

/**
 * Set the callback to call when wakelock list is empty.
 *
 * It will replace the previous callback.
 *
 * The platform is notified that all wakelocks have been released through the
 * callback function configured using this function.
 *
 * @param cb   the callback function
 * @param priv the private data to pass to the callback function
 */
void pm_wakelock_set_list_empty_cb(void (*cb)(void *), void *priv);

/**
 * Core specific function to set wakelock state shared variable.
 *
 * Set cpu specific wakelock state shared variable.
 * For example, when all wakelocks are released and when at least
 * one is taken.
 *
 * @param wl_status Current wakelock status to set.
 */
void pm_wakelock_set_any_wakelock_taken_on_cpu(bool wl_status);

/**
 * Check slaves wakelocks state
 *
 * @return false if all wakelocks are released
 */
bool pm_wakelock_are_other_cpu_wakelocks_taken(void);

/** @} */


/**
 * @defgroup infra_pm_ipc IPC power management
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "infra/pm.h"</tt>
 * </table>
 *
 * @ingroup infra_pm
 * @{
 */

/**
 * Defines the function that pm slaves nodes needs to implement in order
 * to handle power management IPC requests.
 *
 * @param cpu_id the CPU triggering the pm request
 * @param pm_req the pm request type (PM_SHUTDOWN/REBOOT/POWER_OFF)
 * @param param the request parameter
 */
int pm_notification_cb(uint8_t cpu_id, int pm_req, int param);

/** @} */

/**
 * @defgroup infra_pm_hooks Hooks and callbacks
 * @ingroup infra_pm
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "infra/pm.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/infra</tt>
 * </table>
 *
 * @{
 */

/**
 * Board specific hook to provide shutdown.
 *
 * If a board requires a specific action to provide shutdown, it will need to
 * implement this function that will be called by master core's pm shutdown API.
 */
void board_shutdown_hook(void);

/**
 * Board specific hook to provide poweroff.
 *
 * If a board requires a specific action to provide poweroff, it will need to
 * implement this function that will be called by master core's pm poweroff API.
 *
 * to disconnect battery.
 */
void board_poweroff_hook(void);

/**
 * Set the hook for shutdown/reboot events.
 *
 * When set, this hook must call the shutdown_hook_complete function with
 * data as argument before it returns.
 *
 * @param hook Callback for shutdown/reboot events
 */
void pm_register_shutdown_hook(void (*hook)(void (*shutdown_hook_complete)(
						    void *data), void *data));

/** @} */

/** @} */

#endif /* __PM_H_ */
