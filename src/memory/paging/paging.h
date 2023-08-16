#ifndef _PAGING_H
#define _PAGING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// https://wiki.osdev.org/Paging
#define PAGING_CACHE_DISABLED 0b00010000
#define PAGING_WRITE_THROUGH 0b00001000
#define PAGING_ACCESS_FROM_ALL 0b00000100
#define PAGING_IS_WRITABLE 0b00000010
#define PAGING_IS_PRESENT 0b00000001

#define PAGING_TOTAL_ENTRIES_PER_TABLE 1024
#define PAGING_PAGE_SIZE 4096

struct paging_4gb_chunk {
  uint32_t *directory_entry;
};

void enable_paging();
void paging_switch(uint32_t *directory);

struct paging_4gb_chunk *paging_new_4gb(uint8_t flags);
uint32_t *paging_4gb_chunk_get_directory(struct paging_4gb_chunk *chunk);

int paging_get_index(void *virtual_address, uint32_t *directory_idx_out,
                     uint32_t *table_idx_out);
int paging_set(uint32_t *directory, void *virt, uint32_t physic);

#endif