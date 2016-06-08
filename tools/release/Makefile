# Copyright (c) 2016, Intel Corporation. All rights reserved.

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

# OSS release cooking recipe draft
# Need to run from repo top directory

T   := $(CURDIR)/wearable_device_sw
OUT := $(CURDIR)/out
PUB := $(CURDIR)/pub

BUILDVARIANT    := release
BOARD           := curie_101

include $(T)/build/verbosity.mk

clean:
	@echo $(ANSI_RED)"[tRM]"$(ANSI_OFF) $@
	$(AT)rm -rf $(T)/../out

sdk_release:
	$(MAKE) CurieBSP_sdk

include $(T)/projects/curie_common/build/wearable_device_sw_version.mk

_CurieBSP_sdk_dir: SDK_OUT = $(OUT)/curie_hello_curie_101_release
_CurieBSP_sdk_dir: wearable_device_sw Makefile
	rm -rf $(T)/CurieBSP_sdk
	bash -c "rsync -av --exclude={.git} --exclude={.gitignore} --exclude=packages/intel/curie-ble/ $(T) $(T)/CurieBSP_sdk/"

SDK_PACKAGE_NAME := CurieBSP_sdk-$(WEARABLE_DSW_VERSION_MAJOR).$(WEARABLE_DSW_VERSION_MINOR).$(WEARABLE_DSW_VERSION_PATCH)
CurieBSP_sdk: _CurieBSP_sdk_dir
	# Create archive
	(cd $(T) ; tar -I pigz -cf $(SDK_PACKAGE_NAME).tgz CurieBSP_sdk)
	rm -rf $(T)/CurieBSP_sdk
	mkdir -p $(PUB)/sdk/CurieBSP_sdk
	mv $(T)/$(SDK_PACKAGE_NAME).tgz $(PUB)/sdk/CurieBSP_sdk/
	echo "CurisBSP SDK created in $(PUB)/sdk/CurieBSP_sdk/$(SDK_PACKAGE_NAME).tgz"
