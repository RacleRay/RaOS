#include "idt.h"
#include "../kernel.h"
#include "../memory/memory.h"
#include "../config.h"


struct idt_desc idt_descriptors[RAOS_TOTAL_INTERRUPTS];
struct idtr_desc idtr_descriptors;


extern void idt_load(struct idtr_desc* ptr);


void idt_zero() {
    print("Divide by zero error\n");
}


void idt_set(int interrupt_no, void *address) {
    struct idt_desc* desc = &idt_descriptors[interrupt_no];
    desc->offset_1 = (uint32_t)address & 0x0000ffff;
    desc->selector = KERNEL_CODE_SELECTOR;   // define in kernel.asm CODE_SEG
    desc->zero = 0x00;

    // https://wiki.osdev.org/Interrupt_Descriptor_Table
    // 47   | 46  45   | 44	| 43    40
    // P(1)    DPL(1/0)   0   Gate type      
    desc->type_attr = 0xEE;
    desc->offset_2 = (uint32_t)address >> 16;
}


void idt_init() {
    memset(idt_descriptors, 0, sizeof(idt_descriptors));
    idtr_descriptors.limit = sizeof(idt_descriptors) - 1;
    idtr_descriptors.base = (uint32_t)idt_descriptors;

    // regist the handler to div 0.
    idt_set(0, idt_zero);

    idt_load(&idtr_descriptors);
}