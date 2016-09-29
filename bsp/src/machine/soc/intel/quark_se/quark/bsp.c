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

#include <init.h>

#include "infra/ipc.h"
#include "infra/log.h"
#include "infra/system_events.h"

#include "machine/soc/intel/quark_se/quark/soc_setup.h"
#include "machine.h"

/* Console manager setup helper */
#include "infra/console_manager.h"

/* Test command client setup */
#include "machine/soc/intel/quark_se/quark/uart_tcmd_client.h"
#include "infra/tcmd/engine.h"
#include "util/workqueue.h"

T_QUEUE bsp_init(void)
{
	/* Initialize OS abstraction */
	os_init();

#ifdef CONFIG_IPC
	/* Setup IPC and main queue */
	T_QUEUE queue = ipc_setup();
#else
	T_QUEUE queue = queue_create(CONFIG_MAIN_TASK_QUEUE_SIZE, NULL);
	assert(queue);
#endif

	/* Setup the SoC hardware and logs */
	soc_setup();

	/* Start log infrastructure */
	log_start();

#ifdef CONFIG_CONSOLE_MANAGER
	/* Console manager setup */
	console_manager_init();
#endif

#ifdef CONFIG_TCMD
	/* Enable test command engine async support through the main queue */
	tcmd_async_init(queue);
#endif
#ifdef CONFIG_TCMD_CONSOLE_UART
	extern struct device DEVICE_NAME_GET(uart_ns16550_1);
	/* Test commands will use the same port as the log system */
	set_tcmd_uart_dev(DEVICE_GET(uart_ns16550_1));
#endif

#if defined(CONFIG_WORKQUEUE)
	/* Start the workqueue */
	init_workqueue_task();
#endif

#ifdef CONFIG_SYSTEM_EVENTS
	system_events_init();
#endif

	/* Start and init the ARC BSP */
	start_arc(0);

	return queue;
}
