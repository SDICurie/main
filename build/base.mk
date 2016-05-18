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

# Include this makefile in your target-specific Makefile.

# In order to work properly, your project Makefile must define the T and OUT
# variable before including this file.

KCONFIG_FILE = $(OUT)/.config

KBUILD_OUT_DIR := $(OUT)/kbuild

KCONFIG_HEADER := $(KBUILD_OUT_DIR)/config.h

$(KCONFIG_HEADER): $(KCONFIG_FILE)
	@echo "Creating Kconfig header:" $@
	$(AT)mkdir -p $(KBUILD_OUT_DIR)
	$(AT)sed $< -e 's/#.*//' > $@
	$(AT)sed -i $@ -e 's/\(CONFIG_.*\)=/#define \1 /'
# Kconfig uses #define CONFIG_XX 1 instead of CONFIG_XX y for booleans
	$(AT)sed -i $@ -e 's/ y$$/ 1/'

# Provide rules to build the wearable device SDK "thin" static library built-in.a
# (actually an aggregated list of links to actual object files)

#
# The C flags we will be using are the aggregation of:
# - the CFLAGS from the environment
# - the (wrongly-named) KBUILD_CFLAGS passed by the Zephyr build system
# - the TARGET_CFLAGS we previously set in the environment
#
# Note: KBUILD_CFLAGS actually also include the environment CFLAGS, but we
# don't take it for granted (better safe than sorry)
#
# We also add a directive to include the Kconfig header in all C files
COMPUTED_CFLAGS  = -include $(KCONFIG_HEADER)
COMPUTED_CFLAGS += $(CFLAGS) $(KBUILD_CFLAGS) $(TARGET_CFLAGS)

# The library (built-in.a) is built based on:
# - the target build configuration -> KCONFIG_FILE,
# - the target build tools -> CC, AR
# - our computed C flags
$(KBUILD_OUT_DIR)/built-in.a: $(KCONFIG_HEADER) _generated_sources FORCE
	@echo $(ANSI_CYAN)Building library$(ANSI_OFF) $@
	$(AT)$(MAKE) -C $(T) -f $(T)/build/Makefile.build \
		SRC=. \
		OUT=$(KBUILD_OUT_DIR) \
		T=$(T)                \
		KCONFIG=$(KCONFIG_FILE) \
		CFLAGS="$(COMPUTED_CFLAGS)" \
		CC=$(CC) \
		AR=$(AR) \
		PROJECT_PATH=$(PROJECT_PATH)

_generated_sources:
	@echo "Generating source files"
	$(AT)$(MAKE) -C $(T) -f $(T)/build/Makefile.source \
		SRC=. \
		OUT=$(KBUILD_OUT_DIR) \
		T=$(T)                \
		KCONFIG=$(KCONFIG_FILE) \
		PROJECT_PATH=$(PROJECT_PATH)

$(OUT)/libapp.a: $(OUT)/kbuild/built-in.a $(TARGET_LIBS)
	@echo $(ANSI_RED)"[AR]"$(ANSI_OFF) $@
	$(AT)rm -f $@
	$(AT)$(AR) -rcT $@ $^

clean: kbuild_clean

kbuild_clean:
	@echo $(ANSI_RED)[kRM]$(ANSI_OFF) $(KBUILD_OUT_DIR)
	$(AT) rm -rf $(KBUILD_OUT_DIR)
	@echo $(ANSI_RED)[kRM]$(ANSI_OFF) libapp.a
	$(AT) rm -rf $(OUT)/libapp.a

.PHONY: _generated_sources
