#pragma once

#include <stdint.h>
#include <stddef.h>
#include "graphics.h"
#include "font.h"
#include "smooth_icons.h"

extern const uint32_t* icon_folder_pixels;
extern size_t icon_folder_width;
extern size_t icon_folder_height;

extern const uint32_t* icon_settings_pixels;
extern size_t icon_settings_width;
extern size_t icon_settings_height;

extern const uint32_t* icon_terminal_pixels;
extern size_t icon_terminal_width;
extern size_t icon_terminal_height;

extern const uint32_t* icon_trash_pixels;
extern size_t icon_trash_width;
extern size_t icon_trash_height;

extern const uint32_t* icon_start_pixels;
extern size_t icon_start_width;
extern size_t icon_start_height;

extern const uint32_t* icon_user_pixels;
extern size_t icon_user_width;
extern size_t icon_user_height;

extern const uint32_t* icon_sleep_pixels;
extern size_t icon_sleep_width;
extern size_t icon_sleep_height;

extern const uint32_t* icon_restart_pixels;
extern size_t icon_restart_width;
extern size_t icon_restart_height;

extern const uint32_t* icon_shutdown_pixels;
extern size_t icon_shutdown_width;
extern size_t icon_shutdown_height;

extern const uint32_t* icon_cursor_pixels;
extern size_t icon_cursor_width;
extern size_t icon_cursor_height;

// ====================================================================
// Vector-style Anti-Aliased Icons drawn from ARGB arrays
// Looks crisp and modern!
// ====================================================================

static void draw_argb_icon(int x, int y, const uint32_t* icon_data, size_t src_width, size_t src_height, bool selected, size_t icon_size = 32) {
    if (selected) {
        draw_rounded_rect(x - 4, y - 4, icon_size + 8, icon_size + 8, 6, 0x313244);
    }

    if (icon_size == src_width && icon_size == src_height) {
        draw_argb_bitmap(x, y, icon_data, src_width, src_height);
    } else {
        draw_argb_bitmap_scaled(x, y, icon_data, src_width, src_height, icon_size, icon_size);
    }
}

// --- Folder Icon (32x32) ---
static void draw_vector_folder(int x, int y, bool selected = false, size_t icon_size = 32) {
    draw_argb_icon(x, y, icon_folder_pixels, icon_folder_width, icon_folder_height, selected, icon_size);
}

// --- Settings/Gear Icon (32x32) ---
static void draw_vector_settings(int x, int y, bool selected = false, size_t icon_size = 32) {
    draw_argb_icon(x, y, icon_settings_pixels, icon_settings_width, icon_settings_height, selected, icon_size);
}

// --- Terminal Icon (32x32) ---
static void draw_vector_terminal(int x, int y, bool selected = false, size_t icon_size = 32) {
    draw_argb_icon(x, y, icon_terminal_pixels, icon_terminal_width, icon_terminal_height, selected, icon_size);
}

// --- User/Profile Icon (login/avatar) ---
static void draw_vector_user(int x, int y, bool selected = false, size_t icon_size = 32) {
    if (selected) {
        draw_rounded_rect(x - 6, y - 6, icon_size + 12, icon_size + 12, 10, 0x313244);
    }

    size_t head_radius = icon_size / 4;
    size_t head_center_x = x + icon_size / 2;
    size_t head_center_y = y + icon_size / 3;

    draw_filled_circle((int)head_center_x, (int)head_center_y, (int)head_radius + 2, 0x313244);
    draw_filled_circle((int)head_center_x, (int)head_center_y, (int)head_radius, 0xA6ADC8);
    draw_filled_circle((int)head_center_x, (int)head_center_y, (int)head_radius - 4, 0xECEFF4);

    int torso_x = x + (int)(icon_size / 8);
    int torso_y = y + (int)(icon_size * 2 / 3);
    int torso_w = (int)(icon_size - icon_size / 4);
    int torso_h = (int)(icon_size / 4);

    draw_rounded_rect(torso_x, torso_y, torso_w, torso_h, icon_size / 10, 0x313244);
    draw_rounded_rect(torso_x + 3, torso_y + 3, torso_w - 6, torso_h - 6, icon_size / 12, 0xA6ADC8);
}

// --- Trash/Bin Icon (32x32) ---
static void draw_vector_trash(int x, int y, bool selected = false, size_t icon_size = 32) {
    draw_argb_icon(x, y, icon_trash_pixels, icon_trash_width, icon_trash_height, selected, icon_size);
}

// --- Battery Icon (for system tray, small 20x12) ---
static void draw_vector_battery(int x, int y) {
    // Keep primitive drawing for small battery
    draw_rounded_rect(x, y, 20, 12, 3, 0xA6ADC8);
    draw_rect(x + 20, y + 3, 3, 6, 0xA6ADC8);
    draw_rounded_rect(x + 2, y + 2, 14, 8, 2, 0xA6E3A1);
}

// --- WiFi Icon (for system tray, small 14x14) ---
static void draw_vector_wifi(int x, int y) {
    draw_filled_circle(x + 7, y + 12, 12, 0x89B4FA);
    draw_filled_circle(x + 7, y + 12, 9, 0x11111B); // cut
    draw_filled_circle(x + 7, y + 12, 8, 0x89B4FA);
    draw_filled_circle(x + 7, y + 12, 5, 0x11111B); // cut
    draw_filled_circle(x + 7, y + 12, 4, 0x89B4FA);
    draw_filled_circle(x + 7, y + 12, 2, 0x11111B); // cut
    draw_filled_circle(x + 7, y + 12, 2, 0x89B4FA);
    draw_rect(x - 6, y + 13, 28, 14, 0x11111B);
    draw_rect(x - 6, y, 5, 14, 0x11111B);
    draw_rect(x + 15, y, 8, 14, 0x11111B);
}

// --- Start/Home Icon (for dock) ---
static void draw_vector_start(int x, int y, size_t icon_size = 32) {
    draw_argb_icon(x, y, icon_start_pixels, icon_start_width, icon_start_height, false, icon_size);
}
