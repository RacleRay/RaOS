.PHONY:
	clean

all:
	nasm -f bin ./src/boot/boot.asm -o ./bin/boot.bin

before_protected_mode:
	nasm -f bin ./src/boot/before_protected_mode.asm -o ./bin/boot_protected.bin
	dd if=./message.txt >> ../bin/boot_protected.bin
	dd if=/dev/zero bs=512 count=1 >> ./bin/boot_protected.bin

clean:
	rm -rf ./bin/boot.bin
