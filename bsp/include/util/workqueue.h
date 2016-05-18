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

#ifndef __UTIL_WORK_QUEUE_H__
#define __UTIL_WORK_QUEUE_H__

#ifdef CONFIG_MICROKERNEL
/* OS include, needed for task handling */
#include <zephyr.h>
#endif

#include "os/os.h"

/**
 * @defgroup workqueue Work Queues
 *
 * Work Queues implementation.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "util/workqueue.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/util</tt>
 * <tr><th><b>Config flag</b> <td><tt>WORKQUEUE</tt>
 * </table>
 *
 * Workqueues are used to defer work in a fiber / task context.
 * Generally, an interrupt handler will use queue_work() or
 * queue_work_on() in order to execute a callback function in a
 * non-interrupt context.
 *
 * This requires an execution context to be setup. This execution
 * context should call workqueue_init() and either workqueue_run() if
 * the context is fully dedicated to workqueue, or workqueue_poll() if the
 * context is shared between workqueue execution and other things (typically
 * bootloader or other bare metal context only have one execution context and
 * it should be shared between several components.)
 *
 * @ingroup infra
 * @{
 */

/**
 * Initialize the default workqueue
 *
 * This function will initialize the default workqueue and start
 * the workqueue task. This function will be called during bsp
 * initialization.
 */
void init_workqueue_task(void);

/**
 * Post work to the default workqueue
 *
 * This function will post a request in the default system workqueue.
 *
 * \param cb      Callback to execute
 * \param cb_data Data passed to the callback
 *
 * \return E_OS_OK If work properly queued
 */
OS_ERR_TYPE workqueue_queue_work(void (*cb)(void *data), void *cb_data);

/** @} */

#endif /* __INFRA_UTIL_WORK_QUEUE_H__ */
