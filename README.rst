Intel® Curie BSP
################

Intel® Curie BSP is the SDK that will help you developing software on Curie based boards, for
example with the Arduino 101 board (AKA Genuino 101).

For now Curie BSP is compatible with the Arduino 101 board. The environment supports only the
OS Ubuntu GNU/Linux as host.

ONE TIME SETUP
**************
FIXME: update it with final setup once it's defined.

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

``repo init -u <Curie BSP manifest URL>``

Download the sources files:

``repo sync -j 5 -d``

Initialize the environment:

``sudo make -C wearable_device_sw/projects/curie_hello/ one_time_setup``

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

``make flash``

GENERATE DOCUMENTATION
**********************

To generate the documentation in the "doc" directory you can type:

``make doc``

Or you can generate and browse it directly using the default browser:

``make doc_view``
