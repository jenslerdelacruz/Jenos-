#pragma once

#include <stdint.h>

// Write a byte out to the specified port.
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %1, %0" : : "dN" (port), "a" (value));
}

// Read a byte from the specified port.
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

// Wait a very small amount of time (1 to 4 microseconds)
// Useful for PIC remapping.
static inline void io_wait(void) {
    outb(0x80, 0);
}

// Write a 16-bit word to the specified port.
static inline void outw(uint16_t port, uint16_t value) {
    __asm__ volatile ("outw %1, %0" : : "dN" (port), "a" (value));
}

// Read a 16-bit word from the specified port.
static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

// Write a 32-bit dword to the specified port.
static inline void outl(uint16_t port, uint32_t value) {
    __asm__ volatile ("outl %1, %0" : : "dN" (port), "a" (value));
}

// Read a 32-bit dword from the specified port.
static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile ("inl %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}
