#include "interrupts.h"
#include "idt.h"

// Defined in io.h (we'll make this later)
#include "io.h"

isr_t interrupt_handlers[256];

extern void serial_write(const char* s);
extern void serial_write_ln(const char* s);
extern void serial_write_dec(uint32_t value);

static void serial_write_hex(uint32_t value) {
    static const char digits[] = "0123456789ABCDEF";
    char buffer[8];
    for (int i = 7; i >= 0; --i) {
        buffer[7 - i] = digits[(value >> (i * 4)) & 0xF];
    }
    for (int i = 0; i < 8; ++i) {
        char character[2] = {buffer[i], '\0'};
        serial_write(character);
    }
}

static const char* exception_name(uint32_t vector) {
    static const char* const names[32] = {
        "Divide Error", "Debug", "Non-Maskable Interrupt", "Breakpoint",
        "Overflow", "BOUND Range Exceeded", "Invalid Opcode", "Device Not Available",
        "Double Fault", "Coprocessor Segment Overrun", "Invalid TSS",
        "Segment Not Present", "Stack-Segment Fault", "General Protection Fault",
        "Page Fault", "Reserved", "x87 Floating-Point Exception",
        "Alignment Check", "Machine Check", "SIMD Floating-Point Exception",
        "Virtualization Exception", "Control Protection Exception", "Reserved",
        "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
        "Hypervisor Injection Exception", "VMM Communication Exception",
        "Security Exception", "Reserved"
    };
    return vector < 32 ? names[vector] : "Unknown Exception";
}

static __attribute__((noreturn)) void exception_panic(registers_t* regs) {
    serial_write("FATAL: CPU exception ");
    serial_write_dec(regs->int_no);
    serial_write(" (");
    serial_write(exception_name(regs->int_no));
    serial_write_ln(")");
    serial_write("  error=");
    serial_write_hex(regs->err_code);
    serial_write(" eip=");
    serial_write_hex(regs->eip);
    serial_write(" cs=");
    serial_write_hex(regs->cs);
    serial_write(" eflags=");
    serial_write_hex(regs->eflags);
    serial_write_ln("");

    if (regs->int_no == 14) {
        uint32_t fault_address;
        __asm__ volatile ("mov %%cr2, %0" : "=r"(fault_address));
        serial_write("  cr2=");
        serial_write_hex(fault_address);
        serial_write(" access=");
        serial_write((regs->err_code & 1) ? "protection-violation" : "not-present");
        serial_write((regs->err_code & 2) ? ",write" : ",read");
        serial_write((regs->err_code & 4) ? ",user" : ",supervisor");
        if (regs->err_code & 8) serial_write(",reserved-bit");
        if (regs->err_code & 16) serial_write(",instruction-fetch");
        serial_write_ln("");
    }

    for (;;) {
        __asm__ volatile ("cli; hlt" : : : "memory");
    }
}

extern "C" void isr_handler(registers_t* regs) {
    if (regs->int_no < 32) {
        if (interrupt_handlers[regs->int_no]) {
            interrupt_handlers[regs->int_no](regs);
            return;
        }
        exception_panic(regs);
    }
}

static bool pic_irq_is_spurious(uint32_t irq) {
    if (irq != 7 && irq != 15) return false;

    const uint16_t command_port = irq == 7 ? 0x20 : 0xA0;
    outb(command_port, 0x0B);
    return (inb(command_port) & 0x80) == 0;
}

extern "C" void irq_handler(registers_t* regs) {
    if (regs->int_no < 32 || regs->int_no > 47) return;

    const uint32_t irq = regs->int_no - 32;
    const bool spurious = pic_irq_is_spurious(irq);

    if (!spurious && interrupt_handlers[regs->int_no]) {
        interrupt_handlers[regs->int_no](regs);
    }

    if (spurious && irq == 7) return;

    if (irq >= 8) {
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);
}

void register_interrupt_handler(uint8_t n, isr_t handler) {
    interrupt_handlers[n] = handler;
}

// ISRs from Assembly
extern "C" {
    void isr0(); void isr1(); void isr2(); void isr3(); void isr4(); void isr5(); void isr6(); void isr7();
    void isr8(); void isr9(); void isr10(); void isr11(); void isr12(); void isr13(); void isr14(); void isr15();
    void isr16(); void isr17(); void isr18(); void isr19(); void isr20(); void isr21(); void isr22(); void isr23();
    void isr24(); void isr25(); void isr26(); void isr27(); void isr28(); void isr29(); void isr30(); void isr31();
    void irq0(); void irq1(); void irq2(); void irq3(); void irq4(); void irq5(); void irq6(); void irq7();
    void irq8(); void irq9(); void irq10(); void irq11(); void irq12(); void irq13(); void irq14(); void irq15();
}

void init_interrupts() {
    for (int i = 0; i < 256; i++) {
        interrupt_handlers[i] = 0;
    }

    // Exceptions
    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);
    idt_set_gate(1, (uint32_t)isr1, 0x08, 0x8E);
    idt_set_gate(2, (uint32_t)isr2, 0x08, 0x8E);
    idt_set_gate(3, (uint32_t)isr3, 0x08, 0x8E);
    idt_set_gate(4, (uint32_t)isr4, 0x08, 0x8E);
    idt_set_gate(5, (uint32_t)isr5, 0x08, 0x8E);
    idt_set_gate(6, (uint32_t)isr6, 0x08, 0x8E);
    idt_set_gate(7, (uint32_t)isr7, 0x08, 0x8E);
    idt_set_gate(8, (uint32_t)isr8, 0x08, 0x8E);
    idt_set_gate(9, (uint32_t)isr9, 0x08, 0x8E);
    idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
    idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
    idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
    idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
    idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
    idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
    idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
    idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
    idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
    idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
    idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
    idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);

    // Hardware Interrupts (IRQs) remaped to 32-47
    idt_set_gate(32, (uint32_t)irq0, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1, 0x08, 0x8E);
    idt_set_gate(34, (uint32_t)irq2, 0x08, 0x8E);
    idt_set_gate(35, (uint32_t)irq3, 0x08, 0x8E);
    idt_set_gate(36, (uint32_t)irq4, 0x08, 0x8E);
    idt_set_gate(37, (uint32_t)irq5, 0x08, 0x8E);
    idt_set_gate(38, (uint32_t)irq6, 0x08, 0x8E);
    idt_set_gate(39, (uint32_t)irq7, 0x08, 0x8E);
    idt_set_gate(40, (uint32_t)irq8, 0x08, 0x8E);
    idt_set_gate(41, (uint32_t)irq9, 0x08, 0x8E);
    idt_set_gate(42, (uint32_t)irq10, 0x08, 0x8E);
    idt_set_gate(43, (uint32_t)irq11, 0x08, 0x8E);
    idt_set_gate(44, (uint32_t)irq12, 0x08, 0x8E);
    idt_set_gate(45, (uint32_t)irq13, 0x08, 0x8E);
    idt_set_gate(46, (uint32_t)irq14, 0x08, 0x8E);
    idt_set_gate(47, (uint32_t)irq15, 0x08, 0x8E);
}
