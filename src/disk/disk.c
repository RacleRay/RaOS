#include "disk.h"
#include "../config.h"
#include "../io/io.h"
#include "../memory/memory.h"
#include "../status.h"



struct disk disk;

/**
 * @brief Read data from master hard disk.
 *
 * https://wiki.osdev.org/ATA_read/write_sectors
 * https://wiki.osdev.org/ATA_PIO_Mode
 *
 * @param lba Logical block address
 * @param total number of blocks to read.
 * @param buf memory to save data.
 * @return int
 */
int disk_read_sector(int lba, int total, void* buf) {
    // Same as boot.asm:ata_lba_read
    outb(0x1F6, (lba >> 24) | 0xE0);
    outb(0x1F2, total);
    outb(0x1F3, (unsigned char)(lba & 0xff));
    outb(0x1F4, (unsigned char)(lba >> 8));
    outb(0x1F5, (unsigned char)(lba >> 16));
    outb(0x1F7, 0x20);

    unsigned short* ptr = (unsigned short*)buf;
    for (int b = 0; b < total; ++b) {
        char c = insb(0x1F7);
        // wait for the buffer to be ready
        while (!(c & 0x08)) {
            c = insb(0x1F7);
        }

        // copy data from hard disk to memory
        for (int i = 0; i < 256; ++i) {
            *ptr = insw(0x1F0);
            ++ptr;
        }
    }

    return 0;
}


void disk_search_and_init() {
    memset(&disk, 0, sizeof(disk));
    disk.type        = RAOS_DISK_TYPE_REAL;
    disk.sector_size = RAOS_SECTOR_SIZE;
}


/**
 * @brief Get the index`th disk. Passing it to disk_read_block().
 *
 * @param index
 * @return struct disk*
 */
struct disk* disk_get(int index) {
    // TODO: this is simple implementation when only one disk exists.
    if (0 != index) {
        return 0;
    }

    return &disk;
}


/**
 * @brief Abstraction function to read data sectors from disk.
 *
 * @param idisk get from disk_get()
 * @param lba logical block address.
 * @param total total sectors to read.
 * @param buf memory to save data
 * @return int
 */
int disk_read_block(struct disk* idisk, unsigned int lba, int total,
                    void* buf) {
    if (idisk != &disk) {
        return -EIO;
    }

    return disk_read_sector(lba, total, buf);
}