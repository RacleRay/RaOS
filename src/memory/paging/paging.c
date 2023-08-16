#include "paging.h"
#include "../../status.h"
#include "../heap/kheap.h"


static uint32_t *current_directory = 0;

// function prototype, implement in paging.asm
void paging_load_directory(uint32_t *directory);

/**
 * @brief Get 4GB chunk of page table with flags.
 *
 * @param flags
 * @return struct paging_4gb_chunk*
 */
struct paging_4gb_chunk *paging_new_4gb(uint8_t flags) {
  uint32_t *directory =
      (uint32_t *)kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);

  uint32_t offset = 0;
  for (int i = 0; i < PAGING_TOTAL_ENTRIES_PER_TABLE; ++i) {
    uint32_t *entry =
        (uint32_t *)kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);
    for (int pi = 0; pi < PAGING_TOTAL_ENTRIES_PER_TABLE; ++pi) {
      // TODO: modify this to virtual memory mapping.
      entry[pi] = (offset + (pi * PAGING_PAGE_SIZE)) | flags;
    }
    offset += (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE);
    // Tag directory is writable.
    directory[i] = (uint32_t)entry | flags | PAGING_IS_WRITABLE;
  }

  struct paging_4gb_chunk *chunk_4gb =
      (struct paging_4gb_chunk *)kzalloc(sizeof(struct paging_4gb_chunk));
  chunk_4gb->directory_entry = directory;

  return chunk_4gb;
}

/**
 * @brief load page directory
 *
 * @param directory
 */
void paging_switch(uint32_t *directory) {
  paging_load_directory(directory);
  current_directory = directory;
}

uint32_t *paging_4gb_chunk_get_directory(struct paging_4gb_chunk *chunk) {
  return chunk->directory_entry;
}

bool paging_is_aligned(void *addr) {
  return ((uint32_t)addr % PAGING_PAGE_SIZE) == 0;
}

/**
 * @brief Get the directory index and table index of virtual_address.
 * 
 * @param virtual_address 
 * @param directory_idx_out the directory index in directory table
 * @param table_idx_out the index in page entry table
 * @return int 
 */
int paging_get_index(void *virtual_address, uint32_t *directory_idx_out,
                     uint32_t *table_idx_out) {
  int res = 0;

  // invalid page address
  if (!paging_is_aligned(virtual_address)) {
    res = -EINVARG;
    goto out;
  }

  *directory_idx_out = ((uint32_t)virtual_address /
                        (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE));
  *table_idx_out = (((uint32_t)virtual_address %
                     (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE)) /
                    PAGING_PAGE_SIZE);

out:
  return res;
}

/**
 * @brief Mapping virtual address with physical address.
 * 
 * @param directory page directory to use.
 * @param virt virtual address
 * @param physic physical address to be mapped.
 * @return int 
 */
int paging_set(uint32_t* directory, void* virt, uint32_t physic) {
    if (!paging_is_aligned(virt)) {
        return -EINVARG;
    }

    uint32_t directory_idx = 0;
    uint32_t table_idx = 0;
    int res = paging_get_index(virt, &directory_idx, &table_idx);
    if (res < 0) {
        return res;
    }

    uint32_t entry = directory[directory_idx];
    uint32_t* table = (uint32_t*)(entry & 0xfffff000);  // aligned to 4096

    table[table_idx] = physic;

    return 0;
}