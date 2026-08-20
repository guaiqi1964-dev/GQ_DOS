#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "console.h"
#include "printf.h"

/* Limine 基础修订版：请求最新的第 6 版 */
__attribute__((used, section(".limine_requests")))
static volatile uint64_t base_rev[] = LIMINE_BASE_REVISION(6);

/* 帧缓冲请求 */
__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request fb_req = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

/* HHDM（更高半直接映射）请求 */
__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_req = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0
};

/* 请求区段的起止标记 */
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
    /* 确认引导器支持我们的基础修订版 */
    if (!LIMINE_BASE_REVISION_SUPPORTED(base_rev)) {
        hcf();
    }

    /* 确认拿到了帧缓冲 */
    if (fb_req.response == NULL || fb_req.response->framebuffer_count < 1) {
        hcf();
    }

    struct limine_framebuffer *fb = fb_req.response->framebuffers[0];
    console_init(fb);

    printf("========================================\n");
    printf("           GQ_DOS v0.1\n");
    printf("========================================\n");
    printf("\n");
    printf("Hello from the GQ_DOS kernel!\n");
    printf("\n");
    printf("Framebuffer : %lux%lu @ %ubpp\n",
           (unsigned long)fb->width, (unsigned long)fb->height, (unsigned)fb->bpp);
    printf("Resolution  : %lu x %lu glyphs\n",
           (unsigned long)(fb->width / 8), (unsigned long)(fb->height / 8));
    if (hhdm_req.response != NULL) {
        printf("HHDM offset : 0x%lx\n", (unsigned long)hhdm_req.response->offset);
    }
    printf("\n");
    printf("Boot OK. System ready.\n");

    hcf();
}
