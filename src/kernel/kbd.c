#include "kbd.h"
#include "io.h"

#define BUF_SIZE 256

static char g_buf[BUF_SIZE];
static volatile int g_head = 0;
static volatile int g_tail = 0;
static int g_shift = 0;

/* 扫描码集1 小写映射 */
static const char lower_map[128] = {
    [0x02]='1',[0x03]='2',[0x04]='3',[0x05]='4',[0x06]='5',[0x07]='6',
    [0x08]='7',[0x09]='8',[0x0A]='9',[0x0B]='0',
    [0x0C]='-',[0x0D]='=',[0x0E]=0x08,  /* 退格 */
    [0x0F]=0x09,                        /* 制表 */
    [0x10]='q',[0x11]='w',[0x12]='e',[0x13]='r',[0x14]='t',[0x15]='y',
    [0x16]='u',[0x17]='i',[0x18]='o',[0x19]='p',
    [0x1A]='[',[0x1B]=']',[0x1C]=0x0A,  /* 回车 */
    [0x1E]='a',[0x1F]='s',[0x20]='d',[0x21]='f',[0x22]='g',[0x23]='h',
    [0x24]='j',[0x25]='k',[0x26]='l',
    [0x27]=';',[0x28]=0x27,             /* 单引号 */
    [0x29]=0x60,                        /* 反引号 */
    [0x2B]=0x5C,                        /* 反斜杠 */
    [0x2C]='z',[0x2D]='x',[0x2E]='c',[0x2F]='v',[0x30]='b',[0x31]='n',
    [0x32]='m',[0x33]=',',[0x34]='.',[0x35]='/',
    [0x39]=' ',
};

/* 扫描码集1 大写(Shift)映射 */
static const char upper_map[128] = {
    [0x02]='!',[0x03]='@',[0x04]='#',[0x05]='$',[0x06]='%',[0x07]='^',
    [0x08]='&',[0x09]='*',[0x0A]='(',[0x0B]=')',
    [0x0C]='_',[0x0D]='+',
    [0x1A]='{',[0x1B]='}',
    [0x1E]='A',[0x1F]='S',[0x20]='D',[0x21]='F',[0x22]='G',[0x23]='H',
    [0x24]='J',[0x25]='K',[0x26]='L',
    [0x27]=':',[0x28]=0x22,             /* 双引号 */
    [0x29]='~',
    [0x2B]='|',
    [0x2C]='Z',[0x2D]='X',[0x2E]='C',[0x2F]='V',[0x30]='B',[0x31]='N',
    [0x32]='M',[0x33]='<',[0x34]='>',[0x35]='?',
    [0x39]=' ',
};

void kbd_init(void) {
    g_head = 0;
    g_tail = 0;
    g_shift = 0;
}

void kbd_handle_scancode(uint8_t sc) {
    if (sc == 0x2A || sc == 0x36) {   /* Shift 按下 */
        g_shift = 1;
        return;
    }
    if (sc == 0xAA || sc == 0xB6) {   /* Shift 释放 */
        g_shift = 0;
        return;
    }
    if (sc & 0x80) return;            /* 其它键释放，忽略 */

    char c = g_shift ? upper_map[sc] : lower_map[sc];
    if (c != 0) {
        int next = (g_tail + 1) % BUF_SIZE;
        if (next != g_head) {
            g_buf[g_tail] = c;
            g_tail = next;
        }
    }
}

int kbd_has_char(void) {
    return g_head != g_tail;
}

char kbd_getc(void) {
    while (g_head == g_tail) {
        __asm__ volatile("hlt");
        kbd_poll();
    }
    char c = g_buf[g_head];
    g_head = (g_head + 1) % BUF_SIZE;
    return c;
}

/* 轮询 PS/2 键盘 (0x64 状态, 0x60 数据) */
void kbd_poll(void) {
    uint8_t status = inb(0x64);
    if (status & 0x01) {
        uint8_t sc = inb(0x60);
        kbd_handle_scancode(sc);
    }
}
