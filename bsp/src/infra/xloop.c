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

#include "infra/xloop.h"
#include "infra/log.h"
#include "infra/time.h"
#include "infra/message.h"
#include "util/assert.h"
#include "infra/port.h"

/* Wrapped jobs or messages waiting to be posted with delay */
typedef struct delayed_item {
	struct delayed_item *next;
	xloop_t *l;
	uint64_t post_time;
	struct msg_flags *item;
} delayed_item_t;

/* This function is run only in the execution context of this xloop */
static void insert_job_in_delayed_items_list(xloop_job_t *job)
{
	delayed_item_t *le = job->data;
	delayed_item_t *item = (delayed_item_t *)le->l->delayed_items.next;

	if (item == NULL || le->post_time < item->post_time) {
		le->next = item;
		le->l->delayed_items.next = (list_t *)le;
	} else {
		while (item->next &&
		       item->next->post_time < le->post_time) {
			item = item->next;
		}
		le->next = item->next;
		item->next = le;
	}
	bfree(job);
}

void xloop_init_from_queue(xloop_t *l, T_QUEUE q)
{
	l->queue = q;
	l->delayed_items.next = NULL;
}

__noreturn void xloop_run(xloop_t *l)
{
	T_QUEUE_MESSAGE m;

	while (1) {
		delayed_item_t *le = (delayed_item_t *)l->delayed_items.next;
		if (!le) {
			queue_get_message(l->queue, &m, OS_WAIT_FOREVER, NULL);
		} else {
			/* We have a at least one delayed item, use a timeout */
			OS_ERR_TYPE err;
			m = NULL;
			int timeout = le->post_time - get_uptime64_ms();
			if (timeout > 0) {
				queue_get_message(l->queue, &m, timeout, &err);
			} else {
				err = E_OS_ERR_TIMEOUT;
			}
			if (err == E_OS_ERR_TIMEOUT) {
				l->delayed_items.next = (list_t *)le->next;
				queue_send_message(l->queue, le->item, NULL);
				bfree(le);
				assert(m == NULL);
				continue;
			}
		}
		assert(m);
		struct msg_flags *flags = (struct msg_flags *)m;
		if (flags->f_is_job) {
			xloop_job_t *job = (xloop_job_t *)m;
			job->run(job);
		} else {
			struct message *msg = (struct message *)m;
			port_process_message(msg);
		}
	}
}

void xloop_post_message(xloop_t *l, struct message *m)
{
	m->flags.f_is_job = 0;
	queue_send_message(l->queue, m, NULL);
}

void xloop_post_job(xloop_t *l, xloop_job_t *j)
{
	j->flags.f_is_job = 1;
	j->flags.f_queue_head = 0;
	j->loop = l;
	queue_send_message(l->queue, j, NULL);
}

struct func_job {
	xloop_job_t j;
	void (*fn)(void *data);
	void *data;
};

void xloop_func_run(xloop_job_t *data)
{
	struct func_job *fj = (struct func_job *)data;

	fj->fn(fj->data);
	bfree(fj);
}

void xloop_post_func(xloop_t *l, void (*func)(void *param), void *param)
{
	struct func_job *fj = (struct func_job *)balloc(sizeof(*fj), NULL);

	fj->j.run = xloop_func_run;
	fj->j.data = fj;
	fj->fn = func;
	fj->data = param;
	xloop_post_job(l, &fj->j);
}

struct periodic_func {
	xloop_job_t j;
	int (*fn)(void *data);
	void *data;
	int period;
};

void xloop_func_periodic_run(xloop_job_t *data)
{
	struct periodic_func *pf = (struct periodic_func *)data;

	if (pf->fn(pf->data)) {
		bfree(pf);
	} else {
		xloop_post_job_delayed(pf->j.loop, &pf->j, pf->period);
	}
}

void xloop_post_func_periodic(xloop_t *l, int (*func)(void *param),
			      void *param, int period)
{
	struct periodic_func *pf = (struct periodic_func *)balloc(sizeof(*pf),
								  NULL);

	pf->period = period;
	pf->j.run = xloop_func_periodic_run;
	pf->j.data = pf;
	pf->fn = func;
	pf->data = param;
	xloop_post_job_delayed(l, &pf->j, period);
}

void xloop_post_job_delayed(xloop_t *l, xloop_job_t *j, uint32_t delay)
{
	j->flags.f_is_job = 1;
	j->flags.f_queue_head = 0;
	j->loop = l;

	/* Create the item to put in the delayed_items list*/
	delayed_item_t *le = (delayed_item_t *)balloc(sizeof(*le), NULL);
	le->item = (struct msg_flags *)j;
	le->l = l;
	le->post_time = get_uptime64_ms() + delay;

	/* We can't just add it to the list in this execution context to avoid
	 * concurrency issues. So we post a job to ourself so that le is added
	 * in the context of this xloop */
	xloop_job_t *jj = balloc(sizeof(*jj), NULL);
	jj->run = insert_job_in_delayed_items_list;
	jj->data = le;
	xloop_post_job(l, jj);
}
