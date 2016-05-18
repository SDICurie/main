obj-y += list.o
obj-$(CONFIG_WORKQUEUE) += workqueue.o
obj-$(CONFIG_CUNIT_TESTS) += cunit_test.o
obj-$(CONFIG_LOG_CBUFFER) += cbuffer.o
obj-$(CONFIG_CSTORAGE_FLASH_SPI) += cir_storage_flash_spi.o
obj-$(CONFIG_PROFILING) += profiling.o
obj-$(CONFIG_MEMORY_POOLS_BALLOC) += balloc.o
CFLAGS_balloc.o += -I$(CONFIG_MEM_POOL_DEF_PATH)
