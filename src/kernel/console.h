#ifndef GQ_CONSOLE_H
#define GQ_CONSOLE_H

#include <stdint.h>
#include <limine.h>

void console_init(struct limine_framebuffer *fb);
void console_clear(uint32_t color);
void console_putc(char c);
void console_puts(const char *s);

#endif
