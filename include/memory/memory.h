#pragma once
#include <stdint.h>
#include <stddef.h>

// Basic memory utilities
void* memset(void* dest, int val, size_t count);
void* memcpy(void* dest, const void* src, size_t count);

// Alignment helpers
static inline uint32_t align_up(uint32_t val, uint32_t alignment) {
    if (val % alignment == 0) return val;
    return val + (alignment - (val % alignment));
}

static inline uint32_t align_down(uint32_t val, uint32_t alignment) {
    return val - (val % alignment);
}
