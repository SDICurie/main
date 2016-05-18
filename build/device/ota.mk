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

$(OUT)/ota $(OUT)/ota/full/pub $(OUT)/ota/incremental/pub:
	$(AT)mkdir -p $@

# OTA full package
ota_full_package: package ota_tools $(OTA_PACKAGE_TOOL) $(OUT)/ota/full/pub | pub
	$(AT) $(PYTHON) $(OTA_PACKAGE_TOOL) -i $(OUT)/firmware -o $(OUT)/ota/full/pub -p "full"
	$(AT) $(TOOL_SIGN) -f 1 -i $(OUT)/ota/full/pub/package.ota.bin -o $(OUT)/ota/full/pub/package.ota.signed.bin -s $(OTA_SIGNING_KEY)
	$(AT)mkdir -p $(PUB)/device/$(PROJECT)/ota/full/$(BOARD)/$(BUILDVARIANT)
	$(AT)cd $(OUT)/ota/full/pub/ ; \
		cat *.json ; \
		cp *.html *.json $(PUB)/device/$(PROJECT)/ota/full/$(BOARD)/$(BUILDVARIANT)/ ; \
		zip $(if $(VERBOSE),,-q) -r $(PUB)/device/$(PROJECT)/ota/full/$(BOARD)/$(BUILDVARIANT)/$(PROJECT)-$(BOARD)-$(BUILDVARIANT)-ota_full-$(BUILD_TAG).zip *.bin *.json

# OTA incremental package

OTA_FROM_PACKAGES := $(wildcard $(INCREMENT_FROM_DIR)/device/$(PROJECT)/ota/full/$(BOARD)/$(BUILDVARIANT)/*.zip)

define ota_incremental_package_process
	@echo Processing $(1) ; \
	OTA_FULL_ZIP_FILE=$(strip $(1)) ; \
	OTA_FULL_ZIP_DIR=$(strip $(basename $(1))) ; \
	OTA_INCREMENTAL_OUT_DIR=$(strip $(subst full,incremental,$(subst $(INCREMENT_FROM_DIR), $(OUT)/ota/incremental/pub/,  $(basename $(1))))) ; \
	OTA_INCREMENTAL_PUB_DIR=$(strip $(subst full,incremental,$(subst $(INCREMENT_FROM_DIR), $(PUB)/,  $(basename $(1))))) ; \
	OTA_FULL_PACKAGE_NAME=$(strip $(basename $(notdir $(1)))) ; \
	unzip -qq -o $$OTA_FULL_ZIP_FILE -d $$OTA_FULL_ZIP_DIR && \
	dd if=$$OTA_FULL_ZIP_DIR/package.ota.signed.bin of=$$OTA_FULL_ZIP_DIR/from_package.ota.bin bs=1 skip=128 && \
	cp $(OUT)/ota/full/pub/package.ota.bin $$OTA_FULL_ZIP_DIR/to_package.ota.bin && \
	mkdir -p $$OTA_INCREMENTAL_OUT_DIR && \
	mkdir -p $$OTA_INCREMENTAL_PUB_DIR && \
	$(PYTHON) $(OTA_PACKAGE_TOOL) -i $$OTA_FULL_ZIP_DIR -o $$OTA_INCREMENTAL_OUT_DIR -p "incremental" && \
	$(TOOL_SIGN) -f 1 -i $$OTA_INCREMENTAL_OUT_DIR/package_incremental.bin -o $$OTA_INCREMENTAL_OUT_DIR/package_incremental.signed.bin -s $(OTA_SIGNING_KEY) && \
	zip $(if $(VERBOSE),,-qq) -j -D -r $$OTA_INCREMENTAL_PUB_DIR/$(PROJECT)-$(BOARD)-$(BUILDVARIANT)-ota_incremental-$${OTA_FULL_PACKAGE_NAME}_$(BUILD_TAG).zip $$OTA_INCREMENTAL_OUT_DIR/*.bin $$OTA_INCREMENTAL_OUT_DIR/*.json && \
	cp $$OTA_INCREMENTAL_OUT_DIR/*.json $$OTA_INCREMENTAL_PUB_DIR ; \
	cp $$OTA_INCREMENTAL_OUT_DIR/*.html $$OTA_INCREMENTAL_PUB_DIR ; \
	echo ... Done $(1)
endef

.PHONY: ota_incremental_package
ota_incremental_package: ota_full_package $(OUT)/ota/incremental/pub
ifdef INCREMENT_FROM_DIR
	$(foreach from_package, $(OTA_FROM_PACKAGES), $(call ota_incremental_package_process, $(from_package)))
endif

help::
	@echo 'OTA:'
	@echo ' ota_full_package        - generate the ota package for current config '
	@echo ' ota_incremental_package - generate an incremental ota package.' \
		'Use INCREMENT_FROM_DIR to specify the base package.'
	@echo
