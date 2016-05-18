ifeq ($(CONFIG_SBA),y)
obj-$(CONFIG_INTEL_QRK_I2C) += sba_i2c_tst.o
obj-$(CONFIG_INTEL_QRK_SPI) += sba_spi_tst.o
endif
obj-$(CONFIG_LOG_CBUFFER) += cbuffer_test.o
obj-y += wakelock_tst.o
obj-y += list_tst.o
obj-$(CONFIG_SOC_COMPARATOR) += comparator_tst.o
obj-y += timer_tst.o
