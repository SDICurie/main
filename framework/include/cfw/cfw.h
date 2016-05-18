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

#ifndef _CFW_H_
#define _CFW_H_

#include <stdlib.h>

#include "os/os.h"
#include "util/compiler.h"
#include "infra/message.h"

/**
 * @defgroup cfw Component Framework
 * @{
 */

/**
 * @defgroup cfw_user_api CFW User API
 *
 * Defines the API necessary for initializing CFW and implementing clients.
 *
 * CFW initialization:
 * - \ref cfw_init
 *
 * Client implementation:
 * - 1. Initialize the client with \ref cfw_client_init
 * - 2. Optionaly wait for service availability with \ref cfw_register_svc_available
 * - 3. Open a connection to a service with \ref cfw_open_service_conn
 * - 4. Register for events from the opened service with \ref cfw_register_events
 * - Steps 2 & 3 can be done at a glance with \ref cfw_open_service_helper
 * - Steps 2 & 3 & 4 can be done at a glance with \ref cfw_open_service_helper_evt
 *
 * @ingroup cfw
 * @{
 */

/** Maximum number of services managed by the service manager */
#define MAX_SERVICES    16

/**
 * Initialize the CFW and initialize registered services.
 *
 * This is the first function (excepted for cfw_set_queue_for_service()) to call
 * when the CFW is to be used. It has to be called on each cores where the CFW
 * is used.
 *
 * The implementation of this function is system-dependent and may need to be
 * provided externally. For example on a Quark SE core it will deal with
 * muti-core initialization of the CFW slave and proxy.
 *
 * A default implentation is provided as weak symbol so that it can be
 * overrided by custom implementations.
 *
 * A queue is provided at initialization time. This queue will be used by the
 * service manager and all the services that do not use a custom queue.
 *
 * @param queue message queue used by the service manager and the services
 */
void cfw_init(T_QUEUE queue);

/**
 * Set a custom queue for a service.
 *
 * This function is the only one which must be called before cfw_init().
 * This overrides the default queue assigned to the service manager on cfw_init.
 *
 * @param service_id service ID.
 * @param queue message queue to use. It must be valid during the service lifetime.
 */
void cfw_set_queue_for_service(int service_id, T_QUEUE queue);

/**
 * Start the CFW loop.
 *
 * This function start processing events (messages) incoming on the passed
 * queue. It has to be called once all services and clients using this queue
 * are initialized. It never returns.
 *
 * @param queue queue on which messages are consumed.
 */
__noreturn void cfw_loop(T_QUEUE queue);

/**
 * A message used by the CFW.
 */
struct cfw_message {
	struct message m;

	/** The belonging connection of the message */
	void *conn;
	/** The per-call callback passed with the request. */
	void (*cb)(struct cfw_message *);
	/** The private data passed with the request. */
	void *priv;
};

#define CFW_MESSAGE_ID(msg)     MESSAGE_ID(&(msg)->m)
#define CFW_MESSAGE_SRC(msg)    MESSAGE_SRC(&(msg)->m)
#define CFW_MESSAGE_DST(msg)    MESSAGE_DST(&(msg)->m)
#define CFW_MESSAGE_LEN(msg)    MESSAGE_LEN(&(msg)->m)
#define CFW_MESSAGE_TYPE(msg)   MESSAGE_TYPE(&(msg)->m)
#define CFW_MESSAGE_CONN(msg)   (msg)->conn
#define CFW_MESSAGE_CB(msg)     (msg)->cb
#define CFW_MESSAGE_PRIV(msg)   (msg)->priv
#define CFW_MESSAGE_HEADER(msg) (&(msg)->m)

/**
 * Free a given message.
 *
 * This function will take care to send the freeing request to
 * the core that allocated the message. (based on the source port)
 *
 * @param msg message to be freed.
 */
void cfw_msg_free(struct cfw_message *msg);

/**
 * Debug helper to print a message on the log console
 */
void cfw_dump_message(struct cfw_message *msg);

/**
 * Base messages for the service manager API.
 */
enum {
	/** Indication of service availability message */
	MSG_ID_CFW_SVC_AVAIL_EVT = 0x100,
	/** Open service message response*/
	MSG_ID_CFW_OPEN_SERVICE_RSP,
	/** Register for service availability event message response */
	MSG_ID_CFW_REGISTER_SVC_AVAIL_EVT_RSP,
	/** Register for service events message response */
	MSG_ID_CFW_REGISTER_EVT_RSP,
	/** Close connection to a service message response*/
	MSG_ID_CFW_CLOSE_SERVICE_RSP,
	/** Service shutdown response */
	MSG_ID_CFW_SERVICE_SHUTDOWN_RSP,
	/** Last CFW response message ID */
	MSG_ID_CFW_RSP_LAST
};

/**
 * A CFW client.
 *
 * This type is used to communicate with the service manager.
 */
typedef void cfw_client_t;

/**
 * A connection between a CFW client and a service.
 *
 * Such a connection needs to be opened before a client can use a service.
 * It then has to be passed to each calls to a service API.
 */
typedef struct {
	/** Port to reach the service. */
	uint16_t port;
	/** Service id */
	int service_id;
	/** The client part of the connection */
	cfw_client_t *client;
	/**
	 * The server part of the connection.
	 * Passed in the conn field of struct cfw_message for request messages
	 */
	void *server_handle;
} cfw_service_conn_t;

/**
 * Message handler definition.
 */
typedef void (*handle_msg_cb_t)(struct cfw_message *, void *);

