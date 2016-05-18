Debug / Logs Display {#curie_debug}
====================

## Debug
Support is only for Quark for now.

Two ways : either using the console, or using IDE you wish.

You can replace the ELF with app (firmware) below.


### Start server and console client

From the output directory:

On Quark

    make debug_console_qrk ELF=./firmware/quark.elf

On ARC

    make debug_console_arc ELF=./firmware/arc.elf

### Start OpenOCD and GDB server

OpenOCD can be used to connect to the JTAG interface of the
SoC to allow debugging. To do that you can type

    make debug_start

That will launch the OpenOCD binary. You will then be able to connect to the different servers:
    - Telnet to localhost 4444 will connect to the OpenOCD shell
    - ARC GDB to localhost 3333
    - x86 GDB to localhost 3334

From the output directory:

    make debug_start ELF=./firmware/quark.elf

## Logs Display

The UART console backend should be enabled. For more information see @ref console_manager.

### Install screen tool

    sudo apt-get install screen

### Launch command

    screen /dev/ttyUSB1 115200

### Output examples

    2502|QRK|FG_S| INFO| fg_timer started
    2502|QRK|FG_S|ERROR| terminate voltage too low 0 now is 3200
    2220|ARC|SS_S| INFO| [arc_svc_client_connected]18
    2430|ARC|SS_S| INFO| [ss_svc_msg_handler]is subscribing
    2675|QRK|CHGR| INFO| CHARGER State : CHARGE COMPLETE
    2430|ARC| PSH|ERROR| ([sensor_register_evt]ms=2430,sid=0): sensor_register

### Exit screen tool
Kill the log window with the command:

    CTRL^A K

## Panic

### How to decode

For more information see @ref panic_dump.

To decode an address, you can use `addr2line` tool.

    addr2line -pifa -e quark.elf 0x4001ed30

### Dump panic partition

Panic information is automatically stored in embedded flash in a dedicated partition visible through DFU.
As DFU is only available during a few seconds at startup, the board has to be restarted.

To retrieve and parse panic:
- Check alternate exposed with `dfu-util -l`. You should see a partition named panic.
- Dump it locally with `dfu-util -a panic -U panic.bin`
- Parse it with DecodePanic.py Python script: `DecodePanic.py <elf_file> panic.bin`

