#ifndef _IDT_H
#define _IDT_H

#include <stdint.h>


struct idt_desc {
    uint16_t offset_1;      // offset bits 0..15
    uint16_t selector;      // a code segment selector in GDT or LDT
    uint8_t  zero;          // unused, set to 0
    uint8_t  type_attr;     // gate type, dpl, and p fields
    uint16_t offset_2;      // offset bits 16..31
} __attribute__((packed));  // keep it as it is, no alignment


struct idtr_desc {
    uint16_t limit;  // the length of the IDT minus one
    uint32_t base;   // the address of IDT
} __attribute__((packed));


void idt_init();
void enable_interrupts();
void disable_interrupts();

#endif