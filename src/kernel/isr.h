#ifndef GQ_ISR_H
#define GQ_ISR_H

#include <stdint.h>

typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rflags, cs, rip;
} registers_t;

extern volatile uint64_t g_ticks;

void exception_handler(registers_t *r);
void irq_handler(registers_t *r);

#endif
