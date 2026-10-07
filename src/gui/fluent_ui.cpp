#include "fluent_ui.h"
#include "font.h"
#include "settings.h"
#include <string.h>

static size_t ui_font_scale() {
    return screen_height >= 900 ? 2 : 1;
}

static void draw_rounded_rect_with_border(int x, int y, int width, int height,
                                          int radius, uint32_t fill_color,
                                          uint32_t border_color, int border_width = 1) {
    draw_rounded_rect(x, y, width, height, radius, border_color);
    if (width > border_width * 2 && height > border_width * 2) {
        int inner_radius = radius > border_width ? radius - border_width : 0;
        draw_rounded_rect(x + border_width, y + border_width,
                          width - border_width * 2, height - border_width * 2,
                          inner_radius, fill_color);
    }
}

// ============================================================================
// Fluent Effects Implementation
// ============================================================================

void JenUI::FluentEffects::DrawShadow(int x, int y, int width, int height, int radius, int depth) {
    // Multi-layer shadow for depth
    for (int i = depth; i > 0; --i) {
        uint32_t shadow_color = (255 - (i * (255 / depth))) << 24;  // Alpha varies
        uint8_t alpha = 255 - (i * (255 / depth));
        draw_rounded_rect(x + i, y + i, width, height, radius, 
                         blend(0x000000, 0x1E1E2E, alpha));
    }
}

void JenUI::FluentEffects::DrawAcrylicPanel(int x, int y, int width, int height, 
                                             int radius, uint32_t tint_color, uint8_t opacity) {
    // Acrylic effect: tinted semi-transparent panel
    uint32_t blended = blend(tint_color, Colors::Background, opacity);
    draw_rounded_rect_with_border(x, y, width, height, radius, blended,
                                  Colors::BorderLight, 2);
}

void JenUI::FluentEffects::DrawMicaBackground(int x, int y, int width, int height, int radius) {
    // Mica effect: subtle gradient with layered appearance
    for (int row = 0; row < height; ++row) {
        float progress = (float)row / height;
        uint32_t base = Colors::BackgroundAlt;
        uint32_t target = Colors::SurfaceBase;
        uint32_t line_color = LerpColor(base, target, progress);
        
        for (int col = 0; col < width; ++col) {
            draw_pixel(x + col, y + row, line_color);
        }
    }
    
    // Border
    draw_rounded_rect(x, y, width, height, radius, Colors::BorderLight);
}

void JenUI::FluentEffects::BlurRegion(int x, int y, int width, int height, int radius) {
    // Simple box blur (separable Gaussian approximation)
    if (radius <= 0 || width <= 0 || height <= 0) return;
    
    // Horizontal pass
    for (int row = y; row < y + height; ++row) {
        for (int col = x; col < x + width; ++col) {
            uint32_t sum_r = 0, sum_g = 0, sum_b = 0;
            int count = 0;
            
            for (int dy = -radius; dy <= radius; ++dy) {
                int ny = row + dy;
                if (ny >= (int)0 && ny < (int)screen_height) {
                    uint32_t pixel = get_pixel(col, ny);
                    sum_r += (pixel >> 16) & 0xFF;
                    sum_g += (pixel >> 8) & 0xFF;
                    sum_b += pixel & 0xFF;
                    count++;
                }
            }
            
            if (count > 0) {
                uint32_t avg_r = sum_r / count;
                uint32_t avg_g = sum_g / count;
                uint32_t avg_b = sum_b / count;
                draw_pixel(col, row, (avg_r << 16) | (avg_g << 8) | avg_b);
            }
        }
    }
}

// ============================================================================
// Button Implementation
// ============================================================================

uint32_t JenUI::Button::BlendColors(uint32_t color1, uint32_t color2, uint8_t blend_amount) {
    return blend(color1, color2, blend_amount);
}

uint32_t JenUI::Button::GetCurrentColor() const {
    if (!enabled) return Colors::BackgroundAlt;
    if (is_pressed) return color_pressed;
    if (is_hovered) return color_hover;
    return color_normal;
}

