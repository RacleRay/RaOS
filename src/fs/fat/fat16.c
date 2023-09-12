#include "fat16.h"
#include "../../status.h"
#include "../../string/string.h"
#include "../../disk/disk.h"
#include "../../disk/streamer.h"
#include "../../memory/memory.h"
#include "../../memory/heap/kheap.h"
#include "../../kernel.h"
#include "../../config.h"
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


// items in a directory
struct fat_directory_item {
    uint8_t filename[8];
    uint8_t ext[3];
    uint8_t attribute;  // FAT directory entry attributes bitmasks
    uint8_t reserved;   // for furture usage
    uint8_t creation_time_tenths_of_a_sec;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint16_t high_16_bits_first_cluster;  // subdirectory addr high 16 bits
    uint16_t low_16_bits_first_cluster;   // subdirectory addr low 16 bits
    uint32_t filesize;
} __attribute__((packed));


// directory itself
struct fat_directory {
    struct fat_directory_item* item;
    int32_t total;
    int32_t sector_pos;
    int32_t end_sector_pos;
};


// used in file descriptor to find resourses.
struct fat_item {
    union {
        struct fat_directory_item* item;
        struct fat_directory* directory;
    };

    FAT_ITEM_TYPE type;
};


// file descripter, will be passed to user.
struct fat_file_descriptor {
    struct fat_item* item;
    uint32_t pos;
};


// For FAT16 filesystem internal use.
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
 * @brief Get the number of items in directory.
 * 
 * @param disk 
 * @param directory_start_sector the beginning postion to read data.
 * @return int return the number of items. Or negetive error numbers.
 */
