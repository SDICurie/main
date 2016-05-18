obj-$(CONFIG_QUARK_SE_PANIC_DEFAULT) += panic_stubs.o panic.o
obj-y += reboot.o
obj-$(CONFIG_QUARK_SE_QUARK_SOC_SETUP) += soc_setup.o
obj-y += pm_pinmux.o
obj-$(CONFIG_DEEPSLEEP) += low_power.o
obj-$(CONFIG_LOG_BACKEND_USB) += log_backend_usb.o
obj-$(CONFIG_QUARK_SE_QUARK_LOG_BACKEND_UART) += log_backend_uart.o
obj-$(CONFIG_QUARK_SE_QUARK_LOG_BACKEND_FLASH) += log_backend_flash.o
obj-$(CONFIG_TCMD_CONSOLE_UART) += uart_tcmd_client.o
obj-$(CONFIG_QUARK_SE_QUARK_SOC_CONFIG) += soc_config.o
obj-$(CONFIG_IPC) += ipc.o
obj-y += bsp.o
obj-y += boot.o
obj-$(CONFIG_QUARK_SE_PROPERTIES_STORAGE) += properties_storage_soc_flash.o
obj-y += pm_pupdr.o
obj-y += idle.o
obj-y += pm_pupdr_tcmd.o
cflags-$(CONFIG_PROFILING) += -finstrument-functions -finstrument-functions-exclude-file-list=idle.c
