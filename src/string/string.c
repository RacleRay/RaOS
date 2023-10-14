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


/**
 * @brief String length with specific terminator.
 *
 * @param str
 * @param max max length
 * @param terminator ending flag.
 * @return int
 */
int strnlen_terminator(const char* str, size_t max, char terminator) {
    int i = 0;
    for (i = 0; i < max; ++i) {
        if (str[i] == '\0' || str[i] == terminator) {
            break;
        }
    }
    return i;
}


// strncpy is better.
char* strcpy(char* dest, const char* src) {
    char* begin = dest;
    while (*src != 0) {
        *dest = *src;
        ++src;
        ++dest;
    }
    *dest = 0x00;
    return begin;
}


/**
 * @brief When n > length of src, src will be ended replace with 0x00, then
          copy src to dest.
 * 
 * @param dest 
 * @param src 
 * @param n 
 * @return char* 
 */
char* strncpy(char* dest, const char* src, int n) {
    int i = 0;
    for (i = 0; i < n - 1; i++) {
        if (src[i] == 0x00) break;
        dest[i] = src[i];
    }

    dest[i] = 0x00;
    return dest;
}


/**
 * @brief Compare two string.
 *
 * @param str1
 * @param str2
 * @param n max length to compare.
 * @return int 0 means same ; else return str1 - str2
 */
int strncmp(const char* str1, const char* str2, int n) {
    unsigned char ch1, ch2;

    while (n-- > 0) {
        ch1 = (unsigned char)*(str1++);
        ch2 = (unsigned char)*(str2++);
        if (ch1 != ch2) {
            return ch1 - ch2;
        }
        if (ch1 == '\0') break;
    }

    return 0;
}


/**
 * @brief Compare two string ignoring case.
 *
 * @param str1
 * @param str2
 * @param n max length to compare.
 * @return int 0 means same ; else return str1 - str2
 */
int istrncmp(const char* str1, const char* str2, int n) {
    unsigned char ch1, ch2;

    while (n-- > 0) {
        ch1 = (unsigned char)*(str1++);
        ch2 = (unsigned char)*(str2++);
        if (ch1 != ch2 && tolower(ch1) != tolower(ch2)) {
            return ch1 - ch2;
        }
        if (ch1 == '\0') break;
    }

    return 0;
}


bool isdigit(char c) {
    return c >= 48 && c <= 57;  // '0', '9'
}

int chtoi(char c) {
    return c - '0';  // c - 48
}

char tolower(char ch) {
    if (ch >= 65 && ch <= 90) {
        ch += 32;
    }
    return ch;
}