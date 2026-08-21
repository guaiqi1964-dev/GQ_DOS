#include "serial.h"
#include "io.h"
#include <stdint.h>

#define COM1 0x3F8

void serial_init(void) {
    outb(COM1 + 1, 0x00);   /* 禁用中断 */
    outb(COM1 + 3, 0x80);   /* 使能 DLAB */
    outb(COM1 + 0, 0x03);   /* 波特率除数低字节 (38400) */
    outb(COM1 + 1, 0x00);   /* 除数高字节 */
    outb(COM1 + 3, 0x03);   /* 8N1 */
    outb(COM1 + 2, 0xC7);   /* 使能 FIFO */
    outb(COM1 + 4, 0x0B);   /* IRQ 使能, RTS/DSR */
}

static int tx_ready(void) {
    return inb(COM1 + 5) & 0x20;
}

void serial_putc(char c) {
    while (!tx_ready()) { }
    outb(COM1, (uint8_t)c);
}

void serial_puts(const char *s) {
    while (*s) {
        serial_putc(*s++);
    }
}

void serial_print_num(uint64_t n) {
    char buf[32];
    int i = 0;
    if (n == 0) {
        serial_putc('0');
        return;
    }
    while (n > 0) {
        buf[i++] = (char)('0' + (n % 10));
        n /= 10;
    }
    while (i > 0) {
        serial_putc(buf[--i]);
    }
}
