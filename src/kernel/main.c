#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "console.h"
#include "printf.h"
#include "serial.h"
#include "idt.h"
#include "apic.h"
#include "kbd.h"
#include "isr.h"
#include "ata.h"
#include "fat32.h"
#include "string.h"

__attribute__((used, section(".limine_requests")))
static volatile uint64_t base_rev[] = LIMINE_BASE_REVISION(6);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request fb_req = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_req = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t req_start[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t req_end[] = LIMINE_REQUESTS_END_MARKER;

static void hcf(void) {
    for (;;) {
        asm volatile("hlt");
    }
}

static int blk_read(uint64_t lba, void *buf, uint32_t count, void *ctx) {
    (void)ctx;
    return ata_read_sectors(lba, buf, count);
}

void kmain(void) {
    if (!LIMINE_BASE_REVISION_SUPPORTED(base_rev)) {
        hcf();
    }
    if (fb_req.response == NULL || fb_req.response->framebuffer_count < 1) {
        hcf();
    }

    struct limine_framebuffer *fb = fb_req.response->framebuffers[0];
    console_init(fb);
    serial_init();

    printf("========================================\n");
    printf("           GQ_DOS v0.2\n");
    printf("========================================\n");
    printf("\n");

    /* 文件系统测试：从磁盘读取 hello.txt */
    fat32_t fs;
    if (fat32_init(&fs, blk_read, NULL) == 0) {
        uint32_t cluster = 0, size = 0;
        if (fat32_lookup(&fs, "HELLO.TXT", &cluster, &size) == 0) {
            char content[512];
            memset(content, 0, sizeof(content));
            fat32_read_file(&fs, cluster, size, content);
            printf("[fs] hello.txt: %s\n", content);
        } else {
            printf("[fs] HELLO.TXT not found\n");
        }
    } else {
        printf("[fs] FAT32 init failed\n");
    }

    idt_init();
    apic_init();
    kbd_init();
    __asm__ volatile("sti");

    printf("Interrupts enabled (LAPIC timer).\n");
    printf("Type to echo (uptime -> serial):\n");
    printf("> ");

    uint64_t last_second = 0;
    for (;;) {
        __asm__ volatile("hlt");
        kbd_poll();
        while (kbd_has_char()) {
            console_putc(kbd_getc());
        }
        uint64_t seconds = g_ticks / 100;
        if (seconds != last_second) {
            last_second = seconds;
            serial_puts("[uptime ");
            serial_print_num(seconds);
            serial_puts(" s]\n");
        }
    }
}
