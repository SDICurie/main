@defgroup hello curie_hello
@{
@ingroup projects_list

### Hardware Target

This project runs on Arduino 101 (AKA Genuino 101) board only.

### Features

curie_hello is a simple application that:
- displays an "Hello" log from ARC processor every 5s on UART console
- displays an "Hello" log from Quark processor every 5s on UART console
- refreshes the watchdog every ~30 seconds
- toggles the on-board LED with a frequency depending on the voltage put on the 
  AD0 analog input

curie_hello shows the basic use of the @ref services infrastructure:
- the @ref gpio_service server is running on Quark
- the @ref gpio_service client is running on Quark
- the @ref adc_service server is running on ARC
- the @ref adc_service client is running on Quark

### Compile

Assuming @ref build_one_time_setup have been fulfilled.

The project is built 'out of tree'. First a build folder needs to be created (anywhere in the file-system).
Better not to create this folder inside source tree.

The build is done in two steps:
- setup: this prepares the project to the build (create the Makefile, parse the
  project configuration, ...)
- package: the actually creates the set of binary and the files needed to flash
  the board.

\code
user@tlsmachine:~/Curie_BSP$ mkdir build 
user@tlsmachine:~/Curie_BSP$ ls

build/ wearable_device_sw/

user@tlsmachine:~/Curie_BSP$ cd build
user@tlsmachine:~/Curie_BSP/build$ make -f ../wearable_device_sw/projects/curie_hello/Makefile setup
Output directory: /home/user/Curie_BSP/build
Installing new setup
   PROJECT       : curie_hello
   VERSION       : 1.0.0
   BOARD         : curie_101
   BUILDVARIANT  : debug
   BUILD_TAG     : custom_build_user@20160510173802
   BUILD_TYPE    : custom
   WORKWEEK      : 
   BUILDNUMBER   : 

[tMK] Creating /home/user/Curie_BSP/build/Makefile
Done setup

user@tlsmachine:~/Curie_BSP/build$ make package -j16
[...]
user@tlsmachine:~/Curie_BSP/build$ ls
arc       bootloader   build_setup.json  firmware  package_prefix.txt  pub tools
ble_core  bootupdater  cos               Makefile  packages            quark
user@tlsmachine:~/Curie_BSP/build$ 

\endcode

At the end of the build process, the folder 'firmware' contains everything
needed to be flashed on the board. The folder 'pub' contains the same files but
packaged in a ZIP file.

###Generate the documentation

To create the documentation, in the build directory:

\code
user@tlsmachine:~/Curie_BSP/build$ make doc
\endcode

To see it in your favorite browser:
\code
user@tlsmachine:~/Curie_BSP/build$ make doc_view
\endcode

### Flash

Before flashing the compiled project, you need to prepare your @ref arduino_101_hw_setup.
The hardware setup is the same as the one required by the [Zephyr Project]
(https://www.zephyrproject.org/doc/board/arduino_101.html). 

\warning Before flashing your Arduino 101 board, make sure you did a backup of
your original firmware.

\warning If you already flashed a Zephyr application, that means you have have a backup of the 
original Arduino 101 software. To reflash back this backup, use the method provided
by the [Zephyr Project](https://www.zephyrproject.org/doc/board/arduino_101.html#restoring-a-backup).


To flash the board, see @ref flashing.

@}
