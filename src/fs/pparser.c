#include "pparser.h"
#include "../config.h"
#include "../kernel.h"
#include "../memory/heap/kheap.h"
#include "../memory/memory.h"
#include "../status.h"
#include "../string/string.h"


/**
 * @brief Check absolute path which begin with digits disk number followed by :/
 *
 * @param filename
 * @return int
 */
static int path_valid_format(const char* filename) {
    size_t len = strnlen(filename, RAOS_MAX_PATH);
    return (len >= 3 && isdigit(filename[0])
            && memcmp((void*)&filename[1], ":/", 2) == 0);
}

static int path_get_drive_by_path(const char** path) {
    if (!path_valid_format(*path)) {
        return -EBADPATH;
    }

    int drive_no = chtoi((*path)[0]);

    // add 3 bytes to skip drive number, e.g. 0:/
    *path += 3;  // change the path pointer outside.
    return drive_no;
}

static struct path_root* path_create_root(int driver_no) {
    struct path_root* root = kzalloc(sizeof(struct path_root));

    root->drive_no = driver_no;
    root->first    = NULL;

    return root;
}

static const char* path_get_path_part(const char** path) {
    char* path_part = kzalloc(RAOS_MAX_PATH);

    int i = 0;
    while (**path != '/' && **path != 0x00) {
        path_part[i] = **path;
        *path += 1;
        ++i;
    }

    if (**path == '/') {
        *path += 1;
    }

    if (i == 0) {
        kfree(path_part);
        path_part = NULL;
    }

    return path_part;
}

struct path_part* path_parse_path_part(struct path_part* last_part,
                                       const char**      path) {
    const char* path_part_str = path_get_path_part(path);

    if (NULL == path_part_str) {
        return NULL;
    }

    struct path_part* part = kzalloc(sizeof(struct path_part));
    part->part             = path_part_str;
    part->next             = NULL;

    // add to current last_part position.
    if (last_part) {
        last_part->next = part;
    }

    return part;
}

void path_free(struct path_root* root) {
    struct path_part* part = root->first;

    while (part) {
        struct path_part* next = part->next;
        kfree((void*)part->part);  // kmalloc at path_get_path_part
        kfree(part);
        part = next;
    }

    kfree(root);
}


struct path_root* path_parse(const char* path, const char* cur_directory_path) {
    int         drive_no = 0;
    const char* str_path = path;

    struct path_root* path_root = 0;

    if (strlen(path) > RAOS_MAX_PATH) {
        goto out;
    }

    drive_no = path_get_drive_by_path(&str_path);
    if (drive_no < 0) {
        goto out;
    }

    path_root = path_create_root(drive_no);
    if (!path_root) {
        goto out;
    }

    struct path_part* first_part = path_parse_path_part(NULL, &str_path);
    if (!first_part) {
        goto out;
    }

    path_root->first       = first_part;
    struct path_part* part = path_parse_path_part(first_part, &str_path);

    while (part) {
        part = path_parse_path_part(part, &str_path);
    }

out:
    return path_root;
}