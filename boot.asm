ORG 0
BITS 16

; https://wiki.osdev.org/FAT for BIOS parameter block
_start:
    jmp short start
    nop

times 33 db 0  ; for BIOS parameter block


start:
    jmp 0x7c0:step2

step2:
    cli  ; clear interrupts
    sti  ; enable interrupts 

    ; set segment registers manually
    mov ax, 0x7c0  ; cant move directly
    mov ds, ax     ; Data Segment
    mov es, ax     ; Extra Segment

    mov ax, 0x00   ; cant move directly
    mov ss, ax     ; Stack Segment
    mov sp, 0x7c00 ; Stack Pointer

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