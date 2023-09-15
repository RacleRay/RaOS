#ifndef _DISK_H
#define _DISK_H

#include "../fs/file.h"

typedef unsigned int RAOS_DISK_TYPE;

// Represent a real physical hard disk
#define RAOS_DISK_TYPE_REAL 0

struct disk {
    RAOS_DISK_TYPE type;
    int            sector_size;
    int            id;

    struct filesystem* filesystem;
    void*              fs_private;  // used for internel interpratation
};

struct disk* disk_get(int index);

void disk_search_and_init();
int disk_read_block(struct disk* idisk, unsigned int lba, int total, void* buf);

#endif