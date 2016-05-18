@page curie_create_project Create a new project based on Intel&reg; Curie&trade;

Creating a project Makefile is as simple as including the generic
build/project.mk file in your project Makefile and to define a few variables:
- T should point to the top of the SDK source tree
- OUT should point to the output directory
- PROJECT, BOARD, BUILDVARIANT, as defined in the
  [generic build process](@ref curie_build_project),
- quark_DEFCONFIG, arc_DEFCONFIG, that should point to
  valid [build configurations](@ref build_config) for the main and sensor cores.

The Intel&reg; Curie&trade; SDK already includes support for several generic
boards. Please refer to @ref add_board to add support for your own.

Example projects can be found under the projects directory. Here is the content
of a project Makefile:

~~~~~~~~~~
# Here I assume the SDK is in the project source tree, but you would insert
# something specific to your setup
T = $(PROJECT_PATH)/wearable_device_sw

PROJECT      = curie_custom
BOARD        = curie_custom_board
BUILDVARIANT = debug

quark_DEFCONFIG = $(PROJECT_PATH)/quark/defconfig
arc_DEFCONFIG = $(PROJECT_PATH)/arc/defconfig

include $(T)/build/project.mk
~~~~~~~~~~

Notes:
- as with ANY Makefile, all these variables can be overidden on the command line
- the special PROJECT_PATH variable points to the directory containing your
project Makefile

The project-specific sources are not listed in the project Makefile, but should
instead be organized according to a generic @ref build_kbuild, which root folder
MUST be the directory that contains the project Makefile.

The SDK supports also the addition of custom configuration items using optional
[Kconfig](@ref build_config) extension files.

Example:

~~~~~~~~~~
.
├── Kbuild.mk
├── project.Kconfig
├── services.Kconfig
├── drivers.Kconfig
├── arc
│   ├── Kbuild.mk
│   ├── defconfig
│   └── main.c
├── quark
│   ├── Kbuild.mk
│   ├── defconfig
│   └── main.c
├── curie_custom_board.mk
└── Makefile
~~~~~~~~~~
