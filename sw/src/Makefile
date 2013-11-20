all: bist gpmc_tool

clean:
	rm bist gpmc_tool

bist: gpmc.h bist.c gpmc.c
	gcc -o $@ bist.c gpmc.c

gpmc_tool: gpmc.h gpmc_tool.c gpmc.c
	gcc -o $@ gpmc_tool.c gpmc.c


