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

PUB         := $(OUT)/pub
ZEPHYR_BASE := $(T)/external/zephyr

REF_BUILD_INFO ?= "$(PUB)/reference_build_info.json"

.DEFAULT_GOAL := package

ifeq ($(wildcard $(T)/packages/intel/curie-ble/Makefile),)
$(info No BLE image found under [$(T)/packages/intel/curie-ble].)
$(info Please go to https://github.com/01org/curie-bsp/releases/ to download it.)
$(error )
endif

ifneq ("$(wildcard $(T)/packages/intel/curie-ble/prebuilt/image.bin)","")
USE_PREBUILT_BLE_CORE := yes
endif

# Set documentation variables
PROJECT_LOGO := ""
PROJECT_NAME := "Curie SDK"
EXTRA_DOC_INPUT += $(T)/projects/curie_common/doc
EXTRA_IMAGES_INPUT += $(T)/projects/curie_common/doc/images

BOOT_SIGNING_KEY ?= $(T)/build/security/keys/boot_private.der
OTA_SIGNING_KEY ?= $(T)/build/security/keys/ota_private.der

# OTA package tool
OTA_PACKAGE_TOOL := $(OUT)/tools/bin/curie_ota.py
$(OTA_PACKAGE_TOOL): $(OUT)/tools/bin
	$(AT)cp $(T)/projects/curie_common/build/curie_ota.py $(OTA_PACKAGE_TOOL)

# Tools dependencies
HOST_TOOLS += $(KCONFIG_CONF) $(KCONFIG_MCONF)
HOST_TOOLS += ota_tools

include $(T)/build/common_targets.mk

# Boot updater
PROJECT_INCLUDES = $(PROJECT_PATH)/include
bootupdater_FILES = bootupdater.bin
bootupdater_VARS  = VERSION_MAJOR \
                    VERSION_MINOR \
                    VERSION_PATCH \
                    VERSION_STRING_SUFFIX \
                    PROJECT_INCLUDES

$(call generic_target,bootupdater,$(T)/bsp/bootable/bootupdater)

ifeq ($(BUILDVARIANT),debug)
BUILDVARIANT_STRING = D
else
BUILDVARIANT_STRING = R
endif

# Charging OS
cos_FILES = cos.c
cos_VARS  = PROJECT_INCLUDES

$(call generic_target,cos,$(T)/bsp/bootable/cos)

# Bootloader
COS_BLOB         = $(TARGETS_OUT)/cos.c
BOOTLOADER_ROOT ?= $(T)/bsp/bootable/bootloader

ifeq ($(OPTION_PUBLIC_BOARD),n)
SSBL_DEFCONFIG ?= $(subst ",,$(OPTION_PRIVATE_DIR))/bootloader/board/intel/$(BOARD)/$(BOARD)_defconfig
else
SSBL_DEFCONFIG ?= $(BOOTLOADER_ROOT)/board/intel/$(BOARD)/$(BOARD)_defconfig
endif

FSBL_DEFCONFIG ?= $(BOOTLOADER_ROOT)/chip/intel/quark_se/fsbl_defconfig
bootloader_CROSS_COMPILE = $(CROSS_COMPILE_x86)
bootloader_FILES = fsbl/fsbl.bin fsbl/fsbl.elf ssbl/ssbl.bin ssbl/ssbl.elf
bootloader_VARS  = PROJECT_INCLUDES \
                   COS_BLOB         \
                   BOOTLOADER_ROOT  \
                   SSBL_DEFCONFIG   \
                   FSBL_DEFCONFIG \
		   $(OPTIONS_VARS)

bootloader_GOALS = fsbl_menuconfig ssbl_menuconfig

$(call generic_target,bootloader,$(BOOTLOADER_ROOT))

bootloader: cos

override SOC_PATH := $(T)/bsp/src/machine/soc/intel/quark_se
# Quark main
quark_MACHINE_INCLUDE_PATH ?= $(T)/bsp/include/machine/soc/intel/quark_se/quark
quark_FILES = zephyr/quark.elf zephyr/quark.map zephyr/quark.lst

$(call kconfig_target,quark,$(T)/projects/curie_common/quark_main)

# Sensor core
arc_MACHINE_INCLUDE_PATH = $(T)/bsp/include/machine/soc/intel/quark_se/arc
arc_FILES = zephyr/arc.elf zephyr/arc.map zephyr/arc.lst

$(call kconfig_target,arc,$(T)/projects/curie_common/arc_main)

include $(T)/build/emulator.mk

clean::
	$(AT)if [ -d $(OUT)/ble_core ]; then \
		echo $(ANSI_CYAN)Cleaning BLE Core$(ANSI_OFF);  \
		$(MAKE) -C $(OUT)/ble_core/ clean               \
		-f $(T)/packages/intel/curie-ble/Makefile       \
		OUT=$(OUT) T=$(T) BUILDVARIANT=$(BUILDVARIANT); \
		echo $(ANSI_CYAN)Done BLE Core$(ANSI_OFF);      \
	fi
	@echo $(ANSI_RED)[RM]$(ANSI_OFF) $(OUT)/partitions_info.h
	$(AT)rm -rf $(OUT)/partitions_info.h

mrproper::
	$(AT)if [ -d $(OUT)/ble_core ]; then \
		echo $(ANSI_RED)[RM]$(ANSI_OFF) $(OUT)/ble_core; \
		rm -rf $(OUT)/ble_core;                          \
	fi

image: \
	$(OUT)/firmware/ble_core/image.bin \
	$(OUT)/firmware/quark.bin          \
	$(OUT)/firmware/arc.bin            \
	$(OUT)/firmware/quark.signed.bin   \
	$(OUT)/firmware/arc.signed.bin     \
	$(OUT)/firmware/ssbl.signed.bin    \
	$(OUT)/firmware/bootupdater.signed.bin

$(OUT)/firmware/ssbl.signed.bin: $(OUT)/firmware/ssbl.bin
	$(AT)cp $< $@
	$(AT) $(TOOL_SIGN) -f 0 -i $< -o $(OUT)/firmware/ssbl.signed.disabled.bin -s $(BOOT_SIGNING_KEY)

$(OUT)/firmware/bootupdater.signed.bin: bootupdater

$(OUT)/firmware/ble_core:
	mkdir -p $(OUT)/firmware/ble_core

$(OUT)/firmware/ble_core/image.bin: .force
	@echo
	@echo $(ANSI_CYAN)"Building BLE Core"$(ANSI_OFF)
	$(AT)mkdir -p $(OUT)/ble_core
	$(AT)$(MAKE) -f $(T)/packages/intel/curie-ble/Makefile \
		OUT=$(OUT) \
		-C $(OUT)/ble_core/ image T=$(T) BUILDVARIANT=$(BUILDVARIANT)
ifndef USE_PREBUILT_BLE_CORE
	$(AT)$(T)/tools/scripts/build_utils/add_binary_version_header.py \
		--major $(VERSION_MAJOR) \
		--minor $(VERSION_MINOR) \
		--patch $(VERSION_PATCH) \
		--version_string $(PROJECT_SHORT_CODE)BLE00$(BUILDVARIANT_STRING)-$(VERSION_STRING_SUFFIX) $@ $(DEV_NULL)
endif
	@echo $(ANSI_CYAN)"Done BLE Core"$(ANSI_OFF)

$(OUT)/firmware/arc.bin: arc .force
	@echo $(ANSI_CYAN)"Adding Arc version header"$(ANSI_OFF)
	$(AT)cp $(OUT)/arc/zephyr/arc.bin $@
	$(AT)$(T)/tools/scripts/build_utils/add_binary_version_header.py \
		--major $(VERSION_MAJOR) \
		--minor $(VERSION_MINOR) \
		--patch $(VERSION_PATCH) \
		--version_string $(PROJECT_SHORT_CODE)ARC00$(BUILDVARIANT_STRING)-$(VERSION_STRING_SUFFIX) $@ $(DEV_NULL)
	@echo $(ANSI_CYAN)"Done Arc version header"$(ANSI_OFF)

$(OUT)/firmware/%.signed.bin: $(OUT)/firmware/%.bin
	$(AT) $(TOOL_SIGN) -f 0 -i $< -o $@ -s $(BOOT_SIGNING_KEY)

$(OUT)/firmware/quark.bin: quark .force
	@echo $(ANSI_CYAN)"Adding Quark version header"$(ANSI_OFF)
	$(AT)cp $(OUT)/quark/zephyr/quark.bin $@
	$(AT)$(T)/tools/scripts/build_utils/add_binary_version_header.py \
		--major $(VERSION_MAJOR) \
		--minor $(VERSION_MINOR) \
		--patch $(VERSION_PATCH) \
		--version_string $(PROJECT_SHORT_CODE)LAK00$(BUILDVARIANT_STRING)-$(VERSION_STRING_SUFFIX) $@ $(DEV_NULL)
	@echo $(ANSI_CYAN)"Done Quark version header"$(ANSI_OFF)

$(OUT)/firmware/FSRam.bin: $(OUT)/firmware/arc.bin $(OUT)/firmware/quark.bin
	$(AT)$(T)/projects/curie_common/build/scripts/Create384KImage.py \
		-I $(OUT)/firmware/quark.bin -S $(OUT)/firmware/arc.bin -O $(OUT)/firmware/FSRam.bin

.PHONY: __flash_openocd_tree

# Flags used to pre processor mapping headers for linker script
EXTRA_LINKERSCRIPT_CMD_OPT = -I$(T)/bsp/bootable/bootloader/include
EXTRA_LINKERSCRIPT_CMD_OPT += -I$(PROJECT_PATH)/include
EXTRA_LINKERSCRIPT_CMD_OPT += -I$(T)/bsp/include

SNOR_SIZE ?= 16

$(OUT)/firmware/partition.conf.s: __flash_openocd_tree

$(OUT)/firmware/partition.conf: $(OUT)/firmware/partition.conf.s
	@echo $(ANSI_RED)"[mAS]"$(ANSI_OFF) $(OUT)/firmware/partition.conf
	$(AT)$(CROSS_COMPILE_x86)gcc -E -P \
		-o $(OUT)/firmware/partition.conf -ansi -D__ASSEMBLY__ -x assembler-with-cpp \
		$(T)/projects/curie_common/build/config/flash/openocd/partition.conf.s $(EXTRA_LINKERSCRIPT_CMD_OPT)

_create_bootloader_aliases: bootloader
	$(AT)ln -sf $(TARGETS_OUT)/fsbl.bin $(TARGETS_OUT)/fsbl_quark.bin
	$(AT)ln -sf $(TARGETS_OUT)/fsbl.elf $(TARGETS_OUT)/fsbl_quark.elf
	$(AT)ln -sf $(TARGETS_OUT)/ssbl.bin $(TARGETS_OUT)/ssbl_quark.bin
	$(AT)ln -sf $(TARGETS_OUT)/ssbl.elf $(TARGETS_OUT)/ssbl_quark.elf

FLASHTOOL_JSON_INPUT := $(wildcard $(T)/projects/curie_common/build/config/flash/flashtool/*.jsons)
FLASHTOOL_JSON_OUTPUT := $(FLASHTOOL_JSON_INPUT:$(T)/projects/curie_common/build/config/flash/flashtool/%.jsons=$(OUT_FLASH)/%.json)

$(FLASHTOOL_JSON_OUTPUT): $(FLASHTOOL_JSON_INPUT)
	$(AT)$(CROSS_COMPILE_x86)gcc -E -P \
		-o $@ -ansi $(CFLAGS_OPTIONS) -D__ASSEMBLY__ -x assembler-with-cpp \
		 $(@:$(OUT_FLASH)/%.json=$(T)/projects/curie_common/build/config/flash/flashtool/%.jsons) $(EXTRA_LINKERSCRIPT_CMD_OPT)

flash_files:: __flash_openocd_tree $(OUT)/firmware/partition.conf _create_bootloader_aliases $(FLASHTOOL_JSON_OUTPUT)
	@echo $(ANSI_CYAN)"Preparing flash images"$(ANSI_OFF)
	@echo "Copying flash scripts"
	$(AT)cp $(T)/projects/curie_common/factory/custom_factory_data_oem.py $(OUT_FLASH)/custom_factory_data_oem.py
	$(AT)cp $(T)/projects/curie_common/factory/curie_factory_data.py $(OUT_FLASH)/curie_factory_data.py
	@echo "Creating erase images"
	$(AT)dd if=/dev/zero of=$(OUT)/firmware/erase_panic.bin bs=2048 count=2			$(SILENT_DD)
	$(AT)$(T)/projects/curie_common/factory/create_ff_file.py 2048 3 $(OUT)/firmware/erase_factory_nonpersistent.bin
	$(AT)$(T)/projects/curie_common/factory/create_ff_file.py 2048 3 $(OUT)/firmware/erase_factory_persistent.bin
	$(AT)$(T)/projects/curie_common/factory/create_ff_file.py 4096 3 $(OUT)/firmware/erase_events.bin
	$(AT)dd if=/dev/zero of=$(OUT)/firmware/erase_data.bin bs=2048 count=4			$(SILENT_DD)
	$(AT)$(T)/projects/curie_common/factory/create_ff_file.py 1048576 $(SNOR_SIZE) $(OUT)/firmware/erase_snor.bin
	@echo $(ANSI_CYAN)"Done flash scripts"$(ANSI_OFF)

_project_flash: __flash_openocd_tree $(OUT)/firmware/partition.conf $(FLASHTOOL_JSON_OUTPUT)

__flash_openocd_tree::
	$(AT)rsync -avzh $(IN_FLASH)/ $(OUT_FLASH)/
	$(AT)rsync -avzh $(T)/projects/curie_common/build/config/flash/openocd/ $(OUT_FLASH)/
	$(AT)rsync -avzh $(T)/projects/curie_common/build/config/flash/flashtool/*.json $(OUT_FLASH)/

$(PUB)/build_report: | pub
	$(AT)mkdir -p $(PUB)/build_report

build_info: image | pub $(PUB)/build_report _create_bootloader_aliases
	$(AT)$(CROSS_COMPILE_x86)readelf -e $(OUT)/firmware/quark.elf \
		> $(OUT)/firmware/quark.stat
	$(AT)$(CROSS_COMPILE_arc)readelf -e $(OUT)/firmware/arc.elf \
		> $(OUT)/firmware/arc.stat
ifndef USE_PREBUILT_BLE_CORE
	$(AT)$(T)/external/gcc-arm/bin/arm-none-eabi-readelf -e $(OUT)/firmware/ble_core/image.elf \
		> $(OUT)/firmware/ble_core/image.stat
endif
	$(AT)$(CROSS_COMPILE_x86)readelf -e $(OUT)/firmware/fsbl.elf \
		> $(OUT)/firmware/fsbl_quark.stat
	$(AT)$(CROSS_COMPILE_x86)readelf -e $(OUT)/firmware/ssbl_quark.elf \
		> $(OUT)/firmware/ssbl_quark.stat
	$(AT)$(CPP) -include $(OUT)/arc/kbuild/config.h -I$(PROJECT_PATH)/include -I$(T)/bsp/include \
		$(T)/projects/curie_common/build/scripts/build_info_data.h -o $(OUT)/build_info_data.h
	$(AT)$(CPP) -include $(OUT)/arc/kbuild/config.h -I$(PROJECT_PATH)/include -I$(T)/bsp/include \
		$(T)/projects/curie_common/build/scripts/partitions_info.h -o $(OUT)/partitions_info.h
	$(AT)PYTHONPATH="$(PYTHONPATH):$(T)/projects/curie_common/build/scripts/" \
		python $(T)/tools/scripts/build_utils/generate_build_info.py \
			$(OUT) $(PUB)/build_info-$(BUILD_TAG).json \
			> $(OUT)/build_info.json
	$(AT)cat $(OUT)/build_info.json
ifndef USE_PREBUILT_BLE_CORE
	python $(T)/tools/scripts/build_utils/generate_memory_details.py \
		$(OUT)/ $(T)/ $(T)/tools/scripts/build_utils/features_list.json --ble_core 1 \
		> $(PUB)/build_report/$(PROJECT)-$(BOARD)-$(BUILDVARIANT)_memory_report-$(BUILD_TAG).html
else
	$(AT)python $(T)/tools/scripts/build_utils/generate_memory_details.py \
		$(OUT)/ $(T)/ $(T)/tools/scripts/build_utils/features_list.json \
		> $(PUB)/build_report/$(PROJECT)-$(BOARD)-$(BUILDVARIANT)_memory_report-$(BUILD_TAG).html
endif

package: image build_info flash_files | pub
	$(AT)cp $(T)/tools/scripts/panic/*.py $(OUT)/firmware/
	$(AT)mkdir -p $(PUB)/device/$(PROJECT)/image/$(BOARD)/$(BUILDVARIANT)
	$(AT)cd $(OUT)/firmware ; \
		cp ../build_info.json .; \
		zip $(if $(VERBOSE),,-q) -r $(PUB)/device/$(PROJECT)/image/$(BOARD)/$(BUILDVARIANT)/`cat $(OUT)/package_prefix.txt`.zip \
			*.elf *.bin *.cfg *.conf ble_core/* interface/*/* board/* build_info.json auto.json \
			build_info.json crash.json dump.json factory.json flash.json recover.json test.json *


TESTS_REPORT_FILE ?= $(PUB)/test_report.json

tests: package | pub
	PYTHONPATH="$(PYTHONPATH):$(T)/projects/curie_common/build/scripts/" \
		python $(T)/projects/curie_common/build/scripts/tests_run.py $(OUT) $(TESTS_REPORT_FILE)


sanity_check: package | pub
	PYTHONPATH="$(PYTHONPATH):$(T)/projects/curie_common/build/scripts/" \
		python $(T)/projects/curie_common/build/scripts/package_sanity_check.py $(T)/.. $(OUT)
