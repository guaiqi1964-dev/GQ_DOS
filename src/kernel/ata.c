#include "ata.h"
#include "io.h"

#define ATA_DATA       0x1F0
#define ATA_SECT_COUNT 0x1F2
#define ATA_LBA_LOW    0x1F3
#define ATA_LBA_MID    0x1F4
#define ATA_LBA_HIGH   0x1F5
#define ATA_DRIVE      0x1F6
#define ATA_STATUS     0x1F7
#define ATA_COMMAND    0x1F7

/* 28 位 LBA 读扇区（轮询 PIO） */
int ata_read_sectors(uint64_t lba, void *buf, uint32_t count) {
    uint16_t *ptr = (uint16_t *)buf;
    for (uint32_t i = 0; i < count; i++) {
        while (inb(ATA_STATUS) & 0x80) { }   /* 等 BSY 清除 */
        outb(ATA_DRIVE, 0xE0 | ((uint8_t)(lba >> 24) & 0x0F));
        outb(ATA_SECT_COUNT, 1);
        outb(ATA_LBA_LOW, (uint8_t)(lba & 0xFF));
        outb(ATA_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
        outb(ATA_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));
        outb(ATA_COMMAND, 0x20);             /* READ SECTORS */
        while (inb(ATA_STATUS) & 0x80) { }   /* 等 BSY 清除 */
        uint8_t status = inb(ATA_STATUS);
        if (status & 0x01) return -1;        /* ERR */
        if (!(status & 0x08)) return -2;     /* 无 DRQ */
        for (int j = 0; j < 256; j++) {
            ptr[j] = inw(ATA_DATA);
        }
        ptr += 256;
        lba++;
    }
    return 0;
}
