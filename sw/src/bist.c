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

void test_port(unsigned data, unsigned ctrl, unsigned stat){
	int i,r;
	for(i=0; i < 8; i++){
		bus_writebyte(data, 1<<i);
		usleep(2);
		r = bus_readbyte(data);
		if(r!=(1<<i)){
			printf("Error on 0x%X, expected %x, got %x\n", data, 1<<i,r);
		}
	}

	for(i=0; i < 6; i++){
		bus_writebyte(ctrl, 1<<i);
		usleep(2);
		r = bus_readbyte(ctrl);
		if(r!=(1<<i)){
			printf("Error on 0x%X, expected %x, got %x\n", ctrl, 1<<i,r);
		}
	}

}

void bist(){
	int i,j;

	printf("Testing LPT0\n");
	test_port(LPT0_DATA, LPT0_CTRL, LPT0_STAT);
	printf("Testing LPT1\n");
	test_port(LPT1_DATA, LPT1_CTRL, LPT1_STAT);



	for(j=0; j < 1024*1024; j++){
		bus_writebyte(LPT0_DATA, 0xFF);
	}
	for(j=0; j < 1024*1024; j++){
		bus_writebyte(LPT0_DATA, 0);
	}

	bus_writebyte(LPT0_CTRL,1<<5); //0x38 works right

	for(i=0; i < 8; i++){
		bus_writebyte(LPT0_DATA, 1<<i); 
		printf("LPT0_DATA: %X\n", bus_readbyte(LPT0_DATA));
	}

	//LPT0_data works
	//LPT1_data works

	bus_writebyte(LPT0_CTRL,0x2); //0x1 problem
	bus_writebyte(LPT1_CTRL,0x0); 
	usleep(100);
	bus_writebyte(LPT1_CTRL,0x2); //Everything  works, but 0x2 and 0x8 slow

}

void dump_registers(){
	printf("LPT0_DATA: %X\n", bus_readbyte(LPT0_DATA));
	printf("LPT0_STAT: %X\n", bus_readbyte(LPT0_STAT));
	printf("LPT0_CTRL: %X\n", bus_readbyte(LPT0_CTRL));

	printf("LPT1_DATA: %X\n", bus_readbyte(LPT1_DATA));
	printf("LPT1_STAT: %X\n", bus_readbyte(LPT1_STAT));
	printf("LPT1_CTRL: %X\n", bus_readbyte(LPT1_CTRL));
}

void write_register(){
	printf("Select destination...\n");
	printf("1. LPT0_DATA\n");
	printf("2. LPT0_CTRL\n");
	printf("3. LPT1_DATA\n");
	printf("4. LPT1_CTRL\n");

	char c = getchar();	
	while(c=='\n')
		c = getchar(); 

	unsigned reg=0;
	switch(c){
		case '1': reg = LPT0_DATA; break;
		case '2': reg = LPT0_CTRL; break;
		case '3': reg = LPT1_DATA; break;
		case '4': reg = LPT1_CTRL; break;
		default: printf("Error\n"); return;
	}

	printf("Current value: 0x%X, enter new value (hex) 0x: ", bus_readbyte(reg));

	unsigned v=0;
	scanf("%x", &v);
	bus_writebyte(reg,v);
}

int do_menu(){
	printf("1. Run BIST (No stat)\n");
	printf("2. Dump registers\n");
	printf("3. Write Registers\n");
	printf("4. Quit\n");
	printf("Enter command : ");

	char c = getchar();	
	while(c=='\n')
		c = getchar(); 

	switch(c){
		case '1': bist(); break;
		case '2': dump_registers(); break;
		case '3': write_register(); break;
		default: return 1;
	}

	return 0;
}

int main() {
	bus_init();

	while(1){
		if(do_menu())
			break;
	}

	dump_registers();

	bus_writebyte(LPT0_DATA,0x0); 
	bus_writebyte(LPT0_CTRL,0x0);

	bus_writebyte(LPT1_DATA,0x0);  
	bus_writebyte(LPT1_CTRL,0x0); 


	bus_shutdown();
}
