#include "memory/kheap.h"
#include "memory/pmm.h"
#include "memory/memory.h"

// =====================================================================
// Kernel Heap Allocator
//
// Since paging is not enabled yet, the heap lives in physical memory
// right after the kernel. We use PMM to grab physical frames and
// manage them with a linked-list block allocator.
// =====================================================================

extern uint32_t kernel_end; // From linker.ld

// A simple linked-list block allocator
struct heap_block {
    size_t size;           // Size of the block INCLUDING this header
    bool is_free;          // Is it free?
    heap_block* next;      // Next block in the list
    uint32_t magic;        // Magic number to detect corruption
};

#define HEAP_MAGIC 0x12345678
#define KHEAP_INITIAL_SIZE (16 * 1024 * 1024) // 16 MB initial heap

static heap_block* head = nullptr;
static uint32_t heap_start_addr = 0;
static uint32_t current_heap_end = 0;

void kheap_init() {
    // Place the heap right after the kernel in physical memory
    // Align to 4KB boundary
    heap_start_addr = ((uint32_t)&kernel_end + 0xFFF) & ~0xFFF;
    current_heap_end = heap_start_addr + KHEAP_INITIAL_SIZE;

    // Initialize the first block
    head = (heap_block*)heap_start_addr;
    head->size = KHEAP_INITIAL_SIZE;
    head->is_free = true;
    head->next = nullptr;
    head->magic = HEAP_MAGIC;
}

void* kmalloc(size_t size) {
    if (size == 0) return nullptr;

    // Align total size to 4 bytes
    size_t total_size = (size + sizeof(heap_block) + 3) & ~3;

    heap_block* curr = head;
    while (curr) {
        if (curr->magic != HEAP_MAGIC) {
            return nullptr; // Heap corruption
        }

        if (curr->is_free && curr->size >= total_size) {
            // Split block if there's enough room
            if (curr->size >= total_size + sizeof(heap_block) + 4) {
                heap_block* new_block = (heap_block*)((uint8_t*)curr + total_size);
                new_block->size = curr->size - total_size;
                new_block->is_free = true;
                new_block->next = curr->next;
                new_block->magic = HEAP_MAGIC;

                curr->size = total_size;
                curr->next = new_block;
            }

            curr->is_free = false;
            return (void*)((uint8_t*)curr + sizeof(heap_block));
        }

        curr = curr->next;
    }
    return nullptr; // Out of memory
}

void kfree(void* ptr) {
    if (!ptr) return;

    heap_block* block = (heap_block*)((uint8_t*)ptr - sizeof(heap_block));

    if (block->magic != HEAP_MAGIC) {
        return; // Invalid pointer
    }

    block->is_free = true;

    // Coalesce adjacent free blocks
    heap_block* curr = head;
    while (curr) {
        if (curr->is_free && curr->next && curr->next->is_free) {
            curr->size += curr->next->size;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}

void* kzalloc(size_t size) {
    void* ptr = kmalloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

void* operator new(size_t size) {
    return kmalloc(size);
}

void* operator new[](size_t size) {
    return kmalloc(size);
}

void operator delete(void* ptr) noexcept {
    kfree(ptr);
}

void operator delete[](void* ptr) noexcept {
    kfree(ptr);
}

void operator delete(void* ptr, size_t) noexcept {
    kfree(ptr);
}

void operator delete[](void* ptr, size_t) noexcept {
    kfree(ptr);
}
