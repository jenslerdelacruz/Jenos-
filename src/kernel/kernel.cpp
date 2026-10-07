#include <stdint.h>
#include <stddef.h>

#include "gdt.h"
#include "idt.h"
#include "interrupts.h"
#include "pic.h"
#include "keyboard.h"
#include "mouse.h"
#include "graphics.h"
#include "font.h"
#include "icons.h"
#include "wallpaper.h"
#include "settings.h"
#include "fluent_ui.h"
#include "login_screen.h"
#include "settings.h"

// Minimal COM1 serial logger for early debugging (no external files added).
#include "io.h"

static inline void serial_init() {
    // Disable interrupts
    outb(0x3F8 + 1, 0x00);
    // Enable DLAB (set baud rate divisor)
    outb(0x3F8 + 3, 0x80);
    // Set divisor to 3 (lo byte) 38400 baud
    outb(0x3F8 + 0, 0x03);
    // hi byte
    outb(0x3F8 + 1, 0x00);
    // 8 bits, no parity, one stop bit
    outb(0x3F8 + 3, 0x03);
    // Enable FIFO, clear them, with 14-byte threshold
    outb(0x3F8 + 2, 0xC7);
    // IRQs enabled, RTS/DSR set
    outb(0x3F8 + 4, 0x0B);
}

static inline int serial_is_transmit_empty() {
    return inb(0x3F8 + 5) & 0x20;
}

static void serial_putc(char c) {
    while (!(inb(0x3F8 + 5) & 0x20)) { /* wait */ }
    outb(0x3F8 + 0, (uint8_t)c);
}

void serial_write(const char* s) {
    while (*s) {
        serial_putc(*s++);
    }
}

void serial_write_ln(const char* s) {
    serial_write(s);
    serial_putc('\r');
    serial_putc('\n');
}

static void serial_write_hex(uint32_t v) {
    const char* hex = "0123456789ABCDEF";
    serial_putc('0'); serial_putc('x');
    for (int i = 7; i >= 0; --i) {
        uint8_t d = (v >> (i * 4)) & 0xF;
        serial_putc(hex[d]);
    }
}

void serial_write_dec(uint32_t v) {
    if (v == 0) { serial_putc('0'); return; }
    char buf[12]; int i = 0;
    while (v > 0 && i < (int)sizeof(buf)-1) {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    }
    for (int j = i - 1; j >= 0; --j) serial_putc(buf[j]);
}

// Debug variable from graphics for counting pixels written
extern volatile uint32_t debug_draw_count;

// ====================================================================
// OS State Machine
// ====================================================================
enum system_state {
    STATE_LOGIN,
    STATE_DESKTOP,
    STATE_SETTINGS
};

static system_state current_state = STATE_LOGIN;

// Login screen state
char username_buffer[32] = {0};
size_t username_length = 0;
char password_buffer[32] = {0};
size_t password_length = 0;
int active_input_field = 0; // 0 = Username, 1 = Password

// Desktop state
bool settings_open = false; // Used for windowed mode, but we will keep it for compatibility if needed. Actually we'll use current_state = STATE_SETTINGS.
static bool start_menu_open = false;
static int selected_desktop_icon = -1; // -1 = none, 0 = Computer, 1 = Settings, 2 = Terminal, 3 = Trash

extern void reset_mouse_cursor_state();
extern void rtc_read_time(int* hours, int* minutes, int* seconds);

// Forward declarations
void draw_desktop_bg();
static void draw_desktop();
static void draw_start_menu();

// ====================================================================
// Draw Wallpaper
// ====================================================================
static void draw_wallpaper() {
    draw_bitmap_cover(wallpaper_data, WALLPAPER_WIDTH, WALLPAPER_HEIGHT,
                      screen_width, screen_height);
}

// (Old inline draw_login_screen removed — LoginScreenManager handles all login UI)

