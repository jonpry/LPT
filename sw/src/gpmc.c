/*
Copyright (c) 2013, Jon Pry <jonpry@gmail.com>

All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
* Neither the name of the <ORGANIZATION> nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/

#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "gpmc.h"

#define GPMC_BASE 0x50000000

#define GPMC_CHIPSELECTCONFIGDISPLACEMENT (0x30 / 4)

#define GPMC_SYSCONFIG (0x10 / 4)

#define GPMC_CONFIG1 (0x60 / 4)
#define GPMC_CONFIG2 (0x64 / 4)
#define GPMC_CONFIG3 (0x68 / 4)
#define GPMC_CONFIG4 (0x6c / 4)
#define GPMC_CONFIG5 (0x70 / 4)
#define GPMC_CONFIG6 (0x74 / 4)
#define GPMC_CONFIG7 (0x78 / 4)

#define CONFIG1 ((1<<29) | (1<<27) | (1<<25) | (1<<12) | (1<<9) | 3) //No burst, sync, 2 fclk till rising edge, pseudo 16-bit, A/D multiplexed, clk = fclk/4, 25mhz?
#define CONFIG2 0x00080B00 //deassert wr CS on fclk 8, rd on fclk 12, assert on 0
#define CONFIG3 0x00040400 //Deassert adv on 4
#define CONFIG4 0x08040B04 //Deassert we on 8, assert on 4, deassert oe on 12, assert on 4
#define CONFIG5 0x040A080B //4 cycle delay, 11 cycle dvalid, 8 clock to write,  12 clocks to read
#define CONFIG6 0x040404C4 //4 clock turnaround
#define CONFIG7 0x00000f41 //CS0: Set base address 0x0100 0000, 16MB region, and enable CS

static int devmemfd = -1;

//map memory of length "len" at offset "offset"
static void* util_mapmemoryblock(off_t offset, size_t len)
{
	devmemfd = open("/dev/mem", O_RDWR | O_SYNC);
	void* registers = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, devmemfd, offset);
	if (registers == MAP_FAILED) {
		printf("Map failed\n");
	}
	return registers;
}

static void util_unmapmemoryblock(void* block, size_t len)
{
	munmap((void*) block, len);
	if (devmemfd != -1) {
		close(devmemfd);
	}
}

#define REGLEN 0x10000000

static volatile uint32_t* registers = NULL;

static void gpmc_mapregisters()
{
	registers = (uint32_t*) util_mapmemoryblock(GPMC_BASE, REGLEN);
}

static void gpmc_unmapregisters()
{
	util_unmapmemoryblock((void*) registers, REGLEN);
}

void gpmc_setup(void)
{
	gpmc_mapregisters();

	if (registers != MAP_FAILED) {
		int chipselect = 0;
		int displacement = GPMC_CHIPSELECTCONFIGDISPLACEMENT * chipselect;

#ifdef DEBUG
		printf("SYS: 0x%8.8X\n", *(registers + GPMC_SYSCONFIG));

		printf("1: 0x%8.8X\n", *(registers + displacement + GPMC_CONFIG1));
		printf("2: 0x%8.8X\n", *(registers + displacement + GPMC_CONFIG2));
		printf("3: 0x%8.8X\n", *(registers + displacement + GPMC_CONFIG3));
		printf("4: 0x%8.8X\n", *(registers + displacement + GPMC_CONFIG4));
		printf("5: 0x%8.8X\n", *(registers + displacement + GPMC_CONFIG5));
		printf("6: 0x%8.8X\n", *(registers + displacement + GPMC_CONFIG6));
		printf("7: 0x%8.8X\n", *(registers + displacement + GPMC_CONFIG7));
#endif
		//Check to see if gpmc is already initialized
		if(	*(registers + displacement + GPMC_CONFIG1) != CONFIG1 ||
			*(registers + displacement + GPMC_CONFIG2) != CONFIG2 ||
			*(registers + displacement + GPMC_CONFIG3) != CONFIG3 ||
			*(registers + displacement + GPMC_CONFIG4) != CONFIG4 ||
			*(registers + displacement + GPMC_CONFIG5) != CONFIG5 ||
			*(registers + displacement + GPMC_CONFIG6) != CONFIG6 ||
			*(registers + displacement + GPMC_CONFIG7) != CONFIG7 ) {
	
			//Reset the module, TODO: is this needed?
			*(registers + GPMC_SYSCONFIG) = 0x12;
			*(registers + GPMC_SYSCONFIG) = 0x10;

			// disable before playing with the registers..
			*(registers + displacement + GPMC_CONFIG7) = 0x00000000;
			*(registers + displacement + GPMC_CONFIG1) = CONFIG1;
			*(registers + displacement + GPMC_CONFIG2) = CONFIG2;
			*(registers + displacement + GPMC_CONFIG3) = CONFIG3;
			*(registers + displacement + GPMC_CONFIG4) = CONFIG4; 
			*(registers + displacement + GPMC_CONFIG5) = CONFIG5;
	
			*(registers + displacement + GPMC_CONFIG6) = CONFIG6;
			*(registers + displacement + GPMC_CONFIG7) = CONFIG7;
		}
		gpmc_unmapregisters();
	}
}

volatile uint8_t* extbus;

void bus_init() {
	gpmc_setup();
	extbus = (uint8_t*) util_mapmemoryblock(0x01000000, 0x200);
}

void bus_writebyte(uint8_t address, uint8_t data) {
	*(extbus + (address<<1)) = data;
}

uint8_t bus_readbyte(uint8_t address) {
	return *(extbus + (address<<1));
}

void bus_shutdown() {
	util_unmapmemoryblock((void*) extbus, 0x200);
}


