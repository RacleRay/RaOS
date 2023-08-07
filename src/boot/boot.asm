ORG 0x7c00
BITS 16


CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start


; https://wiki.osdev.org/FAT for BIOS parameter block
_start:
    jmp short start
    nop

times 33 db 0  ; for BIOS parameter block


start:
    jmp 0:step2


step2:
    cli  ; clear interrupts
    ; set segment registers manually
    mov ax, 0x00   ; cant move directly
    mov ds, ax     ; Data Segment
    mov es, ax     ; Extra Segment
    mov ss, ax     ; Stack Segment
    mov sp, 0x7c00 ; Stack Pointer
    sti  ; enable interrupts 


; https://wiki.osdev.org/Protected_Mode
.load_protected:
    cli                    ; disable interrupts
    lgdt[gdt_descriptor]   ; load gdt_start to gdt_end
    mov eax, cr0           ; set PE (Protection Enable) bit in CR0 (Control Register 0)
    or eax, 0x1
    mov cr0, eax
    jmp CODE_SEG:load32    ; jmp to CODE_SEG selector and offset to load32


; https://wiki.osdev.org/Global_Descriptor_Table
gdt_start:
gdt_null:
    dd 0x0
    dd 0x0

; offset 0x8
gdt_code:         ; CS should point to this
    dw 0xffff     ; Segment limit first 0-15 bits
    dw 0          ; Base first 0-15 bits
    db 0          ; Base 16-23 bits
    db 0x9a       ; Access byte sets default
    db 11001111b  ; high 4 bits flags and low 4 bits flags
    db 0          ; Base 24-31 bits

; offset 0x10
gdt_data:         ; DS, SS, ES, FS, GS
    dw 0xffff     ; Segment limit first 0-15 bits
    dw 0          ; Base first 0-15 bits
    db 0          ; Base 16-23 bits
    db 0x92       ; Access byte sets default
    db 11001111b  ; high 4 bits flags and low 4 bits flags
    db 0          ; Base 24-31 bits
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; size
    dd gdt_start                ; offset


; 32-bits mode
; Can not access BIOS any more. Can not read from disk like read
; sectors in real mode.
; You have to use disk drivers to communicate with disk, to load 
; the rest of kernel.
[BITS 32]
load32:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x00200000
    mov esp, ebp
    jmp $                 ; infinite jmp


; 512bytes, the 511 and 512 byte must be boot signiture.
times 510-($ - $$) db 0
dw 0xAA55

