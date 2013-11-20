Userspace Software
===

This is the collection of userspace tools provided for the cape. To build the software change to the software directory and type:

make

This will create the file
* gpmc_tool
* bist

BIST
=

BIST is an interactive console application allowing you to run tests on the board and peek/poke registers

GPMC_TOOL
=

GPMC_TOOL is an application that configures the gpmc controller after startup. Any application using the API including BIST can also serve this purpose. 
Running this tool or another app which configures gpmc is necessary before loading the kernel drivers. 
l
