@addtogroup gpio_service
@{

The @ref gpio_service allows the user to control a GPIO and get related events
without having to deal directly with the driver, which may not be compiled on
the same processor.
For details on driver, see @ref common_driver_gpio.

Three kinds of GPIO are reachable:
 - The ARC specific GPIO (two blocks of 8 pin), reachable via the service @b SS_GPIO_SERVICE
 - The common GPIO for QRK and ARC (pin [0..31]), reachable via the service @b SOC_GPIO_SERVICE
 - The AON GPIO (pin [0..5]), reachable via the service @b AON_GPIO_SERVICE

For more detail see hardware documentation.


### How to use gpio pin

According to the GPIO you want to reach, you may first open a connection
(@ref cfw_open_service_helper) to the right GPIO service (SS_GPIO_SERVICE,
SOC_GPIO_SERVICE or AON_GPIO_SERVICE).

You are now a client of GPIO service.

Next you can configure the pin in input mode or output mode using @ref gpio_service_configure :
 - output mode: you can change the pin state using @ref gpio_service_set_state.
 - input mode: you can retrieve the pin state using @ref gpio_service_get_state.
 - input mode: you can retrieve the gpio bank state using @ref gpio_service_get_bank_state.

You can "listen" the pin using @ref gpio_service_listen ; this way, when the gpio state changes,
the GPIO service automatically sends a message to the client.

### Usage example

This example shows a basic usage of the gpio service: create a client, register
it to the service, then configure the gpio as output and toggle it.
In this example, the gpio_ss[12] will be used.

First, create a client for the gpio service and prepare all the needed
callbacks.

\code

#define GPIO_TO_TOGGLE 12
/* gpio service connection handler */
static cfw_client_t * gpio_msg_client = NULL;
/* gpio service handler */
cfw_service_conn_t *gpio_svc_client_conn;

/* Called when gpio_service has an event to send to the client, or when a call
 * to the gpio_service has been handled.
 * As this is a simple GPIO output controller, just print a message. */
static void my_gpio_service_handler(struct cfw_message *msg, void *param)
{
	pr_info(LOG_MODULE_MAIN, "%s: There is nothing to do msg_id: %x", __func__, CFW_MESSAGE_ID(msg));
	cfw_msg_free(msg);
}

/* Called when the client is well registered to the gpio service. Here, the
 * gpio is set to output mode. */
static void my_service_connection_cb(cfw_service_conn_t * handle, void *param)
{
	/* Save the handle, needed to communicate with the service */
	gpio_svc_client_conn = handle;

	/* Configure GPIO output */
	gpio_service_configure(gpio_svc_client_conn, GPIO_TO_TOGGLE, 1 /* output mode */, NULL);
}

/* Initialize the client and register to the service */
static void my_gpio_svc_init()
{
	/* Create a client listenning on the service queue */
	gpio_msg_client = cfw_client_init(
			queue, /* The queue where messages are posted, most of the time the
					* one provided by 'system_setup' */
			my_gpio_service_handler, NULL);

	cfw_open_service_helper(gpio_msg_client,
			SS_GPIO_SERVICE_ID,
			my_service_connection_cb, NULL);
}

\endcode

Now that initialization is done, it is possible in the code to toggle the gpio:

\code
	gpio_service_set_state (gpio_svc_client_conn,
		    		my_pin,
		    		0,
		    		NULL);
\endcode
@}
