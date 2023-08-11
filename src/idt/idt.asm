section .asm

global idt_load
idt_load:
    push ebp
    mov ebp, esp

    ; ebp + 4: the return address of the function caller.
    ; ebp + 8: the first argument passed to this function.
    mov ebx, [ebp + 8]
    lidt [ebx]  ; load the IDT we pass in.

    pop ebp
    ret