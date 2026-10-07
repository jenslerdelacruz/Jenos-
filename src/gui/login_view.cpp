#include "login_view.h"
#include "graphics.h"
#include "string.h"
#include "io.h"

extern void serial_write(const char* s);
extern void serial_write_ln(const char* s);
extern void serial_write_dec(uint32_t v);

// ============================================================================
// Static Member Initialization
// ============================================================================

JenUI::UIElement* LoginView::root_element = nullptr;
JenUI::JXAMLParser* LoginView::parser = nullptr;

bool LoginView::authenticated = false;
bool LoginView::should_transition = false;
bool LoginView::login_failed = false;
int LoginView::failed_attempts = 0;

// ============================================================================
// Initialization
// ============================================================================

void LoginView::Initialize() {
    serial_write_ln("LoginView::Initialize");
    
    // Create parser
    parser = new JenUI::JXAMLParser();
    
    // Register event handlers
    parser->RegisterEventHandler("LoginView", "OnSignInClicked", 
                               (void (*)(JenUI::UIElement*, const char*))OnSignInClicked);
    parser->RegisterEventHandler("LoginView", "OnGuestClicked", 
                               (void (*)(JenUI::UIElement*, const char*))OnGuestClicked);
    parser->RegisterEventHandler("LoginView", "OnCreateUserClicked", 
                               (void (*)(JenUI::UIElement*, const char*))OnCreateUserClicked);
    parser->RegisterEventHandler("LoginView", "OnWindowLoaded", 
                               (void (*)(JenUI::UIElement*, const char*))OnWindowLoaded);
    
    // Load JXAML markup
    // For now, we'll load from embedded string (in production, load from file)
    const char* jxaml_content = R"(<?xml version="1.0"?>
<Window Width="1280" Height="720" Background="#1A1A1F" Name="LoginWindow">
</Window>)";
    
    root_element = parser->LoadFromMemory(jxaml_content, strlen(jxaml_content));
    
    if (root_element) {
        serial_write_ln("LoginView JXAML loaded successfully");
    } else {
        serial_write_ln("ERROR: LoginView JXAML failed to load");
    }
}

void LoginView::Cleanup() {
    if (root_element) {
        parser->ReleaseElement(root_element);
        root_element = nullptr;
    }
    
    if (parser) {
        delete parser;
        parser = nullptr;
    }
}

// ============================================================================
// Rendering
// ============================================================================

void LoginView::Render() {
    if (!parser || !root_element) return;
    
    // Render all UI elements
    parser->Render(root_element);
    
    // Draw error message if login failed
    if (login_failed) {
        ShowLoginError("Invalid username or password");
    }
    
    // Swap buffers
    swap_buffers();
}

// ============================================================================
// Input Handling
// ============================================================================

void LoginView::HandleKeyPress(char key) {
    if (!parser || !root_element) return;
    
    // Special key handling
    if (key == '\n' || key == '\r') {
        // Enter key - attempt login
        OnSignInClicked(root_element, "Click");
        return;
    } else if (key == '\t') {
        // Tab key - switch focus between fields
        // Handled by parser
    }
    
    // Route to parser for regular key input
    parser->HandleKeyPress(root_element, key);
}

void LoginView::HandleMouseClick(int x, int y) {
    if (!parser || !root_element) return;
    
    parser->HandleMouseClick(root_element, x, y);
}

void LoginView::HandleMouseMove(int x, int y) {
    if (!parser || !root_element) return;
    
    parser->HandleMouseMove(root_element, x, y);
}

// ============================================================================
// State Queries
// ============================================================================

bool LoginView::ShouldTransitionToDesktop() {
    return should_transition;
}

bool LoginView::LoginFailed() {
    return login_failed;
}

static bool login_view_string_equals(const char* left, const char* right) {
    while (*left && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

// ============================================================================
// Event Handlers
// ============================================================================

void LoginView::OnSignInClicked(JenUI::UIElement* element, const char* event_name) {
    serial_write_ln("LoginView::OnSignInClicked");
    
    // Get input values
    JenUI::TextInput* username_input = GetUsernameInput();
    JenUI::PasswordInput* password_input = GetPasswordInput();
    
    if (!username_input || !password_input) {
        serial_write_ln("ERROR: UI elements not found");
        return;
    }
    
    const char* username = username_input->GetText();
    const char* password = password_input->GetText();
    
    serial_write("Login attempt: "); serial_write(username); serial_write_ln("");
    
    // Validate input
    if (strlen(username) == 0 || strlen(password) == 0) {
        login_failed = true;
        serial_write_ln("Login failed: Empty credentials");
        return;
    }
    
    // Authenticate
    if (AuthenticateUser(username, password)) {
        authenticated = true;
        should_transition = true;
        failed_attempts = 0;
        serial_write_ln("Login successful!");
    } else {
        login_failed = true;
        failed_attempts++;
        serial_write("Login failed. Attempts: "); 
        serial_write_dec(failed_attempts);
        serial_write_ln("");
    }
}

void LoginView::OnGuestClicked(JenUI::UIElement* element, const char* event_name) {
    serial_write_ln("LoginView::OnGuestClicked");
    LoginAsGuest();
}

void LoginView::OnCreateUserClicked(JenUI::UIElement* element, const char* event_name) {
    serial_write_ln("LoginView::OnCreateUserClicked");
    // Future: Show user creation dialog
}

void LoginView::OnWindowLoaded(JenUI::UIElement* element, const char* event_name) {
    serial_write_ln("LoginView::OnWindowLoaded");
    // Initialize any animations or setup
}

// ============================================================================
// Authentication Logic
// ============================================================================

bool LoginView::AuthenticateUser(const char* username, const char* password) {
    // Simple authentication (in production, would check against user database)
    if (strcmp(username, "admin") == 0 && strcmp(password, "admin") == 0) {
        return true;
    } else if (strcmp(username, "user") == 0 && strcmp(password, "password") == 0) {
        return true;
    }
    
    return false;
}

void LoginView::ShowLoginError(const char* message) {
    // Draw error message on screen
    draw_string(
        (screen_width / 2) - (strlen(message) * 4),
        screen_height / 2 + 250,
        message,
        JenUI::Colors::Error,
        0,
        true
    );
}

void LoginView::LoginAsGuest() {
    authenticated = true;
    should_transition = true;
    serial_write_ln("Logged in as Guest");
}

void LoginView::TransitionToDesktop() {
    should_transition = true;
    serial_write_ln("Transitioning to desktop...");
}

void LoginView::ClearInputFields() {
    JenUI::TextInput* username_input = GetUsernameInput();
    JenUI::PasswordInput* password_input = GetPasswordInput();
    
    if (username_input) username_input->Clear();
    if (password_input) password_input->Clear();
}

// ============================================================================
// UI Element Getters
// ============================================================================

JenUI::TextInput* LoginView::GetUsernameInput() {
    if (!parser || !root_element) return nullptr;
    
    JenUI::UIElement* element = parser->FindElement(root_element, "UsernameInput");
    if (element && element->component && login_view_string_equals(element->type, "TextBox")) {
        return static_cast<JenUI::TextInput*>(element->component);
    }
    
    return nullptr;
}

JenUI::PasswordInput* LoginView::GetPasswordInput() {
    if (!parser || !root_element) return nullptr;
    
    JenUI::UIElement* element = parser->FindElement(root_element, "PasswordInput");
    if (element && element->component && login_view_string_equals(element->type, "PasswordBox")) {
        return static_cast<JenUI::PasswordInput*>(element->component);
    }
    
    return nullptr;
}
