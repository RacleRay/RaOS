#ifndef _MEMORY_H
#define _MEMORY_H

#include <stddef.h>


void* memset(void *ptr, char c, size_t size);
int memcmp(void* s1, void*s2, int count);
void* memcpy(void* dest, void* src, size_t len);

#endif