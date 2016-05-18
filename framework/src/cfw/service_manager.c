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

#include <zephyr.h>
#include <stdbool.h>
#include <string.h>

#include "util/assert.h"
#include "os/os.h"
#include "util/list.h"
#include "infra/log.h"
#include "infra/port.h"
#include "infra/panic.h"
#include "infra/ipc.h"
#include "infra/pm.h"
#include "cfw/cfw.h"
#include "cfw/cfw_service.h"
#include "cfw_internal.h"
#include "cfw/cproxy.h"
#include "services/service_queue.h"
#ifdef CONFIG_PORT_MULTI_CPU_SUPPORT
#include "machine.h" /* NUM_CPU */
#endif
#include <stdint.h>
#include "util/compiler.h"


/* Private internal messages */
#define MSG_ID_NOTIFY_SERVICE_AVAIL  0xff00
#define MSG_ID_SHUTDOWN_SERVICES_REQ 0xff01

#define PANIC_NO_SVC_SLOT 1 /*!< Panic error code when all SVC slots are used */

/**
 * \file service_manager.c implementation of the service_manager
 */
service_t *services[MAX_SERVICES];

static int registered_service_count = 0;

static int service_mgr_port_id = 0;

#ifdef CONFIG_PORT_MULTI_CPU_SUPPORT
struct proxy {
	int port_id;
};
static struct proxy proxies[NUM_CPU];
#endif

/**
 * This function is called when a message needs to be sent through the IPC.
 *
 */

list_head_t service_avail_listeners = { NULL };

struct service_avail_listener {
	list_t l;
	uint16_t port;
	int service_id;
	void *priv;
};

list_head_t service_avail_list = { NULL };

struct service_avail {
	list_t list;
	service_t *svc;
};

struct service_shutdown_message {
	struct message msg;
	void (*shutdown_hook_complete)(void *);
	void *data;
};

static int _find_service(int service_id)
{
	int i;

	for (i = 0; i < MAX_SERVICES; i++) {
		if (services[i] != NULL && services[i]->service_id ==
		    service_id) {
			return i;
		}
	}
	return -1;
}

void add_service_avail_listener(uint16_t port, int service_id, void *priv)
{
	struct service_avail_listener *l =
		(struct service_avail_listener *)balloc(sizeof(*l), NULL);

	l->port = port;
	l->service_id = service_id;
	l->priv = priv;
	pr_debug(LOG_MODULE_CFW, "Register : %d to %d", port, service_id);
	list_add(&service_avail_listeners, &l->l);
}

static void send_service_avail_evt(int service_id, uint16_t port_id,
				   void *param)
{
	cfw_svc_available_evt_msg_t *evt =
		(cfw_svc_available_evt_msg_t *)message_alloc(sizeof(*evt), NULL);

	evt->service_id = service_id;
	CFW_MESSAGE_LEN(&evt->header) = sizeof(*evt);
	CFW_MESSAGE_ID(&evt->header) = MSG_ID_CFW_SVC_AVAIL_EVT;
	CFW_MESSAGE_SRC(&evt->header) = service_mgr_port_id;
	CFW_MESSAGE_DST(&evt->header) = port_id;
	CFW_MESSAGE_TYPE(&evt->header) = TYPE_EVT;
	evt->header.priv = param;

	pr_debug(LOG_MODULE_CFW, "Notify : %d to %d", service_id, port_id);
	cfw_send_message(evt);
}

int notify_service_avail_cb(void *item, void *param)
{
	struct service_avail_listener *l =
		(struct service_avail_listener *)item;
	int service_id = *(int *)param;

	if (l->service_id == service_id) {
		send_service_avail_evt(service_id, l->port, l->priv);
		bfree(l);
		return 1; /* remove from list */
	}
	return 0;
}

void notify_service_avail(int service_id)
{
	int svc_id = service_id;

	list_foreach_del(&service_avail_listeners, notify_service_avail_cb,
			 &svc_id);
}

