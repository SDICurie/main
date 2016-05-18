# This file is used to set options for build environment and compilation flags
# These values can be overriden by adding an options.mk file in your project
#
# Variables shall contain no spaces
#
# Possible values are:
#
# y: option is enabled, and CFLAG will be set
#
# n: option is disabled, and CFLAG will not be set
#
# string, "string" "string1 string2": CFLAG will be set
#
# numerical value (decimal or hexadecimal): CFLAG will be set

OPTION_URL_HELP ?= "http://github.com/01org/curie_bsp"
OPTION_JTAG_INTERFACE_CFG_FILE ?= "interface/ftdi/flyswatter2.cfg"
OPTION_FLASHTOOL_OPENOCD_BINARY ?= openocd_upstream
OPTION_FLASHTOOL_BINARY ?= platformflashtoollitecli
OPTION_FLASH_CONFIGS ?= "usb_full"
OPTION_FLASH_TIMEOUT ?= 30000

OPTION_PUBLIC_BOARD ?= y
OPTION_PUBLIC_CHIP ?= y
OPTION_PUBLIC_CORE ?= y

# Required to export variables
OPTIONS_VARS += $(filter OPTION_%,$(.VARIABLES))
CFLAGS_OPTIONS ?= $(filter-out %=n,$(foreach var,$(OPTIONS_VARS),-D$(var)=$($(var))))
