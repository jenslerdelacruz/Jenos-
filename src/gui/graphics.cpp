#include "graphics.h"
#include "icons.h"
#include "png_icons.h"
#include "io.h"
#include "settings.h"
#include <string.h>

static void* krealloc_sized(void* ptr, size_t old_size, size_t new_size);
extern void* kmalloc(size_t size);
extern void kfree(void* ptr);

#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STBI_NO_FAILURE_STRINGS
#define STBI_MALLOC kmalloc
#define STBI_REALLOC_SIZED krealloc_sized
#define STBI_FREE kfree
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// ============================================
// Bochs Graphics Adapter (BGA) / VBE Extensions
// This driver uses the Bochs VGA framebuffer interface through I/O ports.
// It requires a QEMU-compatible Bochs display device, for example:
//   qemu-system-i386 -kernel jenos.bin -device bochs-display
// ============================================

#define VBE_DISPI_IOPORT_INDEX  0x01CE
#define VBE_DISPI_IOPORT_DATA   0x01CF

#define VBE_DISPI_INDEX_ID      0x0
#define VBE_DISPI_INDEX_XRES    0x1
#define VBE_DISPI_INDEX_YRES    0x2
#define VBE_DISPI_INDEX_BPP     0x3
#define VBE_DISPI_INDEX_ENABLE  0x4

#define VBE_DISPI_DISABLED      0x00
#define VBE_DISPI_ENABLED       0x01
#define VBE_DISPI_LFB_ENABLED   0x40

// PCI Configuration Space Ports
#define PCI_CONFIG_ADDRESS      0xCF8
#define PCI_CONFIG_DATA         0xCFC

static void bga_write(uint16_t index, uint16_t value) {
    outw(VBE_DISPI_IOPORT_INDEX, index);
    outw(VBE_DISPI_IOPORT_DATA, value);
}

static uint32_t pci_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (1 << 31) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

// Read the framebuffer address by scanning PCI Bus 0 for a VGA Display Controller
static uint32_t pci_read_bar0() {
    uint32_t fallback_bar0 = 0;

    for (uint8_t slot = 0; slot < 32; slot++) {
        uint32_t vendor = pci_read(0, slot, 0, 0);
        if ((vendor & 0xFFFF) == 0xFFFF) continue;

        uint32_t class_reg = pci_read(0, slot, 0, 0x08);
        uint8_t class_code = (class_reg >> 24) & 0xFF;
        uint8_t subclass = (class_reg >> 16) & 0xFF;

        if (class_code != 0x03) continue;

        uint32_t bar0 = pci_read(0, slot, 0, 0x10) & 0xFFFFFFF0;
        if (subclass == 0x00) {
            // Prefer the VGA display controller (standard VGA subclass).
            return bar0;
        }

        if (fallback_bar0 == 0) {
            // Keep first VGA-compatible controller as a fallback.
            fallback_bar0 = bar0;
        }
    }

    return fallback_bar0 ? fallback_bar0 : 0xFD000000;
}

uint32_t* volatile framebuffer = 0;
uint32_t* backbuffer = 0;
size_t screen_width = DESIRED_SCREEN_WIDTH;
size_t screen_height = DESIRED_SCREEN_HEIGHT;

// Debug: count pixels written into backbuffer
volatile uint32_t debug_draw_count = 0;

const uint32_t* icon_folder_pixels = icon_folder;
size_t icon_folder_width = 32;
size_t icon_folder_height = 32;

const uint32_t* icon_settings_pixels = icon_settings;
size_t icon_settings_width = 32;
size_t icon_settings_height = 32;

const uint32_t* icon_terminal_pixels = icon_terminal;
size_t icon_terminal_width = 32;
size_t icon_terminal_height = 32;

const uint32_t* icon_trash_pixels = icon_trash;
size_t icon_trash_width = 32;
size_t icon_trash_height = 32;

const uint32_t* icon_start_pixels = icon_start;
size_t icon_start_width = 32;
size_t icon_start_height = 32;

const uint32_t* icon_user_pixels = icon_user;
size_t icon_user_width = 32;
size_t icon_user_height = 32;

const uint32_t* icon_sleep_pixels = nullptr;
size_t icon_sleep_width = 64;
size_t icon_sleep_height = 64;

const uint32_t* icon_restart_pixels = nullptr;
size_t icon_restart_width = 64;
size_t icon_restart_height = 64;

const uint32_t* icon_shutdown_pixels = nullptr;
size_t icon_shutdown_width = 64;
size_t icon_shutdown_height = 64;

const uint32_t* icon_cursor_pixels = icon_cursor;
size_t icon_cursor_width = 24;
size_t icon_cursor_height = 24;

static inline uint32_t lerp_channel(uint32_t lo, uint32_t hi, uint32_t t) {
    return ((lo * (256 - t)) + (hi * t)) >> 8;
}