void send_slave_service_shutdown(service_t *svc, struct cfw_message *rply_msg)
{
	struct cfw_message *msg =
		(struct cfw_message *)message_alloc(sizeof(*msg), NULL);

	CFW_MESSAGE_PRIV(msg) = rply_msg;
	CFW_MESSAGE_SRC(msg) = service_mgr_port_id;
#ifdef CONFIG_PORT_MULTI_CPU_SUPPORT
	CFW_MESSAGE_DST(msg) = proxies[port_get_cpu_id(svc->port_id)].port_id;
#else
	CFW_MESSAGE_DST(msg) = service_mgr_port_id;
#endif
	CFW_MESSAGE_CONN(msg) = (void *)svc->service_id;
	CFW_MESSAGE_ID(msg) = MSG_ID_CFW_SERVICE_SHUTDOWN_REQ;
	cfw_send_message(msg);
}

/*
 * Called when an sync IPC request is issued from a secondary CPU (a proxy).
 *
 * Used to register a service or allocate a port.
 * Should be called in the context of the IPC interrupt.
 *
 * @param cpu_id the cpu_id that originated the request
 * @param request the request id.
 *
 * @return the value to be passed as response to the requestor
 */
static int _cfw_handle_ipc_sync_request(uint8_t cpu_id, int request, int param1,
					int param2,
					void *ptr)
{
#ifdef SVC_MANAGER_DEBUG
	pr_debug(LOG_MODULE_CFW, "%s: from %d, req:%d (%d, %d, %p)", __func__,
		 cpu_id, request, param1,
		 param2,
		 ptr);
#endif
	switch (request) {
	case IPC_REQUEST_REGISTER_SERVICE:
	{
		service_t *svc;
#ifndef CONFIG_SHARED_MEM
		svc = balloc(sizeof(*svc), NULL);
		svc->service_id = param1;
		svc->port_id = param2;
		svc->client_connected = NULL;
		svc->client_disconnected = NULL;
		svc->registered_events_changed = NULL;
		svc->shutdown_request = send_slave_service_shutdown;
#else
		svc = (service_t *)ptr;
#endif
		_cfw_register_service(svc);
	}
	break;

#ifdef CONFIG_PORT_MULTI_CPU_SUPPORT
	case IPC_REQUEST_REGISTER_PROXY:
#ifdef SVC_MANAGER_DEBUG
		pr_debug(LOG_MODULE_CFW,
			 "%s(): proxy registered for cpu %d @port %d",
			 __func__, cpu_id,
			 param1);
#endif
		proxies[cpu_id].port_id = param1;
		break;
#endif

	case IPC_REQUEST_DEREGISTER_SERVICE:
	{
	}
	break;

	default:
		pr_debug(LOG_MODULE_CFW, "%s: unhandled ipc request: %x",
			 __func__,
			 request);
		break;
	}
	return 0;
}

