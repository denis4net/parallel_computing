.PHONY: all

all: main.c
	mkdir bin -p
	gcc --std=c99 -O0 main.c -o bin/main_gcc_O0.elf 
	gcc --std=c99 -O1 main.c -o bin/main_gcc_O1.elf
	gcc --std=c99 -O2 main.c -o bin/main_gcc_O2.elf
	gcc --std=c99 -O3 main.c -o bin/main_gcc_O3.elf
	gcc --std=c99 -O1 -O2 -O3 main.c -o bin/main_gcc_O123.elf
	
	icc -std=c99 -vec-report3 main.c -o bin/main_icc_vec3.elf
	icc -std=c99 -no-vec main.c -o bin/main_icc_novec.elf
	
clean:
	rm -f bin/*.elf