obj-$(CONFIG_QUARK_SE_QUARK) += quark/
obj-$(CONFIG_QUARK_SE_COMMON) += common/
obj-$(CONFIG_QUARK_SE_ARC) += arc/

subdir-cflags-$(CONFIG_QUARK_SE_COMMON) += \
	-I$(T)/bsp/src/machine/soc/intel/quark_se/common
