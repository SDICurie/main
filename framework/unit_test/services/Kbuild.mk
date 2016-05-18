obj-y += service_tests.o
obj-y += adc_service_test.o
obj-y += ui_svc_test.o
obj-$(CONFIG_PWRBTN_GPIO) += pwrbtn_service_test.o

ifeq ($(CONFIG_QUARK_DRIVER_TESTS),y)
obj-y += ll_storage_service_test.o
obj-y += battery_service_test.o
obj-y += properties_service_test.o
endif
subdir-cflags-y += -I$(T)/framework/unit_test
