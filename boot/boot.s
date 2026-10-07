/* multiboot header constants */
.set ALIGN,    1<<0             /* align loaded modules on page boundaries */
.set MEMINFO,  1<<1             /* provide memory map */
.set FLAGS,    ALIGN | MEMINFO  /* multiboot 'flag' field */
.set MAGIC,    0x1BADB002       /* 'magic number' lets bootloader find the header */
.set CHECKSUM, -(MAGIC + FLAGS) /* checksum of above, to prove we are multiboot */

/* Declare multiboot header */
.section .multiboot, "a"
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

/* Set up the stack */
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

/* Entry point */
.section .text
.global _start
.type _start, @function
_start:
	# Set up stack
	mov $stack_top, %esp

	# Clear the direction flag (System V ABI requirement)
	cld

	# Push multiboot info structure pointer (in EBX)
	push %ebx
	# Push multiboot magic number (in EAX)
	push %eax

	# Call the global constructors (if any are added later)
	# call _init

	# Transfer control to the main kernel
	call kernel_main

	/* Infinite loop if the system has nothing more to do */
	cli
1:	hlt
	jmp 1b

.size _start, . - _start
