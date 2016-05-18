@addtogroup cfw
@{

# Component Framework (CFW)

### Terminology

- Node: a node is an instantiation of the component framework. It corresponds to
a CPU of the hardware platform.
- Master: the master node of the platform, it instantiates the service manager.
- Slave: the slaves nodes of the platform, each one instantiates a service
manager proxy.

## Introduction

CFW is the core of the Curie&trade; BSP software stack.

- CFW is based on a service/client model.
- CFW can support multiple services.
- Each service can support multiple clients. Implementation should be asynchronous.
- All exchanges are message-based.
- Messages are exchanged through queues. Each queue must be managed by a dedicated task.
- Any service or client can run virtually on any CPU of the hardware platform.

See \ref messaging for more information on communication between components.

The main component is the Service Manager. It includes the 'default' main loop handler
that will wait for messages to come in a queue and call the appropriate message handler.

CFW must be initialized by calling \ref cfw_init on any core where CFW is used.

    +---------------------+
    |     Client          |
    +---------------------+

    +---------------------+   +-----------------+
    |     Services        |   | Service Manager |
    +---------------------+   +-----------------+

    +---------------------+   +-----------------+
    |    Drivers          |   |    INFRA        |
    +---------------------+   +-----------------+


## Service Manager / Service Manager Proxy

The Service Manager is the core entity of the component framework, it is
responsible for centralizing all services on the platform.

All services should register to the service manager, and all clients that need
to open services should call the Service Manager in order to open the specified
service.

When a service is not in the same node as the Service manager, the service
manager APIs are implemented by a Service Manager Proxy that relays the requests
to the actual Service Manager.


### Service Open Connection

\msc
 Client,API,SM,Service;
 |||;
 Client=>API [label="cfw_register_svc_available", URL="\ref cfw_register_svc_available"];
 API->SM [label="CFW_REGISTER_SVC_AVAILABLE REQ", URL="cfw_register_svc_avail_req_msg_t"];
 Client<<API ;
 Client<=SM [label="CFW_REGISTER_SVC_AVAILABLE RSP", URL="\ref cfw_register_svc_avail_rsp_msg_t"];
 Service=>SM [label="register_service()", URL="\ref cfw_register_service"];
 SM=>Client [label="CFW_SVC_AVAIL_EVT", URL="\ref cfw_svc_available_evt_msg_t"];
 Client=>API [label="cfw_open_service_conn", URL="\ref cfw_open_service_conn"];
 API->SM [label="CFW_OPEN_SERVICE REQ", URL="cfw_open_conn_req_msg_t"];
 SM->Service [label="client_connected"];
 Client<<API ;
 Client<-SM [label="CFW_OPEN_SERVICE RSP", URL="\ref cfw_open_conn_rsp_msg_t"];
\endmsc



### Remote Service Open Connection


\msc
 Client,API,SM,SMProxy,Service;
 |||;
 Client=>API [label="cfw_register_svc_available", URL="\ref cfw_register_svc_available"];
 API->SM [label="CFW_REGISTER_SVC_AVAILABLE", URL="cfw_register_svc_avail_req_msg_t"];
 Client<<API;
 Client<=SM [label="CFW_REGISTER_SVC_AVAILABLE RSP", URL="\ref cfw_register_svc_avail_rsp_msg_t"];
 Service=>SMProxy [label="register_service()", URL="\ref cfw_register_service"];
 SMProxy=>SM [label="CFW_REGISTER_SVC", URL=""];
 SM=>Client [label="CFW_SVC_AVAIL_EVT", URL="\ref cfw_svc_available_evt_msg_t"];
 Client=>API [label="cfw_open_service_conn", URL="\ref cfw_open_service_conn"];
 API->SM [label="CFW_OPEN_SERVICE REQ", URL="cfw_open_conn_req_msg_t"];
 Client<<API;
 SM->SMProxy [label="CFW_OPEN_SERVICE REQ"];
 SMProxy->Service [label="client_connected"];
 Client<-SMProxy [label="CFW_OPEN_SERVICE RSP", URL="\ref cfw_open_conn_rsp_msg_t"];
\endmsc


### Register for Local Service events

\msc
 Client,API,SM,Service;
 |||;
 Client box Service [label="Client is connected to service"];
 Client=>API [label="cfw_register_events", URL="\ref cfw_register_events"];
 API->SM [label="CFW_REGISTER_EVENTS REQ", URL="cfw_register_evt_req_msg_t"];
 SM note SM [label="Service is local, store registration locally"];
 Client<<API;
 Client<-SM [label="CFW_REGISTER_EVENTS RSP", URL="\ref cfw_register_evt_rsp_msg_t"];
\endmsc


### Register for Distant Service events

\msc
 Client,API,SM,SMProxy,Service;
 |||;
 Client box Service [label="Client is connected to service"];
 Client=>API [label="cfw_register_events", URL="\ref cfw_register_events"];
 API->SM [label="CFW_REGISTER_EVENTS REQ", URL="cfw_register_evt_req_msg_t"];
 Client<<API;
 SM note SM [label="Service is distant, this indication not registered locally yet, fw to proxy so SM gets the indication from service. Other local registrations will be stored locally"];
 SM->SMProxy [label="CFW_REGISTER_EVENTS REQ", URL="cfw_register_evt_req_msg_t"];
 Client<-SMProxy [label="CFW_REGISTER_EVENTS RSP", URL="\ref cfw_register_evt_rsp_msg_t"];
\endmsc




## Service

A _service_ is a software component that offers a service (a set of features)
to applications. A service must be multi-client, and should work on an
asynchronous manner.

The service is composed of :
- an _API_ whose purpose is to generate the request messages
- an _implementation_ to
  + handle the request messages
  + send the response message when available.

Services also allow defining some events that can be broadcasted to registered
clients/applications.

Service implementation example:

\code

static void my_service_client_connected(conn_handle_t * conn)
{
	// Do whatever needed for client management.
	conn->priv_data = state;
}

static void my_service_client_disconnected(conn_handle_t * conn)
{
	// Free any per-client state data
}

static void my_service_handle_message(struct cfw_message * msg, void *param)
{
	switch(CFW_MESSAGE_ID(msg)) {
		case MSG_ID_MY_SERVICE_REQUEST_REQ: {
			my_service_request_req_msg_t * req = (my_service_request_req_msg_t *) msg;
			my_service_request_rsp_msg_t * rsp = cfw_alloc_rsp_msg(req, MSG_ID_MY_SERVICE_REQUEST_RSP, sizeof(*rsp));
			// Fill rsp fields
			cfw_send_message(rsp);
		}
		break;
	}
	message_free(msg);
}

static service_t my_service = {
	.service_id = MY_SERVICE_ID,
	.client_connected = my_service_client_connected,
	.client_disconnected = my_service_client_disconnected,
};

void my_service_init(QUEUE_T queue)
{
	cfw_register_service(queue, &my_service, my_service_handle_message, NULL);
}
\endcode

## Client

The client/application is the entity that implements the features of the device.
It generally uses services, infra and sometimes drivers in order to implement
its features.
As the services and drivers all implements asynchronous APIs, the
client/application is a message/event handler.

Client implementation example:

\code

static cfw_client_t * client = NULL;
static cfw_service_conn_t * my_service_conn = NULL;

static void client_handle_cb(struct cfw_message * msg, void * parm)
{
	switch(CFW_MESSAGE_ID(msg)) {
	case MSG_ID_CFW_SVC_AVAIL_EVT_RSP: {
		cfw_svc_available_evt_msg_t * evt = (cfw_svc_available_evt_msg_t*) msg;
		if (msg->service_id == MY_SERVICE_ID) {
			cfw_open_service_conn(client, MY_SERVICE_ID, (void*)MY_SERVICE_ID);
		}
	}
	break;
	case MSG_ID_CFW_OPEN_SERVICE_RSP: {
		cfw_open_conn_rsp_msg_t * rsp = (cfw_open_conn_rsp_msg_t*) msg;
		if ((int) CFW_MESSAGE_PRIV(msg) == MY_SERVICE_ID) {
			// Service connected, start using the service
			my_service_conn = rsp->service_conn;
			my_service_request(my_service_conn, my_service_param, NULL);
		}
	}
	break;
	case MSG_ID_MY_SERVICE_REQUEST_RSP: {
		my_service_request_rsp_msg_t * rsp = (my_service_request_rsp_msg_t *) msg;
		// Do what's needed with rsp
	}
	break;
	}
	message_free(msg);
}

void client_init(QUEUE_T queue)
{
	client = cfw_client_init(queue, client_message_cb, NULL);
	cfw_register_svc_available(client, MY_SERVICE_ID, NULL);
}

\endcode

Alternatively, one can use the cfw_open_service_helper() method as follows:

\code

static cfw_client_t * client = NULL;
static cfw_service_conn_t * my_service_conn = NULL;

static void client_handle_cb(struct cfw_message * msg, void * parm)
{
	switch(CFW_MESSAGE_ID(msg)) {
	case MSG_ID_MY_SERVICE_REQUEST_RSP: {
		my_service_request_rsp_msg_t * rsp = (my_service_request_rsp_msg_t *) msg;
		// Do what's needed with rsp
	}
	break;
	}
	message_free(msg);
}

void on_service_connected(cfw_service_conn_t * conn, void *param)
{
	my_service_conn = conn;
	my_service_request(my_service_conn, my_service_param, NULL);
}

void client_init(QUEUE_T queue)
{
	client = cfw_client_init(queue, client_message_cb, NULL);
	cfw_open_service_helper(client, MY_SERVICE_ID, on_service_connected, NULL);
}

\endcode


## Limited resources
As the system resources are small in the Quark SE 1 SoC (System on Chip),
the number of tasks / fibers should be limited to the minimum.

Sharing a queue for all the logical applications of the platform is a way to
minimize the resource usage.
This means that all applications are running in the same thread,
by using the same queue for CFW client definition.
All events will be multiplexed to the same queue.

If an application needs to implement heavy processing of data, then it could
make sense to create a dedicated task for this processing with a low priority
so it runs when no other service / applications needs the CPU.

@}
@}

