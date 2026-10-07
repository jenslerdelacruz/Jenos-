#pragma once

#include <stdint.h>
#include <stddef.h>
#include "jxaml_parser.h"

// ============================================================================
// LoginView - Application Logic and Event Handling
// MVVM-style view model for the login screen
// ============================================================================

class LoginView {
public:
    // Initialization and cleanup
    static void Initialize();
    static void Cleanup();
    
    // Rendering
    static void Render();
    
    // Input handling
    static void HandleKeyPress(char key);
    static void HandleMouseClick(int x, int y);
    static void HandleMouseMove(int x, int y);
    
    // State queries
    static bool ShouldTransitionToDesktop();
    static bool LoginFailed();
    
    // Event handlers (bound from JXAML)
    static void OnSignInClicked(JenUI::UIElement* element, const char* event_name);
    static void OnGuestClicked(JenUI::UIElement* element, const char* event_name);
    static void OnCreateUserClicked(JenUI::UIElement* element, const char* event_name);
    static void OnWindowLoaded(JenUI::UIElement* element, const char* event_name);
    
private:
    // UI state
    static JenUI::UIElement* root_element;
    static JenUI::JXAMLParser* parser;
    
    // Business logic
    static bool authenticated;
    static bool should_transition;
    static bool login_failed;
    static int failed_attempts;
    
    // Helper methods
    static bool AuthenticateUser(const char* username, const char* password);
    static void ShowLoginError(const char* message);
    static void LoginAsGuest();
    static void TransitionToDesktop();
    static void ClearInputFields();
    
    // Get UI elements by name
    static JenUI::TextInput* GetUsernameInput();
    static JenUI::PasswordInput* GetPasswordInput();
};
