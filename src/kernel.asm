; 32-bits mode
; Can not access BIOS any more. Can not read from disk like read
; sectors in real mode.
; You have to use disk drivers to communicate with disk, to load 
; the rest of kernel.
[BITS 32]
global _start

global problem

extern kernel_main

CODE_SEG equ 0x08   ; kernal code seg
DATA_SEG equ 0x10   ; kernal data seg

_start:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x00200000
    mov esp, ebp

    ; Enable A20 Line
    in al, 0x92
    or al, 2
    out 0x92, al

    call kernel_main
    jmp $                 ; infinite jmp


problem:
    mov eax, 0
    div eax


times 512-($ - $$) db 0   ; alignment to 512 bytes