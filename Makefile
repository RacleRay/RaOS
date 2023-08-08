.PHONY:
	clean


FILES = ./build/kernel.asm.o


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
	nasm -f bin ./src/boot/boot.asm -o ./bin/boot.bin

./build/kernel.asm.o: ./src/kernel.asm
	nasm -f elf -g ./src/kernel.asm -o ./build/kernel.asm.o


before_protected_mode:
	nasm -f bin ./src/boot/before_protected_mode.asm -o ./bin/boot_protected.bin
	dd if=./message.txt >> ../bin/boot_protected.bin
	dd if=/dev/zero bs=512 count=1 >> ./bin/boot_protected.bin

clean:
	rm -f ./bin/boot.bin ./bin/os.bin ./build/kernel.asm.o ./bin/kernel.bin
	rm -f ./build/kernelfull.o
