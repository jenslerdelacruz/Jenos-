#!/bin/bash
set -e
export PATH="/ucrt64/bin:$PATH"

# Directories
BOOT_DIR="boot"
SRC_KERNEL="src/kernel"
SRC_DRIVERS="src/drivers"
SRC_GUI="src/gui"
INCLUDE_DIR="include"
BUILD_DIR="build"

CFLAGS="--target=i686-pc-none-elf -mno-sse -mno-mmx -mno-sse2 -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti -I${INCLUDE_DIR}"

echo "========================================="
echo "  JenOS Kernel Build System"
echo "========================================="

echo "[1/5] Assembling bootloader..."
clang --target=i686-pc-none-elf -c ${BOOT_DIR}/boot.s -o ${BUILD_DIR}/boot.o

echo "[2/6] Compiling memory management..."
clang ${CFLAGS} -c ${SRC_KERNEL}/memory/memory.cpp -o ${BUILD_DIR}/memory.o
clang ${CFLAGS} -c ${SRC_KERNEL}/memory/pmm.cpp    -o ${BUILD_DIR}/pmm.o
clang ${CFLAGS} -c ${SRC_KERNEL}/memory/vmm.cpp    -o ${BUILD_DIR}/vmm.o
clang ${CFLAGS} -c ${SRC_KERNEL}/memory/kheap.cpp  -o ${BUILD_DIR}/kheap.o

echo "[3/6] Compiling kernel core..."
clang ${CFLAGS} -c ${SRC_KERNEL}/kernel.cpp      -o ${BUILD_DIR}/kernel.o
clang ${CFLAGS} -c ${SRC_KERNEL}/gdt.cpp          -o ${BUILD_DIR}/gdt.o
clang ${CFLAGS} -c ${SRC_KERNEL}/idt.cpp          -o ${BUILD_DIR}/idt.o
clang ${CFLAGS} -c ${SRC_KERNEL}/interrupts.cpp   -o ${BUILD_DIR}/interrupts.o
clang ${CFLAGS} -c ${SRC_KERNEL}/pic.cpp          -o ${BUILD_DIR}/pic.o
clang ${CFLAGS} -c ${SRC_KERNEL}/rtc.cpp          -o ${BUILD_DIR}/rtc.o

echo "[4/6] Compiling drivers..."
clang ${CFLAGS} -c ${SRC_DRIVERS}/keyboard.cpp    -o ${BUILD_DIR}/keyboard.o
clang ${CFLAGS} -c ${SRC_DRIVERS}/mouse.cpp       -o ${BUILD_DIR}/mouse.o

echo "[5/6] Compiling GUI & Settings..."
clang ${CFLAGS} -c ${SRC_GUI}/graphics.cpp        -o ${BUILD_DIR}/graphics.o
clang ${CFLAGS} -c ${SRC_GUI}/font.cpp            -o ${BUILD_DIR}/font.o
clang ${CFLAGS} -c ${SRC_GUI}/fluent_ui.cpp       -o ${BUILD_DIR}/fluent_ui.o
clang ${CFLAGS} -c ${SRC_GUI}/login_screen.cpp    -o ${BUILD_DIR}/login_screen.o
clang ${CFLAGS} -c ${SRC_KERNEL}/sysinfo.cpp      -o ${BUILD_DIR}/sysinfo.o
clang ${CFLAGS} -c ${SRC_GUI}/settings_app.cpp    -o ${BUILD_DIR}/settings_app.o

echo "[5.5/6] Assembling stubs..."
clang --target=i686-pc-none-elf -c ${BOOT_DIR}/gdt_flush.s       -o ${BUILD_DIR}/gdt_flush.o
clang --target=i686-pc-none-elf -c ${BOOT_DIR}/interrupt_stubs.s  -o ${BUILD_DIR}/interrupt_stubs.o

echo "[6/6] Linking kernel..."
ld.lld -m elf_i386 -T ${BOOT_DIR}/linker.ld --no-rosegment \
    ${BUILD_DIR}/boot.o \
    ${BUILD_DIR}/gdt_flush.o \
    ${BUILD_DIR}/interrupt_stubs.o \
    ${BUILD_DIR}/memory.o \
    ${BUILD_DIR}/pmm.o \
    ${BUILD_DIR}/vmm.o \
    ${BUILD_DIR}/kheap.o \
    ${BUILD_DIR}/kernel.o \
    ${BUILD_DIR}/gdt.o \
    ${BUILD_DIR}/idt.o \
    ${BUILD_DIR}/interrupts.o \
    ${BUILD_DIR}/pic.o \
    ${BUILD_DIR}/rtc.o \
    ${BUILD_DIR}/keyboard.o \
    ${BUILD_DIR}/font.o \
    ${BUILD_DIR}/mouse.o \
    ${BUILD_DIR}/graphics.o \
    ${BUILD_DIR}/fluent_ui.o \
    ${BUILD_DIR}/login_screen.o \
    ${BUILD_DIR}/sysinfo.o \
    ${BUILD_DIR}/settings_app.o \
    -o jenos.bin

echo ""
echo "========================================="
echo "  BUILD SUCCESSFUL: jenos.bin"
echo "========================================="
echo "Run: qemu-system-i386 -kernel jenos.bin -device bochs-display"
