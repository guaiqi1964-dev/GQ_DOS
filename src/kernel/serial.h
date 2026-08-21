#ifndef GQ_SERIAL_H
#define GQ_SERIAL_H

#include <stdint.h>

void serial_init(void);
void serial_putc(char c);
void serial_puts(const char *s);
void serial_print_num(uint64_t n);

#endif
