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

#ifndef __CFW_SERVICE_H_
#define __CFW_SERVICE_H_

#include "util/list.h"

#include "os/os.h"

#include "cfw/cfw.h"
#include "services/services_ids.h"

struct service;

/**
 * @defgroup cfw_service CFW Service API
 *
 * Defines the API necessary for implementing a CFW service (registering,
 * messages and events handling).
 *
 * A CFW service may be considered like a server and the entities using a
 * service may be considered like clients.
 *
 * A CFW service is uniquely identified by an Id.
 * The Ids reserved for internal services are listed in
 * `framework/include/services/services_ids.h`.
 *
 * To create a new custom service, the Id should be >= `CFW_FIRST_CUSTOM_SERVICE_ID`.
 *
 * Each service must be declared with \ref CFW_DECLARE_SERVICE. This will automatically
 * initialize the service on CFW initialization.
 *
 * Each service have to register with the service manager (@ref cfw_register_service)
 * and specify the message queue which is used to dispatch messages. This can be done
 * in the service initialization function.
 *
 * @ingroup cfw
 * @{
 */

/**
 * A connection between a client and a service.
 * This structure is internal, used only by services implementations internals
 */
typedef struct {
	/** Port to reach the client */
	uint16_t client_port;
	/** The service pointer.*/
	struct service *svc;
	/** Pointer for the service to store client-specific data. */
	void *priv_data;
	/**
	 * The client-side service connection (of type cfw_service_conn_t).
	 * It is only valid in the scope of the client
	 */
	void *client_handle;
} conn_handle_t;

/**
 * Internal definition of a service.
 */
typedef struct service {
	/** Port number assigned to this service by the service manager */
	uint16_t port_id;
	/** Identifier of the service.*/
	int service_id;
	/**
	 * This callback is called when a client connects.
	 * called in the context of the service manager.
	 */
	void (*client_connected)(conn_handle_t *);
	/**
	 * This callback is called when a client disconnects.
	 * Called in the context of the service manager.
	 */
	void (*client_disconnected)(conn_handle_t *);
	/**
	 * This callback is called when the registered event
	 * list is modified.
	 * Called in the context of the service manager.
	 */
	void (*registered_events_changed)(conn_handle_t *);
	/**
	 * This list is used to store the message which has been
	 * deferred by the service to handle concurrent requests.
	 */
	list_head_t deferred_message_list;
	/**
	 * This callback is called during the system shutdown
	 * to handle pending client requests.
	 * Called in the context of the service manager.
	 */
	void (*shutdown_request)(struct service *	svc,
				 struct cfw_message *	rply_message);
} service_t;

/* Private struct: service item registered in the dedicated .services.*
 * section */
/** @cond */
struct _cfw_registered_service {
	const char *name;
	int id;
	void (*init)(int id, T_QUEUE queue);
} _cfw_registered_service;
/** @endcond */

/**
 * Use this macro to declare a new CFW service.
 * @param name service name
 * @param id service identifier
 * @param init service initialization function
 *
 * All declared services will be initialized (by calling the init function) at
 * CFW init time in no particular order.
 *
 * Implementation note: registered services will be automatically added to a
 * dedicated section. This requires the following snippet to be added in the
 * linker script:
 * @code
 *     SECTIONS {
 *         .cfw_services_section : {
 *           . = ALIGN(8);
 *           __cfw_services_start = .;
 *           *(SORT(.cfw_services.*))
 *           __cfw_services_end = .;
 *         }
 *     }
 *     INSERT BEFORE .rodata;
 * @endcode
 */
#define CFW_DECLARE_SERVICE(name, id, init) \
	_CFW_DECLARE_SERVICE_PRESCAN(name, id, init)

/* We use an internal macro to force argument prescan if a macro was passed as
 * argument */
