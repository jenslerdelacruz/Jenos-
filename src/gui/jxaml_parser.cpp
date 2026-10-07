#include "jxaml_parser.h"
#include "font.h"
#include "io.h"
#include <string.h>

namespace JenUI {

static int compare_strings(const char* left, const char* right) {
    while (*left && *left == *right) {
        ++left;
        ++right;
    }
    return (unsigned char)*left - (unsigned char)*right;
}

static char* copy_string(char* destination, const char* source, size_t count) {
    size_t index = 0;
    while (index < count && source[index]) {
        destination[index] = source[index];
        ++index;
    }
    while (index < count) {
        destination[index++] = '\0';
    }
    return destination;
}

static const char* find_character(const char* text, char character) {
    while (*text) {
        if (*text == character) return text;
        ++text;
    }
    return character == '\0' ? text : nullptr;
}

static uint32_t parse_hex(const char* text) {
    uint32_t value = 0;
    while (*text) {
        uint32_t digit;
        if (*text >= '0' && *text <= '9') digit = (uint32_t)(*text - '0');
        else if (*text >= 'a' && *text <= 'f') digit = (uint32_t)(*text - 'a' + 10);
        else if (*text >= 'A' && *text <= 'F') digit = (uint32_t)(*text - 'A' + 10);
        else break;
        value = (value << 4) | digit;
        ++text;
    }
    return value;
}

static int parse_integer(const char* text) {
    int sign = 1;
    int value = 0;
    if (*text == '-') {
        sign = -1;
        ++text;
    } else if (*text == '+') {
        ++text;
    }
    while (*text >= '0' && *text <= '9') {
        value = value * 10 + (*text - '0');
        ++text;
    }
    return sign * value;
}

static float parse_decimal(const char* text) {
    float sign = 1.0f;
    float value = 0.0f;
    float fraction_scale = 0.1f;
    if (*text == '-') {
        sign = -1.0f;
        ++text;
    } else if (*text == '+') {
        ++text;
    }
    while (*text >= '0' && *text <= '9') {
        value = value * 10.0f + (float)(*text - '0');
        ++text;
    }
    if (*text == '.') {
        ++text;
        while (*text >= '0' && *text <= '9') {
            value += (float)(*text - '0') * fraction_scale;
            fraction_scale *= 0.1f;
            ++text;
        }
    }
    return sign * value;
}

// ============================================================================
// UIElement Implementation
// ============================================================================

UIElement::UIElement() : component(nullptr), parent(nullptr), child_count(0), property_count(0) {
    name[0] = '\0';
    type[0] = '\0';
    children = nullptr;
    properties = nullptr;
}

UIElement::~UIElement() {
    for (size_t i = 0; i < child_count; ++i) {
        delete children[i];
    }
    if (children) delete[] children;
    if (properties) delete[] properties;
    if (component) delete component;
}

void UIElement::SetProperty(const char* name, PropertyType type, PropertyValue value) {
    // Find existing property or add new one
    for (size_t i = 0; i < property_count; ++i) {
        if (compare_strings(properties[i].name, name) == 0) {
            properties[i].type = type;
            properties[i].value = value;
            return;
        }
    }
    
    // Add new property
    Property* new_props = new Property[property_count + 1];
    if (properties) {
        memcpy(new_props, properties, sizeof(Property) * property_count);
        delete[] properties;
    }
    properties = new_props;
    
    copy_string(properties[property_count].name, name, sizeof(properties[property_count].name) - 1);
    properties[property_count].name[sizeof(properties[property_count].name) - 1] = '\0';
    properties[property_count].type = type;
    properties[property_count].value = value;
    property_count++;
}

PropertyValue UIElement::GetProperty(const char* name, PropertyType default_type) {
    for (size_t i = 0; i < property_count; ++i) {
        if (compare_strings(properties[i].name, name) == 0) {
            return properties[i].value;
        }
    }
    return PropertyValue();
}

void UIElement::AddChild(UIElement* child) {
    UIElement** new_children = new UIElement*[child_count + 1];
    if (children) {
        memcpy(new_children, children, sizeof(UIElement*) * child_count);
        delete[] children;
    }
    children = new_children;
    children[child_count] = child;
    child->parent = this;
    child_count++;
}

// ============================================================================
// EventHandlerBinding Implementation
// ============================================================================

EventHandlerBinding::EventHandlerBinding() {
    event_name[0] = '\0';
    handler_class[0] = '\0';
    handler_method[0] = '\0';
}

void EventHandlerBinding::Parse(const char* handler_string) {
    // Parse format: "ClassName.MethodName"
    const char* dot = find_character(handler_string, '.');
    if (!dot) return;
    
    size_t class_len = dot - handler_string;
    if (class_len >= sizeof(handler_class)) class_len = sizeof(handler_class) - 1;
    copy_string(handler_class, handler_string, class_len);
    handler_class[class_len] = '\0';
    
    copy_string(handler_method, dot + 1, sizeof(handler_method) - 1);
    handler_method[sizeof(handler_method) - 1] = '\0';
}

void EventHandlerBinding::Invoke(UIElement* element) {
    if (callback) {
        callback(element, event_name);
    }
}

// ============================================================================
// JXAMLParser Implementation
// ============================================================================

JXAMLParser::JXAMLParser() : event_handlers(nullptr), handler_count(0) {}

JXAMLParser::~JXAMLParser() {
    if (event_handlers) delete[] event_handlers;
}

UIElement* JXAMLParser::LoadFromFile(const char* filepath) {
    // Simple file loading (in real implementation, would use kernel file system)
    // For now, we'll load from memory or return nullptr
    return nullptr;
}

UIElement* JXAMLParser::LoadFromMemory(const char* jxaml_content, size_t size) {
    // Parse XML
    XMLNode* root = ParseXML(jxaml_content, size);
    if (!root) return nullptr;
    
    // Build element tree
    UIElement* root_element = BuildElement(root);
    
    // Create components
    CreateComponents(root_element);
    
    return root_element;
}

void JXAMLParser::CreateComponents(UIElement* root) {
    if (!root) return;
    
    // Create component for this element
    root->component = CreateComponent(root);
    
    // Recursively create components for children
    for (size_t i = 0; i < root->child_count; ++i) {
        CreateComponents(root->children[i]);
    }
}

void JXAMLParser::Render(UIElement* root) {
    if (!root || !root->component || !root->component->visible) return;
    
    // Render this component
    root->component->Draw();
    
    // Recursively render children
    for (size_t i = 0; i < root->child_count; ++i) {
        Render(root->children[i]);
    }
}

bool JXAMLParser::HandleMouseClick(UIElement* root, int x, int y) {
    if (!root || !root->component) return false;
    
    // Check if click is within component bounds
    if (root->component->IsPointInside(x, y)) {
        root->component->HandleMouseClick(x, y);
        return true;
    }
    
    // Check children
    for (size_t i = 0; i < root->child_count; ++i) {
        if (HandleMouseClick(root->children[i], x, y)) {
            return true;
        }
    }
    
    return false;
}

bool JXAMLParser::HandleMouseMove(UIElement* root, int x, int y) {
    if (!root || !root->component) return false;
    
    root->component->HandleMouseMove(x, y);
    
    for (size_t i = 0; i < root->child_count; ++i) {
        HandleMouseMove(root->children[i], x, y);
    }
    
    return true;
}

bool JXAMLParser::HandleKeyPress(UIElement* root, char key) {
    if (!root || !root->component) return false;
    
    if (root->component->HandleKeyPress(key)) {
        return true;
    }
    
    for (size_t i = 0; i < root->child_count; ++i) {
        if (HandleKeyPress(root->children[i], key)) {
            return true;
        }
    }
    
    return false;
}

UIElement* JXAMLParser::FindElement(UIElement* root, const char* name) {
    if (!root) return nullptr;
    
    if (compare_strings(root->name, name) == 0) {
        return root;
    }
    
    for (size_t i = 0; i < root->child_count; ++i) {
        UIElement* found = FindElement(root->children[i], name);
        if (found) return found;
    }
    
    return nullptr;
}

UIComponent* JXAMLParser::GetComponent(UIElement* element) {
    return element ? element->component : nullptr;
}

void JXAMLParser::RegisterEventHandler(const char* handler_class, const char* handler_method,
                                       void (*callback)(UIElement*, const char*)) {
    EventHandlerRegistry* new_handlers = new EventHandlerRegistry[handler_count + 1];
    if (event_handlers) {
        memcpy(new_handlers, event_handlers, sizeof(EventHandlerRegistry) * handler_count);
        delete[] event_handlers;
    }
    event_handlers = new_handlers;
    
    copy_string(event_handlers[handler_count].handler_class, handler_class, 63);
    copy_string(event_handlers[handler_count].handler_method, handler_method, 63);
    event_handlers[handler_count].handler_class[63] = '\0';
    event_handlers[handler_count].handler_method[63] = '\0';
    event_handlers[handler_count].callback = callback;
    handler_count++;
}

void JXAMLParser::ReleaseElement(UIElement* element) {
    delete element;
}

// ============================================================================
// XML Parsing (Simplified)
// ============================================================================

JXAMLParser::XMLNode* JXAMLParser::ParseXML(const char* content, size_t size) {
    // Simplified XML parser - in production, use proper XML library
    // For now, return nullptr to indicate parser stub
    // Real implementation would tokenize and build tree
    return nullptr;
}

UIElement* JXAMLParser::BuildElement(XMLNode* xml_node) {
    if (!xml_node) return nullptr;
    
    UIElement* element = new UIElement();
    
    // Set element type from XML node name
    copy_string(element->type, xml_node->name, sizeof(element->type) - 1);
    element->type[sizeof(element->type) - 1] = '\0';
    
    // Copy attributes as properties
    for (size_t i = 0; i < xml_node->attribute_count; ++i) {
        XMLNode* attr = xml_node->attributes[i];
        // Parse and set property
    }
    
    // Add children
    for (size_t i = 0; i < xml_node->child_count; ++i) {
        UIElement* child = BuildElement(xml_node->children[i]);
        if (child) element->AddChild(child);
    }
    
    return element;
}

void JXAMLParser::SetElementProperties(UIElement* element, XMLNode* xml_node) {
    // Set properties from XML attributes
}

// ============================================================================
// Component Creation
// ============================================================================

UIComponent* JXAMLParser::CreateComponent(UIElement* element) {
    if (!element) return nullptr;
    
    if (compare_strings(element->type, "Button") == 0) {
        return CreateButton(element);
    } else if (compare_strings(element->type, "TextBox") == 0) {
        return CreateTextBox(element);
    } else if (compare_strings(element->type, "PasswordBox") == 0) {
        return CreatePasswordBox(element);
    } else if (compare_strings(element->type, "Label") == 0) {
        return CreateLabel(element);
    } else if (compare_strings(element->type, "Panel") == 0) {
        return CreatePanel(element);
    } else if (compare_strings(element->type, "AvatarCircle") == 0) {
        return CreateAvatarCircle(element);
    } else if (compare_strings(element->type, "IconButton") == 0) {
        return CreateIconButton(element);
    }
    
    return nullptr;
}

Button* JXAMLParser::CreateButton(UIElement* element) {
    if (!element) return nullptr;
    
    int x = ParseInt(element->GetProperty("X", PROP_INT).string_value);
    int y = ParseInt(element->GetProperty("Y", PROP_INT).string_value);
    int w = ParseInt(element->GetProperty("Width", PROP_INT).string_value);
    int h = ParseInt(element->GetProperty("Height", PROP_INT).string_value);
    
    const char* content = element->GetProperty("Content", PROP_STRING).string_value;
    uint32_t bg = ParseColor(element->GetProperty("Background", PROP_COLOR).string_value);
    
    Button* btn = new Button(x, y, w, h, content, bg);
    return btn;
}

TextInput* JXAMLParser::CreateTextBox(UIElement* element) {
    if (!element) return nullptr;
    
    int x = ParseInt(element->GetProperty("X", PROP_INT).string_value);
    int y = ParseInt(element->GetProperty("Y", PROP_INT).string_value);
    int w = ParseInt(element->GetProperty("Width", PROP_INT).string_value);
    int h = ParseInt(element->GetProperty("Height", PROP_INT).string_value);
    
    const char* placeholder = element->GetProperty("Placeholder", PROP_STRING).string_value;
    
    TextInput* input = new TextInput(x, y, w, h, 32, placeholder);
    return input;
}

PasswordInput* JXAMLParser::CreatePasswordBox(UIElement* element) {
    if (!element) return nullptr;
    
    int x = ParseInt(element->GetProperty("X", PROP_INT).string_value);
    int y = ParseInt(element->GetProperty("Y", PROP_INT).string_value);
    int w = ParseInt(element->GetProperty("Width", PROP_INT).string_value);
    int h = ParseInt(element->GetProperty("Height", PROP_INT).string_value);
    
    PasswordInput* input = new PasswordInput(x, y, w, h, 32);
    return input;
}

Label* JXAMLParser::CreateLabel(UIElement* element) {
    if (!element) return nullptr;
    
    int x = ParseInt(element->GetProperty("X", PROP_INT).string_value);
    int y = ParseInt(element->GetProperty("Y", PROP_INT).string_value);
    
    const char* text = element->GetProperty("Text", PROP_STRING).string_value;
    uint32_t color = ParseColor(element->GetProperty("Foreground", PROP_COLOR).string_value);
    
    Label* label = new Label(x, y, text, color);
    return label;
}

Panel* JXAMLParser::CreatePanel(UIElement* element) {
    if (!element) return nullptr;
    
    int x = ParseInt(element->GetProperty("X", PROP_INT).string_value);
    int y = ParseInt(element->GetProperty("Y", PROP_INT).string_value);
    int w = ParseInt(element->GetProperty("Width", PROP_INT).string_value);
    int h = ParseInt(element->GetProperty("Height", PROP_INT).string_value);
    
    uint32_t bg = ParseColor(element->GetProperty("Background", PROP_COLOR).string_value);
    
    Panel* panel = new Panel(x, y, w, h, bg);
    return panel;
}

AvatarCircle* JXAMLParser::CreateAvatarCircle(UIElement* element) {
    if (!element) return nullptr;
    
    int x = ParseInt(element->GetProperty("X", PROP_INT).string_value);
    int y = ParseInt(element->GetProperty("Y", PROP_INT).string_value);
    int diameter = ParseInt(element->GetProperty("Diameter", PROP_INT).string_value);
    
    AvatarCircle* avatar = new AvatarCircle(x, y, diameter);
    return avatar;
}

IconButton* JXAMLParser::CreateIconButton(UIElement* element) {
    if (!element) return nullptr;
    
    int x = ParseInt(element->GetProperty("X", PROP_INT).string_value);
    int y = ParseInt(element->GetProperty("Y", PROP_INT).string_value);
    int size = ParseInt(element->GetProperty("Size", PROP_INT).string_value);
    
    IconButton* btn = new IconButton(x, y, size);
    return btn;
}

// ============================================================================
// Helper Methods
// ============================================================================

uint32_t JXAMLParser::ParseColor(const char* color_string) {
    if (!color_string) return 0xFFFFFFFF;
    
    if (color_string[0] == '#') {
        // Parse hex color: #RRGGBB or #RRGGBBAA
        uint32_t color = 0;
        color = parse_hex(color_string + 1);
        return color;
    }
    
    return 0xFFFFFFFF;
}

int JXAMLParser::ParseInt(const char* string) {
    if (!string) return 0;
    return parse_integer(string);
}

float JXAMLParser::ParseFloat(const char* string) {
    if (!string) return 0.0f;
    return parse_decimal(string);
}

bool JXAMLParser::ParseBool(const char* string) {
    if (!string) return false;
    return compare_strings(string, "True") == 0 || compare_strings(string, "true") == 0;
}

}  // namespace JenUI
