ifdef CONFIG_SERVICES_SENSOR_IMPL
obj-y += sensor_svc.o
obj-y += sensor_svc_list.o
obj-y += sensor_svc_utils.o
obj-y += sensor_svc_calibration.o
obj-$(CONFIG_QUARK_SE_ARC) += svc_platform_arc.o
obj-$(CONFIG_QUARK_SE_ARC) += sensor_svc_sensor_core.o
obj-$(CONFIG_QUARK_SE_QUARK) += svc_platform_quark.o
endif
obj-$(CONFIG_SERVICES_SENSOR) += sensor_svc_api.o
obj-$(CONFIG_SERVICES_SENSOR_TCMD) += ss_tcmd_client.o
