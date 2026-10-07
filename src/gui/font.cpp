#include "font.h"
#include "graphics.h"
#include "smooth_font.h"

// blend and get_pixel are now provided by graphics.h

void draw_char(size_t x, size_t y, char c, uint32_t fg_color, uint32_t bg_color, bool transparent_bg) {
    if (c < 0 || c > 127) {
        c = '?';
    }

    const uint8_t* glyph = smooth_font[(uint8_t)c];
    for (int row = 0; row < smooth_font_height; row++) {
        for (int col = 0; col < smooth_font_width; col++) {
            uint8_t alpha = glyph[row * smooth_font_width + col];
            
            if (alpha > 0) {
                uint32_t base_color = transparent_bg ? get_pixel(x + col, y + row) : bg_color;
                uint32_t final_color = blend(fg_color, base_color, alpha);
                draw_pixel(x + col, y + row, final_color);
            } else if (!transparent_bg) {
                draw_pixel(x + col, y + row, bg_color);
            }
        }
    }
}

void draw_string(size_t x, size_t y, const char* str, uint32_t fg_color, uint32_t bg_color, bool transparent_bg) {
    size_t current_x = x;
    size_t current_y = y;

    for (size_t i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            current_y += smooth_font_height + 2; // line spacing
            current_x = x;
            continue;
        }

        draw_char(current_x, current_y, str[i], fg_color, bg_color, transparent_bg);
        current_x += smooth_font_width; // Monospace kerning
    }
}

void draw_string_scaled(size_t x, size_t y, const char* str, uint32_t fg_color,
                        uint32_t bg_color, bool transparent_bg, size_t scale) {
    if (!str || scale == 0) return;

    for (size_t character = 0; str[character] != '\0'; ++character) {
        if (str[character] == '\n') continue;

        char c = str[character];
        if (c < 0 || c > 127) c = '?';
        const uint8_t* glyph = smooth_font[(uint8_t)c];

        for (size_t out_y = 0; out_y < (size_t)smooth_font_height * scale; ++out_y) {
            size_t src_y_fp = (out_y * 256) / scale;
            size_t src_y = src_y_fp >> 8;
            size_t next_y = src_y + 1 < (size_t)smooth_font_height ? src_y + 1 : src_y;
            uint32_t fy = src_y_fp & 0xFF;

            for (size_t out_x = 0; out_x < (size_t)smooth_font_width * scale; ++out_x) {
                size_t src_x_fp = (out_x * 256) / scale;
                size_t src_x = src_x_fp >> 8;
                size_t next_x = src_x + 1 < (size_t)smooth_font_width ? src_x + 1 : src_x;
                uint32_t fx = src_x_fp & 0xFF;

                uint32_t a00 = glyph[src_y * smooth_font_width + src_x];
                uint32_t a10 = glyph[src_y * smooth_font_width + next_x];
                uint32_t a01 = glyph[next_y * smooth_font_width + src_x];
                uint32_t a11 = glyph[next_y * smooth_font_width + next_x];
                uint32_t top = (a00 * (256 - fx) + a10 * fx) >> 8;
                uint32_t bottom = (a01 * (256 - fx) + a11 * fx) >> 8;
                uint8_t alpha = (uint8_t)((top * (256 - fy) + bottom * fy) >> 8);

                if (alpha > 0 || !transparent_bg) {
                    size_t pixel_x = x + character * (size_t)smooth_font_width * scale + out_x;
                    size_t pixel_y = y + out_y;
                    uint32_t background = transparent_bg ? get_pixel(pixel_x, pixel_y) : bg_color;
                    draw_pixel(pixel_x, pixel_y, blend(fg_color, background, alpha));
                }
            }
        }
    }
}
