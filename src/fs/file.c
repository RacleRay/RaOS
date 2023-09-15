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


/**
 * @brief Insert a default FAT16 file system into filesystems global array.
 * 
 */
static void fs_static_load() {
    fs_insert_filesystem(fat16_init());
}


/**
 * @brief New a file descriptor, it will be inserted into filedescriptors global array.
 *        The index of the file descriptor will be the smallest value avilaible.
 * 
 * @param desc_out The file descriptor will be stored in this pointer.
 * @return int 0 if success, otherwise return error code.
 */
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


// Get the file descriptor by index.
static struct file_descriptor* file_get_descriptor(int fd) {
    if (fd <= 0 || fd >= RAOS_MAX_FILE_DESCRIPTORS) {
        return 0;
    }

    return filedescriptors[fd - 1];  // descriptor start at 1
}

/**
 * @brief Insert a file system into filesystems global array.
 * 
 * @param filesystem 
 */
void fs_insert_filesystem(struct filesystem *filesystem) {
    struct filesystem** fs;
    fs = fs_get_free_filesystem();
    if (fs == NULL) {
        // will be repalced by panic later.
        print("fs_insert_filesystem(): no free filesystem avaliable.");
        while (1) {}
    }

    *fs = filesystem;
}


/**
 * @brief Insert a default FAT16 file system into filesystems global array.
 * 
 */
static void fs_load() {
    memset(filesystems, 0, sizeof(filesystems));
    fs_static_load();
}

/**
 * @brief Insert a default FAT16 file system into filesystems global array.
 * 
 */
void fs_init() {
    memset(filedescriptors, 0, sizeof(filedescriptors));
    fs_load();
}


/**
 * @brief This function will invoke the filesystem's resolve function, e.g. fat16_resolve.
          After this function, the filesystem will be binded to the disk.
          We can read write files in filesystem which actually managed by the fat16 filesystem 
          on the really disk.
 * 
 * @param disk disk to resolve.
 * @return struct filesystem* 
 */
struct filesystem* fs_resolve(struct disk* disk) {
    struct filesystem* fs = NULL;

    for (int i = 0; i < RAOS_MAX_FILESYSTEMS; ++i) {
        // resolve return 0, means filesystem i is binded to this disk.
        if (filesystems[i] != NULL && filesystems[i]->resolve(disk) == 0) {
            fs = filesystems[i];
            break;
        }
    }

    return fs;
}


/**
 * @brief Get the file mode.
 * 
 * @param str 
 * @return FILE_MODE 
 */
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

/**
 * @brief This function will parse the filename and mode_str. In the function, FAT16 
 *        will be resolved on the disk.
 *        The file descriptor structure will be filled as follows:
 *            - filesystem: the FAT16 filesystem on the disk.
 *            - private_: the private data of the filesystem.
 * 
 * @param filename 
 * @param mode_str "r", "w", "a"
 * @return int the opened file descriptor number.
 */
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

    // Get the FAT16 file descriptor. Not visible for users.
    // Containing FAT16 item and position(offset) of first item.
    void* descriptor_private_data = disk->filesystem->open(disk, root_path->first, mode);
    if (ISERR(descriptor_private_data)) {
        res = ERROR_I(descriptor_private_data);
        goto out;
    }

    struct file_descriptor* desc = NULL;
    res = file_new_descriptor(&desc);
    if (res < 0) {
        goto out;
    }

    // these things will used in filesystem's read/write function.
    // Used in fat16_read functions.
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


/**
 * @brief read nmemb * size bytes to ptr. It will invoke the FAT16 filesystem's read function.
 * 
 * @param ptr contents out pointer.
 * @param size member size.
 * @param nmemb number of members to read.
 * @param fd target file descriptor to read.
 * @return int number of bytes read.
 */
int fread(void *ptr, uint32_t size, uint32_t nmemb, int fd) {
    int res = 0;
    if (size == 0 || nmemb == 0 || fd < 1) {
        res = -EINVARG;
        goto out;
    }

    // file_descriptor is contructed by fat16_open.
    struct file_descriptor* desc = file_get_descriptor(fd);
    if (!desc) {
        res = -EINVARG;
        goto out;
    }

    // in this case, private_ is the file_descriptor struct.
    res = desc->filesystem->read(desc->disk, desc->private_, size, nmemb, (char *)ptr);

out:
    return res;
}


int fseek(int fd, int offset, FILE_SEEK_MODE whence) {
    int res = 0;

    struct file_descriptor* desc = file_get_descriptor(fd);
    if (!desc) {
        res = -EIO;
        goto out;
    }

    // This will change the file_descriptor's pos member that is the offset of content.
    res = desc->filesystem->seek(desc->private_, offset, whence);

out:
    return res;
}