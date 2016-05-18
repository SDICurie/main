@defgroup ble_stack BLE
@{

# Features

BLE stack provides BLE interface, abstracting most of the complexity of BLE.

It provides APIs covering central and peripheral role.

BLE services can be extended by using the BLE core APIs and implement a new
BLE service/profile. This is achieved by using the ble_core service APIs.

BLE Core provides Zephyr smp/gap/gatt server/gatt client APIs. It may even run
on different CPU (a kind of RPC).

Those APIs are intended to extend the BLE service and should not be used by
an application developper directly!

A [BLE Application library](@ref ble_app_lib) is provided to make the interface
with the BLE stack much easier.

## BLE Services

Currently the following BLE services are implemented and supported:
- Simplified security handling
- @ref ble_gap "BLE GAP service" (UUID: 0x1800)
- @ref ble_bas "BLE DIS service" (UUID: 0x180a): Different device info's (SW version etc) are
  displayed as per BT spec.
- @ref ble_bas "BLE BAS service" (UUID: 0x180f): BLE Battery level service as per BT spec.
  It uses the battery service to update automatically the battery level
  characteristic.
- @ref ble_hrs "BLE HRS service" (UUID: 0x180d): Heart Rate
- @ref ble_rscs "BLE RSCS service" (UUID: 0x1814): only Stride cadence notification is supported
- @ref ble_lns "BLE LNS service" (UUID: 0x1819): only Elevation notification is supported
- BLE ISPP service (UUID: dd97c415-fed9-4766-b18f-ba690d24a06a):
  Intel serial port protocol: A serial port emulation running over BLE

A new BLE service can be easily added. See \ref add_ble_service.

## Sources location

- Include files: `framework/include/lib/ble`
- Source files: `framework/src/lib/ble`

@}

@defgroup add_ble_service Add a BLE service
@{
@ingroup ble_stack
@anchor ble_service


# How-to Add new BLE service

Extending the current Component Framework BLE service into a custom service can be achieved
by using the zephyr gatt APIs.
BLE ISPP can be used an example for a proprietary _BLE service and protocol_.

- As a convention, a new BLE service should be implemented in a ble_XXX.[ch] files (e.g. XXX: gap, bas, dis, etc).
- A function ble_XXX_init() should be implemented and called by an application.
  It should trigger the initialization of the new BLE service by calling bt_gatt_register()
- Each BLE service is defined by const attribute table using the macros found in gatt.h (:
  - BT_GATT_PRIMARY_SERVICE: the UUID of the service
  - BT_GATT_CHARACTERISTIC: the UUID of the characteristics and its properties
  - BT_GATT_DESCRIPTOR: permissions, read & write functions, optionally pointer to data storage
    - read() used init ble controller database
      - if buf pointer is NULL, it needs to return the init/max length if charactristic is readable
      - if buf pointer is not NULL, it needs copy the init data to buf pointer
    - write(): if handler for writeable characteristic
    - data pointer: optional, maybe used for init value or storage for write operation
  - BT_GATT_CCC: for notifiable or indicatable characteristics, must follow above 2 macros
    - the two macro parameters should be NULL as not used. however the macro internal CCC value is updated (see bas) via bt_gatt_attr_write_ccc().
  - additional descriptors for a characteristic maybe defined. however only AFTER above macros
- the service must define an init_svc() and init_svc_complete() function. The complete function sends back the response.
- additional helper functions maybe required to update the ble controller database
- the service may register with connect/disconnect events using ble_service_register_for_conn_st()

A protocol type of service, e.g. ISPP, may also support the protocol events to read/write in addition to ble_update_data()
@}

@defgroup ble_services BLE Services
@{
@ingroup ble_stack
@}
