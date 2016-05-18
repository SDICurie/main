@defgroup projects_list Project list
@{
@}
@page projects_doc Projects
@{
@anchor projects

## Sources Tree

Several project samples are available in _projects_ directory.

Each project includes at least:
\code
+- Kbuild.mk
+- Makefile
+- arc
 +---- defconfig
 +---- KBuild.mk
 +---- main.c
 +---- memory_pool_list.def
+- include
 +---- project_mapping.h
+- quark
 +---- defconfig
 +---- KBuild.mk
 +---- main.c
 +---- memory_pool_list.def

+- doc.mk
+- doc
 +---- project.md
\endcode

- _include/project_mapping.h_ defines the memory partionning.
- _defconfig_ files provides the configuration for each CPU (ARC, QUARK, BLE).
- _memory_pool_list.def_ files defines the pools of buffers used on ARC & QUARK.
- _main.c_ files include the main entry point for ARC and Quark

## Considerations before starting a new project

### Code size and location

Default partitioning and size allocated to Quark and ARC codes can be updated to fit project needs.

For more information see @ref flash_partitioning "Non Volatile Memory partitioning".

### Volatile memory partitioning

Default partitioning and RAM size allocated to Quark and ARC cores can be updated to fit project needs.

This RAM area includes all the stacks, variables, and pools of buffers. Pools of buffers can be updated in `memory_pool_list.def` project file.

For more information see @ref ram_partitioning "Volatile Memory partitioning".

### Configuration

The list of drivers, services, and packages enabled on each core must be configured through `menuconfig` for each core. See @ref build_config.

### Tasks and Fibers

#### Quark

By default one main task is running on Quark. It initialises all the system and may manage the infinite loop to process the messages.

Depending on the build configuration, some other tasks may be automatically created:
- `OS_TASK_TIMER` is dedicated to @ref os_time_timer "timers". It is always enabled.
- `TASK_WORKQUEUE` is dedicated to @ref workqueue "deferred works". It is enabled through `CONFIG_WORKQUEUE` build option.
- `TASK_STORAGE` is dedicated to storage in non-volatile memory. It is enabled through `CONFIG_STORAGE_TASK` build option. This task is automatically enabled by any storage service (@ref ll_storage_service, @ref properties_service, @ref circular_storage_service)
- `TASK_LOGGER` is dedicated to @ref infra_log in circular buffer. It is enabled through `CONFIG_LOG_CBUFFER` build option.

#### ARC

Only one main task is running on ARC. It initialises all the system and may manage the infinite loop to process the messages.

Depending on the build configuration, some fibers may be automatically created:
- Timer fiber is dedicated to @ref os_time_timer "timers". It is always enabled.
- Workqueue fiber is dedicated to @ref workqueue "deferred works". It is enabled through `CONFIG_WORKQUEUE` build option.
- Log fiber is dedicated to @ref infra_log in circular buffer. It is enabled through `CONFIG_LOG_CBUFFER` build option.
- Sensor fiber is dedicated to @ref sensor_core "sensor core". It is enabled through `CONFIG_SENSOR_CORE` build option.

\subpage curie_build_project <br>
\subpage flashing <br>
\subpage curie_create_project <br>
\subpage build_config <br>
\ref projects_list

@}