void internal_handle_message(struct cfw_message *msg, void *param)
{
	int free_msg = 1; /* by default free message */

	switch (CFW_MESSAGE_ID(msg)) {
	case MSG_ID_CFW_OPEN_SERVICE_REQ:
	{
		cfw_open_conn_req_msg_t *req = (cfw_open_conn_req_msg_t *)msg;
		service_t *svc = cfw_get_service(req->service_id);
		if (svc == NULL) {
			pr_error(LOG_MODULE_CFW,
				 "try to open non registered service %d",
				 req->service_id);
			cfw_open_conn_rsp_msg_t *resp =
				(cfw_open_conn_rsp_msg_t *)cfw_alloc_rsp_msg(
					msg,
					MSG_ID_CFW_OPEN_SERVICE_RSP,
					sizeof(*resp));
			resp->status = E_OS_ERR_UNKNOWN;
			cfw_send_message(resp);
			break;
		}
		uint8_t svc_cpu_id = port_get_cpu_id(svc->port_id);
		if (svc_cpu_id != get_cpu_id()) {
#ifdef CONFIG_PORT_MULTI_CPU_SUPPORT
			assert(svc_cpu_id < NUM_CPU);
			pr_debug(LOG_MODULE_CFW,
				 "forward open service to proxy");
			CFW_MESSAGE_DST(msg) = proxies[svc_cpu_id].port_id;
			cfw_send_message(msg);
			free_msg = 0; /* the lower layers will free it! */
#else
			pr_error(LOG_MODULE_CFW,
				 "incorrect cpu_id settings, single cpu!");
#endif
		} else {
			conn_handle_t *conn_handle;

			conn_handle =
				(conn_handle_t *)balloc(sizeof(*conn_handle),
							NULL);
			conn_handle->client_port = CFW_MESSAGE_SRC(msg);
			conn_handle->priv_data = NULL;
			conn_handle->svc = svc;
			conn_handle->client_handle = req->service_conn;
			/* For OPEN_SERVICE, conn is not know yet, it is just alloc'ed.
			 * set it here.*/
			req->header.conn = conn_handle;
			if (svc->client_connected != NULL &&
			    svc_cpu_id == get_cpu_id())
				svc->client_connected(conn_handle);
			cfw_open_conn_rsp_msg_t *resp =
				(cfw_open_conn_rsp_msg_t *)cfw_alloc_rsp_msg(
					msg,
					MSG_ID_CFW_OPEN_SERVICE_RSP,
					sizeof(*resp));
			resp->port = svc->port_id;
			resp->cpu_id = svc_cpu_id;
#ifdef SVC_MANAGER_DEBUG
			pr_debug(LOG_MODULE_CFW,
				 "OPEN_SERVICE: %d, svc:%p port:%d",
				 req->service_id,
				 svc,
				 svc->port_id);
#endif
			resp->svc_server_handle = conn_handle;
			resp->service_conn = req->service_conn;
			cfw_send_message(resp);
		}
		break;
	}

	case MSG_ID_CFW_CLOSE_SERVICE_REQ:
	{
		cfw_close_conn_req_msg_t *req = (cfw_close_conn_req_msg_t *)msg;
		service_t *svc = cfw_get_service(req->service_id);
		if (svc == NULL) {
			pr_debug(LOG_MODULE_CFW,
				 "try close unregistered service %d",
				 req->service_id);
			cfw_close_conn_rsp_msg_t *resp =
				(cfw_close_conn_rsp_msg_t *)cfw_alloc_rsp_msg(
					msg,
					MSG_ID_CFW_CLOSE_SERVICE_RSP,
					sizeof(*resp));
			resp->status = E_OS_ERR_UNKNOWN;
			cfw_send_message(resp);
			break;
		}
		uint8_t svc_cpu_id = port_get_cpu_id(svc->port_id);
		if (svc_cpu_id != get_cpu_id()) {
#ifdef CONFIG_PORT_MULTI_CPU_SUPPORT
			CFW_MESSAGE_DST(msg) = proxies[svc_cpu_id].port_id;
			cfw_send_message(msg);
			free_msg = 0;
#else
			pr_error(LOG_MODULE_CFW, "incorrect cpu_id!");
#endif
		} else {
			cfw_close_conn_rsp_msg_t *resp =
				(cfw_close_conn_rsp_msg_t *)cfw_alloc_rsp_msg(
					msg,
					MSG_ID_CFW_CLOSE_SERVICE_RSP,
					sizeof(*resp));
			conn_handle_t *conn = (conn_handle_t *)msg->conn;
			if (conn != NULL && conn->svc != NULL
			    && conn->svc->client_disconnected != NULL)
				conn->svc->client_disconnected(conn);
			_cfw_unregister_event((conn_handle_t *)msg->conn);
			cfw_send_message(resp);
			/* Free server-side conn */
			bfree(conn);
		}
		break;
	}

	case MSG_ID_CFW_REGISTER_EVT_REQ:
	{
		int *params = (int *)&msg[1];
		int i;
		for (i = 0; i < params[0]; i++) {
			_cfw_register_event((conn_handle_t *)msg->conn,
					    params[i + 1]);
		}
		cfw_register_evt_rsp_msg_t *resp =
			(cfw_register_evt_rsp_msg_t *)cfw_alloc_rsp_msg(
				msg,
				MSG_ID_CFW_REGISTER_EVT_RSP,
				(sizeof(*resp)));
		conn_handle_t *conn = (conn_handle_t *)msg->conn;
		if (conn != NULL && conn->svc != NULL
		    && conn->svc->registered_events_changed != NULL)
			conn->svc->registered_events_changed(conn);

		cfw_send_message(resp);
		break;
	}

	case MSG_ID_CFW_REGISTER_SVC_AVAIL_EVT_REQ:
	{
		bool already_avail = true;
		cfw_register_svc_avail_req_msg_t *req =
			(cfw_register_svc_avail_req_msg_t *)msg;
		int flags = irq_lock();
		if (_find_service(req->service_id) == -1) {
			add_service_avail_listener(CFW_MESSAGE_SRC(msg),
						   req->service_id, msg->priv);
			already_avail = false;
		}
		irq_unlock(flags);
		cfw_register_svc_avail_rsp_msg_t *resp =
			(cfw_register_svc_avail_rsp_msg_t *)cfw_alloc_rsp_msg(
				msg,
				MSG_ID_CFW_REGISTER_SVC_AVAIL_EVT_RSP,
				(sizeof(*resp)));
		cfw_send_message(resp);
		if (already_avail) {
			send_service_avail_evt(req->service_id,
					       CFW_MESSAGE_SRC(msg), msg->priv);
		}
		break;
	}
	case MSG_ID_SHUTDOWN_SERVICES_REQ:
	case MSG_ID_CFW_SERVICE_SHUTDOWN_RSP:
	{
		struct service_avail *sa = (struct service_avail *)list_get(
			&service_avail_list);
		while (sa) {
			pr_debug(LOG_MODULE_CFW,
				 "Shutting down Service with ID: %d",
				 sa->svc->service_id);
			if (sa->svc->shutdown_request) {
				struct cfw_message *rply_msg =
					(struct cfw_message *)message_alloc(
						sizeof(*rply_msg), NULL);
				if (CFW_MESSAGE_ID(msg) ==
				    MSG_ID_SHUTDOWN_SERVICES_REQ) {
					CFW_MESSAGE_PRIV(rply_msg) = msg;
					free_msg = 0;
				} else {
					CFW_MESSAGE_PRIV(rply_msg) =
						CFW_MESSAGE_PRIV(msg);
				}
				CFW_MESSAGE_SRC(rply_msg) = sa->svc->port_id;
				CFW_MESSAGE_DST(rply_msg) = service_mgr_port_id;
				CFW_MESSAGE_ID(rply_msg) =
					MSG_ID_CFW_SERVICE_SHUTDOWN_RSP;
				sa->svc->shutdown_request(sa->svc, rply_msg);
				bfree(sa);
				break;
			} else {
				bfree(sa);
				sa = (struct service_avail *)list_get(
					&service_avail_list);
			}
		}
		if (sa == NULL) {
			pr_debug(LOG_MODULE_CFW, "Services shutdown complete");
			struct service_shutdown_message *ssm =
				(struct service_shutdown_message *)
				CFW_MESSAGE_PRIV(msg);
			ssm->shutdown_hook_complete(ssm->data);
		}
		break;
	}
	case MSG_ID_NOTIFY_SERVICE_AVAIL:
	{
		notify_service_avail(((service_t *)CFW_MESSAGE_PRIV(
					      msg))->service_id);
		struct service_avail *service_avail = (struct service_avail *)
						      balloc(
			sizeof(*
			       service_avail),
			NULL);
		memset(service_avail, 0, sizeof(*service_avail));
		service_avail->svc = (service_t *)CFW_MESSAGE_PRIV(msg);
		list_add_head(&service_avail_list, &service_avail->list);
		break;
	}
	default:
		pr_warning(LOG_MODULE_CFW, "%s: unhandled message id: %x",
			   __func__, CFW_MESSAGE_ID(
				   msg));
		break;
	}
	if (free_msg)
		cfw_msg_free(msg);
}