void JenUI::Button::Draw() {
    if (!visible) return;
    
    uint32_t current_color = GetCurrentColor();
    
    // Draw shadow
    size_t scale = ui_font_scale();
    FluentEffects::DrawShadow(x + 2 * (int)scale, y + 2 * (int)scale,
                              width, height, corner_radius * (int)scale, 2);
    
    // Draw button background
    draw_rounded_rect_with_border(x, y, width, height,
                                  corner_radius * (int)scale, current_color,
                                  Colors::BorderLight, (int)scale);
    
    // Draw label centered
    if (label) {
        int label_width = (int)(strlen(label) * 8 * scale);
        int label_x = x + (width - label_width) / 2;
        int label_y = y + (height - 16 * (int)scale) / 2;
        uint32_t text_color = (current_color == color_pressed) ? Colors::TextPrimary : Colors::TextPrimary;
        draw_string_scaled(label_x, label_y, label, text_color, current_color, true, scale);
    }
}

bool JenUI::Button::HandleMouseClick(int mx, int my) {
    if (!enabled || !visible || !IsPointInside(mx, my)) return false;
    
    if (on_click) {
        on_click();
    }
    return true;
}

bool JenUI::Button::HandleMouseMove(int mx, int my) {
    is_hovered = IsPointInside(mx, my);
    return is_hovered;
}

// ============================================================================
// TextInput Implementation
// ============================================================================

void JenUI::TextInput::Draw() {
    if (!visible) return;
    
    uint32_t border_color = is_focused ? settings_get_accent_color() : Colors::BorderLight;
    
    // Draw shadow
    size_t scale = ui_font_scale();
    FluentEffects::DrawShadow(x + (int)scale, y + (int)scale, width, height,
                              corner_radius * (int)scale, 1);
    
    // Draw input background
    draw_rounded_rect_with_border(x, y, width, height,
                                  corner_radius * (int)scale,
                                  Colors::SurfaceSecondary, border_color,
                                  (int)scale);
    
    // Draw text or placeholder
    int text_x = x + 12 * (int)scale;
    int text_y = y + (height - 16 * (int)scale) / 2;
    
    if (text_length == 0 && !is_focused) {
        draw_string_scaled(text_x, text_y, placeholder, placeholder_color, 0, true, scale);
    } else {
        draw_string_scaled(text_x, text_y, buffer, text_color, 0, true, scale);
        
        // Draw cursor if focused
        if (is_focused) {
            cursor_blink_time = (cursor_blink_time + 1) % 60;  // Blink every 1 second (60 frames)
            if (cursor_blink_time < 30) {  // Visible for first half
                int cursor_x = text_x + (int)text_length * 8 * (int)scale;
                draw_rect(cursor_x, text_y + 2 * (int)scale, 2 * scale,
                          14 * scale, settings_get_accent_color());
            }
        }
    }
}

bool JenUI::TextInput::HandleMouseClick(int mx, int my) {
    is_focused = IsPointInside(mx, my);
    return is_focused;
}

bool JenUI::TextInput::HandleKeyPress(char key) {
    if (!is_focused) return false;
    
    if (key == '\b') {  // Backspace
        if (text_length > 0) {
            buffer[--text_length] = '\0';
        }
    } else if (key >= 32 && key < 127 && text_length < buffer_size - 1) {
        buffer[text_length++] = key;
        buffer[text_length] = '\0';
    }
    
    return true;
}

void JenUI::TextInput::SetText(const char* text) {
    if (!text) {
        Clear();
        return;
    }
    
    size_t len = strlen(text);
    if (len > buffer_size - 1) len = buffer_size - 1;
    
    memcpy(buffer, text, len);
    buffer[len] = '\0';
    text_length = len;
}

void JenUI::TextInput::Clear() {
    memset(buffer, 0, buffer_size);
    text_length = 0;
    cursor_position = 0;
}

// ============================================================================
// PasswordInput Implementation
// ============================================================================