static inline uint32_t lerp_color(uint32_t c0, uint32_t c1, uint32_t t) {
    uint32_t r0 = (c0 >> 16) & 0xFF;
    uint32_t g0 = (c0 >> 8) & 0xFF;
    uint32_t b0 = c0 & 0xFF;

    uint32_t r1 = (c1 >> 16) & 0xFF;
    uint32_t g1 = (c1 >> 8) & 0xFF;
    uint32_t b1 = c1 & 0xFF;

    uint32_t r = lerp_channel(r0, r1, t);
    uint32_t g = lerp_channel(g0, g1, t);
    uint32_t b = lerp_channel(b0, b1, t);

    return (r << 16) | (g << 8) | b;
}

static void* krealloc_sized(void* ptr, size_t old_size, size_t new_size) {
    if (!ptr) return kmalloc(new_size);
    if (new_size == 0) {
        kfree(ptr);
        return nullptr;
    }

    void* new_ptr = kmalloc(new_size);
    if (!new_ptr) return nullptr;

    size_t copy_size = old_size < new_size ? old_size : new_size;
    memcpy(new_ptr, ptr, copy_size);
    kfree(ptr);
    return new_ptr;
}

uint32_t* load_png_image(const unsigned char* data, size_t size, size_t* width, size_t* height) {
    if (!data || size == 0 || !width || !height) return nullptr;

    int w = 0, h = 0, channels = 0;
    unsigned char* decoded = stbi_load_from_memory(data, (int)size, &w, &h, &channels, STBI_rgb_alpha);
    if (!decoded || w <= 0 || h <= 0) return nullptr;

    size_t pixel_count = (size_t)w * (size_t)h;
    uint32_t* pixels = (uint32_t*)kmalloc(pixel_count * sizeof(uint32_t));
    if (!pixels) {
        stbi_image_free(decoded);
        return nullptr;
    }

    for (size_t i = 0; i < pixel_count; i++) {
        uint8_t r = decoded[i * 4 + 0];
        uint8_t g = decoded[i * 4 + 1];
        uint8_t b = decoded[i * 4 + 2];
        uint8_t a = decoded[i * 4 + 3];
        pixels[i] = ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    }

    stbi_image_free(decoded);
    *width = (size_t)w;
    *height = (size_t)h;
    return pixels;
}

void draw_image(size_t x, size_t y, const uint32_t* pixels, size_t width, size_t height) {
    draw_argb_bitmap(x, y, pixels, width, height);
}

void draw_image_scaled(size_t x, size_t y, const uint32_t* pixels, size_t src_width, size_t src_height, size_t dst_width, size_t dst_height) {
    draw_argb_bitmap_scaled(x, y, pixels, src_width, src_height, dst_width, dst_height);
}

static void init_png_icon_resources() {
    size_t width, height;
    uint32_t* decoded;

    decoded = load_png_image(icon_folder_png, icon_folder_png_size, &width, &height);
    if (decoded) {
        icon_folder_pixels = decoded;
        icon_folder_width = width;
        icon_folder_height = height;
    }

    decoded = load_png_image(icon_settings_png, icon_settings_png_size, &width, &height);
    if (decoded) {
        icon_settings_pixels = decoded;
        icon_settings_width = width;
        icon_settings_height = height;
    }

    decoded = load_png_image(icon_terminal_png, icon_terminal_png_size, &width, &height);
    if (decoded) {
        icon_terminal_pixels = decoded;
        icon_terminal_width = width;
        icon_terminal_height = height;
    }

    decoded = load_png_image(icon_trash_png, icon_trash_png_size, &width, &height);
    if (decoded) {
        icon_trash_pixels = decoded;
        icon_trash_width = width;
        icon_trash_height = height;
    }

    decoded = load_png_image(icon_start_png, icon_start_png_size, &width, &height);
    if (decoded) {
        icon_start_pixels = decoded;
        icon_start_width = width;
        icon_start_height = height;
    }

    decoded = load_png_image(icon_user_png, icon_user_png_size, &width, &height);
    if (decoded) {
        icon_user_pixels = decoded;
        icon_user_width = width;
        icon_user_height = height;
    }

    decoded = load_png_image(icon_sleep_png, icon_sleep_png_size, &width, &height);
    if (decoded) {
        icon_sleep_pixels = decoded;
        icon_sleep_width = width;
        icon_sleep_height = height;
    }

    decoded = load_png_image(icon_restart_png, icon_restart_png_size, &width, &height);
    if (decoded) {
        icon_restart_pixels = decoded;
        icon_restart_width = width;
        icon_restart_height = height;
    }

    decoded = load_png_image(icon_shutdown_png, icon_shutdown_png_size, &width, &height);
    if (decoded) {
        icon_shutdown_pixels = decoded;
        icon_shutdown_width = width;
        icon_shutdown_height = height;
    }

    decoded = load_png_image(icon_cursor_png, icon_cursor_png_size, &width, &height);
    if (decoded) {
        icon_cursor_pixels = decoded;
        icon_cursor_width = width;
        icon_cursor_height = height;
    }
}