// ====================================================================
// DESKTOP BACKGROUND (wallpaper + icons + taskbar, no swap_buffers)
// ====================================================================
void draw_desktop_bg() {
    reset_mouse_cursor_state();

    // Draw wallpaper
    draw_wallpaper();

    // --- Desktop Icons (Vector Style) ---
    const size_t desktop_icon_size = 40;
    draw_vector_folder(30, 30, selected_desktop_icon == 0, desktop_icon_size);
    draw_string(22, 80, "Files", 0xFFFFFF, 0, true);

    draw_vector_settings(30, 110, selected_desktop_icon == 1, desktop_icon_size);
    draw_string(18, 160, "Settings", 0xFFFFFF, 0, true);

    draw_vector_terminal(30, 180, selected_desktop_icon == 2, desktop_icon_size);
    draw_string(14, 230, "Terminal", 0xFFFFFF, 0, true);

    draw_vector_trash(30, 250, selected_desktop_icon == 3, desktop_icon_size);
    draw_string(24, 300, "Trash", 0xFFFFFF, 0, true);

    // --- Taskbar ---
    const size_t taskbar_height = 56;
    size_t taskbar_y = screen_height - taskbar_height;
    draw_rect(0, taskbar_y, screen_width, taskbar_height, 0x11111B);
    draw_rect(0, taskbar_y, screen_width, 1, 0x313244);

    const size_t dock_icon_size = 44;
    const size_t dock_spacing = 56;
    const size_t dock_total_width = dock_icon_size * 3 + dock_spacing * 2;
    size_t dock_start = (screen_width - dock_total_width) / 2;
    draw_vector_start(dock_start, taskbar_y + 6, dock_icon_size);
    draw_vector_folder(dock_start + dock_icon_size + dock_spacing, taskbar_y + 6, false, dock_icon_size);
    draw_vector_terminal(dock_start + 2 * (dock_icon_size + dock_spacing), taskbar_y + 6, false, dock_icon_size);

    size_t tray_x = screen_width - 54;
    draw_rect(tray_x - 10, taskbar_y + 7, 1, 26, 0x45475A);
    draw_vector_wifi(tray_x, taskbar_y + 7);
    draw_vector_battery(tray_x + 25, taskbar_y + 14);

    int h, m, s;
    rtc_read_time(&h, &m, &s);
    char time_str[16];
    settings_format_time(h, m, time_str);
    size_t time_length = 0;
    while (time_str[time_length]) ++time_length;
    draw_string(screen_width - 16 - time_length * 8, taskbar_y + 12,
                time_str, 0xCdd6F4, 0x11111B);

    draw_rect(dock_start, taskbar_y + 2, dock_total_width, 2, settings_get_accent_color());
    if (sys_config.high_contrast) {
        draw_rect(0, taskbar_y, screen_width, 1, 0xFFFFFF);
    }
}

// ====================================================================
// DESKTOP (full draw + swap)
// ====================================================================
static void draw_desktop() {
    draw_desktop_bg();

    if (start_menu_open) {
        draw_start_menu();
    }

    swap_buffers();
    mouse_draw_cursor();
}

// Deleted draw_settings_window since it's replaced by the new Settings app
// ====================================================================
// START MENU
// ====================================================================
static void draw_start_menu() {
    int mx = 90, my = 370, mw = 200, mh = 185;

    // Menu shadow
    draw_rounded_rect(mx + 4, my + 4, mw, mh, 12, 0x05050A);
    // Menu body
    draw_rounded_rect(mx, my, mw, mh, 12, 0x1E1E2E);

    // User info at top
    draw_filled_circle(mx + 24, my + 25, 12, 0x313244);
    draw_filled_circle(mx + 24, my + 21, 5, 0xA6ADC8);
    draw_rounded_rect(mx + 16, my + 28, 16, 8, 4, 0xA6ADC8);
    draw_string(mx + 42, my + 18, username_buffer, 0xCdd6F4, 0x1E1E2E);

    // Separator
    draw_rect(mx + 15, my + 48, mw - 30, 1, 0x313244);

    // Menu items
    // Files
    draw_rounded_rect(mx + 10, my + 55, mw - 20, 28, 6, 0x1E1E2E);
    draw_string(mx + 40, my + 62, "Files", 0xCdd6F4, 0x1E1E2E);

    // Settings
    draw_rounded_rect(mx + 10, my + 88, mw - 20, 28, 6, 0x1E1E2E);
    draw_string(mx + 40, my + 95, "Settings", 0xCdd6F4, 0x1E1E2E);

    // Separator
    draw_rect(mx + 15, my + 125, mw - 30, 1, 0x313244);

    // Shut Down
    draw_rounded_rect(mx + 10, my + 135, mw - 20, 35, 8, 0x1E1E2E);
    draw_string(mx + 40, my + 145, "Shut Down", 0xF38BA8, 0x1E1E2E);
}

#include "multiboot.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include "memory/kheap.h"

static void print_text(const char* str, int row) {
    volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
    int i = 0;
    while (str[i]) {
        vga[row * 80 + i] = (uint16_t)str[i] | (0x0F << 8);
        i++;
    }
}

static void print_hex(uint32_t value, int row) {
    char buf[11];
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 0; i < 8; i++) {
        uint8_t nib = (value >> ((7 - i) * 4)) & 0xF;
        buf[2 + i] = (nib < 10) ? ('0' + nib) : ('A' + nib - 10);
    }
    buf[10] = '\0';
    print_text(buf, row);
}

