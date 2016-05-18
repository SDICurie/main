#
# This file is the root of the build tree for each project
#
# The order of declarations of subdirectories is VERY important,
# as it defines which level has precedence for:
# - the include path search order (for header overrides),
# - the object link order (for weak function overrides).
#
# The current order is:
# - project,
# - core,
# - board,
# - soc.
#

ifdef PROJECT_PATH
# Project specific source and includes
obj-y += $(PROJECT_PATH)/
# Also add the project root include path recursively to the CFLAGS
subdir-cflags-y += -I$(PROJECT_PATH)/include
endif

# Core specific include path
subdir-cflags-y += -I$(MACHINE_INCLUDE_PATH)

ifdef BOARD_PATH
# Board specific source and includes
obj-y += $(BOARD_PATH)/
subdir-cflags-y += -I$(BOARD_PATH)/include
endif

ifdef SOC_PATH
# SoC specific source and includes
obj-y += $(SOC_PATH)/
subdir-cflags-y += -I$(SOC_PATH)/include
endif

# Component framework and services
obj-$(CONFIG_CFW) += framework/
subdir-cflags-y += -I$(T)/framework/include

# Infra, drivers and utils
obj-y += bsp/
subdir-cflags-y += -I$(T)/bsp/include

# Packages are optional SDK components
-include packages/*/package.mk
-include packages/*/*/package.mk
-include packages/*/*/*/package.mk
