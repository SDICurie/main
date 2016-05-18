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

# This file describes development flashing interface for all programs and
# projects

.PHONY: flash

OUT_FLASH := $(OUT)/firmware
IN_FLASH ?= $(TOOLCHAIN_DIR)/tools/debugger/openocd/scripts/

FLASH_CLI_OPTS ?= -f $(OUT_FLASH)/flash.json -c $(FLASH_CONFIG) $(if $(VERBOSE),--log-level 5,)

$(OUT_FLASH)/generate_factory_bin_oem.py:
	$(AT)cp $(T)/tools/factory/generate_factory_bin_oem.py $@

$(OUT_FLASH)/generate_factory_bin_product.py:
	$(AT)cp $(T)/tools/factory/generate_factory_bin_product.py $@

$(OUT_FLASH)/factory_data.py:
	$(AT)cp $(T)/tools/factory/factory_data.py $@

ifdef FLASH_CONFIG
flash: flash_files
	$(AT)$(OPTION_FLASHTOOL_BINARY) $(FLASH_CLI_OPTS)
else
flash: flash_files image
	$(AT) echo Flashing device
	$(AT) $(foreach flash_config, $(subst ",,$(OPTION_FLASH_CONFIGS)), $(OPTION_FLASHTOOL_BINARY) --device-detection-timeout $(OPTION_FLASH_TIMEOUT) -f $(OUT_FLASH)/flash.json -c $(flash_config) $(if $(VERBOSE),--log-level 5,) || exit 182;)
endif

flash_files:: $(OUT_FLASH)/generate_factory_bin_oem.py $(OUT_FLASH)/generate_factory_bin_product.py $(OUT_FLASH)/factory_data.py

flash_help:
	@echo FLASH_CONFIG ?= sw variant

help::
	@echo 'Flashing:'
	@echo ' flash		- flash the device'
	@echo ' flash_help	- detailed help on flash'
	@echo
