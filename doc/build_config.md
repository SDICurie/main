@page build_config Defining a custom build configuration

The SDK exposes a set of features to be selected based on the actual hardware
capabilities of each platform.

The consistency of build configurations is guaranteed by rules written in the
[Kconfig language](https://www.kernel.org/doc/Documentation/kbuild/kconfig-language.txt).

## Configuration interface

The build configuration can be customized through a _curses_ interface launched
by the menuconfig target:

    make TARGET_menuconfig

Where TARGET represents the image you want to build (typically a core firmware).

Examples:

    make quark_menuconfig

Main keystrokes used in the interface:
- Left, Right, Up, Down : navigate,
- y: select an option,
- n: unselect an option,
- Space: toggle an option,
- Enter: activate commands at the bottom,
- ?: Get help on the current symbol,
- /: Search a symbol.

The main configuration menu comprises five items:

- "Project" contains project specific options
- "Services" contains the basic application toolkit, ie high-level utility
modules that abstract out the features of the SoC,
- "Packages" contains optional modules that extend the SDK,
- "Advanced" and "Debug" settings allow deep-down configuration of the build,
requiring a high level of understanding of the hardware platform.

Note that configurations defined using the _curses_ interface apply only to the
current build: they will be reset if you run the project setup again.

If you want to permanently save a build configuration, you need to call the
special savedefconfig command:

    make TARGET_savedefconfig

## Basic configuration (for application developers)

The application developers should never need to go further than the three first
menu items in the main configuration menu, assuming that they are using a
predefined project setup provided by an integrator (see the "Advanced
configuration" paragraph).

### Defining and setting project options

The first menu item is a placeholder for the project specific configurations.

It contains project-specific configuration flags defined in the optional
project.Kconfig file located at the top of the project source tree (see the
"Extending build configuration" paragraph).

### Configuring Services

This menu allows application developers to select the @ref services they want
to activate:
- BLE,
- Storage,
- User interface ...

Services can typically be distributed on multiple cores, and you are therefore
given the choice to select:
- a client (API) module,
- a server (Implementation) module.

Client modules can be compiled on every core, because they don't have any
hardware dependencies, thanks to the underlying service IPC mechanism.

Implementations on the other end are only available on some cores due to
specific hardware dependencies: for instance, the sensor service implementation
is only available on the core that controls the sensor subsystem.

Needless to say, it is up to you to make sure that if you select a service API
on one core, you need to make sure the corresponding implementation is available
somewhere on another core.

### Adding optional packages

The "Packages" menu gives access to optional modules contained in the SDK
packages directory: depending on your local setup, your mileage may vary.

## Advanced configuration (for project integrators)

The SDK allows integrators to define new project setups for each core of a given
board.

### Creating a custom core configuration

Custom core configurations are done using low-level options gathered under the
"Advanced Settings" menu.

#### Select the Zephyr variant

In a first step, select the Zephyr OS flavor you want to use: Micro or
Nano (please refer to Zephyr documentation for details).

#### Select Drivers

The Drivers menu contains all drivers maintained in Curie&trade; BSP.

Selecting the proper driver is a prerequisite to allow services built on top of
it to be selected.

Please note that some drivers may not be available for all board/soc/core
combinations.

It is recommended to select by default all drivers that are available for a
given board, letting the application developers unselect them if they are not
used.

#### Select default services

Probably the most important step from an application development perspective:
select the exhaustive list of @ref services the applications may require under
the "Services" menu.

It is important that all services that can be later selected by application
developers be activated by default during the initial setup, because it will
allow the early identification of missing driver prerequisites.

For services that are part of the basic application toolkit, you will have hints
in the configuration menus of the missing dependencies: be aware however that
identifying and selecting all prerequisites for a service is an iterative
process.

Example:

    The User interface service relies on Led drivers.
    The specific Led driver for the board relies on a specific I2C Bus.
    The I2C Bus is available on a specific core.

    Therefore, you will only be able to select the User interface service
    implementation on that specific core, and only once you have selected
    the I2C Bus and the Led driver in that order.

#### Further customizations

The last step consists in customizing the common features the SDK provides (most
of them are located under the "Infra" menu):

- log backends,
- @ref infra_tcmd,
- ...

### Extending the build configuration

The SDK supports several reference boards with their associated drivers, and a
simple extension mechanism should a project need to define its own board or
simply add its own set of drivers.

The SDK set of utility services can also be extended using a similar mechanism.

#### Adding new board specific rules

As explained in @ref add_board, the SDK Kconfig rules can be extended or
overridden by board specific rules.

Extending Kconfig rules means adding additional board specific configuration
flags using the Kconfig syntax.

~~~~~~~~~~
config FANCY_BOARD_USE_LPM
	bool "Use Fancy board low-power-mode"
	default y
~~~~~~~~~~

Overriding existing flags is a bit trickier, as you need to redefine the
corresponding flags with the same type, changing only their default value.

~~~~~~~~~~
config QUARK_SE_USE_INTERNAL_OSCILLATOR
	bool
	default y
~~~~~~~~~~

#### Adding a new service

When parsing service rules, the SDK will look for a services.Kconfig file at the
root of your project directory.

Any service defined in this file will be automatically added to the "Services"
menu.

To define a service, you just need to create a configuration entry for it.
It is nevertheless advised to split the service between a client (API) and a
server (Implementation).

As specified before, services implementations typically depend on the
availability of a driver giving access to the corresponding hardware feature.

Example:

    menu "Heart rate feedback panel"

    config SERVICE_HEART_RATE_PANEL
		bool "Client"

    config SERVICE_HEART_RATE_PANEL_IMPL
		bool "Server"
		depends on DISPLAY

The actual source code for this new service must be available somewhere in the
@ref build_kbuild.

#### Adding a new driver

When parsing driver rules, the SDK will look for a drivers.Kconfig file at the
root of your project directory.

Any driver defined in this file will be automatically added to the "Drivers"
menu.

To define a driver, you just need to create a configuration entry for it.
Drivers typically enable generic hardware features required by the services.

Example:

	config ULPT43PT
		bool "Display driver for ULPT43"
		select DISPLAY

The actual source code for this new driver must be available somewhere in the
@ref build_kbuild.

#### Adding other project specific flags and rules

For any other flag than services or drivers, the SDK will look for a
project.Kconfig file at the root of your project directory.

You can add any new configuration items you may require there, including for
instance subprojects selectors to select the application code (the "main") that
should be used for each core.

All these new items will be located at the bottom of the front configuration
page. It is advised to gather all your items in a menu and giving it a
meaningful title.

Example:

	menu "Project XX"
	...
	config ...

	endmenu
