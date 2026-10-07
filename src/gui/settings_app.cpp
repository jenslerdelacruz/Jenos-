#include "settings.h"
#include "graphics.h"
#include "font.h"
#include "ui_widgets.h"
#include "icons.h"
#include "sysinfo.h"
#include "rtc.h"
#include "mouse.h"

// ====================================================================
// JenOS Settings App — Unique Design (NOT macOS / Windows clone)
// Full-screen application with left sidebar + content panel
// ====================================================================

// Global state
SettingsPage settings_current_page = PAGE_DISPLAY;
os_config sys_config;
static system_info cached_sysinfo;
static bool sysinfo_loaded = false;
static bool power_confirm_visible = false;
static int power_confirm_action = 0;
static bool resolution_apply_failed = false;

#define POWER_ACTION_NONE     0
#define POWER_ACTION_RESTART  1
#define POWER_ACTION_SHUTDOWN 2

extern void open_user_profile();
extern void enter_sleep_mode();
extern void request_restart();
extern void request_shutdown();

static void cancel_power_confirm() {
    power_confirm_visible = false;
    power_confirm_action = POWER_ACTION_NONE;
}

// Layout constants for floating window
#define WINDOW_X     80
#define WINDOW_Y     50
#define WINDOW_W     640
#define WINDOW_H     580

#define SIDEBAR_W    160
#define TITLEBAR_H   40
#define CONTENT_X    (WINDOW_X + SIDEBAR_W)
#define CONTENT_Y    (WINDOW_Y + TITLEBAR_H)
#define CONTENT_W    (WINDOW_W - SIDEBAR_W)
#define CONTENT_H    (WINDOW_H - TITLEBAR_H)

// Page names for sidebar
static const char* page_names[PAGE_COUNT] = {
    "Display",
    "Appearance",
    "Time & Date",
    "Mouse",
    "Keyboard",
    "Sound",
    "Users",
    "Network",
    "Storage",
    "Power",
    "Notifications",
    "Accessibility",
    "Hardware",
    "Kernel",
    "About"
};

// Sidebar icon characters (simple identifiers drawn as colored dots)
static const uint32_t page_icon_colors[PAGE_COUNT] = {
    0x89B4FA, // Display - Blue
    0xCBA6F7, // Appearance - Mauve
    0xF9E2AF, // Time - Yellow
    0xA6E3A1, // Mouse - Green
    0x74C7EC, // Keyboard - Sapphire
    0xFAB387, // Sound - Peach
    0xB4BEFE, // Users - Lavender
    0x89B4FA, // Network - Blue
    0x94E2D5, // Storage - Teal
    0xA6E3A1, // Power - Green
    0xF38BA8, // Notifications - Red
    0xB4BEFE, // Accessibility - Lavender
    0xFAB387, // Hardware - Peach
    0x94E2D5, // Kernel - Teal
    0x89B4FA  // About - Blue
};

// Init default config values
void settings_init_config() {
    sys_config.accent_color_index = 0;
    sys_config.high_contrast = false;
    sys_config.mouse_speed = 5;
    sys_config.mouse_natural_scroll = false;
    sys_config.large_cursor = false;
    sys_config.time_24_hour = true;
    mouse_set_speed(sys_config.mouse_speed);
    mouse_set_natural_scroll(sys_config.mouse_natural_scroll);
    mouse_set_large_cursor(sys_config.large_cursor);
}

uint32_t settings_get_accent_color() {
    static const uint32_t accent_colors[] = {
        0x89B4FA, 0xB4BEFE, 0xA6E3A1, 0xFAB387, 0xCBA6F7, 0x94E2D5
    };
    int index = sys_config.accent_color_index;
    if (index < 0 || index >= 6) index = 0;
    return accent_colors[index];
}

void settings_format_time(int hours, int minutes, char* output) {
    if (sys_config.time_24_hour) {
        output[0] = '0' + ((hours / 10) % 10);
        output[1] = '0' + (hours % 10);
        output[2] = ':';
        output[3] = '0' + ((minutes / 10) % 10);
        output[4] = '0' + (minutes % 10);
        output[5] = '\0';
        return;
    }

    bool is_pm = hours >= 12;
    int display_hour = hours % 12;
    if (display_hour == 0) display_hour = 12;
    output[0] = '0' + (display_hour / 10);
    output[1] = '0' + (display_hour % 10);
    output[2] = ':';
    output[3] = '0' + ((minutes / 10) % 10);
    output[4] = '0' + (minutes % 10);
    output[5] = ' ';
    output[6] = is_pm ? 'P' : 'A';
    output[7] = 'M';
    output[8] = '\0';
}

// ====================================================================
// Helper: integer to string
// ====================================================================
static void int_to_str(int val, char* buf) {
    if (val == 0) { buf[0] = '0'; buf[1] = 0; return; }
    int i = 0;
    char tmp[12];
    bool neg = false;
    if (val < 0) { neg = true; val = -val; }
    while (val > 0) { tmp[i++] = '0' + (val % 10); val /= 10; }
    int j = 0;
    if (neg) buf[j++] = '-';
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = 0;
}

// helper: simple hex to string for PCI IDs
static void hex16_to_str(uint16_t val, char* buf) {
    const char* hex = "0123456789ABCDEF";
    buf[0] = '0'; buf[1] = 'x';
    buf[2] = hex[(val >> 12) & 0xF];
    buf[3] = hex[(val >> 8) & 0xF];
    buf[4] = hex[(val >> 4) & 0xF];
    buf[5] = hex[val & 0xF];
    buf[6] = 0;
}

