#include "fat32.h"
#include "string.h"

#define SECTOR_SIZE 512

static char toupper_ascii(char c) {
    return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;
}

static int read_sectors(fat32_t *fs, uint64_t lba, void *buf, uint32_t count) {
    return fs->read(lba, buf, count, fs->ctx);
}

static int write_sectors(fat32_t *fs, uint64_t lba, const void *buf, uint32_t count) {
    if (!fs->write) return -1;
    return fs->write(lba, buf, count, fs->ctx);
}

int fat32_init(fat32_t *fs, blk_read_fn read, blk_write_fn write, void *ctx) {
    uint8_t bpb[SECTOR_SIZE];
    fs->read = read;
    fs->write = write;
    fs->ctx = ctx;
    if (read_sectors(fs, 0, bpb, 1) != 0) return -1;
    if (bpb[510] != 0x55 || bpb[511] != 0xAA) return -2;

    fs->bytes_per_sector = (uint16_t)(bpb[11] | (bpb[12] << 8));
    fs->sectors_per_cluster = bpb[13];
    fs->reserved_sectors = (uint16_t)(bpb[14] | (bpb[15] << 8));
    uint8_t num_fats = bpb[16];
    fs->fat_size = (uint32_t)(bpb[36] | (bpb[37] << 8) | (bpb[38] << 16) | (bpb[39] << 24));
    fs->root_cluster = (uint32_t)(bpb[44] | (bpb[45] << 8) | (bpb[46] << 16) | (bpb[47] << 24));
    fs->data_start = fs->reserved_sectors + num_fats * fs->fat_size;
    return 0;
}

static uint32_t cluster_to_lba(fat32_t *fs, uint32_t cluster) {
    return fs->data_start + (cluster - 2) * fs->sectors_per_cluster;
}

static uint32_t read_fat_entry(fat32_t *fs, uint32_t cluster) {
    uint8_t fat_sector[SECTOR_SIZE];
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_lba = fs->reserved_sectors + fat_offset / SECTOR_SIZE;
    uint32_t fat_off = fat_offset % SECTOR_SIZE;
    read_sectors(fs, fat_lba, fat_sector, 1);
    uint32_t entry = (uint32_t)(fat_sector[fat_off] |
                               (fat_sector[fat_off + 1] << 8) |
                               (fat_sector[fat_off + 2] << 16) |
                               (fat_sector[fat_off + 3] << 24));
    return entry & 0x0FFFFFFF;
}

static int write_fat_entry(fat32_t *fs, uint32_t cluster, uint32_t value) {
    if (!fs->write) return -1;
    uint8_t fat_sector[SECTOR_SIZE];
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_lba = fs->reserved_sectors + fat_offset / SECTOR_SIZE;
    uint32_t fat_off = fat_offset % SECTOR_SIZE;
    read_sectors(fs, fat_lba, fat_sector, 1);
    fat_sector[fat_off] = (uint8_t)(value & 0xFF);
    fat_sector[fat_off + 1] = (uint8_t)((value >> 8) & 0xFF);
    fat_sector[fat_off + 2] = (uint8_t)((value >> 16) & 0xFF);
    fat_sector[fat_off + 3] = (uint8_t)((value >> 24) & 0xFF);
    write_sectors(fs, fat_lba, fat_sector, 1);
    return 0;
}

static void make_short_name(const char *name, char short_name[11]) {
    memset(short_name, ' ', 11);
    const char *dot = strchr(name, '.');
    size_t base_len = dot ? (size_t)(dot - name) : strlen(name);
    size_t ext_len = dot ? strlen(dot + 1) : 0;
    for (size_t i = 0; i < base_len && i < 8; i++) short_name[i] = toupper_ascii(name[i]);
    if (dot) {
        for (size_t i = 0; i < ext_len && i < 3; i++) short_name[8 + i] = toupper_ascii(dot[1 + i]);
    }
}

