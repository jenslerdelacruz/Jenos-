#include "sysinfo.h"
#include "memory/pmm.h"
#include "memory/kheap.h"
#include <stdint.h>

// ====================================================================
// CPUID — Read CPU vendor and brand strings
// ====================================================================

static inline void cpuid(uint32_t leaf, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx) {
    asm volatile("cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf));
}

static void read_cpu_vendor(char* vendor) {
    uint32_t eax, ebx, ecx, edx;
    cpuid(0, &eax, &ebx, &ecx, &edx);
    
    // vendor = EBX + EDX + ECX
    *((uint32_t*)&vendor[0]) = ebx;
    *((uint32_t*)&vendor[4]) = edx;
    *((uint32_t*)&vendor[8]) = ecx;
    vendor[12] = '\0';
}

static void read_cpu_brand(char* brand) {
    uint32_t eax, ebx, ecx, edx;
    
    // Check if extended CPUID is supported
    cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
    if (eax < 0x80000004) {
        // Not supported, fill with generic name
        const char* fallback = "Unknown CPU";
        for (int i = 0; fallback[i]; i++) brand[i] = fallback[i];
        brand[11] = '\0';
        return;
    }
    
    // Read 48-byte brand string across 3 CPUID leaves
    for (uint32_t i = 0; i < 3; i++) {
        cpuid(0x80000002 + i, &eax, &ebx, &ecx, &edx);
        *((uint32_t*)&brand[i * 16 + 0])  = eax;
        *((uint32_t*)&brand[i * 16 + 4])  = ebx;
        *((uint32_t*)&brand[i * 16 + 8])  = ecx;
        *((uint32_t*)&brand[i * 16 + 12]) = edx;
    }
    brand[48] = '\0';
    
    // Trim leading spaces
    int start = 0;
    while (brand[start] == ' ') start++;
    if (start > 0) {
        int i = 0;
        while (brand[start + i]) {
            brand[i] = brand[start + i];
            i++;
        }
        brand[i] = '\0';
    }
}

// ====================================================================
// PCI Scan — Enumerate devices on bus 0
// ====================================================================

static inline uint32_t pci_config_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (1u << 31) | ((uint32_t)bus << 16) | ((uint32_t)slot << 11) | ((uint32_t)func << 8) | (offset & 0xFC);
    asm volatile("outl %0, %1" : : "a"(address), "Nd"((uint16_t)0xCF8));
    uint32_t result;
    asm volatile("inl %1, %0" : "=a"(result) : "Nd"((uint16_t)0xCFC));
    return result;
}

static void pci_scan(system_info* info) {
    info->pci_device_count = 0;
    
    for (uint8_t slot = 0; slot < 32 && info->pci_device_count < MAX_PCI_DEVICES; slot++) {
        uint32_t vendor_reg = pci_config_read(0, slot, 0, 0);
        uint16_t vendor_id = vendor_reg & 0xFFFF;
        uint16_t device_id = (vendor_reg >> 16) & 0xFFFF;
        
        if (vendor_id == 0xFFFF) continue;
        
        uint32_t class_reg = pci_config_read(0, slot, 0, 0x08);
        uint8_t class_code = (class_reg >> 24) & 0xFF;
        uint8_t subclass = (class_reg >> 16) & 0xFF;
        
        pci_device& dev = info->pci_devices[info->pci_device_count];
        dev.bus = 0;
        dev.slot = slot;
        dev.func = 0;
        dev.vendor_id = vendor_id;
        dev.device_id = device_id;
        dev.class_code = class_code;
        dev.subclass = subclass;
        info->pci_device_count++;
    }
}

// ====================================================================
// Heap Statistics — Walk the heap linked list
// ====================================================================

extern uint32_t kernel_end;

struct heap_block_info {
    size_t size;
    bool is_free;
    void* next;
    uint32_t magic;
};

static void read_heap_stats(system_info* info) {
    // The heap starts right after the kernel, aligned to 4KB
    uint32_t heap_start = ((uint32_t)&kernel_end + 0xFFF) & ~0xFFF;
    uint32_t heap_total = 4 * 1024 * 1024; // KHEAP_INITIAL_SIZE
    
    info->heap_total_kb = heap_total / 1024;
    info->heap_used_kb = 0;
    info->heap_free_kb = 0;
    
    // Walk the block list
    heap_block_info* block = (heap_block_info*)heap_start;
    while (block && block->magic == 0x12345678) {
        if (block->is_free) {
            info->heap_free_kb += block->size / 1024;
        } else {
            info->heap_used_kb += block->size / 1024;
        }
        block = (heap_block_info*)block->next;
    }
}

// ====================================================================
// Main Init
// ====================================================================

void sysinfo_init(system_info* info) {
    // Zero out
    for (size_t i = 0; i < sizeof(system_info); i++) {
        ((uint8_t*)info)[i] = 0;
    }
    
    // CPU
    read_cpu_vendor(info->cpu.vendor);
    read_cpu_brand(info->cpu.brand);
    
    // RAM
    info->total_ram_mb = pmm_get_total_memory() / (1024 * 1024);
    info->used_ram_mb = pmm_get_used_memory() / (1024 * 1024);
    
    // Heap
    read_heap_stats(info);
    
    // PCI
    pci_scan(info);
}
