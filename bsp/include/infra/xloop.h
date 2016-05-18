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

#ifndef __XLOOP_H__
#define __XLOOP_H__

/**
 * @defgroup xloop Execution Loop
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "infra/xloop.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/infra</tt>
 * </table>
 *
 * An execution loop (xloop) provides an execution context based on a queue to
 * which messages and job can be posted.
 *
 * A typical use case is to have one xloop instance per thread with each thread
 * running the never returning xloop_run() function. Application logic can then
 * be event/job based.
 *
 * @ingroup infra
 * @{
 */

#include "os/os.h"
#include "util/list.h"
#include "infra/message.h"
#include "util/compiler.h"

/**
 * An execution loop provides an execution context based on a queue on which
 * messages and job can be posted.
 */
typedef struct xloop {
	T_QUEUE queue;
	list_t delayed_items;
} xloop_t;

/** A job that can be posted to a xloop */
typedef struct xloop_job {
	/** Flags used to determine whether an instance is a message or a job */
	struct msg_flags flags;
	/** Function to call to run the job. It is safe to free this xloop_job_t
	 * instance from within this function if necessary */
	void (*run)(struct xloop_job *data);
	/** Data passed to the run function */
	void *data;
	/** xloop associated with the job */
	xloop_t *loop;
} xloop_job_t;

/**
 * Initialize an execution loop from an existing queue.
 *
 * @param l Execution loop to initialize
 * @param q Queue on which messages and job can be posted
 */
void xloop_init_from_queue(xloop_t *l, T_QUEUE q);

/**
 * Start the execution loop.
 *
 * Start processing incoming messages and jobs on the passed xloop.
 * This function never returns.
 *
 * @param l Execution loop
 */
__noreturn void xloop_run(xloop_t *l);

/**
 * Post a message on the xloop queue. It will be processed later in the
 * execution context of the xloop.
 *
 * @param l xloop instance on which to post the message
 * @param m Message to post
 */
void xloop_post_message(xloop_t *l, struct message *m);

/**
 * Post a job on the xloop queue. It will be run later in the execution context
 * of the xloop.
 *
 * @param l xloop instance on which to post the job
 * @param j Job to post on the xloop queue. It is the responsibility of the
 * caller to allocate and free this instance.
 */
void xloop_post_job(xloop_t *l, xloop_job_t *j);

/**
 * Post a differed job on the xloop queue. The job is guaranteed to be run after
 * the passed time (but with an undetermined delay).
 *
 * @param l xloop instance on which to post the job
 * @param j Job to post on the xloop queue
 * @param delay Delay to wait in ms before posting the job
 */
void xloop_post_job_delayed(xloop_t *l, xloop_job_t *j, uint32_t delay);

/**
 * Post a function job on the xloop queue.
 *
 * @param l xloop instance on which to post the job
 * @param fn Function that will be executed in the context of the xloop
 * @param param Parameter passed to the function
 */
void xloop_post_func(xloop_t *l, void (*fn)(void *param), void *param);

/**
 * Post a pediodic function call on the xloop. The callback function will
 * be called periodically in the context of the xloop. When the callback
 * returns != 0 the periodic call will be canceled.
 *
 * @param l xloop instance on which to post the function call
 * @param fn Function to call in the context of the xloop
 * @param param Parameter passed to the function
 * @param period Period (in ms) at which the function should be called.
 */
void xloop_post_func_periodic(xloop_t *l, int (*fn)(
				      void *param), void *param, int period);

/** @} */

#endif /* __XLOOP_H__ */