// ====================================================================
// KERNEL MAIN
// ====================================================================
extern "C" void kernel_main(uint32_t magic, multiboot_info* mbd) {
    // Initialize serial first so we can debug early.
    serial_init();
    serial_write_ln("kernel_main entered");

    print_text("Booting JenOS...", 0);
    serial_write_ln("Booting JenOS...");

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        serial_write_ln("Error: Bad Magic!");
        print_text("Error: Bad Magic!", 1);
        while (1) __asm__ volatile("hlt");
    }

    serial_write_ln("Initializing PMM...");
    print_text("Initializing PMM...", 1);
    pmm_init(mbd);
    serial_write_ln("PMM initialized");
    
    serial_write_ln("Initializing VMM...");
    print_text("Initializing VMM...", 2);
    vmm_init();
    serial_write_ln("VMM initialized");
    
    serial_write_ln("Initializing KHeap...");
    print_text("Initializing KHeap...", 3);
    kheap_init();
    serial_write_ln("KHeap initialized");

    serial_write_ln("Initializing Core Systems...");
    print_text("Initializing Core Systems...", 4);
    serial_write_ln("init_gdt");
    init_gdt();
    serial_write_ln("GDT initialized");

    serial_write_ln("init_idt");
    init_idt();
    serial_write_ln("IDT initialized");

    serial_write_ln("init_interrupts");
    init_interrupts();
    serial_write_ln("interrupts initialized");

    serial_write_ln("pic_remap");
    pic_remap(0x20, 0x28);
    serial_write_ln("PIC initialized");

    serial_write_ln("init_keyboard");
    init_keyboard();
    serial_write_ln("Keyboard initialized");

    // 3. Initialize Graphics & UI
    serial_write_ln("init_graphics entered");
    init_graphics();
    serial_write_ln("init_graphics returned");
    settings_init_config();

    print_text("Graphics init done", 5);
    serial_write_ln("Graphics init done");

    serial_write("screen_width: "); serial_write_dec((uint32_t)screen_width); serial_write_ln("");
    serial_write("screen_height: "); serial_write_dec((uint32_t)screen_height); serial_write_ln("");
    serial_write("framebuffer addr (pre-check): "); serial_write_hex((uint32_t)framebuffer); serial_write_ln("");
    serial_write("backbuffer addr (pre-check): "); serial_write_hex((uint32_t)backbuffer); serial_write_ln("");

    serial_write_ln("Checking framebuffer/backbuffer");
    if (framebuffer == 0) {
        serial_write_ln("FrameBuffer: NULL");
        print_text("FrameBuffer: NULL", 6);
    } else {
        serial_write_ln("FrameBuffer: OK");
        serial_write("FrameBuffer addr: "); serial_write_hex((uint32_t)framebuffer); serial_write_ln("");
        print_text("FrameBuffer: OK", 6);
        print_hex((uint32_t)framebuffer, 7);
    }
    if (backbuffer == 0) {
        serial_write_ln("BackBuffer: NULL");
        print_text("BackBuffer: NULL", 8);
    } else {
        serial_write_ln("BackBuffer: OK");
        print_text("BackBuffer: OK", 8);
    }

    // Memory Test: dynamically allocate a string!
    char* test_str = (char*)kzalloc(128);
    if (test_str) {
        test_str[0] = 'H'; test_str[1] = 'e'; test_str[2] = 'a'; test_str[3] = 'p';
        test_str[4] = ' '; test_str[5] = 'O'; test_str[6] = 'K'; test_str[7] = '!';
        test_str[8] = 0;
        // We'll draw it on the login screen
    }

    serial_write_ln("draw_login_screen");
    
    // Initialize the LoginScreenManager (Fluent Design login UI)
    LoginScreenManager::Initialize();
    serial_write_ln("LoginScreenManager initialized");
    
    // Render the login screen
    LoginScreenManager::Render();
    serial_write_ln("LoginScreenManager rendered");

    if (test_str) {
        draw_string(10, 10, test_str, 0xA6E3A1, 0, true);
        kfree(test_str);
    }

    // 4. Enable interrupts and hardware
    serial_write_ln("init_mouse");
    init_mouse();
    serial_write_ln("mouse initialized");

    __asm__ volatile("sti");
    serial_write_ln("sti executed, entering hlt loop");

    while (1) {
        __asm__ volatile("hlt");
    }
}

// ====================================================================
// RETURN TO DESKTOP (from Settings)
// ====================================================================
void return_to_desktop() {
    current_state = STATE_DESKTOP;
    extern void draw_desktop();
    draw_desktop();
}

void open_user_profile() {
    settings_current_page = PAGE_USERS;
    settings_draw();
}

