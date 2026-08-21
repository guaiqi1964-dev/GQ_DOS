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
    if (argc < 2) { printf("用法: test_fat32 <镜像文件>\n"); return 1; }
    g_img = fopen(argv[1], "rb");
    if (!g_img) { printf("无法打开镜像 %s\n", argv[1]); return 1; }

    fat32_t fs;
    int rc = fat32_init(&fs, file_read, NULL);
    printf("fat32_init: rc=%d bps=%u spc=%u rsvd=%u fatsz=%u root=%u data=%u\n",
           rc, fs.bytes_per_sector, fs.sectors_per_cluster, fs.reserved_sectors,
           fs.fat_size, fs.root_cluster, fs.data_start);
    if (rc != 0) { fclose(g_img); return 1; }

    uint32_t cluster = 0, size = 0;
    rc = fat32_lookup(&fs, "HELLO.TXT", &cluster, &size);
    printf("fat32_lookup(HELLO.TXT): rc=%d cluster=%u size=%u\n", rc, cluster, size);
    if (rc != 0) { fclose(g_img); return 1; }

    char content[4096];
    memset(content, 0, sizeof(content));
    fat32_read_file(&fs, cluster, size, content);
    printf("文件内容:\n%s\n", content);

    fclose(g_img);
    return 0;
}
