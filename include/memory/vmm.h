#pragma once
#include <stdint.h>
#include <stddef.h>
#include "memory/pmm.h"

// x86 Paging Constants
#define PAGE_SIZE 4096

// Page Table/Directory Entry Flags
#define I86_PTE_PRESENT 0x01
#define I86_PTE_WRITABLE 0x02
#define I86_PTE_USER 0x04
#define I86_PTE_WRITETHROUGH 0x08
#define I86_PTE_NOT_CACHEABLE 0x10
#define I86_PTE_ACCESSED 0x20
#define I86_PTE_DIRTY 0x40
#define I86_PTE_PAT 0x80
#define I86_PTE_CPU_GLOBAL 0x100
#define I86_PTE_LV4_GLOBAL 0x200
#define I86_PTE_FRAME 0xFFFFF000

// A page table entry is just a 32-bit integer
typedef uint32_t pt_entry;
// A page directory entry is just a 32-bit integer
typedef uint32_t pd_entry;

// A page table contains 1024 entries
struct page_table {
    pt_entry m_entries[1024];
};

// A page directory contains 1024 entries (each pointing to a page table)
struct page_directory {
    pd_entry m_entries[1024];
};

// Initialize the Virtual Memory Manager (Paging)
void vmm_init();

// Map a virtual address to a physical address
bool vmm_map_page(void* physical_addr, void* virtual_addr, uint32_t flags);

// Allocate a new physical frame and map it to the virtual address
bool vmm_allocate_page(void* virtual_addr, uint32_t flags);

// Free a mapped virtual page (and optionally free the physical frame)
void vmm_free_page(void* virtual_addr, bool free_physical);

// Helper to enable paging
extern "C" void vmm_enable_paging(uint32_t page_directory_addr);
