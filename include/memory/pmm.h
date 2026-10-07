#pragma once
#include <stdint.h>
#include "multiboot.h"

// The PMM uses 4KB pages (frames)
#define PMM_FRAME_SIZE 4096

// Initialize the PMM using the multiboot information
void pmm_init(multiboot_info* mbd);

// Allocate a single 4KB physical frame. Returns physical address.
uint32_t pmm_alloc_frame();

// Free a 4KB physical frame.
void pmm_free_frame(uint32_t physical_addr);

// Get total available RAM in bytes
uint32_t pmm_get_total_memory();

// Get used RAM in bytes
uint32_t pmm_get_used_memory();