#define _CFW_DECLARE_SERVICE_PRESCAN(name, id, init) \
	const struct _cfw_registered_service __service_ ## name ## _ ## id \
	__section(".cfw_services." # name # id)	   \
		= { # name, id, init }

/**
 * Gets the service manager port id.
 *
 * @return service manager port id.
 */
uint16_t cfw_get_service_mgr_port_id(void);

/**
 * Gets the port id for the given service ID.
 */
uint16_t cfw_get_service_port(int);

/**
 * Allocate a cfw_message of the given size.
 *
 * @param size total message size
 * @return allocated message
 */
struct cfw_message *cfw_alloc_message(int size);

/**
 * Creates a copy of a given message.
 *
 * @param msg message to be cloned.
 * @return cloned message
 */
struct cfw_message *cfw_clone_message(struct cfw_message *msg);

/**
 * Sends a message.
 *
 * The message should be filled with the destination port,
 * source port, message identifier, etc...
 *
 * @param msg message to send.
 */
int _cfw_send_message(struct cfw_message *msg);

/**
 * This macro conveniently casts the parameter to a message header pointer.
 */
#define cfw_send_message(_msg_) _cfw_send_message((struct cfw_message *)(_msg_))

/**
 * Allocates a request message for a service.
 *
 * This will fill the needed common message fields needed to interact
 * with a service.
 *
 * @param conn connection handle
 * @param msg_id message id
 * @param msg_size total size of the message
 * @param priv opaque data that will passed back in the response message
 */
struct cfw_message *cfw_alloc_message_for_service(
	const cfw_service_conn_t *conn, int msg_id, int msg_size, void *priv);

/** Helper macro to allocate a message for a service */
#define CFW_ALLOC_FOR_SVC(t, m, h, i, e, p) \
	t * m = (t *)cfw_alloc_message_for_service(h, i, sizeof(t) + (e), p)

/**
 * Allocates a request message for a service.with a callback
 *
 * This will fill the needed common message fields needed to interact
 * with a service.with callback
 *
 * @param conn connection handle
 * @param msg_id message id
 * @param msg_size total size of the message
 * @param cb callback called upon reception of the request
 * @param priv opaque data that will passed back in the response message
 */
struct cfw_message *cfw_alloc_msg_for_service_cb(
	const cfw_service_conn_t *conn, int msg_id, int msg_size, void (*cb)(
		struct cfw_message *), void *priv);

/**
 * Allocates a response message for a service client.
 *
 * This will fill the needed common message fields needed to interact
 * with a client.
 *
 * @param conn connection handle
 * @param msg_id message id
 * @param msg_size total size of the message
 * @param priv opaque data that will passed back in the response message
 */
struct cfw_message *cfw_alloc_rsp_message_for_client(cfw_service_conn_t *conn,
						     int msg_id, int msg_size,
						     void *priv);

/**
 * Allocate and build a response message for a specific request.
 *
 * @param req request message.
 * @param msg_id id of the response message.
 * @param size size of the response message.
 *
 * @return allocated and initialized response message.
 */
struct cfw_message *cfw_alloc_rsp_msg(const struct cfw_message *req, int msg_id,
				      int size);

/**
 * Allocate an event message.
 *
 * @param svc descriptor of the service allocating the event message.
 * @param msg_id message identifier of the event.
 * @param size size of the message to be allocated.
 *
 * @return allocated event message.
 */
struct cfw_message *cfw_alloc_evt_msg(service_t *svc, int msg_id, int size);

/**
 * Register a service to the system.
 *
 * Registering a service makes it available to all other services
 * and applications.
 *
 * @param queue message queue on which the service is to be run.
 * @param service descriptor of the service to register.
 * @param handle_message message handler of the service.
 * @param param opaque data passed to message handler.
 *
 * @return 0 if request succeeded; and !=0 otherwise.
 */
int cfw_register_service(T_QUEUE queue, service_t *service,
			 handle_msg_cb_t handle_message,
			 void *param);

/**
 * Un-register a service from the system.
 *
 * All other services and applications cannot see it anymore.
 *
 * @param service descriptor of the service to unregister.
 *
 * @return 0 if request succeeded; and !=0 otherwise.
 */
int cfw_unregister_service(service_t *service);

/**
 * Send an indication message to the registered clients.
 *
 * @param msg indication message to send.
 */
void cfw_send_event(struct cfw_message *msg);

/**
 * Defer processing of a message.
 * This allows a service to mark a message as to be processed later. The first
 * deferred message will be posted on the service's queue head when
 * cfw_defer_complete() will be called.
 *
 * @param svc service that defers the message.
 * @param msg message to be deferred.
 *
 * @return 0 if no error occured.
 */
int cfw_defer_message(service_t *svc, struct cfw_message *msg);

/**
 * Continue service message processing.
 * This will get the first message from the service's deferred message list and
 * post it to the service's queue for processing.
 * This function will panic if something goes wrong.
 *
 * @param svc service that completes processing.
 *
 */
int cfw_defer_complete(service_t *svc);

/**
 * Log an error for default handle message.
 *
 * @param module ID of the module related to this message
 * @param msg_id message identifier
 */
void cfw_print_default_handle_error_msg(const char *module, uint16_t msg_id);

/** @} */

#endif /* __CFW_SERVICE_H_ */