void JenUI::PasswordInput::Draw() {
    if (!visible) return;
    
    uint32_t border_color = IsFocused() ? settings_get_accent_color() : Colors::BorderLight;
    
    // Draw shadow
    size_t scale = ui_font_scale();
    FluentEffects::DrawShadow(x + (int)scale, y + (int)scale, width, height,
                              8 * (int)scale, 1);
    
    // Draw input background
    draw_rounded_rect_with_border(x, y, width, height, 8 * (int)scale,
                                  Colors::SurfaceSecondary, border_color,
                                  (int)scale);
    
    // Draw text or placeholder
    int text_x = x + 12 * (int)scale;
    int text_y = y + (height - 16 * (int)scale) / 2;
    
    if (text_length == 0 && !IsFocused()) {
        draw_string_scaled(text_x, text_y, placeholder, Colors::TextTertiary, 0, true, scale);
    } else {
        // Draw asterisks instead of actual password
        char masked[32] = {0};
        for (size_t i = 0; i < text_length && i < 31; ++i) {
            masked[i] = '*';
        }
        masked[text_length] = '\0';
        draw_string_scaled(text_x, text_y, masked, Colors::TextPrimary, 0, true, scale);
        
        // Draw cursor if focused
        if (IsFocused()) {
            int cursor_x = text_x + (int)text_length * 8 * (int)scale;
            draw_rect(cursor_x, text_y + 2 * (int)scale, 2 * scale,
                      14 * scale, settings_get_accent_color());
        }
    }
    
    // Draw password visibility toggle icon (simplified)
    // For now, just draw a small circle in the top-right
    int toggle_x = x + width - 30 * (int)scale;
    int toggle_y = y + (height - 16 * (int)scale) / 2;
    draw_filled_circle(toggle_x, toggle_y + 8 * (int)scale, 6 * (int)scale,
                       show_password ? settings_get_accent_color() : Colors::BorderLight);
}

// ============================================================================
// AvatarCircle Implementation
// ============================================================================

void JenUI::AvatarCircle::Draw() {
    if (!visible) return;
    
    int radius = width / 2;
    int center_x = x + radius;
    int center_y = y + radius;
    
    draw_filled_circle(center_x, center_y, radius, settings_get_accent_color());
    draw_filled_circle(center_x, center_y, radius - border_width, Colors::SurfaceSecondary);

    if (image_data && image_width > 0 && image_height > 0) {
        draw_argb_bitmap_scaled(x + border_width, y + border_width, image_data,
                                image_width, image_height,
                                width - border_width * 2, height - border_width * 2);
        return;
    }

    int head_radius = radius / 4;
    draw_filled_circle(center_x, center_y - radius / 4, head_radius, Colors::TextPrimary);
    int body_width = radius * 6 / 5;
    int body_height = radius * 3 / 5;
    int body_x = center_x - body_width / 2;
    int body_y = center_y + radius / 5;
    draw_rounded_rect(body_x, body_y, body_width, body_height,
                      body_height / 2, Colors::TextPrimary);
}

// ============================================================================
// Panel Implementation
// ============================================================================

void JenUI::Panel::Draw() {
    if (!visible) return;
    
    if (use_acrylic) {
        FluentEffects::DrawAcrylicPanel(x, y, width, height, corner_radius, 
                                       background_color, acrylic_opacity);
    } else {
        // Draw shadow
        FluentEffects::DrawShadow(x + 2, y + 2, width, height, corner_radius, 2);
        
        // Draw panel background
        draw_rounded_rect_with_border(x, y, width, height, corner_radius,
                                      background_color, border_color);
    }
}

// ============================================================================
// ClockDisplay Implementation
// ============================================================================

void JenUI::ClockDisplay::Draw() {
    if (!visible) return;

    char time_str[16];
    settings_format_time(hours, minutes, time_str);
    draw_string_scaled(x, y, time_str, text_color, 0, true, ui_font_scale());
}

// ============================================================================
// IconButton Implementation
// ============================================================================

void JenUI::IconButton::Draw() {
    if (!visible) return;
    
    uint32_t current_bg = is_hovered ? Colors::SurfaceSecondary : Colors::BackgroundAlt;
    
    // Draw button background
    draw_filled_circle(x + width / 2, y + height / 2, width / 2, current_bg);
    
    // Draw border
    draw_filled_circle(x + width / 2, y + height / 2, width / 2, Colors::BorderLight);
    
    // Draw icon if available
    if (icon_data && icon_width > 0 && icon_height > 0) {
        int icon_x = x + (width - icon_width) / 2;
        int icon_y = y + (height - icon_height) / 2;
        draw_argb_bitmap_scaled(icon_x, icon_y, icon_data, icon_width, icon_height, 
                               width - 4, height - 4);
    }
}

