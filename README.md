Karltech LPT-Cape
===

   This repository contains all documentation and software regarding the Karltech LPT-Cape. 
There are several subdirectories containing their own README files. Depending on your use case, 
some of the tools may not be relevant. For example, if you are doing CNC or otherwise banging 
the I/O from userspace, it is probably not necessary to use the kernel driver at all. So please
make sure you are not doing unnecessary work. 

   Subdirs:

* /kernel - Kernel driver patches and configs - Used for creating /dev/lp0 which is used by many userland parallel applications. 
* /dtbs - Device tree overlays
* /sw - Userland software tools for setting up GPMC, bit banging and BIST

LICENSING
==

   The various components of this project are licensed under different terms. The various subdirectories and files
should include information about the separate licenses. The general scheme is that the kernel drivers are GPLv2. All
other software components are BSD compatible. All documentation and schematics are CC-BY-SA 3.0. 

		
