#include "fat16.h"
#include "../../status.h"
#include "../../string/string.h"
#include "../../disk/disk.h"
#include "../../disk/streamer.h"
#include "../../memory/memory.h"
#include "../../memory/heap/kheap.h"
#include <stdint.h>


#define RAOS_FAT16_SIGNATURE 0x29
#define RAOS_FAT16_ENTRY_SIZE 0x02
#define RAOS_FAT16_BAD_SECTOR 0xFF7
#define RAOS_FAT16_UNUSED 0x00

// Not written to disk, but for us programer to read
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


// FAT file defination and notations.
// https://academy.cba.mit.edu/classes/networking_communications/SD/FAT.pdf

struct fat_header_extended {
    uint8_t drive_number;
    uint8_t win_nt_bit;
    uint8_t signature;
    uint32_t volume_id;
    uint8_t volume_id_string[11];
    uint8_t system_id_string[8];
} __attribute__((packed));


// check in boot.asm
struct fat_header {
    uint8_t short_jmp_inst[3];  // boot.asm: short jmp start
    uint8_t oem_indentifier[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_copies;
    uint16_t root_dir_entries;
    uint16_t number_of_sectors;
    uint8_t media_type;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t number_of_heads;
    uint32_t hidden_sectors;
    uint32_t sectors_big;
} __attribute__((packed));


struct fat_h {
    struct fat_header primary_header;
    union fat_h_e {
        struct fat_header_extended extended_header;
    } shared;  // optional, could be something else.
};


struct fat_directory_item {
    uint8_t filename[8];
    uint8_t ext[3];
    uint8_t attribute;  // FAT directory entry attributes bitmasks
    uint8_t reserved;   // for furture usage
    uint8_t creation_time_tenths_of_a_sec;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access;
    uint16_t high_16_bits_first_cluster;  // subdirectory addr or data of files
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint16_t low_16_bits_first_cluster;
    uint32_t filesize;
} __attribute__((packed));


struct fat_directory {
    struct fat_directory_item* item;
    int32_t total;
    int32_t sector_pos;
    int32_t end_sector_pos;
};


struct fat_item {
    union {
        struct fat_directory_item* item;
        struct fat_directory* directory;
    };

    FAT_ITEM_TYPE type;
};


struct fat_item_descriptor {
    struct fat_item* item;
    uint32_t pos;
};


struct fat_private {
    struct fat_h header;
    struct fat_directory root_directory;

    struct disk_stream* cluster_read_stream;  // data cluster
    struct disk_stream* fat_read_stream;      // file allocation table

