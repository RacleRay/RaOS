# nasm -f bin ./boot.asm -o ./boot.bin

## test before_protected_mode
# qemu-system-x86_64 -hda ./bin/boot.bin

qemu-system-x86_64 -hda ./bin/os.bin

