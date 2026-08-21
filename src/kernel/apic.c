#include "apic.h"
#include "io.h"

#define IA32_APIC_BASE   0x1B
#define APIC_SPURIOUS    0x80F
#define APIC_LVT_TIMER   0x832
#define APIC_TIMER_DIV   0x83E
#define APIC_TIMER_INIT  0x838
#define APIC_EOI_REG     0x80B

#define APIC_TIMER_VECTOR 0x30

void apic_init(void) {
    /* 启用 x2APIC + APIC */
    uint64_t base = rdmsr(IA32_APIC_BASE);
    base |= (1ull << 10) | (1ull << 11);
    wrmsr(IA32_APIC_BASE, base);

    /* 伪中断向量 0xFF + APIC 使能 */
    wrmsr(APIC_SPURIOUS, 0x1FF);

    /* LVT 定时器：向量 0x30，周期模式 */
    wrmsr(APIC_LVT_TIMER, APIC_TIMER_VECTOR | (1ull << 17));

    /* 分频 /16，初始计数约 100Hz */
    wrmsr(APIC_TIMER_DIV, 0b0011);
    wrmsr(APIC_TIMER_INIT, 0x4000);
}

void apic_eoi(void) {
    wrmsr(APIC_EOI_REG, 0);
}
