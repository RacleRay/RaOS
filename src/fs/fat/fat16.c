#include "fat16.h"
#include "../../status.h"
#include "../../string/string.h"


#define RAOS_FAT16_SIGNATURE 0x29
#define RAOS_FAT16_ENTRY_SIZE 0x02
#define RAOS_FAT16_BAD_SECTOR 0xFF7
#define RAOS_FAT16_UNUSED 0x00

typedef unsigned int FAT_ITEM_TYPE;
#define FAT_ITEM_TYPE_DIRECTORY 0
#define FAT_ITEM_TYPE_FILE 1

// FAT directory entry attributes bitmask
#define FAT_FILE_READ_ONLY 0x01
#define FAT_FILE_HIDDEN 0x02
#define FAT_FILE_SYSTEM 0x04
#define FAT_FILE_VOLUME_LABEL 0x08
#define FAT_FILE_SUBDIRECTORY 0x10
#define FAT_FILE_ARCHIVED 0x20
#define FAT_FILE_DEVICE 0x40
#define FAT_FILE_RESERVED 0x80


int fat16_resolve(struct disk* disk);
void* fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode);

struct filesystem fat16_fs = {
    .resolve = fat16_resolve,
    .open = fat16_open
};


struct filesystem* fat16_init() {
    strcpy(fat16_fs.name, "FAT16");
    return &fat16_fs;
}


int fat16_resolve(struct disk* disk) {
    // return -EIO;
    return 0;
}


void* fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode) {
    return 0;
}