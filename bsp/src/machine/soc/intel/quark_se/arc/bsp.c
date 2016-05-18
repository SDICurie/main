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

#include "util/workqueue.h"

#include "infra/ipc.h"
#include "infra/log.h"
#include "infra/port.h"
#include "infra/tcmd/engine.h"

#include "machine/soc/intel/quark_se/arc/soc_setup.h"
#include "machine.h"


T_QUEUE bsp_init(void)
{
	/* Start AON Counter */
	SCSS_REG_VAL(SCSS_AONC_CFG) |= 0x00000001;

	/* Enable instruction cache - TODO fix magic numbers */
	uint32_t sreg = _lr(0x11);
	sreg &= 0xfffffffe;
	_sr(sreg, 0x11);  // Bit 0 of Aux Reg 0x11.

	os_init();
	log_init();
	soc_setup();

	port_set_ports_table(shared_data->ports);

	T_QUEUE q = queue_create(64);
	set_cpu_message_sender(CPU_ID_QUARK, ipc_async_send_message);
	set_cpu_free_handler(CPU_ID_QUARK, ipc_async_free_message);
	ipc_async_init(q);

	/* Test Commands initialization */
#ifdef CONFIG_TCMD_ASYNC
	tcmd_async_init(q, "arc");
#endif

	log_start();

#ifdef CONFIG_WORKQUEUE
	init_workqueue_task();
#endif

	return q;
}
