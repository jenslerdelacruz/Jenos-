#pragma once

#include <stdint.h>
#include <stddef.h>
#include "graphics.h"

// ============================================================================
// JenOS Fluent Design System
// Professional UI Framework inspired by Microsoft Fluent Design
// ============================================================================

namespace JenUI {

// ---- Fluent Color Palette ----
// Professional dark theme with blue accents (Fluent Design inspired)
namespace Colors {
    // Backgrounds
    constexpr uint32_t Background       = 0x202124;  // Primary background
    constexpr uint32_t BackgroundAlt    = 0x292A2D;  // Alternative background
    constexpr uint32_t SurfaceBase      = 0x2F3033;  // Surface base
    constexpr uint32_t SurfaceSecondary = 0x3B3D41;  // Secondary surface
    
    // Accents
    constexpr uint32_t AccentPrimary    = 0x0078D4;  // Windows blue
    constexpr uint32_t AccentLight      = 0x50B4F2;  // Light blue
    constexpr uint32_t AccentDark       = 0x004A90;  // Dark blue
    
    // Text
    constexpr uint32_t TextPrimary      = 0xFFFFFF;  // White text
    constexpr uint32_t TextSecondary    = 0xB3B3B3;  // Secondary text
    constexpr uint32_t TextTertiary     = 0x808080;  // Tertiary text
    constexpr uint32_t TextDisabled     = 0x606060;  // Disabled text
    
    // State colors
    constexpr uint32_t Success          = 0x107C10;  // Green
    constexpr uint32_t Warning          = 0xFF8C00;  // Orange
    constexpr uint32_t Error            = 0xE81123;  // Red
    constexpr uint32_t Info             = 0x0078D4;  // Blue
    
    // Borders & Dividers
    constexpr uint32_t BorderLight      = 0x464748;  // Light border
    constexpr uint32_t BorderDark       = 0x2D2E30;  // Dark border
    constexpr uint32_t Divider          = 0x3B3D41;  // Divider line
    
    // Shadows
    constexpr uint32_t ShadowDark       = 0x000000;  // Pure black for shadows
}

// ---- Animation Constants ----
constexpr int ANIMATION_DURATION_FAST    = 150;   // 150ms
constexpr int ANIMATION_DURATION_NORMAL  = 300;   // 300ms
constexpr int ANIMATION_DURATION_SLOW    = 500;   // 500ms
constexpr int ANIMATION_FPS              = 60;    // 60 FPS
constexpr int ANIMATION_FRAME_TIME       = 16;    // ~16ms per frame

// ---- Easing Functions ----
class Easing {
public:
    // Linear easing
    static inline float Linear(float t) {
        return t;
    }
    
    // Ease out cubic (smooth deceleration)
    static inline float EaseOutCubic(float t) {
        float f = t - 1.0f;
        return f * f * f + 1.0f;
    }
    
    // Ease in out cubic (smooth both directions)
    static inline float EaseInOutCubic(float t) {
        if (t < 0.5f) {
            return 4.0f * t * t * t;
        } else {
            float f = 2.0f * t - 2.0f;
            return 0.5f * f * f * f + 1.0f;
        }
    }
    
    // Ease out quad (light deceleration)
    static inline float EaseOutQuad(float t) {
        return -t * (t - 2.0f);
    }
};

// ---- Fluent Effects ----
class FluentEffects {
public:
    // Draw a subtle shadow with multiple layers
    static void DrawShadow(int x, int y, int width, int height, int radius, int depth = 2);
    
    // Draw a rounded rectangle with acrylic/frosted glass effect
    static void DrawAcrylicPanel(int x, int y, int width, int height, int radius, uint32_t tint_color, uint8_t opacity);
    
    // Draw a mica-inspired blurred background panel
    static void DrawMicaBackground(int x, int y, int width, int height, int radius);
    
    // Simple blur effect (separable Gaussian)
    static void BlurRegion(int x, int y, int width, int height, int radius);
};

// ---- Base UI Component ----
struct UIComponent {
    int x, y, width, height;
    bool visible;
    bool enabled;
    
    UIComponent(int x = 0, int y = 0, int w = 0, int h = 0)
        : x(x), y(y), width(w), height(h), visible(true), enabled(true) {}
    
    virtual ~UIComponent() {}
    virtual void Draw() = 0;
    virtual bool HandleMouseClick(int, int) { return false; }
    virtual bool HandleMouseMove(int, int) { return false; }
    virtual bool HandleKeyPress(char) { return false; }
    
    bool IsPointInside(int px, int py) const {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
};

// ---- Button Component ----
class Button : public UIComponent {
    const char* label;
    uint32_t color_normal;
    uint32_t color_hover;
    uint32_t color_pressed;
    bool is_hovered;
    bool is_pressed;
    int corner_radius;
    
public:
    void (*on_click)() = nullptr;
    
    Button(int x, int y, int w, int h, const char* label, uint32_t color = Colors::AccentPrimary)
        : UIComponent(x, y, w, h), label(label), color_normal(color),
          is_hovered(false), is_pressed(false), corner_radius(8) {
        color_hover = BlendColors(color, 0xFFFFFF, 20);
        color_pressed = BlendColors(color, 0x000000, 20);
    }
    
