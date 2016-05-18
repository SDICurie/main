@page bootloader_doc Bootloader

The bootloader is split into 2 sub-modules:
- the First Stage Bootloader (FSBL)
- the Second Stage Bootloader (SSBL)

## First Stage Bootloader

This is the first software to run at boot.
It can be flashed through JTAG or USB.
It includes an XMODEM layer that allows to update/recover the SSBL though UART.

In case of reset it provides resume capabilities:
- run SSBL in case of hardware reset
- run Main application in case of wake up from deep sleep.

## Second Stage Bootloader

At boot it is running after the FSBL.
It is responsible for:
- checking the integrity of the main application image (optional)
- checking the battery level to ensure the main application can run (optional)
- updating the main application through USB/DFU or OTA
- gathering panic information and storing in panic partition in SPI flash.
- booting the main application

## Shared memory with main application

The bootloader publishes some data in a dedicated shared volatile memory:
- FSBL can share function pointers to micro-ecc and sha256
- SSBL can share hardware description/abstraction (ex: board features)
