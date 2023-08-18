#ifndef _PPARSER_H
#define _PPARSER_H

// root path
struct path_root {
    int               drive_no;
    struct path_part* first;
};

// sub parts of file path
struct path_part {
    const char*       part;
    struct path_part* next;
};


struct path_root* path_parse(const char* path, const char* cur_directory_path);

void path_free(struct path_root* root);

#endif