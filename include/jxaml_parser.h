#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include "fluent_ui.h"

// ============================================================================
// JXAML Parser - JenOS XAML Markup Loader
// Parses .jxaml files and creates UI elements dynamically
// ============================================================================

namespace JenUI {

// Forward declarations
class UIElement;
union PropertyValue;
class EventHandler;

// ---- Property Value Variant ----
union PropertyValue {
    int int_value;
    float float_value;
    uint32_t color_value;
    const char* string_value;
    void* pointer_value;
    
    PropertyValue() : pointer_value(nullptr) {}
    PropertyValue(int v) : int_value(v) {}
    PropertyValue(float v) : float_value(v) {}
    PropertyValue(uint32_t v) : color_value(v) {}
    PropertyValue(const char* v) : string_value(v) {}
};

// ---- Property Type ----
enum PropertyType {
    PROP_INT,
    PROP_FLOAT,
    PROP_COLOR,
    PROP_STRING,
    PROP_BOOL,
    PROP_POINTER
};

// ---- UI Element Class ----
class UIElement {
public:
    char name[64];
    char type[64];  // "Button", "TextBox", "Label", etc.
    UIComponent* component;
    UIElement* parent;
    UIElement** children;
    size_t child_count;
    
    // Properties map
    struct Property {
        char name[64];
        PropertyType type;
        PropertyValue value;
    };
    Property* properties;
    size_t property_count;
    
    UIElement();
    ~UIElement();
    
    void SetProperty(const char* name, PropertyType type, PropertyValue value);
    PropertyValue GetProperty(const char* name, PropertyType default_type = PROP_STRING);
    void AddChild(UIElement* child);
};

// ---- Event Handler ----
class EventHandlerBinding {
public:
    char event_name[64];
    char handler_class[64];
    char handler_method[64];
    void (*callback)(UIElement*, const char*) = nullptr;
    
    EventHandlerBinding();
    void Parse(const char* handler_string);
    void Invoke(UIElement* element);
};

// ---- JXAML Parser ----
class JXAMLParser {
public:
    JXAMLParser();
    ~JXAMLParser();
    
    // Load and parse a JXAML file
    UIElement* LoadFromFile(const char* filepath);
    
    // Load from memory buffer
    UIElement* LoadFromMemory(const char* jxaml_content, size_t size);
    
    // Create UI components from element tree
    void CreateComponents(UIElement* root);
    
    // Render all components
    void Render(UIElement* root);
    
    // Handle input
    bool HandleMouseClick(UIElement* root, int x, int y);
    bool HandleMouseMove(UIElement* root, int x, int y);
    bool HandleKeyPress(UIElement* root, char key);
    
    // Find element by name
    UIElement* FindElement(UIElement* root, const char* name);
    
    // Get component from element
    UIComponent* GetComponent(UIElement* element);
    
    // Register event handler
    void RegisterEventHandler(const char* handler_class, const char* handler_method, 
                            void (*callback)(UIElement*, const char*));
    
    // Cleanup
    void ReleaseElement(UIElement* element);
    
private:
    // XML parsing
    struct XMLNode {
        char name[64];
        char value[512];
        XMLNode** attributes;
        size_t attribute_count;
        XMLNode** children;
        size_t child_count;
    };
    
    XMLNode* ParseXML(const char* content, size_t size);
    UIElement* BuildElement(XMLNode* xml_node);
    void SetElementProperties(UIElement* element, XMLNode* xml_node);
    
    // Component creation
    UIComponent* CreateComponent(UIElement* element);
    Button* CreateButton(UIElement* element);
    TextInput* CreateTextBox(UIElement* element);
    PasswordInput* CreatePasswordBox(UIElement* element);
    Label* CreateLabel(UIElement* element);
    Panel* CreatePanel(UIElement* element);
    AvatarCircle* CreateAvatarCircle(UIElement* element);
    IconButton* CreateIconButton(UIElement* element);
    
    // Helper methods
    uint32_t ParseColor(const char* color_string);
    int ParseInt(const char* string);
    float ParseFloat(const char* string);
    bool ParseBool(const char* string);
    
    // Event handler registry
    struct EventHandlerRegistry {
        char handler_class[64];
        char handler_method[64];
        void (*callback)(UIElement*, const char*);
    };
    EventHandlerRegistry* event_handlers;
    size_t handler_count;
};

}  // namespace JenUI