// helper: concat two strings
static void str_cat(char* dst, const char* a, const char* b) {
    int i = 0;
    for (int j = 0; a[j]; j++) dst[i++] = a[j];
    for (int j = 0; b[j]; j++) dst[i++] = b[j];
    dst[i] = 0;
}

// helper: PCI class name
static const char* pci_class_name(uint8_t cls, uint8_t sub) {
    switch (cls) {
        case 0x00: return "Legacy Device";
        case 0x01: return "Storage Controller";
        case 0x02: return "Network Controller";
        case 0x03: return "Display Controller";
        case 0x04: return "Multimedia";
        case 0x05: return "Memory Controller";
        case 0x06: return (sub == 0x00) ? "Host Bridge" : (sub == 0x01) ? "ISA Bridge" : "Bridge Device";
        case 0x07: return "Communication";
        case 0x08: return "System Peripheral";
        case 0x0C: return "Serial Bus";
        default:   return "Other Device";
    }
}

// ====================================================================
// TITLE BAR
// ====================================================================
static void draw_titlebar() {
    // Window Shadow
    draw_rounded_rect(WINDOW_X + 6, WINDOW_Y + 6, WINDOW_W, WINDOW_H, 12, 0x05050A);

    // Main window background
    draw_rounded_rect(WINDOW_X, WINDOW_Y, WINDOW_W, WINDOW_H, 12, JenColor::Base);

    // Title bar — JenOS unique style with gradient stripe
    draw_rounded_rect(WINDOW_X, WINDOW_Y, WINDOW_W, TITLEBAR_H, 12, JenColor::Crust);
    draw_rect(WINDOW_X, WINDOW_Y + TITLEBAR_H - 10, WINDOW_W, 10, JenColor::Crust); // Square bottom
    
    // Accent stripe at the very top (2px)
    draw_rect(WINDOW_X + 10, WINDOW_Y, WINDOW_W - 20, 2, settings_get_accent_color());
    
    // Back arrow (← using triangle shape)
    draw_string(WINDOW_X + 15, WINDOW_Y + 13, "<", settings_get_accent_color(), JenColor::Crust);
    
    // Title with breadcrumb
    draw_string(WINDOW_X + 35, WINDOW_Y + 13, "Settings", JenColor::Text, JenColor::Crust);
    draw_string(WINDOW_X + 115, WINDOW_Y + 13, ">", JenColor::Surface1, JenColor::Crust);
    draw_string(WINDOW_X + 130, WINDOW_Y + 13, page_names[settings_current_page], JenColor::Subtext, JenColor::Crust);
    
    // Close button (right side)
    draw_rounded_rect(WINDOW_X + WINDOW_W - 40, WINDOW_Y + 8, 28, 24, 6, JenColor::Surface0);
    draw_string(WINDOW_X + WINDOW_W - 32, WINDOW_Y + 12, "X", JenColor::Red, JenColor::Surface0);
}

// ====================================================================
// SIDEBAR
// ====================================================================
static int sidebar_scroll_offset = 0;

static void draw_sidebar() {
    // Sidebar background
    draw_rect(WINDOW_X, CONTENT_Y, SIDEBAR_W, CONTENT_H - 10, JenColor::Mantle);
    draw_rounded_rect(WINDOW_X, CONTENT_Y + CONTENT_H - 20, SIDEBAR_W, 20, 12, JenColor::Mantle);
    draw_rect(WINDOW_X, CONTENT_Y + CONTENT_H - 20, SIDEBAR_W, 10, JenColor::Mantle);
    
    // Right border
    draw_rect(WINDOW_X + SIDEBAR_W - 1, CONTENT_Y, 1, CONTENT_H, JenColor::Surface0);
    
    // Draw each category
    int y = CONTENT_Y + 8 - sidebar_scroll_offset;
    for (int i = 0; i < PAGE_COUNT; i++) {
        if (y < CONTENT_Y - 32 || y > CONTENT_Y + CONTENT_H - 34) { y += 34; continue; }
        
        bool selected = (i == (int)settings_current_page);
        
        if (selected) {
            draw_rect(WINDOW_X, y, 3, 30, settings_get_accent_color());
            draw_rounded_rect(WINDOW_X + 5, y, SIDEBAR_W - 10, 30, 8, JenColor::Surface0);
        }
        
        draw_filled_circle(WINDOW_X + 22, y + 15, 5, page_icon_colors[i]);
        
        uint32_t text_color = selected ? JenColor::Text : JenColor::Subtext;
        draw_string(WINDOW_X + 36, y + 8, page_names[i], text_color, 
                    selected ? JenColor::Surface0 : JenColor::Mantle);
        
        y += 34;
    }
}

// ====================================================================
// CONTENT PAGES
// ====================================================================

static void draw_page_header(const char* title, const char* subtitle) {
    draw_string(CONTENT_X + 24, CONTENT_Y + 16, title, JenColor::Text, JenColor::Base);
    if (subtitle) {
        draw_string(CONTENT_X + 24, CONTENT_Y + 36, subtitle, JenColor::Subtext, JenColor::Base);
    }
}

