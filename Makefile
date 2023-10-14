.PHONY:
	clean


FILES = ./build/kernel.asm.o ./build/kernel.o ./build/idt/idt.asm.o ./build/idt/idt.o \
		./build/memory/memory.o ./build/io/io.asm.o ./build/memory/heap/heap.o \
		./build/memory/heap/kheap.o ./build/memory/paging/paging.o \
		./build/memory/paging/paging.asm.o ./build/disk/disk.o ./build/string/string.o \
		./build/fs/pparser.o ./build/disk/streamer.o ./build/fs/file.o \
		./build/fs/fat/fat16.o ./build/gdt/gdt.asm.o ./build/gdt/gdt.o \
		./build/task/tss.asm.o ./build/task/task.o ./build/task/task.asm.o \
		./build/task/process.o ./build/loader/elfloader.o ./build/loader/elf.o

INCLUDES = -I./src
FLAGS = -g -ffreestanding -falign-jumps -falign-functions -falign-labels -falign-loops -fstrength-reduce -fomit-frame-pointer -finline-functions -Wno-unused-function -fno-builtin -Werror -Wno-unused-label -Wno-cpp -Wno-unused-parameter -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -Iinc


all: ./bin/boot.bin ./bin/kernel.bin
	rm -f ./bin/os.bin
	dd if=./bin/boot.bin >> ./bin/os.bin
	dd if=./bin/kernel.bin >> ./bin/os.bin
# append to 100 sectors (512 bytes size)
# dd if=/dev/zero bs=512 count=100 >> ./bin/os.bin
# 16Mb for FAT16 boot  
	dd if=/dev/zero bs=1048576 count=16 >> ./bin/os.bin

# file system test
	sudo mount -t vfat ./bin/os.bin /mnt/d
	sudo cp ./message.txt /mnt/d
	sudo umount /mnt/d

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

./build/gdt/gdt.asm.o: ./src/gdt/gdt.asm
	nasm -f elf -g $^ -o $@

./build/io/io.asm.o: ./src/io/io.asm
	nasm -f elf -g $^ -o $@

./build/memory/paging/paging.asm.o: ./src/memory/paging/paging.asm
	nasm -f elf -g $^ -o $@

./build/task/tss.asm.o: ./src/task/tss.asm
	nasm -f elf -g $^ -o $@

./build/task/task.asm.o: ./src/task/task.asm
	nasm -f elf -g $^ -o $@

./build/kernel.o: ./src/kernel.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -std=gnu99 -c $^ -o $@

./build/idt/idt.o: ./src/idt/idt.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I./src/idt -std=gnu99 -c $^ -o $@

./build/gdt/gdt.o: ./src/gdt/gdt.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I./src/gdt -std=gnu99 -c $^ -o $@

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

./build/string/string.o: ./src/string/string.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I./src/string -std=gnu99 -c $^ -o $@

./build/fs/pparser.o: ./src/fs/pparser.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I./src/fs -std=gnu99 -c $^ -o $@

./build/fs/fat/fat16.o: ./src/fs/fat/fat16.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I./src/fs/fat -I./src/fs -std=gnu99 -c $^ -o $@

./build/fs/file.o: ./src/fs/file.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I./src/fs -std=gnu99 -c $^ -o $@

./build/disk/streamer.o: ./src/disk/streamer.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I./src/disk -std=gnu99 -c $^ -o $@

./build/task/task.o: ./src/task/task.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I./src/task -std=gnu99 -c $^ -o $@

./build/task/process.o: ./src/task/process.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I./src/task -std=gnu99 -c $^ -o $@

./build/loader/elf.o: ./src/loader/elf.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I./src/task -std=gnu99 -c $^ -o $@

./build/loader/elfloader.o: ./src/loader/elfloader.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -I./src/task -std=gnu99 -c $^ -o $@


before_protected_mode:
	nasm -f bin ./src/boot/before_protected_mode.asm -o ./bin/boot_protected.bin
	dd if=./message.txt >> ../bin/boot_protected.bin
	dd if=/dev/zero bs=512 count=1 >> ./bin/boot_protected.bin

clean:
	rm -f ./bin/boot.bin ./bin/os.bin ./bin/kernel.bin
	rm -f ./build/kernelfull.o
	rm -f ${FILES}
