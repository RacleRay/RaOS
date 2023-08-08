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
    ; jmp $


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


[BITS 32]
load32:
    mov eax, 1          ; starting sector
    mov ecx, 100        ; 100 sectors
    mov edi, 0x0100000  ; 1M, the address we load sectors into
    call ata_lba_read   ; communicate with driver to load sectors


; https://wiki.osdev.org/ATA_read/write_sectors
; https://wiki.osdev.org/ATA_PIO_Mode
;     === OLD FATION === 
; The ATA disk specification is built around an older specification called ST506. 
; With ST506, each disk drive was connected to a controller board by two cables 
; -- a data cable, and a command cable. 
; The controller board was plugged into a motherboard bus. 
; The CPU communicated with the controller board through the CPU's IO ports, 
; which were directly connected to the motherboard bus.
ata_lba_read:
    mov ebx, eax        ; backup the LBA
    shr eax, 24         ; shift to the highest 8 bits of LBA to hard disk controller
    mov dx, 0x1F6
    out dx, al          ; out, communicate to the bus
    ; finish sending the highest 8 bits of the lba          

    ; send the total sectors to read
    mov eax, ecx
    mov dx, 0x1F2
    out dx, al
    ; finish send the total sectors to read

    ; send more bits of the LBA
    mov eax, ebx         ; restore the LBA
    mov dx, 0x1F3
    out dx, al

    mov dx, 0x1F4
    mov eax, ebx         ; restore the LBA for memory safe, maybe damaged
    shr eax, 8
    out dx, al

    mov dx, 0x1F5
    mov eax, ebx
    shr eax, 16
    out dx, al

    mov dx, 0x1F7
    mov al, 0x20
    out dx, al

.next_sector:
    push ecx


; 512bytes, the 511 and 512 byte must be boot signiture.
times 510-($ - $$) db 0
dw 0xAA55
