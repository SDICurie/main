Debug / Logs Display {#curie_debug}
====================

## Debug

There are two ways to debug your application: either using the console, or using
your favorite IDE.

### Debug from the console

A debug_console_xx makefile target is provided to start a GDB server and
connect a console client on it:

For debugging the Quark, type from the output directory:

    make debug_console_qrk

or for debugging the ARC:

    make debug_console_arc

In this later case, the program execution will stop for further debugging when
entering the arc main() function.

You can also optionally replace the default ELF file with a custom one, e.g:

    make debug_console_qrk ELF=path/to/curie_hello/firmware/ssbl_quark.elf

### Debug from an IDE

OpenOCD can be used to connect to the JTAG interface of the
SoC to allow debugging. To do that you can type

    make debug_start

That will launch the OpenOCD binary. You will then be able to connect to the different servers:
    - Telnet to localhost 4444 will connect to the OpenOCD shell
    - ARC GDB to localhost 3334
    - x86 GDB to localhost 3333

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