    void Draw() override;
    bool HandleMouseClick(int mx, int my) override;
    bool HandleMouseMove(int mx, int my) override;
    
private:
    uint32_t GetCurrentColor() const;
    static uint32_t BlendColors(uint32_t color1, uint32_t color2, uint8_t blend_amount);
};

// ---- Text Input Component ----
class TextInput : public UIComponent {
protected:  // Changed from private to protected so PasswordInput can access
    char* buffer;
    size_t buffer_size;
    size_t text_length;
    bool is_focused;
    int corner_radius;
    const char* placeholder;
    uint32_t text_color;
    uint32_t placeholder_color;
    int cursor_position;
    int cursor_blink_time;
    
public:
    TextInput(int x, int y, int w, int h, size_t max_length = 32, const char* placeholder = "")
        : UIComponent(x, y, w, h), buffer_size(max_length), text_length(0),
          is_focused(false), corner_radius(8), placeholder(placeholder),
          text_color(Colors::TextPrimary), placeholder_color(Colors::TextTertiary),
          cursor_position(0), cursor_blink_time(0) {
        buffer = new char[max_length + 1]{0};
    }
    
    ~TextInput() override {
        delete[] buffer;
    }
    
    void Draw() override;
    bool HandleMouseClick(int mx, int my) override;
    bool HandleKeyPress(char key) override;
    
    const char* GetText() const { return buffer; }
    void SetText(const char* text);
    void Clear();
    bool IsFocused() const { return is_focused; }
    size_t GetLength() const { return text_length; }
};

// ---- Password Input Component ----
class PasswordInput : public TextInput {
    bool show_password;
    
public:
    PasswordInput(int x, int y, int w, int h, size_t max_length = 32)
        : TextInput(x, y, w, h, max_length, "Enter password"),
          show_password(false) {}
    
    void Draw() override;
    void TogglePasswordVisibility() { show_password = !show_password; }
};

// ---- Avatar Circle Component ----
class AvatarCircle : public UIComponent {
    const uint32_t* image_data;
    size_t image_width;
    size_t image_height;
    uint32_t border_color;
    int border_width;
    
public:
    AvatarCircle(int x, int y, int diameter, const uint32_t* image = nullptr)
        : UIComponent(x, y, diameter, diameter), image_data(image),
          image_width(0), image_height(0), border_color(Colors::AccentPrimary), border_width(3) {}
    
    void SetImage(const uint32_t* data, size_t w, size_t h) {
        image_data = data;
        image_width = w;
        image_height = h;
    }
    
    void Draw() override;
};

// ---- Panel / Card Component ----
class Panel : public UIComponent {
    uint32_t background_color;
    uint32_t border_color;
    int corner_radius;
    int border_width;
    bool use_acrylic;
    uint8_t acrylic_opacity;
    
public:
    Panel(int x, int y, int w, int h, uint32_t bg_color = Colors::SurfaceBase)
        : UIComponent(x, y, w, h), background_color(bg_color),
          border_color(Colors::BorderLight), corner_radius(12),
          border_width(1), use_acrylic(false), acrylic_opacity(200) {}
    
    void SetAcrylicEffect(bool enabled, uint8_t opacity = 200) {
        use_acrylic = enabled;
        acrylic_opacity = opacity;
    }
    
    void Draw() override;
};

// ---- Clock Display Component ----
class ClockDisplay : public UIComponent {
    int hours, minutes, seconds;
    uint32_t text_color;
    
public:
    ClockDisplay(int x, int y)
        : UIComponent(x, y, 120, 60), hours(0), minutes(0), seconds(0), text_color(Colors::TextPrimary) {}
    
    void UpdateTime(int h, int m, int s) {
        hours = h;
        minutes = m;
        seconds = s;
    }
    
    void Draw() override;
};

// ---- Icon Button Component ----
class IconButton : public UIComponent {
    const uint32_t* icon_data;
    size_t icon_width;
    size_t icon_height;
    uint32_t background_color;
    bool is_hovered;
    
public:
    void (*on_click)() = nullptr;
    
    IconButton(int x, int y, int size, const uint32_t* icon = nullptr)
        : UIComponent(x, y, size, size), icon_data(icon),
          icon_width(0), icon_height(0), background_color(Colors::SurfaceSecondary),
          is_hovered(false) {}
    
    void SetIcon(const uint32_t* data, size_t w, size_t h) {
        icon_data = data;
        icon_width = w;
        icon_height = h;
    }
    
    void Draw() override;
    bool HandleMouseClick(int mx, int my) override;
    bool HandleMouseMove(int mx, int my) override;
};

// ---- Text Label Component ----
class Label : public UIComponent {
    const char* text;
    uint32_t text_color;
    size_t font_size;  // 0 = small, 1 = normal, 2 = large, 3 = xl
    
public:
    Label(int x, int y, const char* text = "", uint32_t color = Colors::TextPrimary, size_t size = 1)
        : UIComponent(x, y, 0, 0), text(text), text_color(color), font_size(size) {}
    
    void SetText(const char* new_text) { text = new_text; }
    void Draw() override;
};

}  // namespace JenUI

// ============================================================================
// Fluent Design Helper Functions
// ============================================================================

void DrawFluentButton(int x, int y, int w, int h, const char* label, 
                      uint32_t color = JenUI::Colors::AccentPrimary, 
                      bool hovered = false, bool pressed = false);

void DrawFluentTextBox(int x, int y, int w, int h, const char* text,
                       bool focused = false, const char* placeholder = "");

void DrawFluentPanel(int x, int y, int w, int h, uint32_t bg_color = JenUI::Colors::SurfaceBase);

void DrawFluentShadow(int x, int y, int w, int h, int radius, int depth = 4);

// Transition/animation helper
uint32_t LerpColor(uint32_t from, uint32_t to, float progress);