static void cfw_services_shutdown(void (*shutdown_hook_complete)(
					  void *), void *data)
{
	struct service_shutdown_message *ssm =
		(struct service_shutdown_message *)
		message_alloc(sizeof(*ssm), NULL);

	MESSAGE_ID(&ssm->msg) = MSG_ID_SHUTDOWN_SERVICES_REQ;
	MESSAGE_SRC(&ssm->msg) = service_mgr_port_id;
	MESSAGE_DST(&ssm->msg) = service_mgr_port_id;
	ssm->shutdown_hook_complete = shutdown_hook_complete;
	ssm->data = data;
	cfw_send_message(ssm);
}

/**
 * Indication list
 * Holds a list of receivers.
 */
typedef struct {
	list_t list;
	conn_handle_t *conn_handle;
} indication_list_t;

/**
 * \struct registered_int_list_t holds a list of registered clients to an indication
 *
 * Holds a list of registered receiver for each indication.
 */
typedef struct registered_evt_list_ {
	list_t list; /*! Linking stucture */
	list_head_t lh; /*! List of client */
	int ind; /*! Indication message id */
} registered_evt_list_t;

list_head_t registered_evt_list;

registered_evt_list_t *get_event_registered_list(int msg_id)
{
	registered_evt_list_t *l =
		(registered_evt_list_t *)registered_evt_list.head;

	while (l) {
		if (l->ind == msg_id) {
			return l;
		}
		l = (registered_evt_list_t *)l->list.next;
	}
	return NULL;
}