// ---- DISPLAY PAGE ----
static void draw_page_display() {
    draw_page_header("Display", "Select a supported BGA resolution");
    int cx = CONTENT_X + 24, cy = CONTENT_Y + 60;
    int cw = CONTENT_W - 48;
    
    draw_card(cx, cy, cw, 112);
    draw_string(cx + 16, cy + 12, "Resolution", JenColor::Text, JenColor::Surface0);
    char resolution_str[32];
    char width_str[16];
    char height_str[16];
    char temp_str[32];
    int_to_str((int)screen_width, width_str);
    int_to_str((int)screen_height, height_str);
    str_cat(temp_str, width_str, " x ");
    str_cat(resolution_str, temp_str, height_str);
    draw_string(cx + 16, cy + 32, resolution_str, JenColor::Subtext, JenColor::Surface0);
    int option_x = cx + cw - 210;
    uint32_t selected_color = settings_get_accent_color();
    draw_button(option_x, cy + 30, 92, 30, "1280x720",
                screen_width == 1280 ? selected_color : JenColor::Surface1);
    draw_button(option_x + 100, cy + 30, 94, 30, "1920x1080",
                screen_width == 1920 ? selected_color : JenColor::Surface1);
    draw_string(cx + 16, cy + 72, "32-bit color; modes supported by QEMU BGA",
                JenColor::Subtext, JenColor::Surface0);
    if (resolution_apply_failed) {
        draw_string(cx + 16, cy + 92, "Could not apply mode; current resolution was kept.",
                    JenColor::Red, JenColor::Surface0);
    }
    
    cy += 124;
    draw_card(cx, cy, cw, 50);
    draw_string(cx + 16, cy + 16, "Double buffering: Required and enabled",
                JenColor::Text, JenColor::Surface0);
    
    cy += 62;
    draw_card(cx, cy, cw, 80);
    draw_string(cx + 16, cy + 12, "Graphics Adapter", settings_get_accent_color(), JenColor::Surface0);
    draw_string(cx + 16, cy + 32, "Bochs VBE Graphics Adapter", JenColor::Text, JenColor::Surface0);
    draw_string(cx + 16, cy + 52, "Linear framebuffer (PCI), 32-bit color", JenColor::Subtext, JenColor::Surface0);
}

// ---- APPEARANCE PAGE ----
static void draw_page_appearance() {
    draw_page_header("Appearance", "Customize your JenOS experience");
    int cx = CONTENT_X + 24, cy = CONTENT_Y + 60;
    int cw = CONTENT_W - 48;
    
    // Theme card
    draw_card(cx, cy, cw, 50);
    draw_string(cx + 16, cy + 16, "Theme: Catppuccin Mocha (Dark)", JenColor::Text, JenColor::Surface0);
    
    // Accent color picker
    cy += 62;
    draw_card(cx, cy, cw, 90);
    draw_string(cx + 16, cy + 12, "Accent Color", JenColor::Text, JenColor::Surface0);
    
    uint32_t accent_colors[] = {0x89B4FA, 0xB4BEFE, 0xA6E3A1, 0xFAB387, 0xCBA6F7, 0x94E2D5};
    const char* accent_names[] = {"Blue", "Lavender", "Green", "Peach", "Mauve", "Teal"};
    
    for (int i = 0; i < 6; i++) {
        int bx = cx + 16 + i * 72;
        int by = cy + 38;
        draw_rounded_rect(bx, by, 60, 36, 8, accent_colors[i]);
        if (i == sys_config.accent_color_index) {
            draw_rounded_rect(bx - 2, by - 2, 64, 40, 10, JenColor::Text);
            draw_rounded_rect(bx, by, 60, 36, 8, accent_colors[i]);
        }
        // tiny label
        draw_string(bx + 4, by + 40, accent_names[i], JenColor::Subtext, JenColor::Surface0);
    }
    
    // Font info
    cy += 165;
    draw_card(cx, cy, cw, 50);
    draw_string(cx + 16, cy + 16, "Font: Consolas 8x16 (Anti-aliased)", JenColor::Text, JenColor::Surface0);
}

// ---- TIME & DATE PAGE ----
static void draw_page_time() {
    draw_page_header("Time & Date", "System clock settings");
    int cx = CONTENT_X + 24, cy = CONTENT_Y + 60;
    int cw = CONTENT_W - 48;
    
    // Current time — read from RTC
    int h, m, s;
    rtc_read_time(&h, &m, &s);
    
    (void)s;
    char time_str[16];
    settings_format_time(h, m, time_str);
    
    // Big time display card
    draw_card(cx, cy, cw, 100);
    draw_string(cx + 16, cy + 12, "Current Time", JenColor::Subtext, JenColor::Surface0);
    // Draw time string larger (by repeating)
    draw_string(cx + 16, cy + 38, time_str, settings_get_accent_color(), JenColor::Surface0);
    draw_string(cx + 16, cy + 68, "RTC local time; timezone is managed by the host/CMOS",
                JenColor::Subtext, JenColor::Surface0);
    
    // Source
    cy += 112;
    draw_card(cx, cy, cw, 50);
    draw_string(cx + 16, cy + 16, "Source: CMOS Real-Time Clock (RTC)", JenColor::Text, JenColor::Surface0);
    
    // 24h format toggle
    cy += 62;
    draw_card(cx, cy, cw, 50);
    draw_string(cx + 16, cy + 16, "24-Hour Format", JenColor::Text, JenColor::Surface0);
    draw_toggle(cx + cw - 64, cy + 14, sys_config.time_24_hour);
}

