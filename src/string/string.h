#ifndef _STRING_H
#define _STRING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


size_t strlen(const char* ptr);
size_t strnlen(const char* ptr, size_t max_len);

char* strcpy(char *dest, const char* src);
char* strncpy(char* dest, const char* src, int n);

int strncmp(const char* str1, const char* str2, int n);
int istrncmp(const char* str1, const char* str2, int n);
char* strncpy(char* dest, const char* src, int count);

bool isdigit(char c);
int  chtoi(char c);
char tolower(char ch);

#endif