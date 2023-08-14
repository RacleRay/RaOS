section .asm

extern int21h_handler
extern no_interrupt_handler

global idt_load
global int21h
global no_interrupt
global enable_interrupts
global disable_interrupts


enable_interrupts:
    sti
    ret


disable_interrupts:
    cli
    ret


idt_load:
    push ebp
    mov ebp, esp

    ; ebp + 4: the return address of the function caller.
    ; ebp + 8: the first argument passed to this function.
    mov ebx, [ebp + 8]
    lidt [ebx]  ; load the IDT we pass in.

    pop ebp
    ret

; https://faydoc.tripod.com/cpu/pushad.htm
int21h:
    cli      ; clear interrupts
    pushad   ; Push EAX, ECX, EDX, EBX, original ESP, EBP, ESI, and EDI

    call int21h_handler

    popad
    sti      ; enable interrupts 
    iret


no_interrupt:
    cli      ; clear interrupts
    pushad   ; Push EAX, ECX, EDX, EBX, original ESP, EBP, ESI, and EDI

    call no_interrupt_handler

    popad
    sti      ; enable interrupts 
    iret