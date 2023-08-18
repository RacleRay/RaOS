.PHONY:
	clean


FILES = ./build/kernel.asm.o ./build/kernel.o ./build/idt/idt.asm.o ./build/idt/idt.o ./build/memory/memory.o ./build/io/io.asm.o ./build/memory/heap/heap.o ./build/memory/heap/kheap.o ./build/memory/paging/paging.o ./build/memory/paging/paging.asm.o ./build/disk/disk.o
INCLUDES = -I./src
FLAGS = -g -ffreestanding -falign-jumps -falign-functions -falign-labels -falign-loops -fstrength-reduce -fomit-frame-pointer -finline-functions -Wno-unused-function -fno-builtin -Werror -Wno-unused-label -Wno-cpp -Wno-unused-parameter -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -Iinc


all: ./bin/boot.bin ./bin/kernel.bin
	rm -f ./bin/os.bin
	dd if=./bin/boot.bin >> ./bin/os.bin
	dd if=./bin/kernel.bin >> ./bin/os.bin
# append to 100 sectors (512 bytes size)
	dd if=/dev/zero bs=512 count=100 >> ./bin/os.bin  

# kernel
./bin/kernel.bin: $(FILES)
	i686-elf-ld -g -relocatable $(FILES) -o ./build/kernelfull.o
	i686-elf-gcc -T ./src/linker.ld -o ./bin/kernel.bin -ffreestanding -O0 -nostdlib ./build/kernelfull.o

# boot
./bin/boot.bin: ./src/boot/boot.asm
	nasm -f bin $^ -o $@

./build/kernel.asm.o: ./src/kernel.asm
	nasm -f elf -g $^ -o $@

./build/idt/idt.asm.o: ./src/idt/idt.asm
	nasm -f elf -g $^ -o $@

./build/io/io.asm.o: ./src/io/io.asm
	nasm -f elf -g $^ -o $@

./build/memory/paging/paging.asm.o: ./src/memory/paging/paging.asm
	nasm -f elf -g $^ -o $@

./build/kernel.o: ./src/kernel.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -std=gnu99 -c $^ -o $@

./build/idt/idt.o: ./src/idt/idt.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I./src/idt -std=gnu99 -c $^ -o $@

./build/memory/memory.o: ./src/memory/memory.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I./src/memory -std=gnu99 -c $^ -o $@

./build/memory/heap/heap.o: ./src/memory/heap/heap.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I./src/memory/heap -std=gnu99 -c $^ -o $@

./build/memory/heap/kheap.o: ./src/memory/heap/kheap.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I./src/memory/heap -std=gnu99 -c $^ -o $@

./build/memory/paging/paging.o: ./src/memory/paging/paging.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I./src/memory/paging -std=gnu99 -c $^ -o $@

./build/disk/disk.o: ./src/disk/disk.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I./src/disk -std=gnu99 -c $^ -o $@


before_protected_mode:
	nasm -f bin ./src/boot/before_protected_mode.asm -o ./bin/boot_protected.bin
	dd if=./message.txt >> ../bin/boot_protected.bin
	dd if=/dev/zero bs=512 count=1 >> ./bin/boot_protected.bin

clean:
	rm -f ./bin/boot.bin ./bin/os.bin ./bin/kernel.bin
	rm -f ./build/kernelfull.o
	rm -f ${FILES}
