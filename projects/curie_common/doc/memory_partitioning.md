@page memory_partitioning Memory partitioning

# Quark SE

Quark SE main processors (Quark and ARC) are sharing the same on-die non-volatile memory for code/data and SRAM for volatile data, and optionally an SPI flash for storing user-data.\n
BLE chip has its own dedicated non-volatile memory and SRAM.

@anchor flash_partitioning
## Non Volatile Memory
### Partition Types and Access
#### OTP Partitions
Partition | Purpose | Firmware Update Behaviour | Factory Reset Behaviour
:--- | --- | --- | --- |
OTP Code/Data | The one-time-programming memory area for boot code/data| Can't be updated (1) | Can't be erased(1)
Intel-OTP Provisioning Data | Contains data that identifies the Intel&reg; Curie&trade; Module, Keys... and in general all data that shall not be updatable (@ref oem_data)| Can't be updated (1) | Can't be erased(1)
Customer-OTP Provisioning Data | Contains data that identifies the Customer product, Keys... and in general all data that shall not be updatable (@ref customer_data)| Can't be updated (1) | Can't be erased(1)

Provisioning Data is also referenced as ROM data.

#### Other Non-Volatile-Memory Partitions
Partition | Purpose | Firmware Update Behaviour | Factory Reset Behaviour
:--- | --- | --- | --- |
Bootloader 	| It holds bootloader code / data | Not updated	| Not updated
Recovery	| Not yet supported		| | |
Firmware	| Application/services/os/drivers code&data (both for ARC and Quark)	| Updated	| Not updated
Factory Reset Persistent Runtime Data	| Partition to store data that are updated at runtime by the device, accessible over Properties Storage Service, but not reset during Factory Reset procedure	| Not updated	| Erased
Factory Reset Non Persistent Runtime Data	| Partition to store data that are updated at runtime by the device, accessible over Properties Storage Service, but reset during Factory Reset procedure	| Not updated	| Not erased
Factory Settings	| Contains factory default settings	| Not updated	| Not erased
UserData (0..n)	| User-Data partition (multiple different partitions can be supported)	| Not updated	| Erased


(1) Can't be erased/updated if the OTP bit has been set

#### Memory layout without external SPI flash

![memory layout without external spi flash Figure](memory_layout_without_external_spi_flash.png "Memory layout without external spi flash")

#### Memory layout with external SPI flash

![memory layout with external spi flash Figure](memory_layout_with_external_spi_flash.png "Memory layout with external spi flash")

HW Config Block (first part of the firmware flash block)
 - Start address of ARC code
 - Start address of the context restoration procedure (for wake from deep sleep) for ARC
 - Start address of QRK Code
 - Start address of the context restoration procedure (for wake from deep sleep) for QRK
 - SHA1 of the eventual part of FW that goes beyond 192kB

### Partition Definition
A default layout is defined in <tt>bsp/include/machine/soc/intel/quark_se/quark_se_mapping.h</tt>.\n
This layout can be overriden in the project (<tt>\<project\>/include/project_mapping.h</tt>)

A section is defined for each flash partition in embedded or external SPI flash. It includes:
- the partition identifier
- the flash component
- the start & end blocks

Each section should be declared in <tt>bsp/src/machine/soc/intel/quark_se/quark/soc_config.c</tt> in the _storage_configuration[]_ table. The type of persitency on factory reset must also be specified.

The on-die flash is 384 kBytes. Page size is 2048 bytes. Default partitioning is:
- 60 kB (30 blocks) for bootloader code
- 4 kB (2 blocks) for debug panic data
- 144 kB (72 blocks) for Quark code
- 152 kB (76 blocks) for ARC code
- 6 kB (3 blocks) for factory data non persistent to factory reset
- 6 kB (3 blocks) for factory data persistent to factory reset
- 8 kB (4 blocks) for application data
- 4 kB (2 blocks) for settings

When an SPI flash is available, default partitioning is:
- 1012 kB for FOTA and log
- 1024 kB for application data
- 12 kB for system events

@anchor ram_partitioning
## Volatile Memory

Quark SE is featured with 80 kB of on-die SRAM shared by Quark and ARC processors. By default is SRAM is partitionned as follows:
- 1 kB of data shared between Quark and ARC
- 2 kB of data shared between bootloader and Quark
- 53 kB for Quark
- 24 kB for ARC
The default layout is defined in <tt>bsp/include/machine/soc/intel/quark_se/quark_se_mapping.h</tt>.\n
This layout can be overriden in the project (<tt>\<project\>/include/project_mapping.h</tt>)

### Memory pools

Dynamic memory allocation/free is based on memory pools. See \ref os_mem_alloc for API.
Part of SRAM used by Quark or ARC is organized in pools of fixed size.

Pools are defined in the project for each core in _memory_pool_list.def_.\n
The <tt>DECLARE_MEMORY_POOL(index, size, count)</tt>  macro defines a pool of _count_ buffers of _size_ bytes.

ex: <tt>DECLARE_MEMORY_POOL(0,8,32)</tt> defines a pool of 32 buffers or 8 bytes.

When an allocation is requested for size _sz_, the allocator looks for a free buffer of size >= _sz_. The search is not limited to the pool of the closest size but to any pool of higher size.

Some statistics on the pools usage can be enabled to monitor and optimize the pools definition.
