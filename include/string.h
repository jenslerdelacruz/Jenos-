#pragma once

#include <stddef.h>

void* memcpy(void* dest, const void* src, size_t count);
void* memset(void* dest, int value, size_t count);
void* memmove(void* dest, const void* src, size_t count);
int memcmp(const void* s1, const void* s2, size_t n);
size_t strlen(const char* string);
int strcmp(const char* left, const char* right);