bool JenUI::IconButton::HandleMouseClick(int mx, int my) {
    if (!enabled || !visible || !IsPointInside(mx, my)) return false;
    
    if (on_click) {
        on_click();
    }
    return true;
}

bool JenUI::IconButton::HandleMouseMove(int mx, int my) {
    is_hovered = IsPointInside(mx, my);
    return is_hovered;
}

// ============================================================================
// Label Implementation
// ============================================================================

void JenUI::Label::Draw() {
    if (!visible || !text) return;
    
    size_t scale = ui_font_scale();
    if (font_size == 0 && scale > 1) scale = 1;
    else if (font_size >= 2) ++scale;

    draw_string_scaled(x, y, text, text_color, 0, true, scale);
}

// ============================================================================
// Helper Functions
// ============================================================================

void DrawFluentButton(int x, int y, int w, int h, const char* label, uint32_t color, bool hovered, bool pressed) {
    uint32_t current_color = color;
    if (pressed) {
        current_color = blend(color, 0x000000, 30);
    } else if (hovered) {
        current_color = blend(color, 0xFFFFFF, 20);
    }
    
    JenUI::FluentEffects::DrawShadow(x + 2, y + 2, w, h, 8, 2);
    draw_rounded_rect_with_border(x, y, w, h, 8, current_color,
                                  JenUI::Colors::BorderLight);
    
    if (label) {
        size_t scale = ui_font_scale();
        int label_width = (int)(strlen(label) * 8 * scale);
        int label_x = x + (w - label_width) / 2;
        int label_y = y + (h - 16 * (int)scale) / 2;
        draw_string_scaled(label_x, label_y, label, JenUI::Colors::TextPrimary,
                           current_color, true, scale);
    }
}

void DrawFluentTextBox(int x, int y, int w, int h, const char* text, bool focused, const char* placeholder) {
    uint32_t border = focused ? settings_get_accent_color() : JenUI::Colors::BorderLight;
    
    JenUI::FluentEffects::DrawShadow(x + 1, y + 1, w, h, 8, 1);
    draw_rounded_rect_with_border(x, y, w, h, 8, JenUI::Colors::SurfaceSecondary,
                                  border);
    
    const char* display_text = (text && strlen(text) > 0) ? text : placeholder;
    uint32_t text_color = (text && strlen(text) > 0) ? JenUI::Colors::TextPrimary : JenUI::Colors::TextTertiary;
    size_t scale = ui_font_scale();
    draw_string_scaled(x + 12, y + (h - 16 * (int)scale) / 2, display_text,
                       text_color, 0, true, scale);
}

void DrawFluentPanel(int x, int y, int w, int h, uint32_t bg_color) {
    JenUI::FluentEffects::DrawShadow(x + 2, y + 2, w, h, 12, 2);
    draw_rounded_rect_with_border(x, y, w, h, 12, bg_color,
                                  JenUI::Colors::BorderLight);
}

void DrawFluentShadow(int x, int y, int w, int h, int radius, int depth) {
    JenUI::FluentEffects::DrawShadow(x, y, w, h, radius, depth);
}

uint32_t LerpColor(uint32_t from, uint32_t to, float progress) {
    if (progress <= 0.0f) return from;
    if (progress >= 1.0f) return to;
    
    uint8_t from_r = (from >> 16) & 0xFF;
    uint8_t from_g = (from >> 8) & 0xFF;
    uint8_t from_b = from & 0xFF;
    
    uint8_t to_r = (to >> 16) & 0xFF;
    uint8_t to_g = (to >> 8) & 0xFF;
    uint8_t to_b = to & 0xFF;
    
    uint8_t r = from_r + (uint8_t)((to_r - from_r) * progress);
    uint8_t g = from_g + (uint8_t)((to_g - from_g) * progress);
    uint8_t b = from_b + (uint8_t)((to_b - from_b) * progress);
    
    return (r << 16) | (g << 8) | b;
}
