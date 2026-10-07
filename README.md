# JenOS Kernel

A bare-metal 32-bit operating system kernel written from scratch in C++ and x86 Assembly.

## Project Structure

```
JenOS-Kernel/
├── boot/                    # Bootloader & linker
│   ├── boot.s               # Multiboot-compliant bootloader (Assembly)
│   ├── gdt_flush.s           # GDT register loader (Assembly)
│   ├── interrupt_stubs.s     # ISR/IRQ stub handlers (Assembly)
│   └── linker.ld             # Linker script (memory layout)
│
├── src/
│   ├── kernel/               # Core kernel subsystems
│   │   ├── kernel.cpp        # Main entry point, UI state machine, login & desktop
│   │   ├── gdt.cpp           # Global Descriptor Table setup
│   │   ├── idt.cpp           # Interrupt Descriptor Table setup
│   │   ├── interrupts.cpp    # ISR/IRQ dispatch & handlers
│   │   └── pic.cpp           # Programmable Interrupt Controller (8259)
│   │
│   ├── drivers/              # Hardware drivers
│   │   ├── keyboard.cpp      # PS/2 Keyboard driver
│   │   └── mouse.cpp         # PS/2 Mouse driver (with cursor rendering)
│   │
│   └── gui/                  # Graphics & rendering
│       ├── graphics.cpp      # Framebuffer, BGA GPU driver, drawing primitives
│       └── font.cpp          # 8x8 bitmap font renderer
│
├── include/                  # All header files
│   ├── gdt.h
│   ├── idt.h
│   ├── interrupts.h
│   ├── pic.h
│   ├── io.h                  # x86 I/O port access (inb/outb)
│   ├── keyboard.h
│   ├── mouse.h
│   ├── graphics.h
│   ├── font.h
│   ├── icons.h               # Vector-style icon drawing functions
│   └── wallpaper.h           # Embedded JPEG wallpaper as pixel array
│
├── build/                    # Compiled object files (.o)
├── tools/                    # Utility scripts
│   └── convert_wallpaper.py  # Converts JPEG to C++ header array
│
├── build.sh                  # Build script
├── jenos.bin                 # Compiled kernel binary
└── README.md                 # This file
```

## Building

Requires MSYS2 UCRT64 with `clang` and `lld` installed.

```bash
bash build.sh
```

## Running

```bash
qemu-system-i386 -m 128M -kernel jenos.bin -device bochs-display
```

## Features

- **32-bit Protected Mode** with GDT, IDT, and PIC
- **PS/2 Keyboard & Mouse** drivers with interrupt-driven I/O
- **1920x1080 Full HD Graphics Mode** via Bochs Graphics Adapter (BGA)
- **Vector-style Icons** drawn with circles and rounded rectangles
- **Interactive Login Screen** with username/password fields and guest access
- **Desktop Environment** with wallpaper, taskbar dock, start menu, and windowing