// ---- MOUSE PAGE ----
static void draw_page_mouse() {
    draw_page_header("Mouse & Touchpad", "Pointer speed and behavior");
    int cx = CONTENT_X + 24, cy = CONTENT_Y + 60;
    int cw = CONTENT_W - 48;
    
    // Pointer speed
    draw_card(cx, cy, cw, 70);
    draw_string(cx + 16, cy + 12, "Pointer Speed", JenColor::Text, JenColor::Surface0);
    draw_slider(cx + 16, cy + 40, cw - 32, sys_config.mouse_speed, 10);
    char speed_str[16];
    int_to_str(sys_config.mouse_speed, speed_str);
    draw_string(cx + cw - 36, cy + 52, speed_str, JenColor::Text, JenColor::Surface0);
    
    // Natural scroll
    cy += 82;
    draw_card(cx, cy, cw, 50);
    draw_string(cx + 16, cy + 16, "Natural Scrolling", JenColor::Text, JenColor::Surface0);
    draw_toggle(cx + cw - 64, cy + 14, sys_config.mouse_natural_scroll);
    
    // Cursor style info
    cy += 62;
    draw_card(cx, cy, cw, 50);
    draw_string(cx + 16, cy + 16,
                sys_config.large_cursor ? "Cursor: JenOS geometric (large)"
                                        : "Cursor: JenOS geometric (standard)",
                JenColor::Text, JenColor::Surface0);
}

// ---- KEYBOARD PAGE ----
static void draw_page_keyboard() {
    draw_page_header("Keyboard", "Input settings");
    int cx = CONTENT_X + 24, cy = CONTENT_Y + 60;
    int cw = CONTENT_W - 48;
    
    draw_card(cx, cy, cw, 70);
    draw_string(cx + 16, cy + 12, "Layout", settings_get_accent_color(), JenColor::Surface0);
    draw_string(cx + 16, cy + 36, "US QWERTY (Scancode Set 1)", JenColor::Text, JenColor::Surface0);
    
    cy += 82;
    draw_card(cx, cy, cw, 50);
    draw_string(cx + 16, cy + 16, "IRQ: 1 (PS/2 Keyboard)", JenColor::Subtext, JenColor::Surface0);
    
    cy += 62;
    draw_card(cx, cy, cw, 70);
    draw_string(cx + 16, cy + 12, "Shortcuts", settings_get_accent_color(), JenColor::Surface0);
    draw_string(cx + 16, cy + 36, "Tab = Switch Field  |  Enter = Confirm", JenColor::Subtext, JenColor::Surface0);
}

// ---- SOUND PAGE ----
static void draw_page_sound() {
    draw_page_header("Sound", "Audio hardware status");
    int cx = CONTENT_X + 24, cy = CONTENT_Y + 60;
    int cw = CONTENT_W - 48;
    
    draw_card(cx, cy, cw, 88);
    draw_status_dot(cx + 16, cy + 18, JenColor::Yellow);
    draw_string(cx + 34, cy + 16, "Audio is unavailable", JenColor::Yellow, JenColor::Surface0);
    draw_string(cx + 16, cy + 48, "No sound driver is installed; volume controls are disabled.",
                JenColor::Subtext, JenColor::Surface0);
}

// ---- USERS PAGE ----
static void draw_page_users() {
    draw_page_header("Users & Accounts", "Manage system users");
    int cx = CONTENT_X + 24, cy = CONTENT_Y + 60;
    int cw = CONTENT_W - 48;
    
    extern char username_buffer[32];
    
    // Current user card
    draw_card(cx, cy, cw, 90);
    // Avatar circle
    draw_filled_circle(cx + 40, cy + 45, 25, JenColor::Surface1);
    draw_filled_circle(cx + 40, cy + 37, 10, JenColor::Subtext);
    draw_rounded_rect(cx + 28, cy + 50, 24, 14, 6, JenColor::Subtext);
    
    draw_string(cx + 76, cy + 20, username_buffer, JenColor::Text, JenColor::Surface0);
    draw_string(cx + 76, cy + 40, "Administrator", settings_get_accent_color(), JenColor::Surface0);
    draw_string(cx + 76, cy + 60, "Logged in", JenColor::Green, JenColor::Surface0);
    
    // Account type
    cy += 102;
    draw_card(cx, cy, cw, 50);
    draw_string(cx + 16, cy + 16, "Account Type: Root (Full Access)", JenColor::Text, JenColor::Surface0);
}

// ---- NETWORK PAGE ----
static void draw_page_network() {
    draw_page_header("Network", "Connection settings");
    int cx = CONTENT_X + 24, cy = CONTENT_Y + 60;
    int cw = CONTENT_W - 48;
    
    draw_card(cx, cy, cw, 90);
    draw_status_dot(cx + 16, cy + 18, JenColor::Surface1);
    draw_string(cx + 34, cy + 12, "No Network Adapter", JenColor::Subtext, JenColor::Surface0);
    draw_string(cx + 34, cy + 36, "Network stack not available", JenColor::Surface2, JenColor::Surface0);
    draw_string(cx + 16, cy + 62, "Networking controls will appear when a driver is added.",
                JenColor::Subtext, JenColor::Surface0);
    
    cy += 62;
    draw_card(cx, cy, cw, 50);
    draw_string(cx + 16, cy + 16, "Bluetooth: Not Available", JenColor::Subtext, JenColor::Surface0);
}

// ---- STORAGE PAGE ----
static void draw_page_storage() {
    draw_page_header("Storage", "Disk and filesystem");
    int cx = CONTENT_X + 24, cy = CONTENT_Y + 60;
    int cw = CONTENT_W - 48;
    
    draw_card(cx, cy, cw, 90);
    draw_status_dot(cx + 16, cy + 18, JenColor::Yellow);
    draw_string(cx + 34, cy + 12, "No persistent storage driver", JenColor::Yellow, JenColor::Surface0);
    draw_string(cx + 16, cy + 36, "The kernel heap is volatile RAM, not a disk.",
                JenColor::Text, JenColor::Surface0);
    draw_string(cx + 16, cy + 60, "Files will not persist after a reboot.",
                JenColor::Subtext, JenColor::Surface0);
}

