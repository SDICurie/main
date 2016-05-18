@page add_board Add support for a new board

The SDK supports building targets for custom boards, as long as a Makefile
is provided for it.

## Board Makefile

When building for a specific \<board>, the SDK will look for a \<board>.mk file
first in boards directory from the root of the project tree then under the
build/boards directory.

The \<board>.mk file is primarily used to specify which targets must be built
for a given board.

It can optionally set specific board configuration parameters.

For the main SDK targets based on Kbuild/Kconfig, it can optionally:
- specify where the board specific system hooks are implemented,
- extend or override the Kconfig rules.

When the \<board>.mk file is parsed, the following variables will always be set:
- T: corresponds to the top of the SDK source tree,
- PROJECT_PATH: corresponds to the top of your project source tree,
- OUT: the build output directory

### Board targets

A board target is a firmware to be assembled to run on a specific board core.

Targets are highly specific pieces of software for which a dedicated Makefile
has to be provided.

The SDK includes a target template that allows board Makefile to easily add a
custom target:

~~~~~~~~~~
foo_FILES = bar.bin bar.elf README.foo
$(call generic_target,foo,/path/to/foo)
~~~~~~~~~~

When invoking the generic 'image' Make goal, the SDK will build all targets and
copy them to the $(OUT)/firmware directory.

The SDK already implements most generic build targets for supported SoCs:
- bootloader,
- main core image,
- sensor core image,
- bluetooth core image.

As a consequence, the process of creating a \<board>.mk is greatly simplified:
as a matter of fact, the only requirement to create a minimal Makefile for a
board is to include the common targets for the corresponding SoC.

~~~~~~~~~~
include $(T)/projects/curie_common/build/curie_common_targets.mk
~~~~~~~~~~

### Board configuration

The SDK provides all the generic building blocks to create firmwares for the
supported SoCs.

However, some of them require low-level customizations to take advantage of
 specific board features.

The SDK defines for that purpose configuration variables that take default
values and that can be overridden in the \<board>.mk.

~~~~~~~~~~
# Only 2 Mb of external flash instead of the default 16
SNOR_SIZE ?= 2
include $(T)/projects/curie_common/build/curie_common_targets.mk
~~~~~~~~~~

### Kbuild/Kconfig SDK targets

Some SDK targets (main core, sensor core) are highly configurable thanks to two
mechanims: system hooks and Kconfig rules.

Each \<board>.mk can set a BOARD_PATH variable to specify the location of a
specific directory that contains system hooks and Kconfig rules for the board.

~~~~~~~~~~
BOARD_PATH = $(PROJECT_PATH)/fancyboard/
~~~~~~~~~~

#### Board system hooks

The SDK can abstract some of the board discrepancies using generic configuration
variables, but some are just too complex and require specific code to be written
to override a generic behavior.

These specific pieces of code are referred throughout the SDK as "system hooks".

The BOARD_PATH directory will be added to the source tree when building the SDK
main targets. It must contain a Kbuild.mk file that defines the board specific
system hooks to be compiled.

~~~~~~~~~~
$(BOARD_PATH)/Kbuild.mk:

obj-y += board_shutdown.o panic.o
~~~~~~~~~~

#### Board Kconfig file

When parsing Kconfig rules for the main targets, the SDK will look for a Kconfig
file in the BOARD_PATH directory.

In this Kconfig file, the board can both:
- create new configuration flags to be used in the project or in the board
specific code,
- override default values for existing SDK flags.

Please refer to @ref build_config for the details of the Kconfig syntax.
