#include "string.h"

/**
 * @brief Naive strlen implementation.
 *
 * @param ptr
 * @return size_t
 */
size_t strlen(const char* ptr) {
    size_t cnt = 0;
    while (*ptr != 0) {
        ++cnt;
        ++ptr;
    }
    return cnt;
}

/**
 * @brief String length with limit max_len.
 *
 * @param ptr
 * @param max_len
 * @return size_t
 */
size_t strnlen(const char* ptr, size_t max_len) {
    size_t cnt = 0;
    while (*ptr != 0) {
        ++cnt;
        ++ptr;
        if (cnt == max_len) break;
    }
    return cnt;
}

bool isdigit(char c) {
    return c >= 48 && c <= 57;  // '0', '9'
}

int chtoi(char c) {
    return c - '0';  // c - 48
}