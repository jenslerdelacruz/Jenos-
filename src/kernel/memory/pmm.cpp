#include "memory/pmm.h"
#include "memory/memory.h"

extern uint32_t kernel_end; // Provided by linker.ld

// For simplicity, we assume up to 4GB of RAM.
// 4GB / 4KB (frame size) = 1,048,576 frames.
// 1,048,576 frames / 8 bits per byte = 131,072 bytes (128KB bitmap).
#define PMM_BITMAP_SIZE 131072
static uint8_t pmm_bitmap[PMM_BITMAP_SIZE];

static uint32_t total_memory = 0;
static uint32_t used_memory = 0;

static inline void pmm_set_bit(uint32_t frame) {
    pmm_bitmap[frame / 8] |= (1 << (frame % 8));
}

static inline void pmm_clear_bit(uint32_t frame) {
    pmm_bitmap[frame / 8] &= ~(1 << (frame % 8));
}

static inline bool pmm_test_bit(uint32_t frame) {
    return (pmm_bitmap[frame / 8] & (1 << (frame % 8))) != 0;
}

void pmm_init(multiboot_info* mbd) {
    // Start by marking ALL memory as used/reserved (1)
    memset(pmm_bitmap, 0xFF, PMM_BITMAP_SIZE);
    
    total_memory = 0;
    used_memory = 0;

    // Parse the multiboot memory map to find available RAM
    if (mbd->flags & (1 << 6)) { // Bit 6 indicates mmap is valid
        multiboot_mmap_entry* mmap = (multiboot_mmap_entry*)mbd->mmap_addr;
        uint32_t mmap_end = mbd->mmap_addr + mbd->mmap_length;

        while ((uint32_t)mmap < mmap_end) {
            if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
                total_memory += mmap->len_low;
                
                // Free the frames in this region
                uint32_t start_addr = mmap->addr_low;
                uint32_t end_addr = start_addr + mmap->len_low;
                
                for (uint32_t addr = start_addr; addr < end_addr; addr += PMM_FRAME_SIZE) {
                    pmm_clear_bit(addr / PMM_FRAME_SIZE);
                }
            }
            mmap = (multiboot_mmap_entry*)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
        }
    }

    // Now, we must reserve the lower 1MB (BIOS, VGA, etc)
    for (uint32_t i = 0; i < (1024 * 1024) / PMM_FRAME_SIZE; i++) {
        pmm_set_bit(i);
    }

    // We must also reserve the memory where our kernel is loaded!
    uint32_t kernel_start_frame = 0x100000 / PMM_FRAME_SIZE;
    uint32_t kernel_end_frame = align_up((uint32_t)&kernel_end, PMM_FRAME_SIZE) / PMM_FRAME_SIZE;
    
    for (uint32_t i = kernel_start_frame; i < kernel_end_frame; i++) {
        pmm_set_bit(i);
    }
    
    // Also reserve the memory where the multiboot info is located, and the multiboot modules if any.
    // For simplicity right now, we assume it's safely tucked away or we don't overwrite it immediately.
    
    used_memory = (kernel_end_frame * PMM_FRAME_SIZE);
}

uint32_t pmm_alloc_frame() {
    // Simple first-fit allocator
    // In a production kernel, you'd want a free-list or a stack for O(1) allocation.
    for (uint32_t i = 0; i < (PMM_BITMAP_SIZE * 8); i++) {
        if (!pmm_test_bit(i)) {
            pmm_set_bit(i);
            used_memory += PMM_FRAME_SIZE;
            return i * PMM_FRAME_SIZE;
        }
    }
    return 0; // Out of memory! (0 is reserved, so it acts as NULL)
}

void pmm_free_frame(uint32_t physical_addr) {
    uint32_t frame = physical_addr / PMM_FRAME_SIZE;
    if (pmm_test_bit(frame)) {
        pmm_clear_bit(frame);
        used_memory -= PMM_FRAME_SIZE;
    }
}

uint32_t pmm_get_total_memory() {
    return total_memory;
}

uint32_t pmm_get_used_memory() {
    return used_memory;
}
