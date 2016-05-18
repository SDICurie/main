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

#include <string.h>

#include "util/assert.h"
#include "os/os.h"
#include "infra/port.h"
#include "infra/log.h"

#include "cfw/cfw.h"
#include "cfw/cfw_service.h"

#include "cfw_internal.h"

/**
 * \file client_api.c
 *
 * Implementation of the client interface.
 *
 */

struct service_and_queue {
	list_t l;
	int service_id;
	T_QUEUE queue;
} service_and_queue;

/* The queue used by each service registered locally (on this CPU).
 * If a service is not listed or it's queue is set to NULL, it is assumed to
 * use the same queue as the service manager */
static list_head_t queue_for_service_list;

static bool search_by_id(list_t *l, void *id)
{
	return ((struct service_and_queue *)l)->service_id == *((int *)id);
}
static struct service_and_queue *get_service_and_queue(int service_id)
{
	return (struct service_and_queue *)list_find_first(
		       &queue_for_service_list, search_by_id, &service_id);
}
static T_QUEUE get_queue_for_service(int service_id)
{
	struct service_and_queue *s = get_service_and_queue(service_id);

	return s ? s->queue : NULL;
}

static int free_and_return_true(void *l, void *unused)
{
	bfree(l); return 1;
}
static void clear_service_and_queue_list(void)
{
	list_foreach_del(&queue_for_service_list, free_and_return_true, NULL);
	list_init(&queue_for_service_list);
}

static bool cfw_service_init_done = false;

/* Start & end address of the code section dedicated to cfw_services */
extern const struct _cfw_registered_service __cfw_services_start[];
extern const struct _cfw_registered_service __cfw_services_end[];

#ifndef NDEBUG
static const struct _cfw_registered_service *get_registered_service(
	int service_id)
{
	const struct _cfw_registered_service *s = __cfw_services_start;

	for (; s < __cfw_services_end; ++s)
		if (s->id == service_id)
			return s;
	return NULL;
}
#endif

void cfw_set_queue_for_service(int service_id, T_QUEUE queue)
{
	/* This function is not working if called after a service is init-ed */
	assert(cfw_service_init_done == false);
	struct service_and_queue *s = get_service_and_queue(service_id);
	if (s) {
		s->queue = queue;
		return;
	}
	/* Make sure this service was registered */
	assert(get_registered_service(service_id) != NULL);
	s = balloc(sizeof(service_and_queue), NULL);
	s->service_id = service_id;
	s->queue = queue;
	list_add(&queue_for_service_list, (list_t *)s);
}

void cfw_init_registered_services(T_QUEUE default_queue)
{
	assert(
		(__cfw_services_end -
		 __cfw_services_start) /
		sizeof(struct _cfw_registered_service) <=
		MAX_SERVICES);
	const struct _cfw_registered_service *s = __cfw_services_start;
	for (; s < __cfw_services_end; ++s) {
		T_QUEUE q = get_queue_for_service(s->id);
		s->init(s->id, q == NULL ? default_queue : q);
		pr_info(LOG_MODULE_CFW, "%s service init in progress...",
			s->name);
	}
	cfw_service_init_done = true;
	/* We don't need this array anymore, free all used RAM */
	clear_service_and_queue_list();
}

#ifdef CONFIG_CFW_MASTER
/* Default implementation for the master */
__weak void cfw_init(T_QUEUE queue)
{
	/* CFW init on Quark: initialize the master service manager */
	cfw_service_mgr_init(queue);
	cfw_init_registered_services(queue);
}
#endif

#ifdef CONFIG_CFW_PROXY
/* Default implementation for the slave */
__weak void cfw_init(T_QUEUE queue)
{
	cfw_service_mgr_init_proxy(queue, 1);
	cfw_init_registered_services(queue);
}
#endif

__noreturn void cfw_loop(T_QUEUE queue)
{
	struct cfw_message *message;
	T_QUEUE_MESSAGE m;

	while (1) {
		queue_get_message(queue, &m, OS_WAIT_FOREVER, NULL);
		message = (struct cfw_message *)m;
		if (message != NULL) {
			port_process_message(&message->m);
		}
	}
}

void cfw_open_service_conn(cfw_client_t *client, int service_id, void *priv)
{
	cfw_open_conn_req_msg_t *msg = (cfw_open_conn_req_msg_t *)
				       cfw_alloc_message(sizeof(*msg));

	CFW_MESSAGE_ID(&msg->header) = MSG_ID_CFW_OPEN_SERVICE_REQ;
	CFW_MESSAGE_LEN(&msg->header) = sizeof(*msg);
	CFW_MESSAGE_DST(&msg->header) = cfw_get_service_mgr_port_id();
	CFW_MESSAGE_SRC(&msg->header) =
		((_cfw_client_t *)client)->client_port_id;
	CFW_MESSAGE_TYPE(&msg->header) = TYPE_REQ;

	cfw_service_conn_t *service_conn =
		(cfw_service_conn_t *)balloc(sizeof(*service_conn), NULL);
	service_conn->port = CFW_MESSAGE_SRC(&msg->header);
	service_conn->client = client;
	service_conn->server_handle = NULL;
	service_conn->service_id = service_id;

	msg->service_id = service_id;
	msg->service_conn = service_conn;
	msg->client_cpu_id = port_get_cpu_id(CFW_MESSAGE_SRC(&msg->header));
	msg->header.priv = priv;
	msg->header.conn = NULL;
	cfw_send_message(msg);
}

