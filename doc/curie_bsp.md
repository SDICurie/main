Curie&trade; BSP Reference{#curie_bsp_reference_doc}
=====================

Curie&trade; BSP is a cross-project framework for embedded devices which provides:
 - [Component Framework](@ref cfw) : a high-level component framework for creating and
   interacting with services
 - [Services](@ref services)
    - [Sensor Service](@ref sensor_service)
    - [User Interaction Service](@ref ui_service)
    - [Battery Service](@ref battery_service)
    - [Low Level Storage Service](@ref ll_storage_service)
    - [Properties Storage Service](@ref properties_service)
    - [Circular Storage Service](@ref circular_storage_service)
    - [ADC Service](@ref adc_service)
    - [GPIO Service](@ref gpio_service)
 - [Infrastructure](@ref infra)
    - [IPC](@ref ipc)
    - [Power Management](@ref infra_pm)
    - [Logging](@ref infra_log) / [Panic](@ref infra_panic)
    - [Factory Data](@ref infra_factory_data)
    - [OS abstraction layer](@ref os) allowing to run the same code on several
      operating systems
    - [Test Command Framework](@ref infra_tcmd)
 - [Drivers](@ref drivers)
    - [Device model](@ref infra_device)

### Project samples
 - Several project samples are provided in <em>projects</em> subdirectory.
 - [Presentation](@ref projects)
 - [Build](@ref curie_build_project)

### How Tos

 - [How to add/build a service/driver/board ?](@ref build_config)
 - [How to add a new algorithm in sensors framework ?](@ref how_to_add_new_sensor_algorithm)
 - [How to customize battery properties](@ref how_to_customize_battery_properties)

### Get Started Coding

 - [coding style](@ref coding_style): Before submitting code, make sure it
   complies with our coding style rules