// ---- POWER PAGE ----
static void draw_page_power() {
    draw_page_header("Power", "Power management");
    int cx = CONTENT_X + 24, cy = CONTENT_Y + 60;
    int cw = CONTENT_W - 48;

    draw_card(cx, cy, cw, 50);
    draw_string(cx + 16, cy + 16, "Power Source: QEMU Virtual (AC)", JenColor::Green, JenColor::Surface0);

    cy += 62;
    draw_card(cx, cy, cw, 50);
    draw_string(cx + 16, cy + 16, "Sleep, Restart and Shutdown actions", JenColor::Subtext, JenColor::Surface0);

    cy += 72;
    draw_card(cx, cy, cw, 200);

    int button_w = (cw - 48) / 2;
    int button_h = 140;
    int button_x = cx + 16;
    int button_y = cy + 16;
    int icon_size = 64;

    // User/Profile
    draw_rounded_rect(button_x, button_y, button_w, button_h, 14, JenColor::Surface1);
    draw_argb_icon(button_x + (button_w - icon_size) / 2, button_y + 12, icon_user_pixels, icon_user_width, icon_user_height, false, icon_size);
    draw_string(button_x + 16, button_y + 88, "User Account", JenColor::Text, JenColor::Surface1);
    draw_string(button_x + 16, button_y + 106, "Open profile settings", JenColor::Subtext, JenColor::Surface1);

    // Sleep
    int second_x = button_x + button_w + 16;
    draw_rounded_rect(second_x, button_y, button_w, button_h, 14, JenColor::Surface1);
    draw_argb_icon(second_x + (button_w - icon_size) / 2, button_y + 12, icon_sleep_pixels, icon_sleep_width, icon_sleep_height, false, icon_size);
    draw_string(second_x + 16, button_y + 88, "Sleep Mode", JenColor::Text, JenColor::Surface1);
    draw_string(second_x + 16, button_y + 106, "Suspend session and wake on input", JenColor::Subtext, JenColor::Surface1);

    // Restart
    int third_x = button_x;
    int third_y = button_y + button_h + 16;
    draw_rounded_rect(third_x, third_y, button_w, button_h, 14, JenColor::Surface1);
    draw_argb_icon(third_x + (button_w - icon_size) / 2, third_y + 12, icon_restart_pixels, icon_restart_width, icon_restart_height, false, icon_size);
    draw_string(third_x + 16, third_y + 88, "Restart", JenColor::Text, JenColor::Surface1);
    draw_string(third_x + 16, third_y + 106, "Save session and reboot system", JenColor::Subtext, JenColor::Surface1);

    // Shutdown
    int fourth_x = second_x;
    draw_rounded_rect(fourth_x, third_y, button_w, button_h, 14, JenColor::Surface1);
    draw_argb_icon(fourth_x + (button_w - icon_size) / 2, third_y + 12, icon_shutdown_pixels, icon_shutdown_width, icon_shutdown_height, false, icon_size);
    draw_string(fourth_x + 16, third_y + 88, "Shutdown", JenColor::Text, JenColor::Surface1);
    draw_string(fourth_x + 16, third_y + 106, "Save data and power off", JenColor::Subtext, JenColor::Surface1);

    if (power_confirm_visible) {
        int overlay_w = cw - 40;
        int overlay_h = 120;
        int overlay_x = cx + 20;
        int overlay_y = cy + 48;
        draw_rounded_rect(overlay_x, overlay_y, overlay_w, overlay_h, 14, 0x1E1E2E);
        draw_rect(overlay_x, overlay_y + overlay_h - 1, overlay_w, 1, JenColor::Surface0);

        const char* action_text = power_confirm_action == POWER_ACTION_RESTART ? "Restart" : "Shutdown";
        const char* detail_text = power_confirm_action == POWER_ACTION_RESTART
            ? "Restart will save session and reboot JenOS."
            : "Shutdown will save data and power off the system.";

        draw_string(overlay_x + 20, overlay_y + 16, action_text, settings_get_accent_color(), 0x1E1E2E);
        draw_string(overlay_x + 20, overlay_y + 40, detail_text, JenColor::Subtext, 0x1E1E2E);

        draw_button(overlay_x + 36, overlay_y + 72, 120, 32, "Confirm", JenColor::Green);
        draw_button(overlay_x + overlay_w - 156, overlay_y + 72, 120, 32, "Cancel", JenColor::Surface1);
    }
}

// ---- NOTIFICATIONS PAGE ----
static void draw_page_notifications() {
    draw_page_header("Notifications", "Notification service status");
    int cx = CONTENT_X + 24, cy = CONTENT_Y + 60;
    int cw = CONTENT_W - 48;
    
    draw_card(cx, cy, cw, 72);
    draw_string(cx + 16, cy + 16, "No notification service is running.",
                JenColor::Text, JenColor::Surface0);
    draw_string(cx + 16, cy + 42, "Alerts and Do Not Disturb controls are unavailable.",
                JenColor::Subtext, JenColor::Surface0);
}

// ---- ACCESSIBILITY PAGE ----
static void draw_page_accessibility() {
    draw_page_header("Accessibility", "Make JenOS easier to use");
    int cx = CONTENT_X + 24, cy = CONTENT_Y + 60;
    int cw = CONTENT_W - 48;
    
    draw_card(cx, cy, cw, 50);
    draw_string(cx + 16, cy + 16, "High Contrast", JenColor::Text, JenColor::Surface0);
    draw_toggle(cx + cw - 64, cy + 14, sys_config.high_contrast);
    
    cy += 62;
    draw_card(cx, cy, cw, 50);
    draw_string(cx + 16, cy + 16, "Large Cursor", JenColor::Text, JenColor::Surface0);
    draw_toggle(cx + cw - 64, cy + 14, sys_config.large_cursor);
}

