@page curie_build_project Building a project using the Intel&reg; Curie&trade; SDK

Assuming all [host prerequisites](@ref build_one_time_setup) have been
fulfilled, the process of creating a firmware always includes the following
steps:

1. Create output directory
2. Setup
3. Configuration (optional)
4. Build

Most of these steps actually correspond to GNU make targets that need to be
invoked in sequence, as described in the following paragraphs.

## Create the output directory

First, you need to create the output directory from which all post-setup
commands will be performed.

~~~~~~~~~
mkdir /path/to/out
cd /path/to/out
~~~~~~~~~

## Setup

Then, from the output directory, you invoke setup to define a build configuration.

~~~~~~~~~
make setup -f /path/to/project/Makefile
~~~~~~~~~
or
~~~~~~~~~
make setup -f /path/to/project/Makefile BUILDVARIANT=xx BOARD=yy
~~~~~~~~~

Where xx is _debug_ or _release_, and yy is the name of a board.

The SDK setup will prepare the environment:
- create the build Makefile, including only the relevant targets for the
selected board,
- compile host tools, and
- save setup parameters.

@warning If you want to use a different board, or compile a different build
variant, you need to call _make setup_ again.

### For advanced users: alternative setup invocation

The build/project.mk setup command can actually create the output directory if
you pass it on the command line:

~~~~~~~~~
make setup -C /path/to/project/ OUT=/path/to/out
~~~~~~~~~

## Configuration

Reminder: all configuration commands are available only from the Makefile
generated in the output directory.

The SDK uses a set of Kconfig-based rules to select features to be included in
a build, filtered out by the capabilities of the selected hardware.

Whenever the project build configuration needs to evolve or when you
want to test a specific configuration, you can customize the build using the
Kconfig front-end tool.

~~~~~~~~~
make XX_menuconfig
~~~~~~~~~

Where X is one of:
quark (core), arc (sensors), ble_core (ble)

Example:

~~~~~~~~~
make quark_menuconfig
~~~~~~~~~

Custom configurations can be saved and reused using the following commands:

~~~~~~~~~
make TARGET_savedefconfig
~~~~~~~~~

These options can be extended by the project with custom Kconfig files located
in the [project source tree](@ref build_kbuild).

Refer to @ref build_config for a detailed explanation of how to define a
custom build configuration.


## Build

Reminder: all build commands are available only from the Makefile generated in
the output directory.

~~~~~~~~~
make package
~~~~~~~~~

This will build the project and generate a flashable package. Refer to
the [flashing guide](@ref flashing) to flash this package on the target.
