#ifndef GQ_FAT32_H
#define GQ_FAT32_H

#include <stdint.h>
#include <stddef.h>

/* 块设备读取：读 count 个 512 字节扇区到 buf */
typedef int (*blk_read_fn)(uint64_t lba, void *buf, uint32_t count, void *ctx);

typedef struct {
    blk_read_fn read;
    void *ctx;
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint32_t reserved_sectors;
    uint32_t fat_size;
    uint32_t root_cluster;
    uint32_t data_start;
} fat32_t;

int  fat32_init(fat32_t *fs, blk_read_fn read, void *ctx);
int  fat32_lookup(fat32_t *fs, const char *name, uint32_t *cluster, uint32_t *size);
int  fat32_read_file(fat32_t *fs, uint32_t cluster, uint32_t size, void *buf);

#endif
