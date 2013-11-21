LPT Cape Kernel Patches
===

The kernel patch is known to apply cleanly to the 3.8.13xenomai-bone28.1 branch which is setup from https://github.com/cdsteinkuehler/linux-dev
The patch should work without trouble on any relatively modern linux. 

As things currently are, you *must* build all of the parallel components as modules as the userland helper needs to be run to setup GPMC before 
the parallel driver is loaded. 

Add the following options to your kernel config after applying the patch:

* CONFIG\_PARPORT=m 
* CONFIG\_PARPORT\_BONE=m 
* CONFIG\_PARPORT\_1284=y 
* CONFIG\_PRINTER=m 
* CONFIG\_PPDEV=m 

A defconfig is included that may suit your purposes if you are running stock 3.8.13xenomai-bone28.1. 

The scripts directory includes a script that will load the modules required for use as a standard printer port. 
This script assumes the modules are located in the current directory and the gpmc_tool has been installed to the path. 
If the modules are installed normally, edit the script to use modprobe instead. 

