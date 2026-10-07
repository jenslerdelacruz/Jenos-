#include "login_screen.h"
#include "wallpaper.h"
#include "rtc.h"
#include "string.h"
#include "font.h"
#include "mouse.h"
#include "settings.h"


JenUI::AvatarCircle* LoginScreenManager::avatar_circle = nullptr;
JenUI::Label* LoginScreenManager::welcome_label = nullptr;
JenUI::Label* LoginScreenManager::subtitle_label = nullptr;
JenUI::TextInput* LoginScreenManager::username_input = nullptr;
JenUI::PasswordInput* LoginScreenManager::password_input = nullptr;
JenUI::Button* LoginScreenManager::signin_button = nullptr;
JenUI::Button* LoginScreenManager::guest_button = nullptr;
JenUI::Button* LoginScreenManager::create_user_button = nullptr;
JenUI::ClockDisplay* LoginScreenManager::clock_display = nullptr;
JenUI::IconButton* LoginScreenManager::power_button = nullptr;
JenUI::IconButton* LoginScreenManager::settings_button = nullptr;
JenUI::IconButton* LoginScreenManager::network_button = nullptr;

float LoginScreenManager::panel_fade_progress = 0.0f;
float LoginScreenManager::panel_scale_progress = 0.0f;
int LoginScreenManager::animation_tick = 0;
bool LoginScreenManager::transitioning = false;

extern void rtc_read_time(int* hours, int* minutes, int* seconds);
static int scale_login_value(int value) {
    return (value * (int)screen_height + 360) / 720;
}

void LoginScreenManager::Initialize() {
    int screen_center_x = screen_width / 2;
    int screen_center_y = screen_height / 2;
    int font_scale = screen_height >= 900 ? 2 : 1;

    avatar_circle = new JenUI::AvatarCircle(
        screen_center_x - scale_login_value(40),
        screen_center_y - scale_login_value(216),
        scale_login_value(80),
        nullptr
    );

    const char* welcome_text = "Welcome back";
    const char* subtitle_text = "Sign in to continue";
    welcome_label = new JenUI::Label(
        screen_center_x - (int)(strlen(welcome_text) * 8 * (font_scale + 1)) / 2,
        screen_center_y - scale_login_value(108),
        welcome_text,
        JenUI::Colors::TextPrimary,
        2
    );

    subtitle_label = new JenUI::Label(
        screen_center_x - (int)(strlen(subtitle_text) * 8 * font_scale) / 2,
        screen_center_y - scale_login_value(76),
        subtitle_text,
        JenUI::Colors::TextSecondary,
        1
    );

    int input_width = scale_login_value(340);
    int input_x = screen_center_x - input_width / 2;

    username_input = new JenUI::TextInput(
        input_x,
        screen_center_y - scale_login_value(25),
        input_width,
        scale_login_value(48),
        32,
        "Username"
    );

    password_input = new JenUI::PasswordInput(
        input_x,
        screen_center_y + scale_login_value(38),
        input_width,
        scale_login_value(48),
        32
    );

    signin_button = new JenUI::Button(
        input_x,
        screen_center_y + scale_login_value(105),
        input_width,
        scale_login_value(52),
        "Sign In",
        settings_get_accent_color()
    );

    int secondary_button_width = scale_login_value(160);
    int secondary_button_height = scale_login_value(44);
    int secondary_spacing = scale_login_value(10);
    int secondary_x = input_x;
    int secondary_y = screen_center_y + scale_login_value(173);

    guest_button = new JenUI::Button(
        secondary_x,
        secondary_y,
        secondary_button_width,
        secondary_button_height,
        "Guest",
        JenUI::Colors::SurfaceSecondary
    );

    create_user_button = new JenUI::Button(
        secondary_x + secondary_button_width + secondary_spacing,
        secondary_y,
        secondary_button_width + scale_login_value(20),
        secondary_button_height,
        "Create User",
        JenUI::Colors::SurfaceSecondary
    );

    clock_display = new JenUI::ClockDisplay(scale_login_value(40), scale_login_value(30));

    int icon_button_size = scale_login_value(40);
    int icon_x_base = screen_width - scale_login_value(200);

    power_button = new JenUI::IconButton(
        icon_x_base,
        scale_login_value(30),
        icon_button_size
    );

    settings_button = new JenUI::IconButton(
        icon_x_base + scale_login_value(60),
        scale_login_value(30),
        icon_button_size
    );

    network_button = new JenUI::IconButton(
        icon_x_base + scale_login_value(120),
        scale_login_value(30),
        icon_button_size
    );
}

// ============================================================================
// Rendering
// ============================================================================

void LoginScreenManager::DrawWallpaper() {
    // Premium gradient background (Mica-inspired)
    for (size_t y = 0; y < screen_height; ++y) {
        float y_ratio = (float)y / screen_height;
        
        // Base gradient colors
        uint32_t top_color = 0x1A1A1F;      // Dark blue-gray
        uint32_t middle_color = 0x252530;   // Slightly lighter
        uint32_t bottom_color = 0x1F2530;   // Darker blue
        
        // Blend between colors based on position
        uint32_t line_color;
        if (y_ratio < 0.5f) {
            float local_ratio = y_ratio * 2.0f;
            line_color = LerpColor(top_color, middle_color, local_ratio);
        } else {
            float local_ratio = (y_ratio - 0.5f) * 2.0f;
            line_color = LerpColor(middle_color, bottom_color, local_ratio);
        }
        
        for (size_t x = 0; x < screen_width; ++x) {
            draw_pixel(x, y, line_color);
        }
    }
}

void LoginScreenManager::DrawLoginPanel() {
    int screen_center_x = screen_width / 2;
    int screen_center_y = screen_height / 2;

    int panel_width = scale_login_value(450);
    int panel_height = scale_login_value(520);
    int panel_x = screen_center_x - panel_width / 2;
    int panel_y = screen_center_y - panel_height / 2;
    
    // Apply animation to panel
    if (!transitioning) {
        // Fade and scale animation
        int duration = JenUI::ANIMATION_DURATION_SLOW;
        if (animation_tick < duration) {
            panel_fade_progress = (float)animation_tick / duration;
            panel_fade_progress = JenUI::Easing::EaseOutCubic(panel_fade_progress);
            animation_tick++;
        } else {
            panel_fade_progress = 1.0f;
        }
    }
    
    // Draw premium shadow layers
    uint8_t shadow_alpha = (uint8_t)(100 * panel_fade_progress);
    int shadow_depth = scale_login_value(8);
    for (int i = shadow_depth; i > 0; --i) {
        uint8_t alpha = (uint8_t)(shadow_alpha * i / shadow_depth);
        if (alpha > 0) {
            uint32_t shadow_color = blend(0x000000, JenUI::Colors::Background, alpha);
            draw_rounded_rect(panel_x + i, panel_y + i, panel_width, panel_height,
                              scale_login_value(16), shadow_color);
        }
    }
    
    // Draw main panel with Acrylic effect
    JenUI::FluentEffects::DrawAcrylicPanel(
        panel_x, panel_y, panel_width, panel_height, scale_login_value(16),
        JenUI::Colors::Background, (uint8_t)(200 * panel_fade_progress)
    );
    
    // Draw decorative top accent line
    draw_rect(panel_x + scale_login_value(20), panel_y + 1,
              panel_width - scale_login_value(40), scale_login_value(3),
              JenUI::Colors::AccentLight);
    
    // Draw components inside panel
    if (avatar_circle) avatar_circle->Draw();
    if (welcome_label) welcome_label->Draw();
    if (subtitle_label) subtitle_label->Draw();
    if (username_input) username_input->Draw();
    if (password_input) password_input->Draw();
    if (signin_button) signin_button->Draw();
    if (guest_button) guest_button->Draw();
    if (create_user_button) create_user_button->Draw();
}

void LoginScreenManager::DrawSystemTray() {
    // Top-left: Clock and time
    if (clock_display) {
        int h, m, s;
        rtc_read_time(&h, &m, &s);
        clock_display->UpdateTime(h, m, s);
        clock_display->Draw();
    }
    
    // Top-right: System icons
    if (power_button) power_button->Draw();
    if (settings_button) settings_button->Draw();
    if (network_button) network_button->Draw();
    
    // Draw "Powered by JenOS" text at bottom
    draw_string_scaled(
        screen_width / 2 - (int)(strlen("Powered by JenOS") * 8 * (screen_height >= 900 ? 2 : 1)) / 2,
        screen_height - scale_login_value(30),
        "Powered by JenOS",
        JenUI::Colors::TextTertiary,
        0,
        true,
        screen_height >= 900 ? 2 : 1
    );
}

void LoginScreenManager::DrawAnimations() {
    // Could be used for:
    // - Loading animation
    // - Transition effects
    // - Cursor animations
    // - Status updates (e.g., "Signing in...")
}

void LoginScreenManager::Render() {
    reset_mouse_cursor_state();

    // Clear and draw wallpaper
    DrawWallpaper();
    
    // Draw login panel with animation
    DrawLoginPanel();
    
    // Draw system tray
    DrawSystemTray();
    
    // Draw any ongoing animations
    DrawAnimations();
    
    // Swap buffers to display
    swap_buffers();
}

// ============================================================================
// Input Handling
// ============================================================================

void LoginScreenManager::HandleMouseClick(int mx, int my) {
    if (transitioning) return;
    
    // Check input fields
    if (username_input) {
        username_input->HandleMouseClick(mx, my);
    }
    if (password_input) {
        password_input->HandleMouseClick(mx, my);
    }
    
    // Check buttons
    if (signin_button && signin_button->IsPointInside(mx, my)) {
        const char* username = username_input ? username_input->GetText() : "";
        const char* password = password_input ? password_input->GetText() : "";
        
        if (strlen(username) > 0 && strlen(password) > 0) {
            TryLogin(username, password);
        }
    }
    
    if (guest_button && guest_button->IsPointInside(mx, my)) {
        TransitionToDesktop();
    }
    
    if (create_user_button && create_user_button->IsPointInside(mx, my)) {
        // Future: Show user creation dialog
    }
    
    // Check system buttons
    if (power_button && power_button->IsPointInside(mx, my)) {
        // Future: Show power menu
    }
}

void LoginScreenManager::HandleMouseMove(int mx, int my) {
    if (signin_button) signin_button->HandleMouseMove(mx, my);
    if (guest_button) guest_button->HandleMouseMove(mx, my);
    if (create_user_button) create_user_button->HandleMouseMove(mx, my);
}

void LoginScreenManager::HandleKeyPress(char key) {
    // Tab to switch between fields
    if (key == '\t') {
        if (username_input && username_input->IsFocused()) {
            username_input->HandleMouseClick(0, 0);  // Unfocus
            if (password_input) {
                password_input->HandleMouseClick(password_input->x, password_input->y);  // Focus
            }
        } else if (password_input && password_input->IsFocused()) {
            password_input->HandleMouseClick(0, 0);  // Unfocus
            if (username_input) {
                username_input->HandleMouseClick(username_input->x, username_input->y);  // Focus
            }
        }
    }
    // Enter to sign in
    else if (key == '\r' || key == '\n') {
        const char* username = username_input ? username_input->GetText() : "";
        const char* password = password_input ? password_input->GetText() : "";
        
        if (strlen(username) > 0 && strlen(password) > 0) {
            TryLogin(username, password);
        }
    }
    // Regular input
    else {
        if (username_input && username_input->IsFocused()) {
            username_input->HandleKeyPress(key);
        } else if (password_input && password_input->IsFocused()) {
            password_input->HandleKeyPress(key);
        }
    }
}

// ============================================================================
// Login Logic
// ============================================================================

bool LoginScreenManager::TryLogin(const char* username, const char* password) {
    // Simple authentication (in real OS, this would check credentials)
    if (strcmp(username, "admin") == 0 && strcmp(password, "admin") == 0) {
        TransitionToDesktop();
        return true;
    } else if (strcmp(username, "user") == 0 && strcmp(password, "password") == 0) {
        TransitionToDesktop();
        return true;
    }
    
    // Failed login - could show error message
    return false;
}

void LoginScreenManager::TransitionToDesktop() {
    transitioning = true;
    // This would trigger the actual transition to desktop
    // The kernel.cpp would need to handle this flag
}
