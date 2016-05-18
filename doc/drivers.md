@defgroup drivers Drivers
@{

Quark SE drivers are divided into two categories:
- the peripherals accessible only through the ARC processor; \ref arc_drivers
- the peripherals accessible from any processor; \ref soc_drivers

Some other drivers managing external components are provided. \ref ext_drivers

See \ref infra_device for device tree management, driver structure, and power management.

Driver headers are located in `bsp/include/drivers`.<br>
Driver sources are located in `bsp/src/drivers`.<br>

Each driver can be enabled/disabled through `Advanced Settings/Drivers` configuration menu.

@}
@defgroup common_def Common Definitions
@{
@ingroup drivers
@}
@defgroup soc_drivers SOC Drivers
@{
@ingroup drivers
@}
@defgroup arc_drivers Sensor Subsystem Drivers
@{
@ingroup drivers
Sensor Subsystem (ARC) drivers API accessible only through the ARC processor
@}
@defgroup ext_drivers Other Drivers
@{
@ingroup drivers
@}
