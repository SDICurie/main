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

#include "machine.h"

#include <limits.h>

#include "infra/ipc.h"
#include "infra/port.h"
#include "infra/pm.h"
#include "infra/device.h"
#include "infra/time.h"
#include "util/compiler.h"
#include "util/misc.h"

/* This function is also defined in OS test suite */
#ifndef CONFIG_ARC_OS_UNIT_TESTS
void mbxIsr(void *param)
{
	int sts = MBX_STS(IPC_QRK_SS_ASYNC);

	if (sts & 0x02) {
		MBX_STS(IPC_QRK_SS_ASYNC) = sts;
	}
	ipc_handle_message();
}
#else
extern void mbxIsr(void *param);
#endif

#define MBX(_offset_) (*(volatile unsigned int *)(0xb0800a60 + (_offset_)))

int _mbxPollOut(int data)
{
	//while((* (volatile unsigned int *)(0xB0800000 + 0xA60 + 20)) & 1)
	//    ; // STS
	int flags = irq_lock();

	MBX(4) = (unsigned int)data; //DAT0
	MBX(8) = 1; // DAT1
	MBX(0) = 0x80000000; // CTRL
	while (!MBX(20) & 1)
		;

	while (MBX(20) & 1)
		;  // STS
	irq_unlock(flags);

	if (data == '\n')
		_mbxPollOut('\r');

	/* If end of line, delay to give lakemont
	 * a chance to do some processing */
	if (data == '\r') {
		volatile int count = 10000;
		while (count--) ;
	}
	return data;
}

extern void __printk_hook_install(int (*fn)(int));
extern void __stdout_hook_install(int (*fn)(int));

void soc_setup()
{
	set_cpu_id(CPU_ID_ARC);

	__printk_hook_install(_mbxPollOut);
	__stdout_hook_install(_mbxPollOut);

	ipc_init(IPC_SS_QRK_REQ, IPC_QRK_SS_REQ,
		 IPC_SS_QRK_ACK, IPC_QRK_SS_ACK, CPU_ID_QUARK);

	irq_connect_dynamic(SOC_MBOX_INTERRUPT, ISR_DEFAULT_PRIO, mbxIsr, NULL,
			    0);
	irq_enable(SOC_MBOX_INTERRUPT);


	/* Init wakelocks */
	pm_wakelock_init_mgr();

	/* Init devices */
	init_all_devices();

	/* Sync IPC channel */
	SOC_MBX_INT_UNMASK(IPC_QRK_SS_REQ);
	/* Async IPC channel */
	SOC_MBX_INT_UNMASK(IPC_QRK_SS_ASYNC);

	/* Notify QRK that ARC started. */
	shared_data->arc_ready = 1;
}

void publish_cpu_timeout(uint32_t timeout_tick)
{
	if (timeout_tick == UINT32_MAX) {
		shared_data->arc_next_wakeup_valid = false;
		shared_data->arc_next_wakeup = 0;
	} else {
		shared_data->arc_next_wakeup_valid = true;

		uint32_t timeout32k = get_uptime_32k() + CONVERT_TICKS_TO_32K(
			timeout_tick);
		shared_data->arc_next_wakeup = timeout32k;
	}
	/* Wakeup Quark core so it can acknowledge the request quickly */
	MBX_CTRL(IPC_SS_QRK_ASYNC) = 0x80000000;
}

bool notrace pm_wakelock_are_other_cpu_wakelocks_taken(void)
{
	return shared_data->any_lmt_wakelock_taken;
}

void notrace pm_wakelock_set_any_wakelock_taken_on_cpu(bool wl_status)
{
	shared_data->any_arc_wakelock_taken = wl_status;
	if (wl_status == false) {
		/* Wakeup Quark core so it can re-enter idle task and take this new
		 * information into account quickly, and not waste time before going
		 * in deepsleep */
		MBX_CTRL(IPC_SS_QRK_ASYNC) = 0x80000000;
	}
}
