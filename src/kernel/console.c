#include "console.h"
#include "font8x8_basic.h"

/* 假设 32bpp 帧缓冲 */
static struct limine_framebuffer *g_fb;
static uint32_t *g_pixels;
static uint32_t g_pitch_px;
static uint32_t g_width, g_height;
static uint32_t g_cx, g_cy;
static uint32_t g_fg, g_bg;
static uint32_t g_r_shift, g_g_shift, g_b_shift;

#define GLYPH_W 8
#define GLYPH_H 8

static uint32_t make_color(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << g_r_shift)
         | ((uint32_t)g << g_g_shift)
         | ((uint32_t)b << g_b_shift);
}

int console_init(struct limine_framebuffer *fb) {
    /* 格式校验：代码以 uint32_t 和 pitch/4 访问显存，仅支持 32bpp */
    if (fb->bpp != 32) return -1;

    g_fb = fb;
    g_width = (uint32_t)fb->width;
    g_height = (uint32_t)fb->height;
    g_pitch_px = (uint32_t)(fb->pitch / 4);
    g_pixels = (uint32_t *)fb->address;
    g_r_shift = fb->red_mask_shift;
    g_g_shift = fb->green_mask_shift;
    g_b_shift = fb->blue_mask_shift;
    g_fg = make_color(0xE0, 0xE0, 0xE0);
    g_bg = make_color(0x00, 0x00, 0x00);
    g_cx = 0;
    g_cy = 0;
    console_clear(g_bg);
    return 0;
}

void console_clear(uint32_t color) {
    uint32_t total = g_height * g_pitch_px;
    for (uint32_t i = 0; i < total; i++) {
        g_pixels[i] = color;
    }
    g_cx = 0;
    g_cy = 0;
}

static void scroll_up(void) {
    uint32_t shift = GLYPH_H * g_pitch_px;
    uint32_t total = g_height * g_pitch_px;
    for (uint32_t i = 0; i < total - shift; i++) {
        g_pixels[i] = g_pixels[i + shift];
    }
    for (uint32_t i = total - shift; i < total; i++) {
        g_pixels[i] = g_bg;
    }
    g_cy--;
}

static void draw_glyph(char c) {
    uint32_t px = g_cx * GLYPH_W;
    uint32_t py = g_cy * GLYPH_H;
    const char *glyph = font8x8_basic[(uint8_t)c];
    for (uint32_t y = 0; y < GLYPH_H; y++) {
        uint8_t row = (uint8_t)glyph[y];
        for (uint32_t x = 0; x < GLYPH_W; x++) {
            uint32_t color = (row & (1u << x)) ? g_fg : g_bg;
            g_pixels[(py + y) * g_pitch_px + (px + x)] = color;
        }
    }
}

void console_putc(char c) {
    if (c == '\n') {
        g_cx = 0;
        g_cy++;
    } else if (c == '\r') {
        g_cx = 0;
    } else if (c == '\t') {
        g_cx = (g_cx + 8) & ~7u;
    } else if (c == '\b') {
        if (g_cx > 0) {
            g_cx--;
            draw_glyph(' ');
        }
    } else if ((uint8_t)c >= 0x20 && (uint8_t)c < 0x7f) {
        draw_glyph(c);
        g_cx++;
    }

    uint32_t cols = g_width / GLYPH_W;
    uint32_t rows = g_height / GLYPH_H;
    if (g_cx >= cols) {
        g_cx = 0;
        g_cy++;
    }
    if (g_cy >= rows) {
        scroll_up();
    }
}

void console_puts(const char *s) {
    while (*s) {
        console_putc(*s++);
    }
}
