# RaOS
OS from scratch

---

## bootloader

A generally small program responsible for loading the kernel of an operating system.

The response is to put us into protected mode, which will give us four gigabytes of address space.



- The BIOS is executed directly from ROM. 
- The BIOS generally loads itself into RAM then continues execution from RAM.
- The BIOS will initialize essential hardware.
- The BIOS looks for a bootloader to boot by searching all storage mediums for the boot signature "0x55AA".
- The BIOS loads the bootloader into RAM at absolute address Ox7c00.
- The BIOS instructs the process to perform a jump to absolute address Ox7c00 and begin executing the operating systems bootloader.



The BIOS is a kernel in its self.

> The BIOS contains routines to assist our bootloader in booting our kernel.
> The BIOS is 16 bit code which means only 16 bit code can execute it properly.
> The BIOS routines are generic and a standard (More on that later).


---

### The first step of bootloader

```assembly
; address to load this program, boot sector
ORG 0x7c00
; instruction set
BITS 16

start:
    mov si, message
    call print
    jmp $

print:
    mov bx, 0
    ; if al not equal to 0, this will loop and traverse message chars.
.loop:
    ; load register si into register al, then increment si.
    lodsb
    cmp al, 0
    je .done
    call print_char
    jmp .loop

.done:
    ret

print_char:
    ; interupt to ouptut chars to screen. http://www.ctyme.com/intr/int-10.htm
    mov ah, 0eh
    ; invoke BIOS, output char in register al.
    int 0x10
    ret


message: db 'Hello World!', 0

; 512bytes, the 511 and 512 byte must be boot signiture.
times 510-($ - $$) db 0
dw 0xAA55
```

```sh
nasm -f bin ./boot.asm -o ./boot.bin

# check bin
ndisasm ./boot.bin

# run in qemu
qemu-system-x86_64 -hda ./boot.bin
```

> You can boot it from a USB stick
> ```sh
> sudo dd if=./boot.bin of=/dev/sdb
> ```


## Real Mode

- Only 1M RAM access. Memory access is done through the segmentation memory model.
- Based on the original x86 design. It acts like the 8086 processor from the 1970s.
- No security for memory and hardware.
- 16 bit`s accessible at one time. Only 8bits and 16bits registers can be accessed. The offset of our memory address is up to 65535.



### Segmentation memory model

- Memory is accessed by a segment and an offset.
- Programs can be loaded in different areas of memory without problems.
- Multiple segments are available through the use of segment registers.

#### 8086 segment registers

- CS -- Code Segment
- SS -- Stack Segment
- DS -- Data Segment
- ES -- Extra Segment

#### Absolute Offset

Take the segment register multiply it by 16 and add the offset.

```
ORG 0x7c00
```

(0x7c0 * 16)  + 0 = 0x7c00.

> e.g. 0x7cff
>
> 1. Segment Way 1： 0 offset 0x7cff
> 2. Segment Way 2： 0x7c0 offset 0xFF
> 3. Segment Way 3： 0x7cf offset 0x0F
>
> Calculation
> 	0x7cf * 16 = 0x7cf0
> 	0x7cf0 + 0x0f = 0x7cff



"lodsb" uses the DS:SI register combination to get absolute address.

> e.g.
>
> ```assembly
> org 0
> mov ax, 0x7c0
> mov ds, ax
> mov si, 0x1f
> lodsb
> ```
>
> address is: 0x7c0 * 16 + 0x1f = 0x7c1f

#### Programs can be loaded in different areas of memory without problems

Imagine we have two programs loaded into memory, both were assembled with the origin being zero.

- Program 1 uses segment 0x7c0 for all its segment registers. Program 1 is loaded at address 0x7C00.
- Program 2 uses segment 0x7D0 for all its segment registers. Program 2 is located at address 0x7D00.

We swap our segment registers when switching to the other process. And we restore all the registers of the process we are switching too.



#### Multiple segments are available through the use of segment registers

```assembly
mov byte al, [es:32]
;  es * 16 + 32
```

> e.g.
>
> SS = 0x00
>
> SP(stack pointer) = 0x7c00
>
> Push 0xffff, then SP will decrement by 2 to 0x7bfe.
>
> Then set 0x7bfe and 0x7bff to value 0xffff.


---


## Interrputs

lnterrupts are like subroutines, but you don't need to know the memory address to invoke them. lnterrupts are called through the use of interrupt numbers rather than memory addresses. 

lnterrupts can be setup by the programmer. For example you could set up interrupt “0x32” and let it point to your code.

### Interrputs Table

The table has 256 interrupt handlers. Every entry contains 4 bytes (OFFSET:SEGMENT,  2 bytes: 2bytes).

`SEGMENT * 16 + OFFSET` is the handler`s address. 

The 0x13 interrupt will go to offset (0x13 * 4 = 0x46) from the beginning of Interrupt Vector Table.

  
---


## Disk

The disk itself has no concept of files. The disk itself just holds loads of data blocks called sectors. 

Filesystems are kernel implemented they are not the responsibility of the hard disk. Implementing a filesystem requires the kernel programmer to create a filesystem driver for the target file system.

Data is read and written in sectors typically 512 byte blocks for example. Reading the sector of a disk will return 512 bytes of data for the chosen sector.

> CHS(CYLINDER HEAD SECTOR)
>
> > Sectors are read and written by specifying "head" "track"and "sector".
> >
> > This is old fashioned and not the modern way of reading from a disk drive.

> LBA(LOGICAL BLOCK ADDRESS)
>
> > This is the modern way of reading from a hard disk, rather than specify "head" "track" and "sector" we just specify a number that starts from zero.
> >
> > LBA allows us to read from the disk as if we are reading blocks from a very large file.
> >
> > LBA 0 = first sector on the disk;  LBA 1 = second sector on the disk.

Let`s say we want to read the byte at position 58376 on the disk how do we do it?

LBA = 58376 / 512 = 114. Now if we read that LBA we can load 512 bytes into memory. Next we need to know our offset that our byte is in our buffer.

Offset = 58376 ％ 512 = 8. 

Then 114 * 512 = 58368 ;   58368 + 8 = 58376.

ln 16 bit real mode the BIOS provides interrupt `13h` for disk operations.

ln 32 bit mode you have to create your own disk driver which is a little more complicated.


---





## Reference

[OSDev](https://wiki.osdev.org/FAT)

[ctyme](http://www.ctyme.com/intr/int-10.htm)