list_head_t *get_event_list(int msg_id)
{
	registered_evt_list_t *l = get_event_registered_list(msg_id);

	if (l)
		return &l->lh;
	return NULL;
}

static void send_event_callback(void *item, void *param)
{
	struct cfw_message *msg = (struct cfw_message *)param;
	indication_list_t *ind = (indication_list_t *)item;
	struct cfw_message *m = cfw_clone_message(msg);

	if (m != NULL) {
		CFW_MESSAGE_DST(m) = ind->conn_handle->client_port;
		cfw_send_message(m);
	}
}

void cfw_send_event(struct cfw_message *msg)
{
#ifdef SVC_MANAGER_DEBUG
	pr_debug(LOG_MODULE_CFW, "%s : msg:%d", __func__, CFW_MESSAGE_ID(msg));
#endif
	list_head_t *list = get_event_list(CFW_MESSAGE_ID(msg));
	if (list != NULL) {
		list_foreach(list, send_event_callback, msg);
	}
}

static int unregister_events_cb(void *element, void *param)
{
	indication_list_t *e = (indication_list_t *)element;

	if (e->conn_handle == param) {
		bfree(e);
		return 1;
	}
	return 0;
}

void _cfw_unregister_event(conn_handle_t *h)
{
	registered_evt_list_t *l =
		(registered_evt_list_t *)registered_evt_list.head;

	while (l) {
		list_foreach_del(&l->lh, unregister_events_cb, h);
		l = (registered_evt_list_t *)l->list.next;
	}
}

static bool check_duplicate_handle_cb(list_t *element, void *param)
{
	if (param == ((indication_list_t *)element)->conn_handle) {
		return true;
	}
	return false;
}

