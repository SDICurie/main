ifeq ($(CONFIG_QUARK_SE_QUARK),y)
obj-y += int_stubs.o
endif
obj-$(CONFIG_PACKAGE_MATHLIB) += test_lib_math.o
obj-$(CONFIG_PACKAGE_BALIB) += test_balib.o
obj-y += balib_test/
obj-y += test_critical_section.o
obj-y += test_queue.o
obj-y += test_sema.o
obj-y += test_task.o
obj-y += test_timer.o
obj-y += test_counter.o
obj-y += utility.o
obj-y += test_interrupt.o
obj-y += test_malloc.o
obj-y += test_mutex.o
obj-y += test_stub.o
obj-y += test_suite.o
CFLAGS_test_malloc.o +=-I$(CONFIG_MEM_POOL_DEF_PATH)
