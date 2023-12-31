#include <stdint.h>
#include "config.h"
#include "disk/disk.h"
#include "disk/streamer.h"
#include "gdt/gdt.h"
#include "kernel.h"
#include "fs/pparser.h"
#include "fs/file.h"
#include "memory/memory.h"
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"
#include "string/string.h"
#include "task/tss.h"



uint16_t* video_mem    = 0;
uint16_t  terminal_row = 0;
uint16_t  terminal_col = 0;


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
    video_mem    = (uint16_t*)(0xB8000);
    terminal_row = 0;
    terminal_col = 0;

    for (size_t y = 0; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x) {
            video_mem[(y * VGA_WIDTH) + x] = terminal_make_char(' ', 0);
        }
    }
}


void print(const char* str) {
    size_t len = strlen(str);
    for (size_t i = 0; i < len; ++i) {
        terminal_writechar(str[i], 15);
    }
}


void panic(const char* msg) {
    print(msg);
    while (1) {}
}


static struct paging_4gb_chunk* kernel_chunk = 0;

void kernel_page() {
    kernel_registers();
    paging_switch(kernel_chunk);
}


struct tss tss;

struct gdt gdt_real[RAOS_TOTAL_GDT_SEGMENTS];
struct gdt_structured gdt_structured[RAOS_TOTAL_GDT_SEGMENTS] = {
    {.base = 0x00, .limit = 0x00, .type = 0x00},                // NULL Segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0x9a},           // Kernel code segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0x92},            // Kernel data segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0xf8},              // User code segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0xf2},             // User data segment
    {.base = (uint32_t)&tss, .limit=sizeof(tss), .type = 0xE9}      // TSS Segment
};


// For IDT test
// extern void problem();


void kernel_main() {
    terminal_initialize();

    // load the gdt
    memset(gdt_real, 0, sizeof(gdt_real));
    gdt_structured_to_gdt(gdt_real, gdt_structured, RAOS_TOTAL_GDT_SEGMENTS);
    gdt_load(gdt_real, sizeof(gdt_real));

    // For test
    // video_mem[0] = 0x4103;  // little endian
    // video_mem[0] = terminal_make_char('B', 15);
    // For test
    // terminal_writechar('A', 15);
    // terminal_writechar('B', 15);

    print("Hello RaOS!\n");

    // kerneal heap initialization.
    kheap_init();

    // init file system
    fs_init();

    // Search and initialize the disks.
    disk_search_and_init();

    // IDT initialization.
    idt_init();

    // TSS initialization.
    memset(&tss, 0, sizeof(tss));
    tss.esp0 = 0x600000;   // kernel stack address
    tss.ss0  = KERNEL_DATA_SELECTOR;

    // Load TSS
    tss_load(0x28);  // 0x28 is the offset of TSS segment in GDT.

    // Setup paging
    kernel_chunk = paging_new_4gb(PAGING_IS_WRITABLE | PAGING_IS_PRESENT
                                  | PAGING_ACCESS_FROM_ALL);

    // Switch to kernel paging chunk
    paging_switch(kernel_chunk);

    // Enable paginng
    enable_paging();

    // enable interrupts after IDT initialized.
    enable_interrupts();

    // === Begin === For fopen test 
    int fd = fopen("0:/message.txt", "r");
    if (fd) {
        print("File message.txt opened!\n");
        char buf[32];
        fread(buf, 7, 1, fd);
        print(buf);
        print("\nAfter fseek. \n");

        fseek(fd, 5, SEEK_CUR);
        fread(buf, 7, 1, fd);
        print(buf);
        print("\n\n");

        struct file_stat stat;
        fstat(fd, &stat);

        fclose(fd);
        
        print("file descriptor closed.\n");
    }
    int fd2 = fopen("0:/not_exit.txt", "w");
    if (fd2) {
        print("File not_exit.txt opened!\n");
    }
    // === End === For fopen test

    // === Begin === For disk read test 
    // struct disk_stream* stream = diskstreamer_new(0);
    // diskstreamer_seek(stream, 0x201);  // 184

    // unsigned char ch = 0;  
    // diskstreamer_read(stream, &ch, 1);
    // === End === For disk read test


    // === Begin === For disk block read test
    // char buf[1024] = {0};
    // disk_read_block(disk_get(0), 0, 1, buf);
    // === End === For disk block read test


    // === Begin === For path parser test
    // struct path_root* root_path = path_parse("0:/bin/hello.o", NULL);
    // if (root_path) {
    // }
    // === End === For path parser test


    // === Begin === For paging test
    // char* ptr = kzalloc(4096);  // physical address
    // paging_set(paging_4gb_chunk_get_directory(kernel_chunk), (void*)0x1000,
    // (uint32_t)ptr | PAGING_ACCESS_FROM_ALL | PAGING_IS_PRESENT |
    // PAGING_IS_WRITABLE);

    // char* ptr2 = (char*)0x1000;
    // ptr2[0] = 'A';
    // ptr2[1] = 'B';
    // print(ptr2);
    // print(ptr);
    // === End === For paging test


    // === For IDT test. test div 0 interrupt.
    // problem();

    // === For textmode output test
    // outb(0x60, 0xff);


    // === Begin === For kernel malloc test
    // void* ptr1 = kmalloc(50);
    // void* ptr2 = kmalloc(5000);
    // void* ptr3 = kmalloc(5600);

    // kfree(ptr1);
    // void* ptr4 = kmalloc(50);

    // if (ptr1 || ptr2 || ptr3 || ptr4) { }
    // === End === For kernel malloc test

    while (1) {}

    return;
}