# Copyright (c) 2015, Intel Corporation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
# may be used to endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

#
# Define specific C flags that need to be exported to Makefile.app
#

# TODO remove this one
TARGET_CFLAGS += -I$(T)/bsp/include/machine/soc/intel/quark_se/

TARGET_CFLAGS += -I$(ZEPHYR_BASE)/include/
TARGET_CFLAGS += -I$(ZEPHYR_BASE)/include/microkernel/
TARGET_CFLAGS += -I$(ZEPHYR_BASE)/arch/x86/soc/quark_se/

# ble support, additional include paths.
TARGET_CFLAGS += -I$(T)/framework/src/services/ble_service

# Zephyr specific build configuration for QUARK
ARCH        := x86
KERNEL_TYPE := micro

# For sysgen.h
TARGET_CFLAGS += -I$(OUT)/zephyr/misc/generated/sysgen/
# We use some of the Zephyr internal configuration flags
TARGET_CFLAGS += -include $(OUT)/zephyr/include/generated/autoconf.h
# Flags used to preprocessor mapping headers for linker script
EXTRA_LINKER_CMD_OPT +=-D CONFIG_QUARK_SE_ARC_RAM_SIZE=$(CONFIG_QUARK_SE_ARC_RAM_SIZE)
EXTRA_LINKER_CMD_OPT +=-I$(PROJECT_PATH)/include
EXTRA_LINKER_CMD_OPT +=-I$(T)/bsp/include/
EXTRA_LINKER_CMD_OPT +=-I$(T)/bsp/include/machine/soc/intel/quark_se/
EXTRA_LINKER_CMD_OPT +=-I$(T)/bsp/include/machine/soc/intel/quark_se/quark
EXTRA_LINKER_CMD_OPT +=-DCONFIG_SIGNATURE_HEADER_SIZE=0x80

include $(T)/build/Makefile.zephyr
