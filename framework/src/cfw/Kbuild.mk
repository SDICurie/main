obj-y += cfw_debug.o
obj-y += cproxy.o
obj-$(CONFIG_CFW_CLIENT) += client_api.o
obj-$(CONFIG_CFW_SERVICE) += service_api.o
obj-$(CONFIG_CFW_MASTER) += service_manager.o
obj-$(CONFIG_CFW_PROXY) += service_manager_proxy.o
cflags-$(CONFIG_PROFILING) += -finstrument-functions -finstrument-functions-exclude-file-list=service_manager_proxy.c,service_api.c,client_api.c,cproxy.c,cfw_debug.c
obj-$(CONFIG_CFW_QUARK_SE_HELPERS) += cfw_quark_se_helpers.o