// Local debug serial helpers (duplicate, to avoid depending on kernel serial symbols)
static inline void dbg_serial_putc(char c) {
    while (!(inb(0x3F8 + 5) & 0x20)) { }
    outb(0x3F8 + 0, (uint8_t)c);
}
static void dbg_serial_write(const char* s) {
    while (*s) dbg_serial_putc(*s++);
}
static void dbg_serial_write_ln(const char* s) { dbg_serial_write(s); dbg_serial_putc('\r'); dbg_serial_putc('\n'); }
static void dbg_serial_write_hex(uint32_t v) {
    const char* hex = "0123456789ABCDEF";
    dbg_serial_putc('0'); dbg_serial_putc('x');
    for (int i = 7; i >= 0; --i) {
        uint8_t d = (v >> (i * 4)) & 0xF;
        dbg_serial_putc(hex[d]);
    }
}
static void dbg_serial_write_dec(uint32_t v) {
    char buf[12]; int i = 0;
    if (v == 0) { dbg_serial_putc('0'); return; }
    while (v > 0 && i < (int)sizeof(buf)-1) { buf[i++] = '0' + (v % 10); v /= 10; }
    for (int j = i - 1; j >= 0; --j) dbg_serial_putc(buf[j]);
}

static uint16_t bga_read(uint16_t index) {
    outw(VBE_DISPI_IOPORT_INDEX, index);
    return inw(VBE_DISPI_IOPORT_DATA);
}

void init_graphics() {
    // Step 1: Disable the BGA display while we configure it
    bga_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    
    // Step 2: Set resolution and color depth
    bga_write(VBE_DISPI_INDEX_XRES, screen_width);
    bga_write(VBE_DISPI_INDEX_YRES, screen_height);
    bga_write(VBE_DISPI_INDEX_BPP, 32);
    
    // Step 3: Enable BGA with Linear Frame Buffer
    bga_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);

    // Read back BGA registers to confirm
    dbg_serial_write_ln("[BGA] after enable:");
    dbg_serial_write("[BGA] ID: "); dbg_serial_write_hex((uint32_t)bga_read(VBE_DISPI_INDEX_ID)); dbg_serial_write_ln("");
    dbg_serial_write("[BGA] XRES: "); dbg_serial_write_dec((uint32_t)bga_read(VBE_DISPI_INDEX_XRES)); dbg_serial_write_ln("");
    dbg_serial_write("[BGA] YRES: "); dbg_serial_write_dec((uint32_t)bga_read(VBE_DISPI_INDEX_YRES)); dbg_serial_write_ln("");
    dbg_serial_write("[BGA] BPP: "); dbg_serial_write_dec((uint32_t)bga_read(VBE_DISPI_INDEX_BPP)); dbg_serial_write_ln("");
    dbg_serial_write("[BGA] ENABLE: "); dbg_serial_write_hex((uint32_t)bga_read(VBE_DISPI_INDEX_ENABLE)); dbg_serial_write_ln("");

    // Step 4: Read the framebuffer address from PCI (no paging, direct physical access)
    framebuffer = (uint32_t*)pci_read_bar0();
    dbg_serial_write("[PCI] chosen BAR0: "); dbg_serial_write_hex((uint32_t)framebuffer); dbg_serial_write_ln("");

    // Also dump PCI scan information (vendor/class) for slots 0..31
    for (uint8_t slot = 0; slot < 32; slot++) {
        uint32_t vend = pci_read(0, slot, 0, 0);
        if ((vend & 0xFFFF) == 0xFFFF) continue;
        uint16_t vendor_id = vend & 0xFFFF;
        uint16_t device_id = (vend >> 16) & 0xFFFF;
        uint32_t classreg = pci_read(0, slot, 0, 0x08);
        uint8_t class_code = (classreg >> 24) & 0xFF;
        uint8_t subclass = (classreg >> 16) & 0xFF;
        uint32_t bar0 = pci_read(0, slot, 0, 0x10) & 0xFFFFFFF0;
        dbg_serial_write("[PCI] slot "); dbg_serial_write_dec(slot); dbg_serial_write(" vendor=0x"); dbg_serial_write_hex(vendor_id); dbg_serial_write(" device=0x"); dbg_serial_write_hex(device_id); dbg_serial_write(" class=0x"); dbg_serial_write_hex(classreg); dbg_serial_write(" bar0=0x"); dbg_serial_write_hex(bar0); dbg_serial_write_ln("");
    }

    // Step 5: Allocate Double Buffer (Backbuffer)
    // We use kmalloc to get memory from the kernel heap.
    // screen_width * screen_height * 4
    extern void* kmalloc(size_t size);
    backbuffer = (uint32_t*)kmalloc(screen_width * screen_height * 4);
    dbg_serial_write("[GRAPHICS] backbuffer addr: "); dbg_serial_write_hex((uint32_t)backbuffer); dbg_serial_write_ln("");

    // Test writing/reading framebuffer MMIO if framebuffer is non-null
    if (framebuffer) {
        uint32_t old = framebuffer[0];
        framebuffer[0] = 0xAABBCCDD;
        uint32_t readback = framebuffer[0];
        dbg_serial_write("[MMIO] fb[0] old= "); dbg_serial_write_hex(old); dbg_serial_write(" new= "); dbg_serial_write_hex(readback); dbg_serial_write_ln("");
        // restore
        framebuffer[0] = old;
    }

    // Load embedded PNG icon resources and switch the icon pipeline to real image assets.
    init_png_icon_resources();
}

