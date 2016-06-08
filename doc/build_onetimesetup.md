@page build_one_time_setup One time setup

Prior to using the SDK, you must make sure your host has all the prerequisites.

## Toolchain

The toolchain to build and debug the Curie BSP is available at FIXME.

You can download and uncompress the downloaded tarball anywhere in the file system, provided you have
the rights.

You need to make the SDK build system aware of it by setting the environment
variable TOOLCHAIN_DIR.

For example, if the toolchain is put in /opt/toolchain_curie/:
\code
user@tlsmachine:~/$ export TOOLCHAIN_DIR="/opt/toolchain_curie/"
\endcode

As this environment variable set is necessary to build and flash the SDK, it is
recommended to put the 'export' in the bashrc:
\code
user@tlsmachine:~/$ echo "export TOOLCHAIN_DIR=/opt/toolchain_curie/" >> ~/.bashrc
\endcode


## Install Platform Flash Tool Lite
The SDK requires Intel® Platform Flash Tool Lite to flash the device.
This tool is available at
https://01.org/android-ia/downloads/intel-platform-flash-tool-lite

Download the version for Ubuntu and install it.

## Install the SDK

Create the working directory, for example in your home directory:
\code
user@tlsmachine:~$ mkdir Curie_BSP
user@tlsmachine:~$ cd Curie_BSP/
\endcode

Get the SDK:
\code
TODO: Get sources
user@tlsmachine:~/Curie_BSP$ ls
wearable_device_sw
user@tlsmachine:~/Curie_BSP$
\endcode

## Prepare the system

To build and flash, the SDK needs some system pre-requisites:
- Link Platform Flash Tool Lite to the toolchain
- Create udev rules to link Platform Flash Tool Lite and the JTAG box
  (Flyswatter2)
- Install necessary build dependencies.

A special target available on the SDK common targets has been created for that
purpose. It must be run only once, with superuser privileges:

\code
user@tlsmachine:~/Curie_BSP$ make -C wearable_device_sw/projects/curie_hello/ one_time_setup
\endcode

At this point all the software pre-requisites are met and you can start
hacking.

## Build the project

To build the sample project, see [Curie Hello Sample](@ref hello).

To read general documentation about build, see
[Build a Intel Curie-based Project](@ref curie_build_project).

