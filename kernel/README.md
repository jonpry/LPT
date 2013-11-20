LPT Cape Kernel Patches
===

The kernel patch is known to apply cleanly to the 3.8.13xenomai-bone28.1 branch which is setup from https://github.com/cdsteinkuehler/linux-dev
The patch should work without trouble on any relatively modern linux. 

As things currently are, you *must* build all of the parallel components at runtime as the userland helper needs to be run to setup GPMC before 
the parallel driver is loaded. 

Add the following options to your kernel config after applying the patch:

CONFIG_PARPORT=M
CONFIG_PARPORT_BONE=m
CONFIG_PARPORT_1284=y
CONFIG_PRINTER=m
CONFIG_PPDEV=m

A defconfig is included that may suit your purposes if you are running stock 3.8.13xenomai-bone28.1. 