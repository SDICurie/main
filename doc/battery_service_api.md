@addtogroup battery_service
@{

## Features

The battery service has multiple functionalities:
 - determine battery voltage
 - determine battery temperature
 - determine battery capacity level ([%] - called State Of Charge)
 - determine charging source
 - determine if charger is connected or not
 - determine if battery is charging

Note: Battery Temperature and Voltage are read periodically (interval time depends on the project).

Also, Event messages are sent to notify system when following events appear:
 - MSG_ID_BATTERY_SERVICE_LEVEL_UPDATED_EVT when Battery level changed
 - MSG_ID_BATTERY_SERVICE_LEVEL_LOW_EVT when Battery level reached Low Level Alarm (configurable)
 - MSG_ID_BATTERY_SERVICE_LEVEL_CRITICAL_EVT when Battery level reached Critical Level Alarm (configurable)
 - MSG_ID_BATTERY_SERVICE_LEVEL_SHUTDOWN_EVT when Battery level reached Shutdown Level Alarm - A shutdown is recommended.

By default this event is sent when battery voltage is below 3200 mV (defined as FG_DFLT_SHUTDOWN_ALARM_THRESHOLD in adc_fuel_gauge_api.c)
 - MSG_ID_BATTERY_SERVICE_FULLY_CHARGED_EVT when Battery is fully charged
 - MSG_ID_BATTERY_SERVICE_CHARGER_CONNECTED_EVT when Charger is connected
 - MSG_ID_BATTERY_SERVICE_CHARGER_DISCONNECTED_EVT when Charger is disconnected

## Architecture

To be able to export those information, Battery Service is based on the following architecture

@image html battery_service_architecture.png "Battery Service block description"

## Developer Manual

### 1- Initialization

To interact with every service implemented within the platform, the service has to be opened using cfw_open_service_conn with the corresponding ID.

The ID of the Battery service  is BATTERY_SERVICE_ID.

\code
cfw_client_t *client = cfw_client_init(queue, your_handle_function, NULL);
cfw_open_service_conn(client, BATTERY_SERVICE_ID, NULL);
\endcode

### 2- Attach to an Event

In order to catch an event sent by Battery Service, first you have to be registered

This could be done like that :

\code
int events[] = {MSG_ID_BATTERY_SERVICE_LEVEL_UPDATED_EVT, MSG_ID_BATTERY_SERVICE_CHARGER_CONNECTED_EVT};
cfw_register_events(client, events, sizeof(event)/sizeof(int), CFW_MESSAGE_PRIV(msg));
\endcode

### 3- Receive an Event or Response message

To receive a message from Battery Service, within your_handle_function, you have to parse the message.
Here is an example:

\code
static void your_handle_function(struct cfw_message * msg, void *param)
{
    switch(CFW_MESSAGE_ID(msg)) {
    case MSG_ID_BATTERY_SERVICE_GET_BATTERY_INFO_RSP:
	switch(((battery_service_info_request_rsp_msg_t *)msg)->batt_info_id) {
	case BS_CMD_BATT_VBATT:
       	 	bat_status = ((battery_service_info_request_rsp_msg_t *) msg)->status;
        	if (BATTERY_STATUS_SUCCESS == bat_status) {
           	 pr_info(LOG_MODULE_MAIN, "Battery Voltage:\t%d[mV]\n",((battery_service_info_request_rsp_msg_t *)msg)->battery_voltage.bat_vol);
       		 }
        	break;
   	 case BATTERY_DATA_CHARGER_STATUS:
       	 	bat_status = ((battery_service_info_request_rsp_msg_t *) msg)->status;
        	if (BATTERY_STATUS_SUCCESS == bat_status) {
        	pr_info(LOG_MODULE_MAIN,"the charger have been connected");
		}
        	break;
    	default:
        	break;
	}

    default:
        break;
    }
    cfw_msg_free(msg);
}
\endcode

@anchor how_to_customize_battery_properties
### 4- Battery Lookup Tables

Several Battery Lookup Tables are located in battery_LUT directory (array dflt_lookup_tables).
The table to use is selected by configuration option (battery type).
A new table can be defined according to your battery properties.

### 5- Shutdown Level Alarm

Default Shutdown Level Alarm is set by configuration option (VOLT_SHUTDOWN_THRESHOLD).
This could be changed according to your battery properties.


@}
