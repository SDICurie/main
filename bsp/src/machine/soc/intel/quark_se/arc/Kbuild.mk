obj-$(CONFIG_QUARK_SE_ARC_SOC_SETUP) += soc_setup.o
obj-$(CONFIG_QUARK_SE_ARC_SOC_CONFIG) += soc_config.o
obj-y += pm_pupdr.o
obj-y += low_power.o
obj-$(CONFIG_QUARK_SE_PANIC_DEFAULT) += panic.o panic_stub.o
obj-y += bsp.o
