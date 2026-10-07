/* interrupt_stubs.s */

.macro ISR_NOERRCODE num
  .global isr\num
  .type isr\num, @function
  isr\num:
    cli
    push $0
    push $\num
    jmp isr_common_stub
.endm

.macro ISR_ERRCODE num
  .global isr\num
  .type isr\num, @function
  isr\num:
    cli
    push $\num
    jmp isr_common_stub
.endm

.macro IRQ num, irq_num
  .global irq\irq_num
  .type irq\irq_num, @function
  irq\irq_num:
    cli
    push $0
    push $\num
    jmp irq_common_stub
.endm

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_ERRCODE   21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_ERRCODE   29
ISR_ERRCODE   30
ISR_NOERRCODE 31

IRQ 32, 0
IRQ 33, 1
IRQ 34, 2
IRQ 35, 3
IRQ 36, 4
IRQ 37, 5
IRQ 38, 6
IRQ 39, 7
IRQ 40, 8
IRQ 41, 9
IRQ 42, 10
IRQ 43, 11
IRQ 44, 12
IRQ 45, 13
IRQ 46, 14
IRQ 47, 15

.extern isr_handler
.extern irq_handler

isr_common_stub:
    pusha                    # Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax
    mov %ds, %ax             # Lower 16-bits of eax = ds.
    movzx %ax, %eax
    push %eax                # save the data segment descriptor

    mov $0x10, %ax           # load the kernel data segment descriptor
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    cld
    push %esp                # pass the interrupt frame by pointer
    call isr_handler
    add $4, %esp

    pop %ebx                 # reload the original data segment descriptor
    mov %bx, %ds
    mov %bx, %es
    mov %bx, %fs
    mov %bx, %gs

    popa                     # Pops edi,esi,ebp...
    add $8, %esp             # Cleans up the pushed error code and pushed ISR number
    iret                     # pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP

irq_common_stub:
    pusha
    mov %ds, %ax
    movzx %ax, %eax
    push %eax

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    cld
    push %esp                # pass the interrupt frame by pointer
    call irq_handler
    add $4, %esp

    pop %ebx
    mov %bx, %ds
    mov %bx, %es
    mov %bx, %fs
    mov %bx, %gs

    popa
    add $8, %esp
    iret

.global idt_flush
.type idt_flush, @function
idt_flush:
    mov 4(%esp), %eax        # Get the pointer to the IDT, passed as a parameter.
    lidt (%eax)              # Load the IDT pointer.
    ret
