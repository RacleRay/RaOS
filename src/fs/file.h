#ifndef _FILE_H
#define _FILE_H

#include "pparser.h"
#include <stdint.h>

typedef unsigned int FILE_SEEK_MODE;

enum {
    SEEK_SET,
    SEEK_CUR,
    SEEK_END
};

typedef unsigned int FILE_MODE;

enum {
    FILE_MODE_READ,
    FILE_MODE_WRITE,
    FILE_MODE_APPEND,
    FILE_MODE_INVALID
};

typedef unsigned int FILE_STAT_FLAGS;

enum {
    FILE_STAT_READ_ONLY = 0b00000001,
    // ...
};

struct disk;

struct file_stat {
    FILE_STAT_FLAGS flags;
    uint32_t size;
};

typedef void*(*FS_OPEN_FUNCTION)(struct disk* disk, struct path_part* path, FILE_MODE mode);

// read nmemb * size bytes to out. fsprivate is actually the file descriptor struct object.
typedef int (*FS_READ_FUNCTION)(struct disk* disk, void* fsprivate, uint32_t size, uint32_t nmemb, char* out);

typedef int (*FS_RESOLVE_FUNCTION)(struct disk* disk);  // check if it`s valid fs

typedef int (*FS_SEEK_FUNCTION)(void* fsprivate, uint32_t offset, FILE_SEEK_MODE seek_mode);

typedef int (*FS_STAT_FUNCTION)(struct disk* disk, void* fsprivate, struct file_stat* stat);

typedef int (*FS_CLOSE_FUNCTION)(void* fsprivate);

struct filesystem {
    // return 0 if it`s valid fs.
    FS_RESOLVE_FUNCTION resolve;
    FS_OPEN_FUNCTION open;
    FS_READ_FUNCTION read;
    FS_SEEK_FUNCTION seek;
    FS_STAT_FUNCTION stat;
    FS_CLOSE_FUNCTION close;

    char name[20];
};

struct file_descriptor {
    int index;
    struct filesystem* filesystem;

    // for filesystem downstream process.
    void* private_;

    // Where file descriptor lie on.
    struct disk* disk;
};


void fs_init();
int fopen(const char* filename, const char* mode);
// read nmemb * size bytes to ptr.
int fread(void* ptr, uint32_t size, uint32_t nmemb, int fd);

int fseek(int fd, int offset, FILE_SEEK_MODE whence);

int fstat(int fd, struct file_stat *stat);

// Close the file descriptor and the fat file decriptor in internal FAT16 filesystem.
int fclose(int fd);

void fs_insert_filesystem(struct filesystem* filesystem);

struct filesystem* fs_resolve(struct disk* disk);

#endif