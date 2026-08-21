#include "pit.h"
#include "io.h"

void pit_init(uint32_t freq) {
    uint32_t divisor = 1193182 / freq;
    outb(0x43, 0x36);  /* 通道0, 低/高字节, 模式3(方波) */
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)(divisor >> 8));
}
