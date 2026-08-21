#include "isr.h"
#include "printf.h"
#include "io.h"
#include "pic.h"
#include "kbd.h"
#include "apic.h"

volatile uint64_t g_ticks = 0;

static void hcf(void) {
    for (;;) {
        __asm__ volatile("hlt");
    }
}

void exception_handler(registers_t *r) {
    printf("\nEXCEPTION vector=%lu err=%lu rip=0x%lx\n", r->int_no, r->err_code, r->rip);
    printf("System halted.\n");
    hcf();
}

void irq_handler(registers_t *r) {
    if (r->int_no == 0x30) {
        /* LAPIC 定时器 */
        g_ticks++;
        apic_eoi();
        return;
    }
    uint64_t irq = r->int_no - 0x20;
    if (irq == 0) {
        g_ticks++;
    } else if (irq == 1) {
        uint8_t sc = inb(0x60);
        kbd_handle_scancode(sc);
    }
    pic_send_eoi((uint8_t)irq);
}