/**
 * Creates a client for the component framework.
 *
 * A client can later be connected to a service by creating a cfw_service_conn_t
 * between them.
 *
 * Implementation is different in the master and the slave contexts.
 * The master context will be pseudo-synchronous, while the slave
 * implementation will actually pass a message to the master context
 * in order to register a new client.
 *
 * @param queue message queue used by the client.
 * @param cb callback that will be called for each message reception.
 * @param param opaque data passed back along with the message to the callback.
 */
cfw_client_t *cfw_client_init(void *queue, handle_msg_cb_t cb, void *param);

/**
 * Response message to the cfw_open_service_conn() API.
 */
typedef struct {
	/** Common response message header */
	struct cfw_message header;
	/** response status code.*/
	int status;
	/** Port to reach this service */
	uint16_t port;
	/** cpu_id of service */
	uint8_t cpu_id;
	/** Service handle for accessing the service */
	void *svc_server_handle;
	/** Client side service connection */
	cfw_service_conn_t *service_conn;
} cfw_open_conn_rsp_msg_t;

/**
 * Opens a connection to the specified service.
 *
 * The connection instance is returned in the OPEN_SERVICE response message.
 *
 * @msc
 * Client,"CFW API","Service Manager";
 *
 * Client=>"CFW API" [label="cfw_open_connection"];
 * Client<<"CFW API" ;
 * Client<-"Service Manager" [label="CFW_OPEN_SERVICE_RSP", URL="\ref cfw_open_conn_rsp_msg_t", ID="2"];
 *
 * @endmsc
 *
 * @param client CFW client as returned by \ref cfw_client_init.
 * @param service_id unique service identifier.
 * @param priv opaque data passed back with the OPEN_SERVICE_RSP message in the \ref cfw_open_conn_rsp_msg_t.
 */
void cfw_open_service_conn(cfw_client_t *client, int service_id, void *priv);

/**
 * Response message to the cfw_close_service_conn() API.
 */
typedef struct {
	/** common response message header */
	struct cfw_message header;
	/** response status code.*/
	int status;
} cfw_close_conn_rsp_msg_t;

/**
 * Closes a connection to a service.
 *
 * @param conn connection handle as returned in the \ref cfw_open_conn_rsp_msg_t.
 * @param priv opaque data passed back in the CLOSE_SERVICE_RSP message.
 */
void cfw_close_service_conn(const cfw_service_conn_t *conn, void *priv);

/**
 * Response message to the cfw_register_events() API.
 */
typedef struct {
	/** common response message header */
	struct cfw_message header;
} cfw_register_evt_rsp_msg_t;

/**
 * Register to service events.
 *
 * @param conn connection handle as returned in the \ref cfw_open_conn_rsp_msg_t.
 * @param msg_ids array of event message ids to register to.
 * @param size size of msg_ids array.
 * @param priv opaque data passed back with the REGISTER_EVT_RSP in the \ref cfw_register_evt_rsp_msg_t.
 */
void cfw_register_events(const cfw_service_conn_t *conn, int *msg_ids, int size,
			 void *priv);

/**
 * Response message to the cfw_register_svc_available() API.
 */
typedef struct {
	/** common response message header */
	struct cfw_message header;
} cfw_register_svc_avail_rsp_msg_t;


/**
 * Event sent to clients that called cfw_register_svc_available() api.
 * It notifies of the availability of a service.
 */
typedef struct {
	/** common message header */
	struct cfw_message header;
	/** Service id of the newly available service. */
	int service_id;
} cfw_svc_available_evt_msg_t;

/**
 * Registers to service availability events.
 *
 * Whenever a service is registered, the clients that registered to service_available will
 * receive a cfw_svc_available_evt_msg_t message whenever a service is available.
 *
 * @param client CFW client as returned by \ref cfw_client_init.
 * @param service_id the unique service identifier.
 * @param priv opaque data sent back with the REGISTER_SVC_AVAIL_EVT_RSP message in the \ref cfw_svc_available_evt_msg_t.
 */
void cfw_register_svc_available(cfw_client_t *client, int service_id,
				void *priv);

/**
 * Helper function to handle service connection and event registration.
 *
 * General pattern for a client is to register for service availability,
 * then open the service, and then optionnaly register for service's events.
 * This helper function allows to hide this process behind one async function
 * call. The callback will be called when the service is connected.
 * The callback function will be called in the context of the client queue
 * that was passed to the cfw_client_init call.
 *
 * @param client CFW client as returned by \ref cfw_client_init.
 * @param service_id unique service identifier.
 * @param events event list we want to register to
 * @param event_count number of events in the list
 * @param cb callback function that will be called whenever the process is complete.
 * @param cb_data data passed to the cb function
 */
void cfw_open_service_helper_evt(cfw_client_t *client, uint16_t service_id,
				 int *events, int event_count, void (*cb)(
					 cfw_service_conn_t *, void
					 *),
				 void *cb_data);

/**
 * Helper function similar to cfw_open_service_helper_evt() when no event have
 * to be registered.
 *
 * @param client CFW client as returned by \ref cfw_client_init.
 * @param service_id unique service identifier.
 * @param cb callback function called when the service is opened
 * @param cb_data data passed to the callback
 */
void cfw_open_service_helper(cfw_client_t *client, uint16_t service_id,
			     void (*cb)(cfw_service_conn_t *,
					void *), void *cb_data);


/** @} */
/** @} */

#endif /* #ifndef _CFW_H_ */
