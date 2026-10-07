#pragma once
#include <stdint.h>
#include <stddef.h>

// ====================================================================
// System Information API
// Reads real hardware info via CPUID, PCI, PMM
// ====================================================================

struct cpu_info {
    char vendor[13];   // e.g. "GenuineIntel"
    char brand[49];    // e.g. "QEMU Virtual CPU"
};

struct pci_device {
    uint8_t bus, slot, func;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
};

#define MAX_PCI_DEVICES 32

struct system_info {
    cpu_info cpu;
    uint32_t total_ram_mb;
    uint32_t used_ram_mb;
    uint32_t heap_total_kb;
    uint32_t heap_used_kb;
    uint32_t heap_free_kb;
    pci_device pci_devices[MAX_PCI_DEVICES];
    int pci_device_count;
};

void sysinfo_init(system_info* info);
