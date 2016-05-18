THIS_DIR = $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST))))
T        = $(abspath $(THIS_DIR)/..)

ifeq ($(ARCH),)
$(error No ARCH specified)
endif

include $(T)/build/Makefile.toolchain

all clean:
	@CC=$(CC) AR=$(AR) CROSS_COMPILE=$(CROSS_COMPILE) $(MAKE) $@

strip:
	@$(STRIP) $(ARGS)