// ---- HARDWARE PAGE ----
static void draw_page_hardware() {
    draw_page_header("Hardware", "System hardware information");
    int cx = CONTENT_X + 24, cy = CONTENT_Y + 60;
    int cw = CONTENT_W - 48;
    
    if (!sysinfo_loaded) {
        sysinfo_init(&cached_sysinfo);
        sysinfo_loaded = true;
    }
    
    // CPU
    draw_card(cx, cy, cw, 70);
    draw_string(cx + 16, cy + 12, "Processor", settings_get_accent_color(), JenColor::Surface0);
    draw_string(cx + 16, cy + 32, cached_sysinfo.cpu.brand, JenColor::Text, JenColor::Surface0);
    char vendor_line[64];
    str_cat(vendor_line, "Vendor: ", cached_sysinfo.cpu.vendor);
    draw_string(cx + 16, cy + 52, vendor_line, JenColor::Subtext, JenColor::Surface0);
    
    // RAM
    cy += 82;
    draw_card(cx, cy, cw, 80);
    draw_string(cx + 16, cy + 12, "Memory", settings_get_accent_color(), JenColor::Surface0);
    
    char ram_buf[32]; int_to_str(cached_sysinfo.total_ram_mb, ram_buf);
    char ram_line[64]; str_cat(ram_line, "Total RAM: ", ram_buf); str_cat(ram_line, ram_line, " MB");
    draw_string(cx + 16, cy + 32, ram_line, JenColor::Text, JenColor::Surface0);
    
    int ram_pct = cached_sysinfo.total_ram_mb > 0 ? (cached_sysinfo.used_ram_mb * 100 / cached_sysinfo.total_ram_mb) : 0;
    draw_progress_bar(cx + 16, cy + 56, cw - 32, ram_pct);
    
    // PCI Devices
    cy += 92;
    draw_card(cx, cy, cw, 140);
    char pci_title[32];
    int_to_str(cached_sysinfo.pci_device_count, ram_buf);
    str_cat(pci_title, "PCI Devices (", ram_buf);
    str_cat(pci_title, pci_title, " found)");
    draw_string(cx + 16, cy + 12, pci_title, settings_get_accent_color(), JenColor::Surface0);
    
    int dy = cy + 32;
    for (int i = 0; i < cached_sysinfo.pci_device_count && i < 6; i++) {
        pci_device& d = cached_sysinfo.pci_devices[i];
        const char* cls_name = pci_class_name(d.class_code, d.subclass);
        char hex_str[8];
        hex16_to_str(d.vendor_id, hex_str);
        char dev_line[64];
        str_cat(dev_line, hex_str, "  ");
        str_cat(dev_line, dev_line, cls_name);
        draw_string(cx + 16, dy, dev_line, JenColor::Text, JenColor::Surface0);
        dy += 18;
    }
}

// ---- KERNEL PAGE ----
static void draw_page_kernel() {
    draw_page_header("Kernel", "JenOS kernel information");
    int cx = CONTENT_X + 24, cy = CONTENT_Y + 60;
    int cw = CONTENT_W - 48;
    
    draw_card(cx, cy, cw, 90);
    draw_string(cx + 16, cy + 12, "JenOS Kernel", settings_get_accent_color(), JenColor::Surface0);
    draw_string(cx + 16, cy + 32, "Version: 1.0.0-dev", JenColor::Text, JenColor::Surface0);
    draw_string(cx + 16, cy + 52, "Architecture: i686 (32-bit)", JenColor::Text, JenColor::Surface0);
    draw_string(cx + 16, cy + 72, "Compiler: Clang (freestanding)", JenColor::Subtext, JenColor::Surface0);
    
    // Memory Management
    cy += 102;
    draw_card(cx, cy, cw, 90);
    draw_string(cx + 16, cy + 12, "Memory Management", settings_get_accent_color(), JenColor::Surface0);
    draw_string(cx + 16, cy + 32, "PMM: Bitmap Physical Allocator", JenColor::Text, JenColor::Surface0);
    draw_string(cx + 16, cy + 52, "VMM: Identity Mapped (No Paging)", JenColor::Text, JenColor::Surface0);
    draw_string(cx + 16, cy + 72, "Heap: Linked-list block allocator (16 MB)", JenColor::Text, JenColor::Surface0);
    
    // Interrupts
    cy += 102;
    draw_card(cx, cy, cw, 70);
    draw_string(cx + 16, cy + 12, "Interrupts", settings_get_accent_color(), JenColor::Surface0);
    draw_string(cx + 16, cy + 32, "PIC: Remapped (IRQ 0x20-0x2F)", JenColor::Text, JenColor::Surface0);
    draw_string(cx + 16, cy + 52, "IDT: 256 entries loaded", JenColor::Text, JenColor::Surface0);
}

