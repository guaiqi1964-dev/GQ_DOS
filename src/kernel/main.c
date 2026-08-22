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
#include "shell.h"

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

void kmain(void) {
    if (!LIMINE_BASE_REVISION_SUPPORTED(base_rev)) {
        hcf();
    }
    if (fb_req.response == NULL || fb_req.response->framebuffer_count < 1) {
        hcf();
    }

    struct limine_framebuffer *fb = fb_req.response->framebuffers[0];
    serial_init();
    serial_puts("[fb ");
    serial_print_num(fb->bpp);
    serial_puts("bpp ");
    serial_print_num(fb->width);
    serial_puts("x");
    serial_print_num(fb->height);
    serial_puts("]\n");
    if (console_init(fb) != 0) {
        serial_puts("[console_init FAIL]\n");
        hcf();
    }

    printf("========================================\n");
    printf("           GQ_DOS v0.4\n");
    printf("========================================\n");
    printf("\n");

    idt_init();
    apic_init();
    kbd_init();
    __asm__ volatile("sti");

    printf("GQ_DOS shell ready. Type 'help' for commands.\n");
    shell_run();
    hcf();
}
