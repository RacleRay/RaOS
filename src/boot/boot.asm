ORG 0x7c00
BITS 16


CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start


; https://wiki.osdev.org/FAT for BIOS parameter block
; 3 bytes space
jmp short start
nop

; times 33 db 0  ; for BIOS parameter block

; https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system
; FAT16 Header
OEMIdentifier db 'RAOS    '  ; 8 bytes
BytesPerSector dw 0x200      ; usually ignored.
SectorsPerCluster db 0x80
ReservedSectors dw 200       ; will save the entire kernel
FATCopies db 0x02
RootDirEntries dw 0x40
NumSectors dw 0x00           ; ignored
MediaType db 0xF8
SectorsPerFat dw 0x100
SectorsPerTrack dw 0x20
NumberOfHeads dw 0x40
HiddenSectors dd 0x00
SectorsBig dd 0x773594

; Extended BPB( Dos 4.0 )
DriveNumber db 0x80
WinNTBit db 0x00
Signature db 0x29
VolumeID dd 0xD105
VolumeIDString db 'RAOS BOOT  '  ; 11 bytes
SystemIDString db 'FAT16   '


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
    mov ecx, 100        ; 100 sectors to read
    mov edi, 0x0100000  ; 1M, the address we load sectors into
    call ata_lba_read   ; communicate with driver to load sectors
    jmp CODE_SEG:0x0100000


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
    mov ebx, eax        ; backup the LBA (Logical  Block Address)
    shr eax, 24         ; shift to the highest 8 bits of LBA to hard disk controller
    or eax, 0xE0        ; select the master drive
    mov dx, 0x1F6       ; write the highest 8 bits to port 0x1F6
    out dx, al          ; out, communicate to the bus
    ; finish sending the highest 8 bits of the lba          

    ; send the total sectors to read
    mov eax, ecx        ; read the number of sectors we need to eax
    mov dx, 0x1F2
    out dx, al
    ; finish send the total sectors to read

    ; send the low 8 bits of the LBA
    mov eax, ebx         ; restore the LBA (Logical  Block Address)
    mov dx, 0x1F3        ; write it to port 0x1F3
    out dx, al
    ; finish send the low 8 bits of the LBA

    ; send the 8-16 bits of the LBA
    mov dx, 0x1F4
    mov eax, ebx         ; restore the LBA for memory safe, maybe damaged
    shr eax, 8
    out dx, al
    ; finish send the 8-16 bits of the LBA

    ; send the 16-24 bits of the LBA
    mov dx, 0x1F5
    mov eax, ebx         ; restore the LBA
    shr eax, 16
    out dx, al
    ; finish send the 16-24 bits of the LBA

    ; start to read the disk data    
    mov dx, 0x1F7        ; set the status
    mov al, 0x20
    out dx, al

; read all sectors into memory
.next_sector:
    push ecx             ; save for later

.try_again:
    mov dx, 0x1F7        ; set the port to read
    in al, dx            ; read status from port to al
    test al, 8           ; test if data == 0x1000
    jz .try_again        ; jump when test al, 8 fails, reading is not finished.

    ; read 256 words at a time (512 bytes)
    mov ecx, 256
    mov dx, 0x1F0        ; set the port to read

    ; https://faydoc.tripod.com/cpu/insw.htm
    ; a 286/386/486+ CPU instruction that allows the transfer of large amounts of data 
    ; while using only a single instruction. 
    ; Data is transferred at the maximum rate allowed by the PC’s data bus.
    ; Every sample is 1 word in size, which equals 2 bytes of data.
    ; Input word from I/O port specified in DX into memory location specified in ES:(E)DI
    ; At this case, it will store the data in 0x1F0 to edi(0x0100000) address.
    rep insw              ; read port data to ES:DI, two bytes at a time.
    pop ecx               ; restore pushed ecx, total number of sectors we read.
    loop .next_sector     ; decrease the ecx and loop

    ; end of reading
    ret

; 512bytes, the 511 and 512 byte must be boot signiture.
times 510-($ - $$) db 0
dw 0xAA55