bool graphics_set_resolution(size_t width, size_t height) {
    if ((width != 1280 || height != 720) &&
        (width != 1920 || height != 1080)) return false;

    uint32_t* new_backbuffer = (uint32_t*)kmalloc(width * height * sizeof(uint32_t));
    if (!new_backbuffer) return false;

    size_t old_width = screen_width;
    size_t old_height = screen_height;
    uint32_t* old_backbuffer = backbuffer;

    bga_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    bga_write(VBE_DISPI_INDEX_XRES, (uint16_t)width);
    bga_write(VBE_DISPI_INDEX_YRES, (uint16_t)height);
    bga_write(VBE_DISPI_INDEX_BPP, 32);
    bga_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);

    if (bga_read(VBE_DISPI_INDEX_XRES) != width ||
        bga_read(VBE_DISPI_INDEX_YRES) != height ||
        bga_read(VBE_DISPI_INDEX_BPP) != 32) {
        bga_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
        bga_write(VBE_DISPI_INDEX_XRES, (uint16_t)old_width);
        bga_write(VBE_DISPI_INDEX_YRES, (uint16_t)old_height);
        bga_write(VBE_DISPI_INDEX_BPP, 32);
        bga_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);
        kfree(new_backbuffer);
        return false;
    }

    screen_width = width;
    screen_height = height;
    backbuffer = new_backbuffer;
    clear_screen(0);
    if (old_backbuffer) kfree(old_backbuffer);
    return true;
}

void swap_buffers() {
    if (!framebuffer || !backbuffer) return;
    // Copy backbuffer to framebuffer pixel by pixel (memcpy may not work with MMIO)
    size_t pixels = screen_width * screen_height;
    for (size_t i = 0; i < pixels; i++) {
        framebuffer[i] = backbuffer[i];
    }
}

void present_region(size_t x, size_t y, size_t width, size_t height) {
    if (!framebuffer || !backbuffer || x >= screen_width || y >= screen_height) return;

    if (width > screen_width - x) width = screen_width - x;
    if (height > screen_height - y) height = screen_height - y;

    for (size_t row = 0; row < height; ++row) {
        size_t offset = (y + row) * screen_width + x;
        for (size_t col = 0; col < width; ++col) {
            framebuffer[offset + col] = backbuffer[offset + col];
        }
    }
}

void draw_pixel(size_t x, size_t y, uint32_t color) {
    if (x >= screen_width || y >= screen_height) return;
    if (sys_config.high_contrast) {
        uint32_t red = (color >> 16) & 0xFF;
        uint32_t green = (color >> 8) & 0xFF;
        uint32_t blue = color & 0xFF;
        uint32_t luminance = (red * 299 + green * 587 + blue * 114) / 1000;
        color = luminance < 80 ? 0x000000 :
                luminance < 180 ? 0xFFFF00 : 0xFFFFFF;
    }
    if (backbuffer) {
        backbuffer[y * screen_width + x] = color;
        // Increment debug counter for pixels written
        debug_draw_count++;
    }
}

void draw_rect(size_t x, size_t y, size_t width, size_t height, uint32_t color) {
    for (size_t i = 0; i < height; i++) {
        for (size_t j = 0; j < width; j++) {
            draw_pixel(x + j, y + i, color);
        }
    }
}

void draw_bitmap(const uint32_t* pixels, size_t src_width, size_t src_height, size_t dst_x, size_t dst_y) {
    for (size_t y = 0; y < src_height; y++) {
        for (size_t x = 0; x < src_width; x++) {
            draw_pixel(dst_x + x, dst_y + y, pixels[y * src_width + x]);
        }
    }
}

