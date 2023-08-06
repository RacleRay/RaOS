ORG 0
BITS 16

; https://wiki.osdev.org/FAT for BIOS parameter block
_start:
    jmp short start
    nop

times 33 db 0  ; for BIOS parameter block


start:
    jmp 0x7c0:step2


; ------- handle_zero/handle_one interrupts test -------
; ; a interrupt handler when meet 0
; handle_zero:
;     mov ah, 0eh
;     mov al, 'A'
;     mov bx, 0x00
;     int 0x10
;     iret  ; return from interrupt handler

; handle_one:
;     mov ah, 0eh
;     mov al, 'B'
;     mov bx, 0x00
;     int 0x10
;     iret  ; return from interrupt handler
; -------------------------------------------------------


step2:
    cli  ; clear interrupts

    ; set segment registers manually
    mov ax, 0x7c0  ; cant move directly
    mov ds, ax     ; Data Segment
    mov es, ax     ; Extra Segment

    mov ax, 0x00   ; cant move directly
    mov ss, ax     ; Stack Segment
    mov sp, 0x7c00 ; Stack Pointer

    sti  ; enable interrupts 

    ; ------- for handle_zero/handle_one interrupts test -------
    ; ; if there is not the ss, the default will be ds.
    ; mov word[ss:0x00], handle_zero
    ; mov word[ss:0x02], 0x7c0

    ; mov word[ss:0x04], handle_one
    ; mov word[ss:0x06], 0x7c0

    ; int 0
    ; ; divide 0 also invoke handle_zero
    ; ; mov ax, 0x00
    ; ; div ax

    ; int 1
    ; ----------------------------------------------------------
    
    ; ------- for loader message test --------------------------
    ; mov si, message
    ; call print
    ; ----------------------------------------------------------

    ; ref: http://www.ctyme.com/intr/rb-0607.htm
    mov ah, 2  ; read sector command
    mov al, 1  ; read sector one, 512 bytes
    mov ch, 0  ; low eight bits of cylinder number
    mov cl, 2  ; read sector two (the sector number)
    mov dh, 0  ; head number
    mov bx, buffer

    int 0x13
    ; if the carry flag sets on error, jmp to show error_message
    jc error

    mov si, buffer
    call print

    jmp $      ; infinite jmp

error:
    mov si, error_message
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

; ------- loader message test -------------------------
; message: db 'Hello World!', 0
; -----------------------------------------------------

; write string ended with 0
error_message: db 'Fail to load sectors.', 0

; 512bytes, the 511 and 512 byte must be boot signiture.
times 510-($ - $$) db 0
dw 0xAA55

; Symbol tags the data from 513 byte which added by Makefile dd command.
; At es:bx address.  ref: http://www.ctyme.com/intr/rb-0607.htm
buffer: