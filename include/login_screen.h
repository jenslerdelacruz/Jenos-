#pragma once

#include <stdint.h>
#include <stddef.h>
#include "graphics.h"
#include "fluent_ui.h"

// ============================================================================
// JenOS Login Screen (Fluent Design Version)
// Premium desktop operating system login experience
// ============================================================================

// Login screen component manager
class LoginScreenManager {
public:
    static void Initialize();
    static void Render();
    static void HandleMouseClick(int mx, int my);
    static void HandleMouseMove(int mx, int my);
    static void HandleKeyPress(char key);
    static bool TryLogin(const char* username, const char* password);
    static void TransitionToDesktop();
    static bool ShouldTransitionToDesktop() { return transitioning; }
    
private:
    static void DrawLoginPanel();
    static void DrawSystemTray();
    static void DrawWallpaper();
    static void DrawAnimations();
    
    // UI Components
    static JenUI::AvatarCircle* avatar_circle;
    static JenUI::Label* welcome_label;
    static JenUI::Label* subtitle_label;
    static JenUI::TextInput* username_input;
    static JenUI::PasswordInput* password_input;
    static JenUI::Button* signin_button;
    static JenUI::Button* guest_button;
    static JenUI::Button* create_user_button;
    static JenUI::ClockDisplay* clock_display;
    static JenUI::IconButton* power_button;
    static JenUI::IconButton* settings_button;
    static JenUI::IconButton* network_button;
    
    // Animation state
    static float panel_fade_progress;
    static float panel_scale_progress;
    static int animation_tick;
    static bool transitioning;
};
