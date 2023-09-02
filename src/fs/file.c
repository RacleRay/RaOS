#include "file.h"
#include "../config.h"
#include "../kernel.h"
#include "../string/string.h"
#include "../memory/memory.h"
#include "../memory/heap/kheap.h"
#include "../disk/disk.h"
#include "../string/string.h"
#include "../status.h"
#include "fat/fat16.h"


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
    fs_insert_filesystem(fat16_init());
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
        // resolve return 0, means filesystem i is binded to this disk.
        if (filesystems[i] != NULL && filesystems[i]->resolve(disk) == 0) {
            fs = filesystems[i];
            break;
        }
    }

    return fs;
}


static FILE_MODE file_get_mode_by_string(const char* str) {
    FILE_MODE mode = FILE_MODE_INVALID;

    if (strncmp(str, "r", 1) == 0) {
        mode = FILE_MODE_READ;
    } else if (strncmp(str, "w", 1) == 0) {
        mode = FILE_MODE_WRITE;
    } else if (strncmp(str, "a", 1) == 0) {
        mode = FILE_MODE_APPEND;
    }

    return mode;
}


int fopen(const char *filename, const char *mode_str) {
    int res = 0;

    struct path_root* root_path = path_parse(filename, NULL);
    if (!root_path) {
        res = -EINVARG;
        goto out;
    }

    // if only have a root path.
    if (!root_path->first) {
        res = -EINVARG;
        goto out;
    }

    // read exsiting disk
    struct disk* disk = disk_get(root_path->drive_no);
    if (!disk) {
        res = -EIO;
        goto out;
    }

    // check if there is a filesystem 
    if (!disk->filesystem) {
        res = -EIO;
        goto out;
    }

    FILE_MODE mode = file_get_mode_by_string(mode_str);
    if (mode == FILE_MODE_INVALID) {
        res = -EINVARG;
        goto out;
    }

    void* descriptor_private_data = disk->filesystem->open(disk, root_path->first, mode);
    if (ISERR(descriptor_private_data)) {
        res = ERROR_I(descriptor_private_data);
        goto out;
    }

    struct file_descriptor* desc = 0;
    res = file_new_descriptor(&desc);
    if (res < 0) {
        goto out;
    }

    desc->filesystem = disk->filesystem;
    desc->private_ = descriptor_private_data;
    desc->disk = disk;
    res = desc->index;  //file descriptor index

out:
    if (res < 0) {
        return 0;
    }

    return res;
}