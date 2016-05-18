Flashing {#flashing}
========

## Prerequisites

- Make sure that  @ref build_one_time_setup is done and tools are well installed.
- Make sure that the hardware setup related to your [project](@ref projects_list) is in place.

## Flash with Intel Platform Flash Tool GUI

### Flashing Bootloader through JTAG

The following steps allow you to flash the bootloader image. This flashing has to
be done only once.

1. With Intel Platform Flash Tool Lite, in "Flash" tab, select your flash image
  "flash.json".
2. Select "jtag_rom+bootloader" in "Configuration".
3. Once selected, you can flash it using "Start to flash" button (on the
Flyswatter2 Hardware).

![Flash Bootloader using JTAG](@ref flash_local_file.png)

With the new bootloader flashed on the board, you can stop using the Flyswatter2
to flash and use the standard USB port of the board to flash.

### Flashing the whole image through USB

This step will flash the device using the DFU protocol.

1. Select "usb_full" in "Configuration".
2. Flash using "Start to flash" button.

@warning You must flash with "usb_full" in less than 10 seconds after the
first flash. If you are not sure about timing, you can push the reset button
and flash with "usb_full" (less than 10 seconds after the reset).

![Flash Image using DFU](@ref flash_local_file_2.png)

## Flashing from Development Environment using CLI

#### Flashing Bootloader through JTAG

From the output directory:

\code
user@tlsmachine:~/$ make package 
user@tlsmachine:~/$ make flash FLASH_CONFIG=jtag_x86_rom+bootloader
\endcode

#### Flashing the whole image through USB

This command will flash the whole image through the standard USB port of the
board. There is no need of Flyswatter2 box.

First push the reset button of the board then type on the output directory:

\code
user@tlsmachine:~/$ make flash
\endcode

### Infos
#### JTAG based FLASH_CONFIG

JTAG based flash configuration | Description
------------------------------ | -----------
jtag_x86_rom+bootloader        | Flash rom and bootloader

#### USB based FLASH_CONFIG

Note : available 10s after boot.

USB based flash configuration | Description
----------------------------- | -----------
usb_full                      | Flash applications on Intel&reg; Curie&trade; hardware

