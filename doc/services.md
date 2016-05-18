@defgroup services Services
@{
Component framework services definition.

Headers are available in <tt>framework/include/services</tt>.<br>
Sources are located in <tt>framework/src/services</tt>.

Each service can be enabled/disabled through _Services_ configuration menu.

For each service, there are at least 2 options:
- SERVICE_xxx_IMPL : this option concerns the service implementation.
  It should be enabled on the node that includes the service.
- SERVICES_xxx : this option concerns the service API.
  It should be enabled on any node that includes a client to the service.

For more information on:
- Component Framework, see \ref cfw.
- Service API, see \ref cfw_service.
- Client API, see \ref cfw_user_api.
@}