void draw_bitmap_scaled(const uint32_t* pixels, size_t src_width, size_t src_height, size_t dst_x, size_t dst_y, size_t dst_width, size_t dst_height) {
    if (!pixels) return;

    if (src_width == dst_width && src_height == dst_height) {
        draw_bitmap(pixels, src_width, src_height, dst_x, dst_y);
        return;
    }

    bool upscaling = dst_width > src_width || dst_height > src_height;
    if (upscaling) {
        // Use nearest-neighbor for upscaling image wallpaper. This keeps details sharp
        // instead of softening them through bilinear blur.
        for (size_t j = 0; j < dst_height; j++) {
            size_t src_y = (j * src_height) / dst_height;
            if (src_y >= src_height) src_y = src_height - 1;
            for (size_t i = 0; i < dst_width; i++) {
                size_t src_x = (i * src_width) / dst_width;
                if (src_x >= src_width) src_x = src_width - 1;
                draw_pixel(dst_x + i, dst_y + j, pixels[src_y * src_width + src_x]);
            }
        }
        return;
    }

    size_t max_x = src_width - 1;
    size_t max_y = src_height - 1;
    size_t x_step = dst_width > 1 ? ((max_x << 8) / (dst_width - 1)) : 0;
    size_t y_step = dst_height > 1 ? ((max_y << 8) / (dst_height - 1)) : 0;

    for (size_t j = 0; j < dst_height; j++) {
        size_t src_y_fp = j * y_step;
        size_t y0 = src_y_fp >> 8;
        size_t y1 = y0 < max_y ? y0 + 1 : y0;
        uint32_t fy = src_y_fp & 0xFF;

        for (size_t i = 0; i < dst_width; i++) {
            size_t src_x_fp = i * x_step;
            size_t x0 = src_x_fp >> 8;
            size_t x1 = x0 < max_x ? x0 + 1 : x0;
            uint32_t fx = src_x_fp & 0xFF;

            uint32_t c00 = pixels[y0 * src_width + x0];
            uint32_t c10 = pixels[y0 * src_width + x1];
            uint32_t c01 = pixels[y1 * src_width + x0];
            uint32_t c11 = pixels[y1 * src_width + x1];

            uint32_t top = lerp_color(c00, c10, fx);
            uint32_t bottom = lerp_color(c01, c11, fx);
            uint32_t final = lerp_color(top, bottom, fy);

            draw_pixel(dst_x + i, dst_y + j, final);
        }
    }
}

void draw_bitmap_cover(const uint32_t* pixels, size_t src_width, size_t src_height,
                       size_t dst_width, size_t dst_height) {
    if (!pixels || src_width == 0 || src_height == 0 ||
        dst_width == 0 || dst_height == 0) return;

    size_t crop_x = 0;
    size_t crop_y = 0;
    size_t visible_width = src_width;
    size_t visible_height = src_height;

    if (src_width * dst_height > dst_width * src_height) {
        visible_width = (src_height * dst_width) / dst_height;
        crop_x = (src_width - visible_width) / 2;
    } else {
        visible_height = (src_width * dst_height) / dst_width;
        crop_y = (src_height - visible_height) / 2;
    }

    for (size_t y = 0; y < dst_height; ++y) {
        size_t src_y_fp = (crop_y << 8);
        if (dst_height > 1 && visible_height > 1) {
            src_y_fp += (y * (visible_height - 1) << 8) / (dst_height - 1);
        }
        size_t y0 = src_y_fp >> 8;
        size_t y1 = y0 + 1 < src_height ? y0 + 1 : y0;
        uint32_t fy = src_y_fp & 0xFF;

        for (size_t x = 0; x < dst_width; ++x) {
            size_t src_x_fp = (crop_x << 8);
            if (dst_width > 1 && visible_width > 1) {
                src_x_fp += (x * (visible_width - 1) << 8) / (dst_width - 1);
            }
            size_t x0 = src_x_fp >> 8;
            size_t x1 = x0 + 1 < src_width ? x0 + 1 : x0;
            uint32_t fx = src_x_fp & 0xFF;

            uint32_t top = lerp_color(pixels[y0 * src_width + x0],
                                      pixels[y0 * src_width + x1], fx);
            uint32_t bottom = lerp_color(pixels[y1 * src_width + x0],
                                         pixels[y1 * src_width + x1], fx);
            draw_pixel(x, y, lerp_color(top, bottom, fy));
        }
    }
}

static inline uint32_t lerp_fixed(uint32_t v0, uint32_t v1, uint32_t t) {
    return ((v0 * (255 - t)) + (v1 * t)) >> 8;
}