void enter_sleep_mode() {
    draw_rect(0, 0, screen_width, screen_height, 0);
    swap_buffers();
    __asm__ volatile ("sti; hlt" : : : "memory");
    settings_draw();
}

void request_restart() {
    __asm__ volatile ("cli" : : : "memory");

    for (volatile uint32_t timeout = 0; timeout < 1000000; ++timeout) {
        if ((inb(0x64) & 0x02) == 0) {
            outb(0x64, 0xFE);
            break;
        }
    }

    struct __attribute__((packed)) idt_pointer {
        uint16_t limit;
        uint32_t base;
    } invalid_idt = {0, 0};
    __asm__ volatile ("lidt %0; int3" : : "m"(invalid_idt) : "memory");
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

void request_shutdown() {
    __asm__ volatile ("cli" : : : "memory");
    outw(0x0604, 0x2000);
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

// ====================================================================
// KEYBOARD INPUT HANDLER
// ====================================================================
void handle_keypress(char c) {
    if (current_state == STATE_SETTINGS) {
        settings_handle_key(c);
        return;
    }

    if (current_state == STATE_LOGIN) {
        LoginScreenManager::HandleKeyPress(c);
        
        // Check if login was successful
        if (LoginScreenManager::ShouldTransitionToDesktop()) {
            current_state = STATE_DESKTOP;
            draw_desktop();
        } else {
            LoginScreenManager::Render();
        }
        return;
    }
}

// ====================================================================
// MOUSE CLICK HANDLER
// ====================================================================
void handle_mouse_click(int x, int y) {
    if (current_state == STATE_SETTINGS) {
        settings_handle_click(x, y);
        return;
    }

    if (current_state == STATE_LOGIN) {
        LoginScreenManager::HandleMouseClick(x, y);
        
        // Check if login was successful
        if (LoginScreenManager::ShouldTransitionToDesktop()) {
            current_state = STATE_DESKTOP;
            draw_desktop();
        } else {
            LoginScreenManager::Render();
        }
        return;
    } else if (current_state == STATE_DESKTOP) {

        // If start menu is open, handle its clicks first
        if (start_menu_open) {
            int mx = 90, my = 370;
            // Settings item: y = my+88..my+116
            if (x >= mx + 10 && x <= mx + 190 && y >= my + 88 && y <= my + 116) {
                start_menu_open = false;
                current_state = STATE_SETTINGS;
                settings_draw();
                return;
            }
            // Shut Down item: y = my+135..my+170
            if (x >= mx + 10 && x <= mx + 190 && y >= my + 135 && y <= my + 170) {
                // Return to login screen (simulated shut down)
                current_state = STATE_LOGIN;
                settings_open = false;
                start_menu_open = false;
                selected_desktop_icon = -1;
                username_length = 0; username_buffer[0] = 0;
                password_length = 0; password_buffer[0] = 0;
                active_input_field = 0;
                LoginScreenManager::Initialize();
                LoginScreenManager::Render();
                return;
            }
            // Click anywhere else closes the menu
            start_menu_open = false;
            draw_desktop();
            return;
        }

        // Desktop icon clicks
        // Files: around x=30..62, y=30..56
        if (x >= 22 && x <= 70 && y >= 26 && y <= 70) {
            selected_desktop_icon = 0;
            draw_desktop();
        }
        // Settings: around x=30..62, y=90..124
        else if (x >= 22 && x <= 70 && y >= 86 && y <= 134) {
            selected_desktop_icon = 1;
            current_state = STATE_SETTINGS;
            settings_draw();
        }
        // Terminal: around x=30..62, y=155..186
        else if (x >= 22 && x <= 70 && y >= 151 && y <= 196) {
            selected_desktop_icon = 2;
            draw_desktop();
        }
        // Trash: around x=30..62, y=220..252
        else if (x >= 22 && x <= 70 && y >= 216 && y <= 262) {
            selected_desktop_icon = 3;
            draw_desktop();
        }
        // Start/Home button in dock
        else {
            const int taskbar_height = 56;
            int taskbar_y = (int)screen_height - taskbar_height;
            const int dock_icon_size = 44;
            const int dock_spacing = 56;
            const int dock_total_width = dock_icon_size * 3 + dock_spacing * 2;
            int button_x = (int)((screen_width - dock_total_width) / 2);
            if (x >= button_x && x <= button_x + dock_icon_size && y >= taskbar_y && y <= taskbar_y + taskbar_height) {
                start_menu_open = true;
                draw_desktop();
                draw_start_menu();
                return;
            }
            // Click on empty desktop deselects
            if (selected_desktop_icon != -1) {
                selected_desktop_icon = -1;
                draw_desktop();
            }
        }
    }
}