void cfw_close_service_conn(const cfw_service_conn_t *service_conn, void *priv)
{
	cfw_close_conn_req_msg_t *msg = (cfw_close_conn_req_msg_t *)
					cfw_alloc_message(sizeof(*msg));

	CFW_MESSAGE_ID(&msg->header) = MSG_ID_CFW_CLOSE_SERVICE_REQ;
	CFW_MESSAGE_LEN(&msg->header) = sizeof(*msg);
	CFW_MESSAGE_DST(&msg->header) = cfw_get_service_mgr_port_id();
	CFW_MESSAGE_SRC(&msg->header) =
		((_cfw_client_t *)service_conn->client)->client_port_id;
	msg->header.priv = priv;
	msg->header.conn = service_conn->server_handle;
	msg->service_id = service_conn->service_id;
	msg->inst = NULL;
	cfw_send_message(msg);
}

void cfw_register_events(const cfw_service_conn_t *c, int *msg_ids, int size,
			 void *priv)
{
	int msg_size = sizeof(struct cfw_message) + sizeof(int) * (size + 1);
	int i;
	struct cfw_message *msg = cfw_alloc_message_for_service(
		c, MSG_ID_CFW_REGISTER_EVT_REQ, msg_size,
		priv);

	CFW_MESSAGE_DST(msg) = cfw_get_service_mgr_port_id();
	((int *)&msg[1])[0] = size;
	for (i = 0; i < size; i++) {
		((int *)(&msg[1]))[i + 1] = msg_ids[i];
	}
	cfw_send_message(msg);
}

void cfw_register_svc_available(cfw_client_t *client, int service_id,
				void *priv)
{
	cfw_register_svc_avail_req_msg_t *msg =
		(cfw_register_svc_avail_req_msg_t *)cfw_alloc_message(sizeof(*
									     msg));

	CFW_MESSAGE_ID(&msg->header) = MSG_ID_CFW_REGISTER_SVC_AVAIL_EVT_REQ;
	CFW_MESSAGE_LEN(&msg->header) = sizeof(*msg);
	CFW_MESSAGE_SRC(&msg->header) =
		((_cfw_client_t *)client)->client_port_id;
	CFW_MESSAGE_DST(&msg->header) = cfw_get_service_mgr_port_id();
	CFW_MESSAGE_TYPE(&msg->header) = TYPE_REQ;
	msg->header.priv = priv;
	msg->header.conn = NULL;
	msg->service_id = service_id;
	cfw_send_message(msg);
}

struct cfw_message *cfw_alloc_message_for_service(
	const cfw_service_conn_t *c,
	int
	msg_id,
	int
	msg_size,
	void *
	priv)
{
	return cfw_alloc_msg_for_service_cb(c, msg_id, msg_size, NULL, priv);
}

struct cfw_message *cfw_alloc_msg_for_service_cb(
	const cfw_service_conn_t *	c,
	int
	msg_id,
	int
	msg_size,
	void				(
		*			cb)(
		struct			cfw_message *),
	void *
	priv)
{
	struct cfw_message *msg = (struct cfw_message *)cfw_alloc_message(
		msg_size);

	CFW_MESSAGE_ID(msg) = msg_id;
	CFW_MESSAGE_LEN(msg) = msg_size;
	CFW_MESSAGE_SRC(msg) = ((_cfw_client_t *)c->client)->client_port_id;
	CFW_MESSAGE_DST(msg) = c->port;
	CFW_MESSAGE_TYPE(msg) = TYPE_REQ;
	CFW_MESSAGE_CB(msg) = cb;
	msg->priv = priv;
	msg->conn = c->server_handle;
	return msg;
}

struct cfw_message *cfw_alloc_rsp_message_for_client(cfw_service_conn_t *c,
						     int msg_id, int msg_size,
						     void *priv)
{
	struct cfw_message *msg = (struct cfw_message *)cfw_alloc_message(
		msg_size);

	CFW_MESSAGE_ID(msg) = msg_id;
	CFW_MESSAGE_LEN(msg) = msg_size;
	CFW_MESSAGE_SRC(msg) = c->port;
	CFW_MESSAGE_DST(msg) = ((_cfw_client_t *)c->client)->client_port_id;
	CFW_MESSAGE_TYPE(msg) = TYPE_RSP;
	msg->priv = priv;
	msg->conn = c;
	return msg;
}

struct cfw_message *cfw_alloc_message(int size)
{
	return (struct cfw_message *)message_alloc(size, NULL);
}
