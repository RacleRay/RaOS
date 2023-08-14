#include "kernel.h"
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kheap.h"
#include <stdint.h>
#include <stddef.h>


uint16_t* video_mem = 0;
uint16_t terminal_row = 0;
uint16_t terminal_col = 0;


uint16_t terminal_make_char(char ch, char color) {
    // in memory bits, not effected by endian.
    return color << 8 | ch;
}


void terminal_putchar(size_t x, size_t y, char c, char color) {
    video_mem[(y * VGA_WIDTH) + x] = terminal_make_char(c, color);
}


void terminal_writechar(char c, char color) {
    if (c == '\n') {
        ++terminal_row;
        terminal_col = 0;
        return;
    }

    terminal_putchar(terminal_col, terminal_row, c, color);
    ++terminal_col;
    if (terminal_col >= VGA_WIDTH) {
        terminal_col = 0;
        ++terminal_row;
    }
}


/**
 * @brief clear the video memory, that generated by BIOS.
 * 
 */
void terminal_initialize() {
    video_mem = (uint16_t*)(0xB8000);
    terminal_row = 0;
    terminal_col = 0;

    for (size_t y = 0; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x) {
            video_mem[(y * VGA_WIDTH) + x] = terminal_make_char(' ', 0);
        }
    }
}


/**
 * @brief Get the string length ended by 0
 * 
 * @param str 
 * @return size_t 
 */
size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) {
        ++len;
    }
    return len;
}


void print(const char* str) {
    size_t len = strlen(str);
    for (size_t i = 0; i < len; ++i) {
        terminal_writechar(str[i], 15);
    }
}


// For IDT test
// extern void problem();


void kernel_main() {
    (void)terminal_initialize();

    // For test
    // video_mem[0] = 0x4103;  // little endian
    // video_mem[0] = terminal_make_char('B', 15);
    // For test
    // terminal_writechar('A', 15);
    // terminal_writechar('B', 15);

    print("Hello RaOS!\n");

    kheap_init();

    (void)idt_init();

    // For IDT test. test div 0 interrupt.
    // problem();

    // For textmode output test
    // outb(0x60, 0xff);

    // For kernel malloc test
    void* ptr1 = kmalloc(50);
    void* ptr2 = kmalloc(5000);
    void* ptr3 = kmalloc(5600);

    kfree(ptr1);
    void* ptr4 = kmalloc(50);

    if (ptr1 || ptr2 || ptr3 || ptr4) { }


    return;
}