void draw_argb_bitmap_scaled(size_t x, size_t y, const uint32_t* pixels, size_t src_width, size_t src_height, size_t dst_width, size_t dst_height) {
    if (!pixels) return;
    if (src_width == dst_width && src_height == dst_height) {
        draw_argb_bitmap(x, y, pixels, src_width, src_height);
        return;
    }

    bool upscaling = dst_width > src_width || dst_height > src_height;
    if (upscaling) {
        for (size_t j = 0; j < dst_height; j++) {
            size_t src_y = (j * src_height) / dst_height;
            if (src_y >= src_height) src_y = src_height - 1;
            for (size_t i = 0; i < dst_width; i++) {
                size_t src_x = (i * src_width) / dst_width;
                if (src_x >= src_width) src_x = src_width - 1;
                uint32_t px = pixels[src_y * src_width + src_x];
                uint8_t alpha = (px >> 24) & 0xFF;
                if (alpha == 0) continue;
                uint32_t fg = px & 0xFFFFFF;
                uint32_t bg = get_pixel(x + i, y + j);
                draw_pixel(x + i, y + j, blend(fg, bg, alpha));
            }
        }
        return;
    }

    size_t max_x = src_width - 1;
    size_t max_y = src_height - 1;
    size_t x_step = dst_width > 1 ? ((max_x << 16) / (dst_width - 1)) : 0;
    size_t y_step = dst_height > 1 ? ((max_y << 16) / (dst_height - 1)) : 0;

    for (size_t j = 0; j < dst_height; j++) {
        size_t src_y_fp = j * y_step;
        size_t y0 = src_y_fp >> 16;
        size_t y1 = y0 < max_y ? y0 + 1 : y0;
        uint32_t fy = (src_y_fp >> 8) & 0xFF;

        for (size_t i = 0; i < dst_width; i++) {
            size_t src_x_fp = i * x_step;
            size_t x0 = src_x_fp >> 16;
            size_t x1 = x0 < max_x ? x0 + 1 : x0;
            uint32_t fx = (src_x_fp >> 8) & 0xFF;

            uint32_t p00 = pixels[y0 * src_width + x0];
            uint32_t p10 = pixels[y0 * src_width + x1];
            uint32_t p01 = pixels[y1 * src_width + x0];
            uint32_t p11 = pixels[y1 * src_width + x1];

            uint32_t a00 = (p00 >> 24) & 0xFF;
            uint32_t a10 = (p10 >> 24) & 0xFF;
            uint32_t a01 = (p01 >> 24) & 0xFF;
            uint32_t a11 = (p11 >> 24) & 0xFF;

            uint32_t r00 = ((p00 >> 16) & 0xFF) * a00;
            uint32_t g00 = ((p00 >> 8) & 0xFF) * a00;
            uint32_t b00 = (p00 & 0xFF) * a00;

            uint32_t r10 = ((p10 >> 16) & 0xFF) * a10;
            uint32_t g10 = ((p10 >> 8) & 0xFF) * a10;
            uint32_t b10 = (p10 & 0xFF) * a10;

            uint32_t r01 = ((p01 >> 16) & 0xFF) * a01;
            uint32_t g01 = ((p01 >> 8) & 0xFF) * a01;
            uint32_t b01 = (p01 & 0xFF) * a01;

            uint32_t r11 = ((p11 >> 16) & 0xFF) * a11;
            uint32_t g11 = ((p11 >> 8) & 0xFF) * a11;
            uint32_t b11 = (p11 & 0xFF) * a11;

            uint32_t top_a = lerp_fixed(a00, a10, fx);
            uint32_t bottom_a = lerp_fixed(a01, a11, fx);
            uint32_t final_a = lerp_fixed(top_a, bottom_a, fy);

            uint32_t top_r = lerp_fixed(r00, r10, fx);
            uint32_t top_g = lerp_fixed(g00, g10, fx);
            uint32_t top_b = lerp_fixed(b00, b10, fx);

            uint32_t bottom_r = lerp_fixed(r01, r11, fx);
            uint32_t bottom_g = lerp_fixed(g01, g11, fx);
            uint32_t bottom_b = lerp_fixed(b01, b11, fx);

            uint32_t final_r = lerp_fixed(top_r, bottom_r, fy);
            uint32_t final_g = lerp_fixed(top_g, bottom_g, fy);
            uint32_t final_b = lerp_fixed(top_b, bottom_b, fy);

            if (final_a == 0) continue;

            final_r = (final_r + (final_a >> 1)) / final_a;
            final_g = (final_g + (final_a >> 1)) / final_a;
            final_b = (final_b + (final_a >> 1)) / final_a;

            uint32_t fg = (final_r << 16) | (final_g << 8) | final_b;
            uint32_t bg = get_pixel(x + i, y + j);
            draw_pixel(x + i, y + j, blend(fg, bg, final_a));
        }
    }
}

void draw_argb_bitmap(size_t x, size_t y, const uint32_t* pixels, size_t width, size_t height) {
    if (!pixels) return;
    for (size_t row = 0; row < height; row++) {
        for (size_t col = 0; col < width; col++) {
            uint32_t px = pixels[row * width + col];
            uint8_t alpha = (px >> 24) & 0xFF;
            if (alpha == 0) continue;
            uint32_t fg = px & 0xFFFFFF;
            uint32_t bg = get_pixel(x + col, y + row);
            draw_pixel(x + col, y + row, blend(fg, bg, alpha));
        }
    }
}

