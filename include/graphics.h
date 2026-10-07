#pragma once

#include <stdint.h>
#include <stddef.h>

#define DESIRED_SCREEN_WIDTH 1920
#define DESIRED_SCREEN_HEIGHT 1080

void init_graphics();
bool graphics_set_resolution(size_t width, size_t height);
void draw_pixel(size_t x, size_t y, uint32_t color);
void draw_rect(size_t x, size_t y, size_t width, size_t height, uint32_t color);
void draw_filled_circle(int x0, int y0, int radius, uint32_t color);
void draw_rounded_rect(int x, int y, int width, int height, int radius, uint32_t color);
void clear_screen(uint32_t color);
void draw_bitmap(const uint32_t* pixels, size_t src_width, size_t src_height, size_t dst_x, size_t dst_y);
void draw_bitmap_scaled(const uint32_t* pixels, size_t src_width, size_t src_height, size_t dst_x, size_t dst_y, size_t dst_width, size_t dst_height);
void draw_bitmap_cover(const uint32_t* pixels, size_t src_width, size_t src_height,
                       size_t dst_width, size_t dst_height);
void draw_argb_bitmap(size_t x, size_t y, const uint32_t* pixels, size_t width, size_t height);
void draw_argb_bitmap_scaled(size_t x, size_t y, const uint32_t* pixels, size_t src_width, size_t src_height, size_t dst_width, size_t dst_height);
uint32_t* load_png_image(const unsigned char* data, size_t size, size_t* width, size_t* height);
void draw_image(size_t x, size_t y, const uint32_t* pixels, size_t width, size_t height);
void draw_image_scaled(size_t x, size_t y, const uint32_t* pixels, size_t src_width, size_t src_height, size_t dst_width, size_t dst_height);
void draw_char(char c, size_t x, size_t y, uint32_t color);
void draw_string(size_t x, size_t y, const char* str, uint32_t fg_color, uint32_t bg_color = 0, bool transparent_bg = true);
void draw_cursor(size_t x, size_t y, size_t scale = 1);
void present_region(size_t x, size_t y, size_t width, size_t height);
void swap_buffers();

extern size_t screen_width;
extern size_t screen_height;
extern uint32_t* volatile framebuffer;
extern uint32_t* backbuffer;

static inline uint32_t blend(uint32_t fg, uint32_t bg, uint8_t alpha) {
    if (alpha == 0) return bg;
    if (alpha == 255) return fg;
    
    uint32_t rb = fg & 0xFF00FF;
    uint32_t g  = fg & 0x00FF00;
    
    uint32_t rb_bg = bg & 0xFF00FF;
    uint32_t g_bg  = bg & 0x00FF00;
    
    uint32_t rb_out = ((rb * alpha + rb_bg * (255 - alpha)) >> 8) & 0xFF00FF;
    uint32_t g_out  = ((g * alpha + g_bg * (255 - alpha)) >> 8) & 0x00FF00;
    
    return rb_out | g_out;
}

static inline uint32_t get_pixel(size_t x, size_t y) {
    if (!backbuffer || x >= screen_width || y >= screen_height) return 0;
    return backbuffer[y * screen_width + x];
}
