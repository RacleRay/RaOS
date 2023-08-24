#ifndef _STRING_H
#define _STRING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


size_t strlen(const char* ptr);
size_t strnlen(const char* ptr, size_t max_len);

char* strcpy(char *dest, const char* src);

bool isdigit(char c);
int  chtoi(char c);

#endif