    struct disk_stream* directory_stream;     // directory
};


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


/**
 * @brief init the streamer to read the disk data.
 * 
 * @param disk 
 * @param private 
 */
static void fat16_init_private(struct disk* disk, struct fat_private* private) {
    memset(private, 0, sizeof(struct fat_private));
    private->cluster_read_stream = diskstreamer_new(disk->id); 
    private->fat_read_stream = diskstreamer_new(disk->id);
    private->directory_stream = diskstreamer_new(disk->id);
}


/**
 * @brief Size in bytes.
 * 
 * @param disk 
 * @param sector 
 * @return int 
 */
static int fat16_sector_to_absolute(struct disk* disk, int sector) {
    return sector * disk->sector_size;
}


/**
 * @brief Get number of items in directory.
 * 
 * @param disk 
 * @param directory_start_sector the beginning postion to read data.
 * @return int return the number of items. Or negetive error numbers.
 */
static int fat16_get_total_items_for_directory(struct disk* disk, uint32_t directory_start_sector) {
    struct fat_directory_item item;
    struct fat_directory_item empty_item;
    memset(&empty_item, 0, sizeof(empty_item));

    struct fat_private* fat_private = disk->fs_private;

    int res = 0;

    // get to the directory data sector
    int directory_start_pos = directory_start_sector * disk->sector_size;
    struct disk_stream* stream = fat_private->directory_stream;
    if (diskstreamer_seek(stream, directory_start_pos) != RAOS_ALL_OK) {
        res = -EIO;
        goto out;
    }

    // at the test case: sudo cp ./message.txt /mnt/d
    // There will be two file name record for message.txt in mordern system.
    // One for long name, one for short name.
    // So i will be 2.
    // Read directory items.
    int i = 0;
    while (1) {
        if (diskstreamer_read(stream, &item, sizeof(item)) != RAOS_ALL_OK) {
            res = -EIO;
            goto out;
        }

        // blank, it is at end
        if (item.filename[0] == 0x00) {
            break;
        }

        // unused item tag
        if (item.filename[0] == 0xE5) {
            continue;
        }

        ++i;
    }

    res = i;

out:
    return res;
}


/**
 * @brief Read root directory data info from disk.
 * 
 * @param disk disk structure
 * @param fat_private fat_header and maybe more info.
 * @param directory the output
 * @return int 0: OK.
 */
static int fat16_get_root_directory(struct disk* disk, struct fat_private* fat_private, struct fat_directory* directory) {
    struct fat_header* primary_header = &fat_private->header.primary_header;
    
    // https://academy.cba.mit.edu/classes/networking_communications/SD/FAT.pdf
    // skip the fat headers and reserved_sectors
    int root_dir_sector_pos = (primary_header->fat_copies * primary_header->sectors_per_fat) + primary_header->reserved_sectors;
    int root_dir_entries = primary_header->root_dir_entries;  // n items
    int root_dir_size = (root_dir_entries * sizeof(struct fat_directory_item));
 
    // not used
    // int total_sectors = root_dir_size / disk->sector_size;
    // if (root_dir_size % disk->sector_size) {
    //     total_sectors++;
    // }

    int res = 0;
    int total_items = fat16_get_total_items_for_directory(disk, root_dir_sector_pos);
    struct fat_directory_item* dir = kzalloc(root_dir_size);
    if (!dir) {
        res = -ENOMEM;
        goto out;
    }

    struct disk_stream* stream = fat_private->directory_stream;
    if (diskstreamer_seek(stream, fat16_sector_to_absolute(disk, root_dir_sector_pos)) != RAOS_ALL_OK) {
        res = -EIO;
        goto out;
    }

    if (diskstreamer_read(stream, dir, root_dir_size) != RAOS_ALL_OK) {
        res = -EIO;
        goto out;
    }

    directory->item = dir;
    directory->total = total_items;
    directory->sector_pos = root_dir_sector_pos;
    directory->end_sector_pos = root_dir_sector_pos + (root_dir_size / disk->sector_size);

out:
    return res;
}


/**
 * @brief FAT16 filesystem interprator.
 * 
 * @param disk 
 * @return int 0: OK
 */
int fat16_resolve(struct disk* disk) {
    int res = 0;
    struct fat_private* fat_private = kzalloc(sizeof(struct fat_private));
    memset(fat_private, 0, sizeof(struct fat_private));
    fat16_init_private(disk, fat_private);

    disk->fs_private = fat_private;
    disk->filesystem = &fat16_fs;   // attached to disk struct.

    // bytes data read stream
    struct disk_stream* stream = diskstreamer_new(disk->id);
    if (!stream) {
        res = -ENOMEM;
        goto out;
    }

    // read fat_header data
    if (diskstreamer_read(stream, &fat_private->header, sizeof(fat_private->header)) != RAOS_ALL_OK) {
        res = -EIO;
        goto out;
    }

    // check if it is not fat16 filesystem
    if (fat_private->header.shared.extended_header.signature != RAOS_FAT16_SIGNATURE) {
        res = -EFSNOTUS;
        goto out;
    }

    // fill in the root_directory data structure.
    if (fat16_get_root_directory(disk, fat_private, &fat_private->root_directory) != RAOS_ALL_OK) {
        res = -EIO;
        goto out;
    }
    
out:
    if (stream) {
        diskstreamer_close(stream);
    }

    if (res < 0) {
        kfree(fat_private);
        disk->fs_private = NULL;
    }

    return res;
}


void* fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode) {
    return 0;
}