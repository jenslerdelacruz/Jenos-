#include "keyboard.h"
#include "interrupts.h"
#include "io.h"

// We will send key presses to the UI layer in kernel.cpp
extern void handle_keypress(char c);

const char scancode_to_ascii[] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-', 0, '5', 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static void keyboard_callback(registers_t* regs) {
    (void)regs;
    
    // Read the scancode from the keyboard controller data port (0x60)
    uint8_t scancode = inb(0x60);
    if (!(scancode & 0x80)) {
        if (scancode < sizeof(scancode_to_ascii)) {
            char ch = scancode_to_ascii[scancode];
            if (ch != 0) {
                handle_keypress(ch);
            }
        }
    }
}

void init_keyboard() {
    register_interrupt_handler(33, &keyboard_callback);
}
