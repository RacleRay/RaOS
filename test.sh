# nasm -f bin ./boot.asm -o ./boot.bin

# qemu-system-x86_64 -hda ./bin/boot.bin

qemu-system-x86_64 -hda ./bin/os.bin