// ---- ABOUT PAGE ----
static void draw_page_about() {
    draw_page_header("About JenOS", "System information");
    int cx = CONTENT_X + 24, cy = CONTENT_Y + 60;
    int cw = CONTENT_W - 48;
    
    // Logo area
    draw_card(cx, cy, cw, 120);
    // "J" logo rendered as text
    draw_string(cx + 16, cy + 16, "JenOS", settings_get_accent_color(), JenColor::Surface0);
    draw_string(cx + 16, cy + 40, "Version 1.0.0", JenColor::Text, JenColor::Surface0);
    draw_string(cx + 16, cy + 60, "A bare-metal operating system", JenColor::Subtext, JenColor::Surface0);
    draw_string(cx + 16, cy + 80, "written in C++ from scratch.", JenColor::Subtext, JenColor::Surface0);
    draw_string(cx + 16, cy + 100, "Runs on real x86 hardware.", JenColor::Subtext, JenColor::Surface0);
    
    // System summary
    cy += 132;
    draw_card(cx, cy, cw, 160);
    draw_string(cx + 16, cy + 12, "System Summary", settings_get_accent_color(), JenColor::Surface0);
    
    if (!sysinfo_loaded) {
        sysinfo_init(&cached_sysinfo);
        sysinfo_loaded = true;
    }
    
    draw_string(cx + 16, cy + 36, cached_sysinfo.cpu.brand, JenColor::Text, JenColor::Surface0);
    
    char ram_buf[16]; int_to_str(cached_sysinfo.total_ram_mb, ram_buf);
    char ram_line[64]; str_cat(ram_line, "RAM: ", ram_buf); str_cat(ram_line, ram_line, " MB");
    draw_string(cx + 16, cy + 56, ram_line, JenColor::Text, JenColor::Surface0);
    
    char width_str[16];
    char height_str[16];
    int_to_str((int)screen_width, width_str);
    int_to_str((int)screen_height, height_str);
    char resolution_str[32];
    char tmp_str[32];
    str_cat(tmp_str, width_str, "x");
    str_cat(resolution_str, tmp_str, height_str);
    draw_string(cx + 16, cy + 76, resolution_str, JenColor::Text, JenColor::Surface0);
    draw_string(cx + 16, cy + 96, "Boot: Multiboot 1 (GRUB/QEMU)", JenColor::Text, JenColor::Surface0);
    
    extern char username_buffer[32];
    char user_line[64]; str_cat(user_line, "User: ", username_buffer);
    draw_string(cx + 16, cy + 116, user_line, JenColor::Text, JenColor::Surface0);
    draw_string(cx + 16, cy + 136, "License: Open Source", JenColor::Subtext, JenColor::Surface0);
}

// ====================================================================
// MAIN DRAW FUNCTION
// ====================================================================
void settings_draw() {
    // Draw the desktop (wallpaper + taskbar) as background first
    extern void draw_desktop_bg();
    draw_desktop_bg();

    // Draw the floating window (shadow + chrome + sidebar + content)
    draw_titlebar();  // draws window shadow + background + title bar
    draw_sidebar();

    // Draw content area background (just the right panel)
    draw_rect(CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H, JenColor::Base);
    // Clip bottom-right corner to match rounded window
    draw_rounded_rect(WINDOW_X, WINDOW_Y, WINDOW_W, WINDOW_H, 12, 0x00000000); // no-op placeholder
    
    // Draw current page
    switch (settings_current_page) {
        case PAGE_DISPLAY:        draw_page_display(); break;
        case PAGE_APPEARANCE:     draw_page_appearance(); break;
        case PAGE_TIME_DATE:      draw_page_time(); break;
        case PAGE_MOUSE:          draw_page_mouse(); break;
        case PAGE_KEYBOARD:       draw_page_keyboard(); break;
        case PAGE_SOUND:          draw_page_sound(); break;
        case PAGE_USERS:          draw_page_users(); break;
        case PAGE_NETWORK:        draw_page_network(); break;
        case PAGE_STORAGE:        draw_page_storage(); break;
        case PAGE_POWER:          draw_page_power(); break;
        case PAGE_NOTIFICATIONS:  draw_page_notifications(); break;
        case PAGE_ACCESSIBILITY:  draw_page_accessibility(); break;
        case PAGE_HARDWARE:       draw_page_hardware(); break;
        case PAGE_KERNEL:         draw_page_kernel(); break;
        case PAGE_ABOUT:          draw_page_about(); break;
        default: break;
    }
    
    swap_buffers();
    mouse_draw_cursor();
}

