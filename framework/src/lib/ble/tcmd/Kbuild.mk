obj-$(CONFIG_TCMD_BLE) += ble_tcmd.o
obj-$(CONFIG_TCMD_BLE_DTM) += ble_dtm_tcmd.o

CFLAGS_ble_tcmd.o = -I$(T)/external/zephyr/drivers/nble_curie/
CFLAGS_ble_dtm_tcmd.o = -I$(T)/external/zephyr/drivers/nble_curie/
