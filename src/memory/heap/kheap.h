#ifndef _KHEAP_H
#define _KHEAP_H

#include <stddef.h>
#include <stdint.h>

void kheap_init();

void* kmalloc(size_t size);
void* kzalloc(size_t size);
void  kfree(void* ptr);

#endif