void clear_screen(uint32_t color) {
    if (!backbuffer) return;
    size_t count = screen_width * screen_height;
    for (size_t i = 0; i < count; i++) {
        backbuffer[i] = color;
    }
}

static const uint8_t font8x8_basic[128][8] = {
    // space..~ only, full table omitted for brevity; define basic alphanumerics
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 0x20 space
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00}, // !
    {0x36,0x36,0x24,0x00,0x00,0x00,0x00,0x00}, // "
    {0x36,0x36,0x7F,0x36,0x7F,0x36,0x36,0x00}, // #
    {0x0C,0x3E,0x03,0x1E,0x30,0x1F,0x0C,0x00}, // $
    {0x00,0x63,0x33,0x18,0x0C,0x66,0x63,0x00}, // %
    {0x1C,0x36,0x1C,0x6E,0x3B,0x33,0x6E,0x00}, // &
    {0x06,0x06,0x03,0x00,0x00,0x00,0x00,0x00}, // '
    {0x18,0x0C,0x06,0x06,0x06,0x0C,0x18,0x00}, // (
    {0x06,0x0C,0x18,0x18,0x18,0x0C,0x06,0x00}, // )
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, // *
    {0x00,0x0C,0x0C,0x3F,0x0C,0x0C,0x00,0x00}, // +
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x06}, // ,
    {0x00,0x00,0x00,0x3F,0x00,0x00,0x00,0x00}, // -
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x00}, // .
    {0x60,0x30,0x18,0x0C,0x06,0x03,0x01,0x00}, // /
    {0x3E,0x63,0x73,0x7B,0x6F,0x67,0x3E,0x00}, // 0
    {0x0C,0x1C,0x0C,0x0C,0x0C,0x0C,0x3F,0x00}, // 1
    {0x3E,0x63,0x30,0x1C,0x06,0x63,0x7F,0x00}, // 2
    {0x3E,0x63,0x30,0x1C,0x30,0x63,0x3E,0x00}, // 3
    {0x30,0x38,0x34,0x32,0x7F,0x30,0x30,0x00}, // 4
    {0x7F,0x03,0x3F,0x60,0x60,0x63,0x3E,0x00}, // 5
    {0x1C,0x06,0x03,0x3F,0x63,0x63,0x3E,0x00}, // 6
    {0x7F,0x63,0x30,0x18,0x0C,0x0C,0x0C,0x00}, // 7
    {0x3E,0x63,0x63,0x3E,0x63,0x63,0x3E,0x00}, // 8
    {0x3E,0x63,0x63,0x3E,0x30,0x18,0x0E,0x00}, // 9
    {0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x00}, // :
    {0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x06}, // ;
    {0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00}, // <
    {0x00,0x00,0x3F,0x00,0x00,0x3F,0x00,0x00}, // =
    {0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00}, // >
    {0x3E,0x63,0x30,0x18,0x0C,0x00,0x0C,0x00}, // ?
    {0x3E,0x63,0x6F,0x6F,0x6F,0x03,0x3E,0x00}, // @
    {0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00}, // A
    {0x3F,0x66,0x66,0x3E,0x66,0x66,0x3F,0x00}, // B
    {0x3C,0x66,0x03,0x03,0x03,0x66,0x3C,0x00}, // C
    {0x1F,0x36,0x66,0x66,0x66,0x36,0x1F,0x00}, // D
    {0x7F,0x03,0x03,0x3E,0x03,0x03,0x7F,0x00}, // E
    {0x7F,0x03,0x03,0x3E,0x03,0x03,0x03,0x00}, // F
    {0x3C,0x66,0x03,0x3B,0x63,0x66,0x3C,0x00}, // G
    {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00}, // H
    {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, // I
    {0x1E,0x0C,0x0C,0x0C,0x0C,0x6C,0x38,0x00}, // J
    {0x67,0x66,0x6C,0x78,0x6C,0x66,0x67,0x00}, // K
    {0x03,0x03,0x03,0x03,0x03,0x03,0x7F,0x00}, // L
    {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00}, // M
    {0x63,0x67,0x6F,0x7B,0x73,0x63,0x63,0x00}, // N
    {0x3E,0x63,0x63,0x63,0x63,0x63,0x3E,0x00}, // O
    {0x3F,0x66,0x66,0x3F,0x03,0x03,0x03,0x00}, // P
    {0x3E,0x63,0x63,0x63,0x6B,0x67,0x3D,0x00}, // Q
    {0x3F,0x66,0x66,0x3F,0x6C,0x66,0x67,0x00}, // R
    {0x3E,0x63,0x07,0x1E,0x38,0x63,0x3E,0x00}, // S
    {0x7F,0x5A,0x18,0x18,0x18,0x18,0x3C,0x00}, // T
    {0x63,0x63,0x63,0x63,0x63,0x63,0x3E,0x00}, // U
    {0x63,0x63,0x63,0x63,0x63,0x36,0x1C,0x00}, // V
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00}, // W
    {0x63,0x63,0x36,0x1C,0x1C,0x36,0x63,0x00}, // X
    {0x66,0x66,0x66,0x3C,0x18,0x18,0x3C,0x00}, // Y
    {0x7F,0x63,0x31,0x18,0x0C,0x46,0x7F,0x00}, // Z
};

