#pragma once
#include <stdint.h>
#include <stddef.h>

// Initialize the kernel heap.
// Must be called after VMM is initialized.
void kheap_init();

// Allocate a contiguous block of memory
void* kmalloc(size_t size);

// Free a previously allocated block
void kfree(void* ptr);

// Allocate and zero memory
void* kzalloc(size_t size);
