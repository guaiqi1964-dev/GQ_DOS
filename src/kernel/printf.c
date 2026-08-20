#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "printf.h"
#include "console.h"

static void print_unsigned(uint64_t v, uint32_t base, bool upper) {
    char buf[32];
    int i = 0;
    if (v == 0) {
        console_putc('0');
        return;
    }
    while (v > 0) {
        uint32_t d = (uint32_t)(v % base);
        buf[i++] = (d < 10) ? (char)('0' + d) : (char)((upper ? 'A' : 'a') + d - 10);
        v /= base;
    }
    while (i > 0) {
        console_putc(buf[--i]);
    }
}

static void print_signed(int64_t v) {
    if (v < 0) {
        console_putc('-');
        v = -v;
    }
    print_unsigned((uint64_t)v, 10, false);
}

int printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    for (const char *p = fmt; *p; p++) {
        if (*p != '%') {
            console_putc(*p);
            continue;
        }
        p++;
        int is_long = 0;
        while (*p == 'l') { is_long++; p++; }

        switch (*p) {
        case '%':
            console_putc('%');
            break;
        case 'c':
            console_putc((char)va_arg(ap, int));
            break;
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (s == NULL) s = "(null)";
            while (*s) console_putc(*s++);
            break;
        }
        case 'd':
        case 'i':
            if (is_long >= 1) print_signed((int64_t)va_arg(ap, long));
            else print_signed((int64_t)va_arg(ap, int));
            break;
        case 'u':
            if (is_long >= 1) print_unsigned((uint64_t)va_arg(ap, unsigned long), 10, false);
            else print_unsigned((uint64_t)va_arg(ap, unsigned int), 10, false);
            break;
        case 'x':
        case 'X': {
            bool upper = (*p == 'X');
            if (is_long >= 1) print_unsigned((uint64_t)va_arg(ap, unsigned long), 16, upper);
            else print_unsigned((uint64_t)va_arg(ap, unsigned int), 16, upper);
            break;
        }
        case 'p':
            console_puts("0x");
            print_unsigned((uint64_t)va_arg(ap, void *), 16, false);
            break;
        default:
            console_putc('%');
            console_putc(*p);
            break;
        }
    }

    va_end(ap);
    return 0;
}
