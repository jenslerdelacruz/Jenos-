#include "memory/memory.h"

void* memset(void* dest, int val, size_t count) {
    uint8_t* ptr = (uint8_t*)dest;
    while (count--) {
        *ptr++ = (uint8_t)val;
    }
    return dest;
}

void* memcpy(void* dest, const void* src, size_t count) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while (count--) {
        *d++ = *s++;
    }
    return dest;
}

size_t strlen(const char* string) {
    size_t length = 0;
    while (string[length]) {
        ++length;
    }
    return length;
}

int strcmp(const char* left, const char* right) {
    while (*left && *left == *right) {
        ++left;
        ++right;
    }
    return (uint8_t)*left - (uint8_t)*right;
}
