#pragma once

#include <stdint.h>
#include <stddef.h>

void draw_char(size_t x, size_t y, char c, uint32_t fg_color, uint32_t bg_color, bool transparent_bg);
void draw_string(size_t x, size_t y, const char* str, uint32_t fg_color, uint32_t bg_color, bool transparent_bg);
void draw_string_scaled(size_t x, size_t y, const char* str, uint32_t fg_color,
                        uint32_t bg_color, bool transparent_bg, size_t scale);