static void make_display_name(const char short_name[11], char out[13]) {
    int j = 0;
    for (int i = 0; i < 8 && short_name[i] != ' '; i++) out[j++] = short_name[i];
    if (short_name[8] != ' ') {
        out[j++] = '.';
        for (int i = 8; i < 11 && short_name[i] != ' '; i++) out[j++] = short_name[i];
    }
    out[j] = '\0';
}

int fat32_lookup(fat32_t *fs, const char *name, uint32_t *cluster, uint32_t *size) {
    char short_name[11];
    make_short_name(name, short_name);
    uint32_t cluster_bytes = fs->sectors_per_cluster * fs->bytes_per_sector;
    if (cluster_bytes > 16384) return -3;
    uint8_t dir_buf[16384];
    uint32_t cur = fs->root_cluster;
    while (cur >= 2 && cur < 0x0FFFFFF8) {
        uint32_t lba = cluster_to_lba(fs, cur);
        read_sectors(fs, lba, dir_buf, fs->sectors_per_cluster);
        for (uint32_t off = 0; off < cluster_bytes; off += 32) {
            uint8_t *ent = dir_buf + off;
            if (ent[0] == 0x00) return -1;
            if (ent[0] == 0xE5) continue;
            if (ent[11] == 0x0F) continue;
            if (memcmp(ent, short_name, 11) == 0) {
                uint32_t cl = (uint32_t)(ent[26] | (ent[27] << 8)) |
                             ((uint32_t)(ent[20] | (ent[21] << 8)) << 16);
                uint32_t sz = (uint32_t)(ent[28] | (ent[29] << 8) | (ent[30] << 16) | (ent[31] << 24));
                if (cluster) *cluster = cl;
                if (size) *size = sz;
                return 0;
            }
        }
        cur = read_fat_entry(fs, cur);
    }
    return -1;
}

int fat32_read_file(fat32_t *fs, uint32_t cluster, uint32_t size, void *buf) {
    uint8_t *out = (uint8_t *)buf;
    uint32_t cur = cluster;
    uint32_t cluster_bytes = fs->sectors_per_cluster * fs->bytes_per_sector;
    while (cur >= 2 && cur < 0x0FFFFFF8 && size > 0) {
        uint32_t lba = cluster_to_lba(fs, cur);
        uint32_t to_read = size < cluster_bytes ? size : cluster_bytes;
        uint32_t secs = (to_read + SECTOR_SIZE - 1) / SECTOR_SIZE;
        read_sectors(fs, lba, out, secs);
        out += to_read;
        size -= to_read;
        cur = read_fat_entry(fs, cur);
    }
    return 0;
}

int fat32_dir_list(fat32_t *fs, void (*cb)(const char *name, uint32_t size, void *ud), void *ud) {
    uint32_t cluster_bytes = fs->sectors_per_cluster * fs->bytes_per_sector;
    if (cluster_bytes > 16384) return -3;
    uint8_t dir_buf[16384];
    uint32_t cur = fs->root_cluster;
    while (cur >= 2 && cur < 0x0FFFFFF8) {
        uint32_t lba = cluster_to_lba(fs, cur);
        read_sectors(fs, lba, dir_buf, fs->sectors_per_cluster);
        for (uint32_t off = 0; off < cluster_bytes; off += 32) {
            uint8_t *ent = dir_buf + off;
            if (ent[0] == 0x00) return 0;
            if (ent[0] == 0xE5) continue;
            if (ent[11] == 0x0F) continue;
            char display[13];
            make_display_name((char *)ent, display);
            uint32_t sz = (uint32_t)(ent[28] | (ent[29] << 8) | (ent[30] << 16) | (ent[31] << 24));
            cb(display, sz, ud);
        }
        cur = read_fat_entry(fs, cur);
    }
    return 0;
}

