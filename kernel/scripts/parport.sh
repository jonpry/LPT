#!/bin/sh
#assumes all modules are in current directory, gpmc_tool in path
gpmc_tool
insmod parport.ko
#lpt0 and lpt1
insmod parport_bone.ko io=0x10000F0,0x1000178 delay=1000
#lpt0 only
#insmod parport_bone.ko io=0x10000F0 delay=1000
#lpt1 only
#insmod parport_bone.ko io=0x1000178 delay=1000

insmod ppdev.ko
insmod lp.ko


