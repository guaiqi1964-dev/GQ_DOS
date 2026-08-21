#include "idt.h"
#include <stdint.h>

struct idt_entry {
    uint16_t offset_1;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_2;
    uint32_t offset_3;
    uint32_t zero;
} __attribute__((packed));

struct idtr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry g_idt[256];
static struct idtr g_idtr;

/* NASM 桩函数表 */
extern uint64_t isr_stub_table[];
extern uint64_t irq_stub_table[];
extern void apic_timer_stub(void);

static void set_gate(int n, uint64_t handler) {
    g_idt[n].offset_1 = (uint16_t)(handler & 0xFFFF);
    g_idt[n].selector = 0x28;   /* Limine 提供的 64 位内核代码段 */
    g_idt[n].ist = 0;
    g_idt[n].type_attr = 0x8E;  /* present, DPL0, 32 位中断门类型(0xE) */
    g_idt[n].offset_2 = (uint16_t)((handler >> 16) & 0xFFFF);
    g_idt[n].offset_3 = (uint32_t)(handler >> 32);
    g_idt[n].zero = 0;
}

void idt_init(void) {
    for (int i = 0; i < 32; i++) {
        set_gate(i, isr_stub_table[i]);
    }
    for (int i = 0; i < 16; i++) {
        set_gate(0x20 + i, irq_stub_table[i]);
    }
    /* APIC 定时器向量 0x30 */
    set_gate(0x30, (uint64_t)apic_timer_stub);
    g_idtr.limit = (uint16_t)(sizeof(g_idt) - 1);
    g_idtr.base = (uint64_t)&g_idt;
    __asm__ volatile("lidt %0" : : "m"(g_idtr));
}
