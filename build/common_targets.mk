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
# This file declares the common build rules to be used by top-level Makefiles.
#
# It is typically included by SoC-specific Makefiles that will:
# - add specific build targets
# - extend the generic help, image, clean and mrproper rules.
#

ifndef T
$(error No top level source directory T specified)
endif

ifndef OUT
$(error No output directory (OUT) specified)
endif

ifndef PROJECT
$(error No project short name (PROJECT) specified)
endif

export OUT
export PROJECT_PATH
export INTERNAL_BSP_PATH

pub:
	$(AT)mkdir -p $(PUB)

.force:

# Help

include $(T)/build/help.mk

help::
	@echo 'Base targets:'
	@echo ' clean    - remove all build artifacts, preserving configuration'
	@echo ' mrproper - clean and restore setup configuration'
	@echo ' image    - build image for current config'
	@echo ' package  - generate the package for current config in pub/'
	@echo
	@echo 'Metadata:'
	@echo ' build_info   - create a json file reporting footprint of ram and flash consumption and other infos'
	@echo ' build_report - create an HTML report for display e.g. in CI builders summary'
	@echo

# Documentation

# Set documentation default parameters
PROJECT_LOGO ?= ""
PROJECT_NAME ?= ""

include $(T)/build/doc.mk

#### host rules
include $(T)/build/host/tools.mk
include $(T)/build/host/check.mk

# kconfig tools are required for some targets
include $(T)/build/host/kconfig.mk
HOST_TOOLS += $(KCONFIG_CONF)

#### device rules
include $(T)/build/device/flash.mk
include $(T)/build/device/debug.mk
include $(T)/build/device/ota.mk

#### Packages

help::
	@echo "Packages:"

include $(T)/build/Makefile.package

help::
	@echo

#### config rules

$(OUT)/%/.config: $(PKG_OUT)/Kconfig
	$(AT)$(MAKE) -f $(T)/build/config.mk defconfig \
		T=$(T) \
		OUT=$(OUT)/$*/ \
		PKG_OUT=$(PKG_OUT) \
		KCONFIG_ROOT=$(T)/Kconfig \
		DEFCONFIG=$($*_DEFCONFIG)

%_menuconfig: $(OUT)/%/.config
	$(AT)$(MAKE) -f $(T)/build/config.mk menuconfig \
		T=$(T) \
		OUT=$(OUT)/$*/ \
		PKG_OUT=$(PKG_OUT) \
		KCONFIG_ROOT=$(T)/Kconfig

%_savedefconfig:
	$(AT)$(MAKE) -f $(T)/build/config.mk savedefconfig \
		T=$(T) \
		OUT=$(OUT)/$*/ \
		PKG_OUT=$(PKG_OUT) \
		KCONFIG_ROOT=$(T)/Kconfig \
		DEFCONFIG=$(if $(DEFCONFIG),$(DEFCONFIG),$(OUT)/$*/baseconfig)

#### Declare global variables for pretty output
include $(T)/build/verbosity.mk

#
# Image build targets
#
#
# To add a new target, use the target template defined in Makefile.target:
#
# $(call generic_target,<name>,<path>)
#
# This will create the following "target" goals:
# - <name>          : make all on the Makefile under <path>
# - <name>_clean    : make clean on the Makefile under <path>
# - <name>_mrproper : wipe out target build directory
#
# Variables prefixed by the target name will automatically be converted
# to generic varialbes and passed to the target Makefile
#
# Example:
#
# arc_DEFCONFIG -> DEFCONFIG
#
include $(T)/build/Makefile.target

# The generic image goal creates all targets
image: targets

# The generic clean goal cleans all targets
clean:: targets_clean packages_clean
	@echo $(ANSI_RED)[RM]$(ANSI_OFF) $(TARGETS_OUT)/*
	$(AT)rm -rf $(TARGETS_OUT)/*
	@echo $(ANSI_RED)[RM]$(ANSI_OFF) $(OUT)/build_info*
	$(AT)rm -rf $(OUT)/build_info*

# The generic mrproper goal resets all targets
mrproper:: targets_mrproper clean

.PHONY: image clean mrproper

help::
	@echo 'Specific buid targets:'
	@echo
