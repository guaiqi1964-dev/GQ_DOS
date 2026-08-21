#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "fat32.h"

static FILE *g_img;

static int file_read(uint64_t lba, void *buf, uint32_t count, void *ctx) {
    (void)ctx;
    fseek(g_img, (long)(lba * 512), SEEK_SET);
    fread(buf, 512, count, g_img);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) { printf("用法: test_fat32 <镜像>\n"); return 1; }
    g_img = fopen(argv[1], "rb");
    if (!g_img) { printf("无法打开 %s\n", argv[1]); return 1; }

    fat32_t fs;
    int rc = fat32_init(&fs, file_read, NULL, NULL);
    printf("fat32_init: rc=%d\n", rc);
    if (rc != 0) { fclose(g_img); return 1; }

    uint32_t cluster = 0, size = 0;
    rc = fat32_lookup(&fs, "HELLO.TXT", &cluster, &size);
    printf("lookup(HELLO.TXT): rc=%d size=%u\n", rc, size);
    if (rc == 0) {
        char content[4096];
        memset(content, 0, sizeof(content));
        fat32_read_file(&fs, cluster, size, content);
        printf("内容: %s\n", content);
    }

    printf("目录列举:\n");
    fat32_dir_list(&fs, (void (*)(const char*, uint32_t, void*))0, NULL);
    (void)0;

    fclose(g_img);
    return 0;
}
