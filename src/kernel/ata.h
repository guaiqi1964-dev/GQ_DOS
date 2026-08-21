#ifndef GQ_ATA_H
#define GQ_ATA_H

#include <stdint.h>

int ata_read_sectors(uint64_t lba, void *buf, uint32_t count);
int ata_write_sectors(uint64_t lba, const void *buf, uint32_t count);

#endif
