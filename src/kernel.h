#ifndef _KERNEL_H
#define _KERNEL_H

#define VGA_WIDTH 80
#define VGA_HEIGHT 20

#define ERROR(val) (void*)(val)
#define ERROR_I(val) (int)(val)
#define ISERR(val) ((int)(val) < 0)

void panic(const char* msg);
void print(const char* str);
void kernel_main();
void kernel_page();
void kernel_registers();

#endif
