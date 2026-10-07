#include "mouse.h"
#include "interrupts.h"
#include "io.h"
#include "graphics.h"
#include "icons.h"
#include "settings.h"

int mouse_x = 0;
int mouse_y = 0;
int old_mouse_x = 0;
int old_mouse_y = 0;

uint8_t mouse_cycle = 0;
int8_t  mouse_byte[3];

// Wait for the mouse to be ready for reads or writes
static void mouse_wait(uint8_t a_type) {
    uint32_t _time_out = 100000;
    if (a_type == 0) {
        while (_time_out--) {
            if ((inb(0x64) & 1) == 1) return;
        }
        return;
    } else {
        while (_time_out--) {
            if ((inb(0x64) & 2) == 0) return;
        }
        return;
    }
}

static void mouse_write(uint8_t a_write) {
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(1);
    outb(0x60, a_write);
}

static uint8_t mouse_read() {
    mouse_wait(0);
    return inb(0x60);
}

#define CURSOR_BUFFER_SIZE 44

// Buffer to save the pixels underneath the cursor
uint32_t saved_pixels[CURSOR_BUFFER_SIZE * CURSOR_BUFFER_SIZE];
uint8_t cursor_drawn = 0;
static int pointer_speed = 5;
static bool natural_scroll = false;
static bool large_cursor = false;
static int x_speed_remainder = 0;
static int y_speed_remainder = 0;

void mouse_set_speed(int speed) {
    if (speed < 1) speed = 1;
    if (speed > 10) speed = 10;
    pointer_speed = speed;
    x_speed_remainder = 0;
    y_speed_remainder = 0;
}

void mouse_set_natural_scroll(bool enabled) {
    natural_scroll = enabled;
}

void mouse_set_large_cursor(bool enabled) {
    if (large_cursor != enabled) cursor_drawn = 0;
    large_cursor = enabled;
}

static int cursor_scale() {
    return large_cursor ? 2 : 1;
}

static int cursor_region_size() {
    return 22 * cursor_scale();
}

void reset_mouse_cursor_state() {
    cursor_drawn = 0;
}

// Read a pixel from the backbuffer
static uint32_t read_pixel(size_t x, size_t y) {
    extern uint32_t* backbuffer;
    extern size_t screen_width;
    extern size_t screen_height;
    if (!backbuffer || x >= screen_width || y >= screen_height) return 0;
    return backbuffer[y * screen_width + x];
}

// Save the pixels underneath the cursor area
static void save_under_cursor(int x, int y) {
    int size = cursor_region_size();
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            saved_pixels[i * CURSOR_BUFFER_SIZE + j] = read_pixel(x + j, y + i);
        }
    }
}

// Restore the saved pixels to erase the cursor cleanly
static void restore_under_cursor(int x, int y) {
    int size = cursor_region_size();
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            draw_pixel(x + j, y + i, saved_pixels[i * CURSOR_BUFFER_SIZE + j]);
        }
    }
}

static void clamp_mouse_position() {
    if (mouse_x < 0) mouse_x = 0;
    if (mouse_y < 0) mouse_y = 0;
    if (mouse_x > (int)screen_width - cursor_region_size())
        mouse_x = (int)screen_width - cursor_region_size();
    if (mouse_y > (int)screen_height - cursor_region_size())
        mouse_y = (int)screen_height - cursor_region_size();
}

// Draw the graphical mouse cursor
void mouse_draw_cursor() {
    uint8_t had_cursor = cursor_drawn;
    int previous_x = old_mouse_x;
    int previous_y = old_mouse_y;

    if (had_cursor) {
        restore_under_cursor(previous_x, previous_y);
    }

    clamp_mouse_position();

    // Step 2: Save the pixels at the new position BEFORE drawing the cursor
    save_under_cursor(mouse_x, mouse_y);

    // Step 3: Draw the new cursor
    draw_cursor(mouse_x, mouse_y, cursor_scale());

    int region_size = cursor_region_size();
    if (had_cursor) present_region(previous_x, previous_y, region_size, region_size);
    present_region(mouse_x, mouse_y, region_size, region_size);

    old_mouse_x = mouse_x;
    old_mouse_y = mouse_y;
    cursor_drawn = 1;
}

static void mouse_callback(registers_t* regs) {
    uint8_t status = inb(0x64);
    
    if (!(status & 1)) return; // No data
    if (!(status & 0x20)) return; // Not from mouse

    int8_t mouse_in = inb(0x60);

    switch(mouse_cycle) {
        case 0:
            if (mouse_in & 0x08) {
                mouse_byte[0] = mouse_in;
                mouse_cycle++;
            }
            break;
        case 1:
            mouse_byte[1] = mouse_in;
            mouse_cycle++;
            break;
        case 2:
            mouse_byte[2] = mouse_in;
            
            {
                if (!(mouse_byte[0] & 0xC0)) {
                    int scaled_x = mouse_byte[1] * pointer_speed + x_speed_remainder;
                    int scaled_y = mouse_byte[2] * pointer_speed + y_speed_remainder;
                    int dx = scaled_x / 5;
                    int dy = scaled_y / 5;
                    x_speed_remainder = scaled_x - dx * 5;
                    y_speed_remainder = scaled_y - dy * 5;
                    mouse_x += dx;
                    mouse_y += natural_scroll ? dy : -dy;
                }
                clamp_mouse_position();
            }

            // Detect mouse clicks (left click)
            static uint8_t left_down = 0;
            if ((mouse_byte[0] & 0x01) && !left_down) {
                left_down = 1;
                extern void handle_mouse_click(int x, int y);
                handle_mouse_click(mouse_x, mouse_y);
            } else if (!(mouse_byte[0] & 0x01)) {
                left_down = 0;
            }

            // Erase old cursor and draw new one
            mouse_draw_cursor();
            
            mouse_cycle = 0;
            break;
    }
}

void init_mouse() {
    uint8_t _status;

    extern size_t screen_width;
    extern size_t screen_height;
    mouse_x = (int)screen_width / 2;
    mouse_y = (int)screen_height / 2;
    mouse_set_speed(sys_config.mouse_speed);
    mouse_set_natural_scroll(sys_config.mouse_natural_scroll);
    mouse_set_large_cursor(sys_config.large_cursor);
    old_mouse_x = mouse_x;
    old_mouse_y = mouse_y;

    mouse_wait(1);
    outb(0x64, 0xA8);

    mouse_wait(1);
    outb(0x64, 0x20);
    mouse_wait(0);
    _status = (inb(0x60) | 2);
    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, _status);

    mouse_write(0xF6);
    mouse_read();

    mouse_write(0xF4);
    mouse_read();

    register_interrupt_handler(44, &mouse_callback);

    // Initial draw
    mouse_draw_cursor();
}
