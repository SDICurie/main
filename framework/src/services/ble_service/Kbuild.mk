obj-$(CONFIG_SERVICES_BLE_IMPL) += ble_service.o
obj-$(CONFIG_SERVICES_BLE) += ble_service_api.o
obj-$(CONFIG_SERVICES_BLE) += nble_driver.o
obj-$(CONFIG_SERVICES_BLE_IMPL) += ble_service_utils.o
obj-$(CONFIG_BLE_CORE_TEST) += test/

# for the nordic rpc functions
CFLAGS_ble_service.o = -I$(T)/external/zephyr/drivers/nble_curie/
CFLAGS_nble_driver.o = -I$(T)/external/zephyr/drivers/nble_curie/
CFLAGS_ble_service_test.o = -I$(T)/external/zephyr/drivers/nble_curie/
