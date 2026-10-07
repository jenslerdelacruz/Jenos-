#pragma once
#include <stdint.h>
#include <stddef.h>
#include "graphics.h"
#include "font.h"
#include "settings.h"

// ====================================================================
// JenOS UI Widget Library
// Unique design language: Catppuccin Mocha + JenOS accent
// ====================================================================

// Color Palette — JenOS Design System
namespace JenColor {
    constexpr uint32_t Base      = 0x1E1E2E;  // Main background
    constexpr uint32_t Mantle    = 0x181825;  // Deeper background
    constexpr uint32_t Crust     = 0x11111B;  // Darkest
    constexpr uint32_t Surface0  = 0x313244;  // Card backgrounds
    constexpr uint32_t Surface1  = 0x45475A;  // Borders, subtle
    constexpr uint32_t Surface2  = 0x585B70;  // Disabled text
    constexpr uint32_t Overlay0  = 0x6C7086;
    constexpr uint32_t Text      = 0xCDD6F4;  // Main text
    constexpr uint32_t Subtext   = 0xA6ADC8;  // Secondary text
    constexpr uint32_t Blue      = 0x89B4FA;  // Primary accent
    constexpr uint32_t Sapphire  = 0x74C7EC;  // Secondary accent
    constexpr uint32_t Green     = 0xA6E3A1;  // Success / ON
    constexpr uint32_t Red       = 0xF38BA8;  // Danger / Close
    constexpr uint32_t Peach     = 0xFAB387;  // Warning
    constexpr uint32_t Yellow    = 0xF9E2AF;  // Highlight
    constexpr uint32_t Lavender  = 0xB4BEFE;  // Selected sidebar
    constexpr uint32_t Mauve     = 0xCBA6F7;  // Special
    constexpr uint32_t Teal      = 0x94E2D5;  // Info
}

// ---- Toggle Switch ----
// Returns the drawn bounding box for click detection
struct WidgetRect { int x, y, w, h; };

static inline void draw_toggle(int x, int y, bool on) {
    uint32_t bg = on ? settings_get_accent_color() : JenColor::Surface1;
    // Pill background (44x22)
    draw_rounded_rect(x, y, 44, 22, 11, bg);
    // Knob
    int knob_x = on ? (x + 24) : (x + 2);
    draw_filled_circle(knob_x + 9, y + 11, 8, 0xFFFFFF);
}

// ---- Slider ----
static inline void draw_slider(int x, int y, int width, int value, int max_val) {
    // Track background
    draw_rounded_rect(x, y + 6, width, 8, 4, JenColor::Surface1);
    // Filled portion
    int fill_w = (value * width) / max_val;
    if (fill_w > 0) {
        draw_rounded_rect(x, y + 6, fill_w, 8, 4, settings_get_accent_color());
    }
    // Knob
    int knob_x = x + fill_w - 8;
    if (knob_x < x) knob_x = x;
    draw_filled_circle(knob_x + 8, y + 10, 8, JenColor::Lavender);
    draw_filled_circle(knob_x + 8, y + 10, 5, 0xFFFFFF);
}

// ---- Radio Button ----
static inline void draw_radio(int x, int y, bool selected, const char* label) {
    // Outer circle
    draw_filled_circle(x + 8, y + 8, 8, selected ? JenColor::Blue : JenColor::Surface1);
    // Inner dot
    if (selected) {
        draw_filled_circle(x + 8, y + 8, 4, 0xFFFFFF);
    } else {
        draw_filled_circle(x + 8, y + 8, 6, JenColor::Mantle);
    }
    // Label
    draw_string(x + 22, y + 1, label, JenColor::Text, JenColor::Base);
}

// ---- Section Header ----
static inline void draw_section_header(int x, int y, int width, const char* title) {
    draw_string(x, y, title, JenColor::Blue, JenColor::Base);
    draw_rect(x, y + 18, width, 1, JenColor::Surface0);
}

// ---- Info Row (Label: Value) ----
static inline void draw_info_row(int x, int y, int width, const char* label, const char* value) {
    draw_string(x, y, label, JenColor::Subtext, JenColor::Base);
    // Right-align value (approximate: 8px per char)
    int val_len = 0;
    for (int i = 0; value[i]; i++) val_len++;
    int vx = x + width - val_len * 8;
    draw_string(vx, y, value, JenColor::Text, JenColor::Base);
}

// ---- Card / Panel ----
static inline void draw_card(int x, int y, int w, int h) {
    uint32_t color = sys_config.high_contrast ? 0x000000 : JenColor::Surface0;
    draw_rounded_rect(x, y, w, h, 10, color);
}

// ---- Button ----
static inline void draw_button(int x, int y, int w, int h, const char* label, uint32_t color) {
    draw_rounded_rect(x, y, w, h, 8, color);
    // Center text
    int len = 0;
    for (int i = 0; label[i]; i++) len++;
    int tx = x + (w - len * 8) / 2;
    int ty = y + (h - 16) / 2;
    draw_string(tx, ty, label, JenColor::Crust, color);
}

// ---- Progress Bar ----
static inline void draw_progress_bar(int x, int y, int width, int percent) {
    draw_rounded_rect(x, y, width, 12, 6, JenColor::Surface1);
    int fill = (percent * width) / 100;
    if (fill > 0) {
        uint32_t col = (percent > 80) ? JenColor::Red : (percent > 50) ? JenColor::Yellow : JenColor::Green;
        draw_rounded_rect(x, y, fill, 12, 6, col);
    }
}

// ---- Status Indicator ----
static inline void draw_status_dot(int x, int y, uint32_t color) {
    draw_filled_circle(x + 5, y + 5, 5, color);
}

