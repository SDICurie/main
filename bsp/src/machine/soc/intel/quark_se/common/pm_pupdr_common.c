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

#include "machine/soc/intel/quark_se/pm_pupdr.h"
#include "machine.h"
#include "os/os.h"
#include "infra/log.h"
#include "infra/device.h"
#include "infra/system_events.h"
#include "infra/ipc.h"
#include "infra/panic.h"

#include "quark_se_common.h"

#define PM_SHUTDOWN_TIMEOUT 5000

static void (*pm_shutdown_hook)(void (*shutdown_hook_complete)(void *),
				void *) = NULL;
static enum pupdr_request shutdown_hook_complete_arg = PUPDR_NO_REQ;

static enum pupdr_request pm_req = PUPDR_NO_REQ;
static int pm_state;

static void pm_timeout_cb(void *priv)
{
	pr_error(LOG_MODULE_QUARK_SE, "shutdown timeout");
	/* Force shutdown */
	pm_shutdown();
}

void pm_register_shutdown_hook(void (*hook)(void (*shutdown_hook_complete)(
						    void *data), void *data))
{
	/* Panic if pm_shutdown_hook has been already set */
	if (pm_shutdown_hook)
		panic(E_OS_ERR);
	else
		pm_shutdown_hook = hook;
}

static void pm_shutdown_complete(enum pupdr_request req, int param)
{
	/* Send PM IPC request to all slaves */
	ipc_request_sync_int(IPC_REQUEST_INFRA_PM, req, param, NULL);

	switch (req) {
	case PUPDR_POWER_OFF:
		pr_debug(LOG_MODULE_QUARK_SE, "Poweroff %d", param);
		break;
	case PUPDR_SHUTDOWN:
		pr_debug(LOG_MODULE_QUARK_SE, "Shutdown %d", param);
		break;
	case PUPDR_REBOOT:
		pr_debug(LOG_MODULE_QUARK_SE, "Reboot %d", param);
		break;
	default:
		pr_error(LOG_MODULE_QUARK_SE, "Unknown req %d", req);
		return;
	}

	pm_req = req;

	return;
}

static void pm_shutdown_event_shutdown_hook_complete()
{
	pr_debug(LOG_MODULE_QUARK_SE, "SVC Shutdown callback");
	log_suspend();
	pm_shutdown_complete(shutdown_hook_complete_arg, pm_state);
}

void pm_shutdown_request(enum pupdr_request req, int param)
{
	/* start timeout timer */
	timer_create(pm_timeout_cb, NULL, PM_SHUTDOWN_TIMEOUT, false, true,
		     NULL);
	log_flush();
	pm_state = param;

#ifdef CONFIG_SYSTEM_EVENTS
	system_event_push_shutdown(req, param);
#endif

	/* Call pm_shutdown_hook if exists */
	if (pm_shutdown_hook) {
		shutdown_hook_complete_arg = req;
		pm_shutdown_hook(&pm_shutdown_event_shutdown_hook_complete,
				 NULL);
	} else
		pm_shutdown_complete(req, param);
}

void pm_shutdown(void)
{
	/* Suspend devices in interrupt lock context */
	suspend_devices(PM_SHUTDOWN);
	pr_debug(LOG_MODULE_QUARK_SE, "Shutdown: %d", pm_req);
	/* Shutdown platform */
	pm_core_shutdown(pm_req, pm_state);
}

#ifdef CONFIG_DEEPSLEEP
bool pm_is_deepsleep_allowed()
{
	if (!shared_data->arc_ready) {
		/* Slave not ready */
		return false;
	}
	if (!pm_wakelock_is_list_empty()) {
		/* Wakelock on current core */
		return false;
	}

	return pm_is_core_deepsleep_allowed();
}
#endif

bool pm_is_shutdown_allowed()
{
	if (pm_req == PUPDR_NO_REQ) {
		return false;
	}

	return pm_is_core_shutdown_allowed();
}
