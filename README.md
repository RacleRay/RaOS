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





## Reference

[OSDev](https://wiki.osdev.org/FAT)

[ctyme](http://www.ctyme.com/intr/int-10.htm)