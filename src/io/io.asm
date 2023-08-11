section .asm

global insb
global insw
global outb
global outw

; reference: https://c9x.me/x86/
;            https://c9x.me/x86/html/file_module_x86_id_139.html
insb:
    push ebp
    mov ebp, esp

    xor eax, eax       ; set 0
    mov edx, [ebp + 8]
    in al, dx          ; Input byte from I/O port in DX into AL.

    pop ebp
    ret


insw:
    push ebp
    mov ebp, esp

    xor eax, eax
    mov edx, [ebp + 8]
    in ax, dx          ; Input word from I/O port in DX into AX

    pop ebp
    ret


outb:
    push ebp
    mov ebp, esp

    mov eax, [ebp + 12]
    mov edx, [ebp + 8]
    out dx, al          ; Output byte in AL to I/O port address in DX

    pop ebp
    ret


outw:
    push ebp
    mov ebp, esp

    mov eax, [ebp + 12]
    mov edx, [ebp + 8]
    out dx, ax          ; Output word in AX to I/O port address in DX

    pop ebp
    ret