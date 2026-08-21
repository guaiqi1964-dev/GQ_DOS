#ifndef GQ_PIC_H
#define GQ_PIC_H

#include <stdint.h>

void pic_remap(void);
void pic_send_eoi(uint8_t irq);

#endif
