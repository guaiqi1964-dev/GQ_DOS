#ifndef GQ_KBD_H
#define GQ_KBD_H

#include <stdint.h>

void kbd_init(void);
void kbd_handle_scancode(uint8_t scancode);
void kbd_poll(void);
int kbd_has_char(void);
char kbd_getc(void);

#endif
