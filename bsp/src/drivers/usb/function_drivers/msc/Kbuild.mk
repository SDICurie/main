#subdir-cflags-y += -I$(T)/bsp/bootable/bootloader/include/usb/
subdir-cflags-$(CONFIG_USB_MSC) += -I$(T)/bsp/include/drivers/msc
obj-$(CONFIG_USB_MSC)	+= msc.o
obj-$(CONFIG_USB_MSC)	+= msc_function.o
obj-$(CONFIG_USB_MSC)	+= scsi_command_handler.o
obj-$(CONFIG_USB_MSC)	+= msc_file_system.o
obj-$(CONFIG_USB_MSC)	+= emfat.o


