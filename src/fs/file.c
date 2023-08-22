#include "file.h"
#include "../config.h"
#include "../string/string.h"
#include "../memory/memory.h"
#include "../memory/heap/kheap.h"
#include "../status.h"


struct filesystem* filesystems[RAOS_MAX_FILESYSTEMS];
struct file_descriptor* filedescriptors[RAOS_MAX_FILE_DESCRIPTORS];

extern void print(const char *str);


/**
 * @brief get free file system.
 * 
 * @return struct filesystem**  if no fs avaliable, return 0
 */
static struct filesystem** fs_get_free_filesystem() {
    for (int i = 0; i < RAOS_MAX_FILESYSTEMS; ++i) {
        if (filesystems[i] == NULL) {
            return &filesystems[i];
        }
    }

    return 0;
}


static void fs_static_load() {
    // fs_insert_filesystem(fat_init());
}


static int file_new_descriptor(struct file_descriptor** desc_out) {
    int res = -ENOMEM;
    for (int i = 0; i < RAOS_MAX_FILE_DESCRIPTORS; ++i) {
        if (filedescriptors[i] == 0) {
            struct file_descriptor* desc = kzalloc(sizeof(struct file_descriptor));
            desc->index = i + 1;  // descriptor start at 1
            filedescriptors[i] = desc;
            *desc_out = desc;
            res = 0;
            break;
        }
    }

    return res;
}


static struct file_descriptor* file_get_descriptor(int fd) {
    if (fd <= 0 || fd >= RAOS_MAX_FILE_DESCRIPTORS) {
        return 0;
    }

    return filedescriptors[fd - 1];  // descriptor start at 1
}


void fs_insert_filesystem(struct filesystem *filesystem) {
    struct filesystem** fs;
    fs = fs_get_free_filesystem();
    if (fs == 0) {
        // will be repalced by panic later.
        print("fs_insert_filesystem ERROR.");
        while (1) {}
    }

    *fs = filesystem;
}


void fs_load() {
    memset(filesystems, 0, sizeof(filesystems));
    fs_static_load();
}


void fs_init() {
    memset(filedescriptors, 0, sizeof(filedescriptors));
    fs_load();
}


struct filesystem* fs_resolve(struct disk* disk) {
    struct filesystem* fs = 0;

    for (int i = 0; i < RAOS_MAX_FILESYSTEMS; ++i) {
        if (filesystems[i] != NULL && filesystems[i]->resolve(disk) == 0) {
            fs = filesystems[i];
            break;
        }
    }

    return fs;
}


int fopen(const char *filename, const char *mode) {
    return -EIO;
}