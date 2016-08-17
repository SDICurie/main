SDI version of Intel® Curie BSP

################
This Curie BSP is the forked version of Intel Curie BSP plus some add-on features from Intel Smart Device Innovations team. 
More introduction to the original Intel BSP is here: https://github.com/CurieBSP/main/blob/master/README.rst.

This Curie BSP doesn't intend to replace the original Intel BSP. The reason to keep this one is to make it possible 
and flexible to add some additional features for some customized projects while still keeping the original BSP 
as much as possible. 

ONE TIME SETUP
**************

Curie BSP needs a set of external projects to be built. The repo tool is currently used to fetch
all the needed repositories.

Setup repo:

``mkdir ~/bin``
``wget http://commondatastorage.googleapis.com/git-repo-downloads/repo -O ~/bin/repo``
``chmod a+x ~/bin/repo``

In ~/.bashrc add:

``PATH=$PATH:~/bin``

Create your directory (eg. Curie_BSP):

``mkdir Curie_BSP && cd $_``

Initialize your repo:

``repo init -u https://github.com/SDICurie/manifest``

Download the sources files:

``repo sync -j 5 -d``

Initialize the environment:

``make -C wearable_device_sw/projects/curie_hello/ one_time_setup``


BUILD AND FLASH A PROJECT
*************************

Curie BSP must be built out-of-tree, in a dedicated output directory whose location is chosen
by the end user.

The recommended procedure during development phase is the following:

Prepare your build environment:

``mkdir -p ./out``
``cd ./out``

Setup your project, for example based on the curie_hello project:

``make -f ../wearable_device_sw/projects/curie_hello/Makefile setup``

Compile:

``make image -j 32``

Flash:

You first need to flash the bootloader on the board using JTAG. This have to be done only once:

``make flash FLASH_CONFIG="jtag_x86_rom+bootloader"``

Then you can flash your board using the standard USB port:

``make flash``
then press the reset button on the board to start the flashing.

GENERATE DOCUMENTATION
**********************

To generate the documentation in the "doc" directory you can type:

``make doc``

Or you can generate and browse it directly using the default browser:

``make doc_view``