void _cfw_register_event(conn_handle_t *h, int msg_id)
{
#ifdef SVC_MANAGER_DEBUG
	pr_debug(LOG_MODULE_CFW, "%s : msg:%d port %d h:%p", __func__, msg_id,
		 h->client_port,
		 h);
#endif
	registered_evt_list_t *ind =
		(registered_evt_list_t *)get_event_registered_list(msg_id);

	if (ind == NULL) {
		ind = (registered_evt_list_t *)balloc(sizeof(*ind), NULL);
		ind->ind = msg_id;
		list_init(&ind->lh);
		list_add(&registered_evt_list, &ind->list);
	}

	if (!list_find_first(&ind->lh, check_duplicate_handle_cb, h)) {
		indication_list_t *e = (indication_list_t *)balloc(sizeof(*e),
								   NULL);
		e->conn_handle = h;
		list_add(&ind->lh, (list_t *)e);
	}
}

service_t *cfw_get_service(int service_id)
{
	int index;

	index = _find_service(service_id);
	return (index == -1) ? NULL : services[index];
}

uint16_t cfw_get_service_port(int service_id)
{
	service_t *svc = cfw_get_service(service_id);

	if (svc != NULL) {
		return svc->port_id;
	}
	return -1;
}

uint16_t notrace cfw_get_service_mgr_port_id()
{
	return service_mgr_port_id;
}

void cfw_service_mgr_init(void *queue)
{
	/* Set the Synchronous IPC handler, used for communication with
	 * service manager proxies */
	ipc_sync_set_user_callback(_cfw_handle_ipc_sync_request);

	/* Install a hook called before BSP shutdown, giving a chance to shutdown
	 * services in a clean way. */
	pm_register_shutdown_hook(cfw_services_shutdown);

	uint16_t port_id = port_alloc(queue);
	port_set_handler(port_id, (void (*)(struct message *,
					    void *))internal_handle_message,
			 NULL);
	service_mgr_port_id = port_id;
#ifdef SVC_MANAGER_DEBUG
	pr_debug(LOG_MODULE_CFW, "%s queue: %p", __func__, queue);
#endif
}

void _add_service(service_t *svc)
{
	int i;

	for (i = 0; i < MAX_SERVICES; i++) {
		if (services[i] == NULL) {
			services[i] = svc;
			registered_service_count++;
			return;
		}
	}
	panic(PANIC_NO_SVC_SLOT); /* No more free slots. */
}

int _cfw_register_service(service_t *svc)
{
	pr_debug(LOG_MODULE_CFW, "%s: %p id:%d port: %d\n", __func__, svc,
		 svc->service_id,
		 svc->port_id);
	int flags = irq_lock();
	if (_find_service(svc->service_id) != -1) {
		irq_unlock(flags);
		pr_error(LOG_MODULE_CFW,
			 "Error: service %d already registered\n",
			 svc->service_id);
		return -1;
	}

	_add_service(svc);

	irq_unlock(flags);
	struct cfw_message *svc_avail = (struct cfw_message *)message_alloc(
		sizeof(*svc_avail), NULL);
	CFW_MESSAGE_ID(svc_avail) = MSG_ID_NOTIFY_SERVICE_AVAIL;
	CFW_MESSAGE_SRC(svc_avail) = service_mgr_port_id;
	CFW_MESSAGE_DST(svc_avail) = service_mgr_port_id;
	CFW_MESSAGE_PRIV(svc_avail) = (void *)svc;
	cfw_send_message(svc_avail);

	return 0;
}

int _cfw_unregister_service(service_t *svc)
{
	int index;

#ifdef SVC_MANAGER_DEBUG
	pr_debug(LOG_MODULE_CFW, "%s", __func__);
#endif
	if ((index = _find_service(svc->service_id)) == -1) {
		pr_error(LOG_MODULE_CFW, "Error: service %d was not registered",
			 svc->service_id);
		return -1;
	}
	registered_service_count--;
	services[index] = NULL;
	return 0;
}

/**
 * \deprecated
 * Only accepted usage is tests
 */
int cfw_service_registered(int service_id)
{
	return _find_service(service_id) != -1;
}