static const uint8_t* get_char_bitmap(char c) {
    if (c >= 'a' && c <= 'z') {
        c -= 32;
    }
    if (c < 0x20 || c > 0x5A) return font8x8_basic[0];
    return font8x8_basic[c - 0x20];
}

void draw_filled_circle(int x0, int y0, int radius, uint32_t color) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x * x + y * y <= radius * radius) {
                draw_pixel(x0 + x, y0 + y, color);
            }
        }
    }
}

void draw_rounded_rect(int x, int y, int width, int height, int radius, uint32_t color) {
    if (radius > width / 2) radius = width / 2;
    if (radius > height / 2) radius = height / 2;

    // Draw main center block
    draw_rect(x, y + radius, width, height - 2 * radius, color);
    
    // Draw top and bottom blocks
    draw_rect(x + radius, y, width - 2 * radius, radius, color);
    draw_rect(x + radius, y + height - radius, width - 2 * radius, radius, color);

    // Draw the 4 corners
    draw_filled_circle(x + radius, y + radius, radius, color);
    draw_filled_circle(x + width - radius - 1, y + radius, radius, color);
    draw_filled_circle(x + radius, y + height - radius - 1, radius, color);
    draw_filled_circle(x + width - radius - 1, y + height - radius - 1, radius, color);
}

void draw_char(char c, size_t x, size_t y, uint32_t color) {
    const uint8_t* bitmap = get_char_bitmap(c);
    for (size_t row = 0; row < 8; row++) {
        uint8_t bits = bitmap[row];
        for (size_t col = 0; col < 8; col++) {
            if (bits & (1 << col)) {
                draw_pixel(x + col, y + row, color);
            }
        }
    }
}

void draw_string(const char* text, size_t x, size_t y, uint32_t color) {
    size_t pos = 0;
    while (text[pos]) {
        draw_char(text[pos], x + pos * 8, y, color);
        pos++;
    }
}

// ====================================================================
// Mouse cursor rendering with alpha-blended icon data
// ====================================================================

void draw_cursor(size_t x, size_t y, size_t scale) {
    if (scale == 0) return;
    // Unique JenOS vector cursor (geometric arrow)
    // Dark outline, white fill, blue accent
    uint32_t outline = 0x11111B;
    uint32_t fill = 0xFFFFFF;
    uint32_t accent = settings_get_accent_color();

    // Draw the cursor manually (16x22)
    // 0 = transparent, 1 = outline, 2 = fill, 3 = accent
    static const uint8_t shape[22][16] = {
        {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,2,2,1,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,2,2,2,1,0,0,0,0,0,0,0,0,0,0,0},
        {1,2,2,2,2,1,0,0,0,0,0,0,0,0,0,0},
        {1,2,2,2,2,2,1,0,0,0,0,0,0,0,0,0},
        {1,2,2,2,2,2,2,1,0,0,0,0,0,0,0,0},
        {1,2,2,2,2,2,2,2,1,0,0,0,0,0,0,0},
        {1,2,2,2,2,2,2,2,2,1,0,0,0,0,0,0},
        {1,2,2,2,2,2,2,2,2,2,1,0,0,0,0,0},
        {1,2,2,2,2,2,2,2,2,2,2,1,0,0,0,0},
        {1,2,2,2,2,2,2,2,2,2,2,2,1,0,0,0},
        {1,2,2,2,2,2,2,1,1,1,1,1,1,1,0,0},
        {1,2,2,2,1,2,2,1,0,0,0,0,0,0,0,0},
        {1,2,2,1,0,1,2,2,1,0,0,0,0,0,0,0},
        {1,2,1,0,0,1,2,2,1,0,0,0,0,0,0,0},
        {1,1,0,0,0,0,1,2,2,1,0,0,0,0,0,0},
        {0,0,0,0,0,0,1,2,2,1,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,1,3,3,1,0,0,0,0,0},
        {0,0,0,0,0,0,0,1,3,3,1,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    };

    for (size_t row = 0; row < 22; row++) {
        for (size_t col = 0; col < 16; col++) {
            uint8_t type = shape[row][col];
            uint32_t color = type == 1 ? outline : type == 2 ? fill :
                             type == 3 ? accent : 0;
            if (type == 0) continue;
            for (size_t dy = 0; dy < scale; ++dy) {
                for (size_t dx = 0; dx < scale; ++dx) {
                    draw_pixel(x + col * scale + dx, y + row * scale + dy, color);
                }
            }
        }
    }
}
