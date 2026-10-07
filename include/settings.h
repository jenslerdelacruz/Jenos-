#pragma once
#include <stdint.h>

// ====================================================================
// Settings Application — State & Configuration
// ====================================================================

enum SettingsPage {
    PAGE_DISPLAY = 0,
    PAGE_APPEARANCE,
    PAGE_TIME_DATE,
    PAGE_MOUSE,
    PAGE_KEYBOARD,
    PAGE_SOUND,
    PAGE_USERS,
    PAGE_NETWORK,
    PAGE_STORAGE,
    PAGE_POWER,
    PAGE_NOTIFICATIONS,
    PAGE_ACCESSIBILITY,
    PAGE_HARDWARE,
    PAGE_KERNEL,
    PAGE_ABOUT,
    PAGE_COUNT
};

// Runtime configuration (persists while OS runs)
struct os_config {
    // Appearance
    int accent_color_index;    // 0=Blue, 1=Lavender, 2=Green, 3=Peach, 4=Mauve, 5=Teal
    bool high_contrast;
    
    // Mouse
    int mouse_speed;           // 1..10, default 5
    bool mouse_natural_scroll; // false = standard
    bool large_cursor;
    
    // Time
    bool time_24_hour;
};

// Global settings state
extern SettingsPage settings_current_page;
extern os_config sys_config;
uint32_t settings_get_accent_color();
void settings_format_time(int hours, int minutes, char* output);

// Settings app interface
void settings_draw();
void settings_handle_click(int x, int y);
void settings_handle_key(char c);
void settings_init_config();
