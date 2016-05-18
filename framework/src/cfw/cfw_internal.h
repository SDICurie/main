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

#ifndef __CFW_INTERNAL_H__
#define __CFW_INTERNAL_H__

#include "infra/log.h"
#include "cfw/cfw.h"
#include "cfw/cfw_service.h"
#include "infra/ipc_requests.h"

DEFINE_LOG_MODULE(LOG_MODULE_CFW, " CFW")

/*
 * Defines the messages identifiers passed in the IPC layer when doing
 * synchronous IPC requests between service managers (proxy and master).
 */
/**
 * Registers a service.
 * This request is always flowing from a slave to the master.
 */
#define IPC_REQUEST_REGISTER_SERVICE   0x11

/**
 * Unregisters a service.
 */
#define IPC_REQUEST_DEREGISTER_SERVICE 0x12

/**
 * Registers a Service Manager Proxy to the Service Manager.
 * This request always flow from a slave to the master.
 */
#define IPC_REQUEST_REGISTER_PROXY     0x14


/** Internal REQ messages used by the service manager */
enum {
	/** Open service message request */
	MSG_ID_CFW_OPEN_SERVICE_REQ = MSG_ID_CFW_RSP_LAST,
	/** Register for service availability event message request*/
	MSG_ID_CFW_REGISTER_SVC_AVAIL_EVT_REQ,
	/** Register for service events message request */
	MSG_ID_CFW_REGISTER_EVT_REQ,
	/** Close connection to a service message request*/
	MSG_ID_CFW_CLOSE_SERVICE_REQ,
	/** Shutdown service request */
	MSG_ID_CFW_SERVICE_SHUTDOWN_REQ,
	/** Last CFW request message ID */
	MSG_ID_CFW_REQ_LAST
};

typedef struct {
	list_head_t helper_list;
	handle_msg_cb_t handle_msg;
	void *data;
	uint16_t client_port_id;
} _cfw_client_t;

/**
 * \struct cfw_open_conn_req_msg_t
 * request message sent by cfw_open_connection() API.
 */
typedef struct {
	/** common message header */
	struct cfw_message header;
	/** service to open connection to */
	int service_id;
	/** client side service connection */
	cfw_service_conn_t *service_conn;
	/** client side cpu id, required for remote node services */
	uint8_t client_cpu_id;
} cfw_open_conn_req_msg_t;

/**
 * \struct cfw_close_conn_req_msg_t
 * request message sent by cfw_close_connection() API.
 */
typedef struct {
	/** common message header */
	struct cfw_message header;
	/** service id to close */
	int service_id;
	/** service to open connection to */
	void *inst;
} cfw_close_conn_req_msg_t;


typedef struct {
	/** common message header */
	struct cfw_message header;
	/** indication message identifier.
	 * all subsequent indication with this identifier will be sent
	 * to the src port of this request message.
	 */
	int evt;
} cfw_register_evt_req_msg_t;

typedef struct {
	struct cfw_message header;
	int service_id;
} cfw_register_svc_avail_req_msg_t;

/* Services list reference */
extern service_t *services[];

/**
 * Initialize the service manager.
 *
 * @param queue main queue on which the CFW service will be initialized.
 */
void cfw_service_mgr_init(void *queue);

/**
 * Initialize the service manager proxy.
 *
 * @param queue main queue on which the CFW service will be initialized.
 * @param svc_mgr_port the port number where to reach the service manager master
 */
void cfw_service_mgr_init_proxy(T_QUEUE queue, uint16_t svc_mgr_port);

/**
 * Initialize all services registered using the CFW_DECLARE_SERVICE macro.
 *
 * @param default_queue queue to use when no specific queue was assigned to the
 * service (using the cfw_set_queue_for_service() functions).
 */
void cfw_init_registered_services(T_QUEUE default_queue);

int _cfw_register_service(service_t *svc);

int _cfw_unregister_service(service_t *svc);

/**
 * Register events requested by a client.
 *
 * This is called when a client connects to a service.
 *
 * @param handle handle of the connection of the service.
 * @param msgId the indication message id that we want to receive.
 * @param handle the connection handle for the client to connect
 *          to events.
 */
void _cfw_register_event(conn_handle_t *handle, int msgId);

/**
 * Unregister events registered by a client.
 *
 * This is called when a client disconnects from a service.
 *
 * @param h the connection handle for the client to disconnect
 *          from events.
 */
void _cfw_unregister_event(conn_handle_t *h);

/**
 * Gets the service matching the specified ID.
 *
 * @param service_id ID of the desired service.
 *
 * @return the pointer to the matching service or NULL.
 */
service_t *cfw_get_service(int service_id);

#endif /* __CFW_INTERNAL_H__ */