int fat32_create(fat32_t *fs, const char *name) {
    if (!fs->write) return -1;
    char short_name[11];
    make_short_name(name, short_name);
    uint32_t cluster_bytes = fs->sectors_per_cluster * fs->bytes_per_sector;
    if (cluster_bytes > 16384) return -3;
    uint8_t dir_buf[16384];

    /* 找空闲目录项 */
    uint32_t cur = fs->root_cluster;
    uint32_t found_lba = 0, found_off = 0;
    int found = 0;
    while (cur >= 2 && cur < 0x0FFFFFF8 && !found) {
        uint32_t lba = cluster_to_lba(fs, cur);
        read_sectors(fs, lba, dir_buf, fs->sectors_per_cluster);
        for (uint32_t off = 0; off < cluster_bytes; off += 32) {
            uint8_t *ent = dir_buf + off;
            if (ent[0] == 0x00 || ent[0] == 0xE5) {
                found_lba = lba + off / fs->bytes_per_sector;
                found_off = off % fs->bytes_per_sector;
                found = 1;
                break;
            }
        }
        if (!found) cur = read_fat_entry(fs, cur);
    }
    if (!found) return -2;

    /* 分配空闲簇 */
    uint32_t free_cluster = 0;
    for (uint32_t c = 2; c < 0x0FFFFFF0; c++) {
        if (read_fat_entry(fs, c) == 0x00000000) { free_cluster = c; break; }
    }
    if (free_cluster == 0) return -4;

    /* 写目录项 */
    uint8_t ent[32];
    memset(ent, 0, 32);
    memcpy(ent, short_name, 11);
    ent[11] = 0x20;
    ent[20] = (uint8_t)(free_cluster >> 16);
    ent[21] = (uint8_t)(free_cluster >> 24);
    ent[26] = (uint8_t)(free_cluster & 0xFF);
    ent[27] = (uint8_t)((free_cluster >> 8) & 0xFF);

    uint8_t sec[SECTOR_SIZE];
    read_sectors(fs, found_lba, sec, 1);
    memcpy(sec + found_off, ent, 32);
    write_sectors(fs, found_lba, sec, 1);

    /* FAT 标记簇为结束 */
    write_fat_entry(fs, free_cluster, 0x0FFFFFFF);
    return 0;
}

int fat32_delete(fat32_t *fs, const char *name) {
    if (!fs->write) return -1;
    char short_name[11];
    make_short_name(name, short_name);
    uint32_t cluster_bytes = fs->sectors_per_cluster * fs->bytes_per_sector;
    if (cluster_bytes > 16384) return -3;
    uint8_t dir_buf[16384];
    uint32_t cur = fs->root_cluster;
    while (cur >= 2 && cur < 0x0FFFFFF8) {
        uint32_t lba = cluster_to_lba(fs, cur);
        read_sectors(fs, lba, dir_buf, fs->sectors_per_cluster);
        for (uint32_t off = 0; off < cluster_bytes; off += 32) {
            uint8_t *ent = dir_buf + off;
            if (ent[0] == 0x00) return -1;
            if (ent[0] == 0xE5) continue;
            if (ent[11] == 0x0F) continue;
            if (memcmp(ent, short_name, 11) == 0) {
                uint32_t cl = (uint32_t)(ent[26] | (ent[27] << 8)) |
                             ((uint32_t)(ent[20] | (ent[21] << 8)) << 16);
                uint32_t elba = lba + off / fs->bytes_per_sector;
                uint32_t eoff = off % fs->bytes_per_sector;
                uint8_t sec[SECTOR_SIZE];
                read_sectors(fs, elba, sec, 1);
                sec[eoff] = 0xE5;
                write_sectors(fs, elba, sec, 1);
                /* 释放簇链 */
                uint32_t c = cl;
                while (c >= 2 && c < 0x0FFFFFF8) {
                    uint32_t next = read_fat_entry(fs, c);
                    write_fat_entry(fs, c, 0);
                    c = next;
                }
                return 0;
            }
        }
        cur = read_fat_entry(fs, cur);
    }
    return -1;
}