static int fat16_get_total_items_for_directory(struct disk* disk, uint32_t directory_start_sector) {
    struct fat_directory_item item;

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

        // blank. it is at end
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


// Remove the redundant space
static void fat16_to_proper_string(char** out, const char* in) {
    // terminator or space
    while (*in != 0x00 && *in != 0x20) {
        **out = *in;
        (*out)++;
        in++;
    }

    if (*in == 0x20) {
        **out = 0x00;  // terminated
    }
}


/**
 * @brief Get the full filename returned in out pointer.
 * 
 * @param item 
 * @param out filename returned.
 * @param out_len max length of filename.
 */
static void fat16_get_full_relative_filename(struct fat_directory_item* item, char* out, int out_len) {
    memset(out, 0, out_len);

    char* out_tmp = out;
    fat16_to_proper_string(&out_tmp, (const char*)item->filename);
    
    // process file extension
    if (item->ext[0] != 0x00 && item->ext[0] != 0x20) {
        *(out_tmp++) = '.';
        fat16_to_proper_string(&out_tmp, (const char*)item->ext); 
    }
}


static void fat16_free_directory(struct fat_directory *directory) {
    if (!directory) {
        return;
    }

    if (directory->item) {
        kfree(directory->item);
    }

    kfree(directory);
}


static void fat16_fat_item_free(struct fat_item *item) {
    if (item->type == FAT_ITEM_TYPE_DIRECTORY) {
        fat16_free_directory(item->directory);
    }
    else if (item->type == FAT_ITEM_TYPE_FILE) {
        kfree(item->item);
    }

    kfree(item);
}


// get the first cluster address of the fat directory item.
static inline uint32_t fat16_get_first_cluster_for_directory_item(struct fat_directory_item* item) {
    return (item->high_16_bits_first_cluster << 16) | (item->low_16_bits_first_cluster);
}


/**
 * @brief Read the FAT table entry for the cluster.
 * 
 * @param disk 
 * @param cluster_pos the index of the cluster in the FAT table.
 * @return uint32_t FAT table entry; -EIO: error; -ENOMEM: out of memory
 */
static uint32_t fat16_get_fat_entry(struct disk* disk, int cluster_pos) {
    int res = -1;

    struct fat_private *private = disk->fs_private;
    struct disk_stream *stream = private->fat_read_stream;
    if (!stream) {
        goto out;
    }

    uint32_t fat_table_position = private->header.primary_header.reserved_sectors * disk->sector_size;
    res = diskstreamer_seek(stream, fat_table_position * (cluster_pos * RAOS_FAT16_ENTRY_SIZE));
    if (res < 0) {
        goto out;
    }

    uint32_t result = 0;
    res = diskstreamer_read(stream, &result, sizeof(result));
    if (res < 0) {
        goto out;
    }

    res = result;
out:
    return res;
}


/**
 * @brief Get the cluster position for offset.
 * 
 * @param disk 
 * @param start_cluster 
 * @param offset 
 * @return int cluster position number; -EIO: error; -ENOMEM: out of memory
 */
static int fat16_get_cluster_for_offset(struct disk* disk, int start_cluster, int offset) {
    int res = 0;

    struct fat_private *private = disk->fs_private;
    int size_of_cluster_bytes = private->header.primary_header.sectors_per_cluster * disk->sector_size;
    
    int cluster_pos = start_cluster;
    int nclusters = offset / size_of_cluster_bytes;
    for (int i = 0; i < nclusters; i++) {
        // FAT table entry bytes for cluster.
        int entry = fat16_get_fat_entry(disk, cluster_pos);
        // Last entry of a file
        if (entry == 0xFF8 || entry == 0xFFF) {
            res = -EIO;
            goto out;
        }

        // bad sector 
        if (entry == RAOS_FAT16_BAD_SECTOR) {
            res = -EIO;
            goto out;
        }

        // reserved sector
        if (entry == 0xFF0 || entry == 0xFF6) {
            res = -EIO;
            goto out;
        }

        // Ignored sector
        if (entry == 0x00) {
            res = -EIO;
            goto out;
        }

        // next cluster position
        cluster_pos = entry;
    }

    res = cluster_pos;
out:
    return res;
}


/**
 * @brief Read items of the directory from disk, started at (start_cluster + offset), returned at 'out' pointer.
 *        We wil read cluster by cluster until we reach the total bytes.
 * 
 * @param disk 
 * @param start_cluster 
 * @param offset offset from the start_cluster.
 * @param total total bytes to read.
 * @param out items stored in out pointer.
 * @return int 
 */
static int fat16_read_items_of_directory(struct disk* disk, int start_cluster, int offset, int total, void* out) {
    int res = 0;
    struct fat_private *private = disk->fs_private;
    struct disk_stream *stream = private->cluster_read_stream;

    // the first cluster position
    int cluster_pos = fat16_get_cluster_for_offset(disk, start_cluster, offset);
    if (cluster_pos < 0) {
        res = cluster_pos;
        goto out;
    }

    int size_of_cluster_bytes = private->header.primary_header.sectors_per_cluster * disk->sector_size;
    int offset_from_cluster_pos = offset % size_of_cluster_bytes;

    int starting_sector = private->root_directory.end_sector_pos + ((start_cluster - 2) * private->header.primary_header.sectors_per_cluster);
    // start position in bytes.
    int starting_pos = (starting_sector * disk->sector_size) + offset_from_cluster_pos;
    int total_to_read = total > size_of_cluster_bytes ? size_of_cluster_bytes : total;
    
    res = diskstreamer_seek(stream, starting_pos);
    if (res != RAOS_ALL_OK) {
        goto out;
    }

    res = diskstreamer_read(stream, out, total_to_read);
    if (res != RAOS_ALL_OK) {
        goto out;
    }

    total -= total_to_read;
    if (total > 0) {
        // recursivly read the next cluster.
        res = fat16_read_items_of_directory(disk, start_cluster, offset + total_to_read, total, out + total_to_read);
    }

out:
    return res;
}


/**
 * @brief Load the fat_directory structure and the corresponding sub-items from disk.
 * 
 * @param disk 
 * @param item fat directory item will be used to search for sub-items.
 * @return struct fat_directory* directory loaded.
 */
static struct fat_directory* fat16_load_fat_directory(struct disk* disk, struct fat_directory_item* item) {
    int res = 0;
    if (!(item->attribute & FAT_FILE_SUBDIRECTORY)) {
        res = -EINVARG;
        goto out;
    }

    struct fat_directory* directory = kzalloc(sizeof(struct fat_directory));
    if (!directory) {
        res = -ENOMEM;
        goto out;
    }
    struct fat_private *fat_private = disk->fs_private;

    // Find the first cluster position of the item.
    uint32_t first_cluster = fat16_get_first_cluster_for_directory_item(item);

    // get the sector position of the item. Ignore the first two reserved clusters.
    uint32_t sector_pos = fat_private->root_directory.end_sector_pos + \
        ((first_cluster - 2) * fat_private->header.primary_header.sectors_per_cluster);

    // get the number of items in the directory.
    uint32_t total_items = fat16_get_total_items_for_directory(disk, sector_pos);
    
    // fill in the directory structure.
    directory->total = total_items;
    int directory_size = directory->total * sizeof(struct fat_directory_item);
    directory->item = kzalloc(directory_size);
    if (!directory->item) {
        res = -ENOMEM;
        goto out;
    }

    // load the items into directory->item member.
    res = fat16_read_items_of_directory(disk, first_cluster, 0x00, directory_size, directory->item);
    if (res != RAOS_ALL_OK) {
        goto out;
    }

out:
    if (res != RAOS_ALL_OK) {
        fat16_free_directory(directory);
    }
    return directory;
}


// Clone directory item, remember to free it later.
static struct fat_directory_item* fat16_clone_directory_item(struct fat_directory_item* item, int size) {
    struct fat_directory_item* item_copy = NULL;
    if  (size < sizeof(struct fat_directory_item)) {
        goto out;
    }

    item_copy = kzalloc(size);
    if (!item_copy) {
        goto out;
    }

    memcpy(item_copy, item, size);
out:
    return item_copy;
}


/**
 * @brief Get the fat item. If the item is a directory, then load the directory structure. 
 *        If the item is a file, then clone the item.
 * 
 * @param disk 
 * @param item 
 * @return struct fat_item* target fat item pointer.
 */
static struct fat_item* fat16_new_fat_item_for_directory_item(struct disk* disk, struct fat_directory_item* item) {
    struct fat_item* f_item = kzalloc(sizeof(struct fat_item));
    if (!f_item) {
        return NULL;
    }

    if (item->attribute & FAT_FILE_SUBDIRECTORY) {
        f_item->directory = fat16_load_fat_directory(disk, item);
        f_item->type = FAT_ITEM_TYPE_DIRECTORY;
    } else {
        // clone item instead use the original one.
        f_item->type = FAT_ITEM_TYPE_FILE;
        f_item->item = fat16_clone_directory_item(item, sizeof(struct fat_directory_item));
    }

    return f_item;
}


/**
 * @brief Find the fat item in the directory structure.
 * 
 * @param disk 
 * @param directory directory to search.
 * @param name target name.
 * @return struct fat_item* 
 */
static struct fat_item* fat16_find_item_in_directory(struct disk* disk, struct fat_directory* directory, const char* name) {
    struct fat_item* f_item = NULL;
    
    char tmp_filename[RAOS_MAX_PATH];

    for (int i = 0; i < directory->total; i++) {
        // get the full filename in tmp_filename.
        fat16_get_full_relative_filename(&directory->item[i], tmp_filename, sizeof(tmp_filename));

        if (istrncmp(tmp_filename, name, sizeof(tmp_filename)) == 0) {
            // found it and create a new item.
            f_item = fat16_new_fat_item_for_directory_item(disk, &directory->item[i]);
        }
    }

    return f_item;
}


/**
 * @brief Get the fat item of the path. It could be a file or a directory. 
 *        If it is a directory, then load the directory structure with corresponding items.
 * 
 * @param disk 
 * @param path 
 * @return struct fat_item* 
 */
static struct fat_item* fat16_get_directory_entry(struct disk* disk, struct path_part* path) {
    struct fat_private* fat_private = disk->fs_private;

    // find the base part in root directory. e.g. 0:/a/b/c , then find the very first path part a
    struct fat_item* root_item = fat16_find_item_in_directory(disk, &fat_private->root_directory, path->part);
    if (!root_item) {
        goto out;
    }

    // next part search the path part splited by '/'
    struct path_part* next_part = path->next;
    struct fat_item* current_item = root_item;
    while (next_part != 0) {   
        // cant find it
        if (current_item->type != FAT_ITEM_TYPE_DIRECTORY) {
            current_item = 0;
            break;
        }

        // recursivly find the path part utill next_part is NULL
        struct fat_item *tmp_item = fat16_find_item_in_directory(disk, current_item->directory, next_part->part);
        fat16_fat_item_free(current_item);
        current_item = tmp_item;
        next_part = next_part->next;
    }

out:
    return current_item;
}



/**
 * @brief Get file descriptor for a file at path.
 * 
 * @param disk disk to read from.
 * @param path file path.
 * @param mode open mode.
 * @return void* file descriptor (struct fat_file_descriptor)
 */
void* fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode) {
    int err_code = 0;
    if (mode != FILE_MODE_READ) {
        err_code = -ERDONLY;
        goto err_out;
    }

    struct fat_file_descriptor* file_descriptor = kzalloc(sizeof(struct fat_file_descriptor));
    if (!file_descriptor) {
        err_code = -ENOMEM;
        goto err_out;
    }

    file_descriptor->item = fat16_get_directory_entry(disk, path);
    if (!file_descriptor->item) {
        err_code = -EIO;
        goto err_out;
    }

    file_descriptor->pos = 0;
    return file_descriptor;

err_out:
    if (file_descriptor) {
        kfree(file_descriptor);
    }

    return ERROR(err_code);
}