

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
>
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



[READ SECTOR(S) INTO MEMORY](http://www.ctyme.com/intr/rb-0607.htm):  this shows how to use `interrput 13` to read data sectors into memory.



---



## Protected Mode

Protected mode is a processor state in x86 architectures which gives access to memory protection, more than 4GB address space.

Protected mode allows you to protect memory from being accessed. It prevents user program talking with hardware.

<img src="images/dev_mannul/image-20230806180838731.png" alt="image-20230806180838731" style="zoom:50%;" />

Ring 1 and Ring 2 usually are at device driver privilege level. Ring 3 is at user application level.

### Memory schemes

#### Selectors

Use segmentation registers as selector registers that point to data structures that describe `memory ranges` and `the premissions(ring level)`. 

#### Paging

Memory is virtual and what you address can point to somewhere entirely different in memory. It makes memory control easier to control.

Paging is the most popular choice for memory schemes with kernel or operating system.

Now we can address up to `4GB` of memory with 32-bit registers.  We are no longer limited to the `1MB` of memory provided by `real mode`.



### Entering Protected Mode

Before switching to protected mode, you must:

- Disable interrupts, including [NMI](https://wiki.osdev.org/Non_Maskable_Interrupt) (as suggested by Intel Developers Manual).
- Enable the [A20 Line](https://wiki.osdev.org/A20_Line).
- Load the [Global Descriptor Table](https://wiki.osdev.org/Global_Descriptor_Table) with segment descriptors suitable for code, data, and stack.

The Global Descriptor Table (GDT) is a binary data structure specific to the [IA-32](https://wiki.osdev.org/IA32_Architecture_Family) and [x86-64](https://wiki.osdev.org/X86-64) architectures. It contains entries telling the CPU about memory [segments](https://wiki.osdev.org/Segmentation). A similar [Interrupt Descriptor Table](https://wiki.osdev.org/Interrupt_Descriptor_Table) exists containing [task](https://wiki.osdev.org/Task) and [interrupt](https://wiki.osdev.org/Interrupts) descriptors.



GDB test:

```sh
target remote | qemu-system-x86_64 -hda ./boot.bin -S -gdb stdio
```

<img src="images/dev_mannul/image-20230807161317319.png" alt="image-20230807161317319" style="zoom:40%;" />



### A20 Line

The A20 Address Line is the physical representation of the 21st bit (number 20, counting from 0) of any memory access.

> On most newer computers starting with the IBM PS/2, the chipset has a FAST A20 option that can quickly enable the A20 line. 

```assembly
in al, 0x92   ; read from port 0x92 by processor bus
or al, 2      ; change 1 bit
out 0x92, al  ; write back by processor bus
```

> ```bash
> sudo apt install build-essential libgmp3-dev bison libmpfr-dev texinfo  libisl-dev
> ```
>
> libcloog-isl-dev can`t find in package sourcess.



---



## Cross Compiler Install

Follow tutorial [GCC Cross-Compiler - OSDev Wiki](https://wiki.osdev.org/GCC_Cross-Compiler) install  [Binutils](https://gnu.org/software/binutils/) and  [GCC](https://gnu.org/software/gcc/)( [note](https://stackoverflow.com/questions/9253695/building-gcc-requires-gmp-4-2-mpfr-2-3-1-and-mpc-0-8-0): run ./contrib/download_prerequisites before you run gcc-x.y.z/configure ).

 We will no longer have the standard library and the system standard library. You have to write them yourself.

We can use the new compiler like this:

```sh
$HOME/opt/cross/bin/$TARGET-gcc --version
```

To use your new compiler simply by invoking `$TARGET-gcc`, add `$HOME/opt/cross/bin` to your `$PATH` by typing:

```sh
export PATH="$HOME/opt/cross/bin:$PATH"
```

> Some test command:
>
> ```sh
> # in gdb  0x100000 the code segment start.
> add-symbol-file ./build/kernelfull.o 0x100000
> 
> break _start
> 
> target remote | qemu-system-x86_64 -S -gdb stdio -hda ./bin/os.bin
> ```


---

## Text Mode

Text mode allows you to write ASCII to video memory. And it support 16 unique colors. By using text mode, there is no need to set individual screen pixels for printing characters.

You write ASCII characters into memory starting at address 0xB8000 for colored displays, or for monochrome displays address 0xB0000. Each ASCII character written to this memory has its pixel equivalent outputted to the monitor.

[Default EGA 16-color palette](https://en.wikipedia.org/wiki/Enhanced_Graphics_Adapter): 

| Index | Default palette number |  Default palette color   | rgbRGB | Hexadecimal |
| :---: | :--------------------: | :----------------------: | :----: | :---------: |
|   0   |           0            |          Black           | 000000 |   #000000   |
|   1   |           1            |           Blue           | 000001 |   #0000AA   |
|   2   |           2            |          Green           | 000010 |   #00AA00   |
|   3   |           3            |           Cyan           | 000011 |   #00AAAA   |
|   4   |           4            |           Red            | 000100 |   #AA0000   |
|   5   |           5            |         Magenta          | 000101 |   #AA00AA   |
|  20   |           6            |          Brown           | 010100 |   #AA5500   |
|   7   |           7            |    White / light gray    | 000111 |   #AAAAAA   |
|  56   |           8            | Dark gray / bright black | 111000 |   #555555   |
|  57   |           9            |       Bright Blue        | 111001 |   #5555FF   |
|  58   |           10           |       Bright green       | 111010 |   #55FF55   |
|  59   |           11           |       Bright cyan        | 111011 |   #55FFFF   |
|  60   |           12           |        Bright red        | 111100 |   #FF5555   |
|  61   |           13           |      Bright magenta      | 111101 |   #FF55FF   |
|  62   |           14           |      Bright yellow       | 111110 |   #FFFF55   |
|  63   |           15           |       Bright white       | 111111 |   #FFFFFF   |

We can use byte 0 to set ASCII character, and byte 1 to set color. E.g. `0xB8000 = 'A' 0xB8001 = 0x00` will set row 0 column 0 to `black 'A'`. `0xB8002 = 'B' 0xB8003 = 0x00` will set row 0 column 1 to `black 'B'`.


---

## Reference

[OSDev](https://wiki.osdev.org/FAT)

[ctyme](http://www.ctyme.com/intr/int-10.htm)