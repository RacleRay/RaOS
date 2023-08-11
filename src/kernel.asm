; 32-bits mode
; Can not access BIOS any more. Can not read from disk like read
; sectors in real mode.
; You have to use disk drivers to communicate with disk, to load 
; the rest of kernel.
[BITS 32]
global _start

; For IDT test
; ---------------------------
; global problem
; ---------------------------

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

    ; Remap the master PIC
    mov al, 00010001b   ; b4=1: init ; b3=0: Edge ; b1=0: Cascade ; b0=1: need 4th init step
    out 0x20, al        ; tell master
    
    mov al, 0x20        ; master IRQ0 should be on INT 0x20 (just after intel exceptions)
    out 0x21, al

    mov al, 00000001b   ; b4=0: FNM ; b3-2=00: master/slave set by hardware; b1=0: not AEOI; b0=1: x86 mode 
    out 0x21, al
    ; End remap the master PIC

    ; NOTE: there maybe some time frame before you set up the IDT.
    ; NOTE: this will be fixed later.
    sti

    call kernel_main
    jmp $                 ; infinite jmp


; For IDT test
; ---------------------------
; problem:
;     mov eax, 0
;     div eax
; ---------------------------


times 512-($ - $$) db 0   ; alignment to 512 bytes