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

include $(T)/build/Makefile.toolchain


.PHONY: _tmux-exists _pft-exists _set-openocd-as-flasher-external-tool

# Checks for pft
_pft-exists:
	@which $(OPTION_FLASHTOOL_BINARY) > /dev/null || ! echo Please install flashing tool from https://01.org/android-ia/downloads/intel-platform-fj


PIGZ_BINARY := $(shell which pigz)
_pigz-exists:
	@which pigz > /dev/null || ! echo "Please install pigz"

# Checks for tmux
TMUX_BINARY := $(shell which tmux)
_tmux-exists:
	@which tmux > /dev/null || ! echo "Please install tmux"

# Platform Flash Tool
PLATFORMFLASHTOOL_MIN_VERSION="5.5.1"
_check-platformflashtool_version:
	@if [ `which $(OPTION_FLASHTOOL_BINARY) | grep -c 'not found'` -ne 0 ]; then echo "Please install Platform FLash Tool. See https://01.org/android-ia/downloads/intel-platform-flash-tool-lite" ; exit 1 ; fi
	# Check the version of platformflashtool
	@python $(T)/build/host/check_pft_version.py $(PLATFORMFLASHTOOL_MIN_VERSION)

# openocd stuff
_set-openocd-as-flasher-external-tool:
	$(OPTION_FLASHTOOL_BINARY) --set-external-tool openocd_upstream:$(OPENOCD)

install-packages-required-by-emulator:
	@if [ `grep -c "Ubuntu 12.04" /etc/issue` -eq 1 ];then sudo apt-get build-dep qemu-kvm;else sudo apt-get build-dep qemu;fi
	sudo apt-get install libexpat1-dev libcairo-dev

# Doxygen
DOXYGEN_MIN_VERSION="1.8.0"
_check-doxygen_version:
	# Check the version of Doxygen
	@python $(T)/build/host/check_doxygen_version.py $(DOXYGEN_MIN_VERSION)

# udev stuff
IN_UDEV_RULES := $(T)/tools/deploy/udev
OUT_UDEV_RULES := /etc/udev/rules.d
$(OUT_UDEV_RULES)/%.rules: $(IN_UDEV_RULES)/%.rules
	@sudo cp $< $@
_udev-rules: $(OUT_UDEV_RULES)/99-openocd.rules \
    $(OUT_UDEV_RULES)/99-tty.rules

# Main API, implement the per-project sub-targets
one_time_setup: _udev-rules _set-openocd-as-flasher-external-tool
	@echo Installing base build dependencies
	sudo apt-get install python gawk git-core diffstat unzip zip texinfo gcc-multilib \
		build-essential chrpath libtool libc6:i386 doxygen graphviz tmux     \
		libc6-dev-i386 uncrustify mscgen vim-common pigz libdbus-1-dev \
		libglib2.0-dev
	@$(MAKE) -s _check-doxygen_version
	@echo Installing kconfig front-ends dependencies
	sudo apt-get install autoconf pkg-config gperf flex bison libncurses5-dev
	@echo Installing protobuf compiler dependencies
	sudo apt-get install protobuf-compiler python-protobuf
	@$(MAKE) -s install-packages-required-by-emulator

check_host_setup: _tmux-exists \
                  _pft-exists \
                  _pigz-exists \
                  _check-platformflashtool_version

.PHONY: one_time_setup check_host_setup
