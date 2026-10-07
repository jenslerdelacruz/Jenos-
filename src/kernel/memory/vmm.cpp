#include "memory/vmm.h"
#include "memory/memory.h"

// =====================================================================
// VMM - Virtual Memory Manager
// 
// CURRENT STATE: Paging is NOT enabled yet.
// We keep all the paging infrastructure ready for when we need process
// isolation (user-mode). For now, the kernel runs in flat physical
// memory mode, which is simpler and avoids the complexity of mapping
// the entire kernel + framebuffer correctly.
//
// When we add multitasking/user processes, we will:
// 1. Identity-map ALL of physical RAM (not just 4MB)
// 2. Map the framebuffer
// 3. Enable paging
// =====================================================================

static bool paging_enabled = false;

void vmm_init() {
    // Paging is deferred until we implement user-mode processes.
    // The kernel runs fine in flat physical memory for now.
    paging_enabled = false;
}

bool vmm_map_page(void* physical_addr, void* virtual_addr, uint32_t flags) {
    (void)physical_addr;
    (void)virtual_addr;
    (void)flags;
    // No-op when paging is disabled
    return true;
}

bool vmm_allocate_page(void* virtual_addr, uint32_t flags) {
    (void)virtual_addr;
    (void)flags;
    // No-op when paging is disabled
    return true;
}

void vmm_free_page(void* virtual_addr, bool free_physical) {
    (void)virtual_addr;
    (void)free_physical;
    // No-op when paging is disabled
}
