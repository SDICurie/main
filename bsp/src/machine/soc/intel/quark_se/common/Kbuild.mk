obj-$(CONFIG_IPC) += ipc.o
obj-y += mem_tcmd.o
obj-$(CONFIG_QUARK_SE_UPTIME) += uptime.o
obj-$(CONFIG_QUARK_SE_USB_SETUP) += usb_setup.o
obj-y += pm_pupdr_common.o
obj-$(CONFIG_FACTORY_DATA) += factory_tcmd_helper.o
