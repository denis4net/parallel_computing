.PHONY: all

all:
	icc -std=c99 -vec-report3 main_purec.c -o bin/main_icc_vec3.elf
	icc -std=c99 -O1 main_intrisics.c -o bin/main_icc_intrisics.elf
	
	
clean:
	rm -f bin/*.elf