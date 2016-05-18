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

#include "infra/log.h"
#include "util/workqueue.h"
#include "infra/xloop.h"

xloop_t default_work_queue;

OS_ERR_TYPE workqueue_queue_work(void (*cb)(void *data), void *cb_data)
{
	xloop_post_func(&default_work_queue, cb, cb_data);
	return E_OS_OK;
}

/* Definition of the private task "TASK_WORKQUEUE" */
#if defined(CONFIG_MICROKERNEL)
#include <zephyr.h>
#include "microkernel/task.h"
DEFINE_TASK(TASK_WORKQUEUE, 6, workqueue_task_port,
	    CONFIG_ZEPHYR_WORKQUEUE_TASK_STACKSIZE,
	    0);
/**
 * This function is there for compatibility issue between the original
 * "workqueue_task" definition and the requested prototype by the macro
 * DEFINE_TASK from the file "microkernel/task.h"
 */
void workqueue_task_port(void)
{
	extern void workqueue_task(void *param);
	workqueue_task(&default_work_queue);
}
#else
#include "nanokernel.h"
#define WORKQUEUE_TASK_STACKSIZE 640
char wq_stack[WORKQUEUE_TASK_STACKSIZE];
/**
 * This function is there for compatibility issue between the original
 * "workqueue_task" definition and the requested prototype by the func
 * task_fiber_start.
 */
void workqueue_task_port(int d1, int d2)
{
	extern void workqueue_task(void *param);
	workqueue_task(&default_work_queue);
}
#endif

void workqueue_task(void *param)
{
	pr_debug(LOG_MODULE_UTIL, "Start workqueue");
	xloop_run(&default_work_queue);
}

void init_workqueue_task(void)
{
	T_QUEUE q = queue_create(10);

	pr_debug(LOG_MODULE_UTIL, "Initializing workqueue");

	xloop_init_from_queue(&default_work_queue, q);

#if defined(CONFIG_MICROKERNEL)
	task_start(TASK_WORKQUEUE);
#else
	task_fiber_start(&wq_stack[0], WORKQUEUE_TASK_STACKSIZE,
			 (nano_fiber_entry_t)workqueue_task_port, 0, 0, 50, 0);
#endif
}