// ====================================================================
// CLICK HANDLER
// ====================================================================
void settings_handle_click(int x, int y) {
    // Ignore clicks outside the window
    if (x < WINDOW_X || x > WINDOW_X + WINDOW_W || y < WINDOW_Y || y > WINDOW_Y + WINDOW_H) {
        // Click outside window — close it
        extern void return_to_desktop();
        return_to_desktop();
        return;
    }

    // Close button (top right of window)
    int close_x = WINDOW_X + WINDOW_W - 40;
    if (x >= close_x && x <= close_x + 28 && y >= WINDOW_Y + 8 && y <= WINDOW_Y + 32) {
        extern void return_to_desktop();
        return_to_desktop();
        return;
    }
    
    // Back arrow (top left of window)
    if (x >= WINDOW_X + 10 && x <= WINDOW_X + 30 && y >= WINDOW_Y + 8 && y <= WINDOW_Y + 32) {
        extern void return_to_desktop();
        return_to_desktop();
        return;
    }
    
    // Sidebar click — select category
    if (x >= WINDOW_X && x < WINDOW_X + SIDEBAR_W && y > WINDOW_Y + TITLEBAR_H) {
        int rel_y = y - (WINDOW_Y + TITLEBAR_H) - 8 + sidebar_scroll_offset;
        int index = rel_y / 34;
        if (index >= 0 && index < PAGE_COUNT) {
            settings_current_page = (SettingsPage)index;
            settings_draw();
        }
        return;
    }
    
    // Content area controls
    int cx = CONTENT_X + 24;
    int cw = CONTENT_W - 48;
    int toggle_x1 = cx + cw - 64;
    int toggle_x2 = toggle_x1 + 44;
    
    if (settings_current_page == PAGE_DISPLAY) {
        int option_x = cx + cw - 210;
        int option_y = CONTENT_Y + 60 + 30;
        if (y >= option_y && y <= option_y + 30) {
            size_t width = 0;
            size_t height = 0;
            if (x >= option_x && x <= option_x + 92) {
                width = 1280;
                height = 720;
            } else if (x >= option_x + 100 && x <= option_x + 194) {
                width = 1920;
                height = 1080;
            }

            if (width && (width != screen_width || height != screen_height)) {
                reset_mouse_cursor_state();
                if (graphics_set_resolution(width, height)) {
                    resolution_apply_failed = false;
                    settings_draw();
                } else {
                    resolution_apply_failed = true;
                    settings_draw();
                }
            }
            return;
        }
    }

    if (settings_current_page == PAGE_MOUSE) {
        int slider_x = cx + 16;
        int slider_width = cw - 32;
        int slider_y = CONTENT_Y + 60 + 40;
        if (y >= slider_y && y <= slider_y + 18 &&
            x >= slider_x && x <= slider_x + slider_width) {
            int value = ((x - slider_x) * 10) / slider_width;
            if (value < 1) value = 1;
            if (value > 10) value = 10;
            sys_config.mouse_speed = value;
            mouse_set_speed(value);
            settings_draw();
            return;
        }
    }

    if (x >= toggle_x1 && x <= toggle_x2) {
        switch (settings_current_page) {
            case PAGE_MOUSE: {
                int toggle_y = CONTENT_Y + 60 + 82 + 14;
                if (y >= toggle_y && y <= toggle_y + 30) {
                    sys_config.mouse_natural_scroll = !sys_config.mouse_natural_scroll;
                    mouse_set_natural_scroll(sys_config.mouse_natural_scroll);
                    settings_draw();
                    return;
                }
                break;
            }
            case PAGE_TIME_DATE: {
                int toggle_y = CONTENT_Y + 60 + 112 + 62 + 14;
                if (y >= toggle_y && y <= toggle_y + 30) {
                    sys_config.time_24_hour = !sys_config.time_24_hour;
                    settings_draw();
                    return;
                }
                break;
            }
            case PAGE_ACCESSIBILITY: {
                int t1 = CONTENT_Y + 60 + 14;
                int t2 = CONTENT_Y + 60 + 62 + 14;
                if (y >= t1 && y <= t1 + 30) {
                    sys_config.high_contrast = !sys_config.high_contrast;
                    settings_draw();
                    return;
                }
                if (y >= t2 && y <= t2 + 30) {
                    sys_config.large_cursor = !sys_config.large_cursor;
                    mouse_set_large_cursor(sys_config.large_cursor);
                    settings_draw();
                    return;
                }
                break;
            }
            default:
                break;
        }
    }
    
    // Appearance — accent color selection
    if (settings_current_page == PAGE_APPEARANCE) {
        int acy = CONTENT_Y + 60 + 62 + 38;
        if (y >= acy && y <= acy + 36) {
            for (int i = 0; i < 6; i++) {
                int bx = cx + 16 + i * 72;
                if (x >= bx && x <= bx + 60) {
                    sys_config.accent_color_index = i;
                    settings_draw();
                    return;
                }
            }
        }
    }

    if (settings_current_page == PAGE_POWER) {
        int button_w = (cw - 48) / 2;
        int button_h = 140;
        int button_x = cx + 16;
        int button_y = CONTENT_Y + 60 + 72 + 16;
        int second_x = button_x + button_w + 16;
        int third_y = button_y + button_h + 16;
        int fourth_x = second_x;

        if (power_confirm_visible) {
            int overlay_w = cw - 40;
            int overlay_x = cx + 20;
            int overlay_y = CONTENT_Y + 60 + 72 + 48;
            int yes_x = overlay_x + 36;
            int no_x = overlay_x + overlay_w - 156;
            int btn_y = overlay_y + 72;

            if (x >= yes_x && x <= yes_x + 120 && y >= btn_y && y <= btn_y + 32) {
                if (power_confirm_action == POWER_ACTION_RESTART) {
                    request_restart();
                    return;
                } else if (power_confirm_action == POWER_ACTION_SHUTDOWN) {
                    request_shutdown();
                    return;
                }
            }

            if (x >= no_x && x <= no_x + 120 && y >= btn_y && y <= btn_y + 32) {
                cancel_power_confirm();
                settings_draw();
                return;
            }

            return;
        }

        if (x >= button_x && x <= button_x + button_w && y >= button_y && y <= button_y + button_h) {
            open_user_profile();
            return;
        }

        if (x >= second_x && x <= second_x + button_w && y >= button_y && y <= button_y + button_h) {
            enter_sleep_mode();
            return;
        }

        if (x >= button_x && x <= button_x + button_w && y >= third_y && y <= third_y + button_h) {
            power_confirm_visible = true;
            power_confirm_action = POWER_ACTION_RESTART;
            settings_draw();
            return;
        }

        if (x >= fourth_x && x <= fourth_x + button_w && y >= third_y && y <= third_y + button_h) {
            power_confirm_visible = true;
            power_confirm_action = POWER_ACTION_SHUTDOWN;
            settings_draw();
            return;
        }
    }
}

// ====================================================================
// KEY HANDLER
// ====================================================================
void settings_handle_key(char c) {
    if (c == 27) { // ESC
        extern void return_to_desktop();
        return_to_desktop();
    }
}
