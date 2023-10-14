#include "paging.h"
#include "../../status.h"
#include "../heap/kheap.h"


static uint32_t* current_directory = 0;

// function prototype, implement in paging.asm
void paging_load_directory(uint32_t* directory);


/**
 * @brief Get 4GB chunk of page table with flags.
 *
 * @param flags
 * @return struct paging_4gb_chunk*
 */
struct paging_4gb_chunk* paging_new_4gb(uint8_t flags) {
    uint32_t* directory =
        (uint32_t*)kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);

    uint32_t offset = 0;
    for (int i = 0; i < PAGING_TOTAL_ENTRIES_PER_TABLE; ++i) {
        uint32_t* entry = (uint32_t*)kzalloc(sizeof(uint32_t)
                                             * PAGING_TOTAL_ENTRIES_PER_TABLE);
        for (int pi = 0; pi < PAGING_TOTAL_ENTRIES_PER_TABLE; ++pi) {
            entry[pi] = (offset + (pi * PAGING_PAGE_SIZE)) | flags;
        }
        offset += (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE);
        // Tag directory is writable.
        directory[i] = (uint32_t)entry | flags | PAGING_IS_WRITABLE;
    }

    struct paging_4gb_chunk* chunk_4gb =
        (struct paging_4gb_chunk*)kzalloc(sizeof(struct paging_4gb_chunk));
    chunk_4gb->directory_entry = directory;

    return chunk_4gb;
}

/**
 * @brief load page directory
 *
 * @param directory
 */
void paging_switch(struct paging_4gb_chunk* directory) {
    paging_load_directory(directory->directory_entry);
    current_directory = directory->directory_entry;
}

uint32_t* paging_4gb_chunk_get_directory(struct paging_4gb_chunk* chunk) {
    return chunk->directory_entry;
}

bool paging_is_aligned(void* addr) {
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
int paging_get_index(void* virtual_address, uint32_t* directory_idx_out,
                     uint32_t* table_idx_out) {
    int res = 0;

    // invalid page address
    if (!paging_is_aligned(virtual_address)) {
        res = -EINVARG;
        goto out;
    }

    *directory_idx_out =
        ((uint32_t)virtual_address
         / (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE));
    *table_idx_out = (((uint32_t)virtual_address
                       % (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE))
                      / PAGING_PAGE_SIZE);

out:
    return res;
}


uint32_t paging_get(uint32_t* directory, void* virt) {
    uint32_t directory_index = 0;
    uint32_t table_index     = 0;
    paging_get_index(virt, &directory_index, &table_index);

    uint32_t  entry = directory[directory_index];
    uint32_t* table = (uint32_t*)(entry & 0xfffff000);
    return table[table_index];
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
    uint32_t table_idx     = 0;
    int      res           = paging_get_index(virt, &directory_idx, &table_idx);
    if (res < 0) {
        return res;
    }

    uint32_t  entry = directory[directory_idx];
    uint32_t* table = (uint32_t*)(entry & 0xfffff000);  // aligned to 4096

    table[table_idx] = physic;

    return 0;
}


void paging_free_4gb(struct paging_4gb_chunk* chunk) {
    for (int i = 0; i < 1024; i++) {
        uint32_t entry = chunk->directory_entry[i];
        // the lower 000 is flags
        uint32_t* table = (uint32_t*)(entry & 0xfffff000);
        kfree(table);
    }

    kfree(chunk->directory_entry);
    kfree(chunk);
}


// map the virtual address to physical address
int paging_map(struct paging_4gb_chunk* directory, void* virt, void* phys,
               int flags) {
    if (((unsigned int)virt % PAGING_PAGE_SIZE)
        || ((unsigned int)phys % PAGING_PAGE_SIZE)) {
        return -EINVARG;
    }

    return paging_set(directory->directory_entry, virt, (uint32_t)phys | flags);
}


int paging_map_range(struct paging_4gb_chunk* directory, void* virt, void* phys,
                     int count, int flags) {
    int res = 0;
    for (int i = 0; i < count; i++) {
        res = paging_map(directory, virt, phys, flags);
        if (res < 0) break;
        virt += PAGING_PAGE_SIZE;
        phys += PAGING_PAGE_SIZE;
    }

    return res;
}


// map the virtual addresses to physical addresses
int paging_map_to(struct paging_4gb_chunk* directory, void* virt, void* phys,
                  void* phys_end, int flags) {
    int res = 0;
    if ((uint32_t)virt % PAGING_PAGE_SIZE) {
        res = -EINVARG;
        goto out;
    }
    if ((uint32_t)phys % PAGING_PAGE_SIZE) {
        res = -EINVARG;
        goto out;
    }
    if ((uint32_t)phys_end % PAGING_PAGE_SIZE) {
        res = -EINVARG;
        goto out;
    }

    if ((uint32_t)phys_end < (uint32_t)phys) {
        res = -EINVARG;
        goto out;
    }

    uint32_t total_bytes = phys_end - phys;
    int      total_pages = total_bytes / PAGING_PAGE_SIZE;
    res = paging_map_range(directory, virt, phys, total_pages, flags);
out:
    return res;
}


void* paging_align_address(void* ptr) {
    if ((uint32_t)ptr % PAGING_PAGE_SIZE) {
        return (void*)((uint32_t)ptr + PAGING_PAGE_SIZE
                       - ((uint32_t)ptr % PAGING_PAGE_SIZE));
    }

    return ptr;
}


void* paging_align_to_lower_page(void* addr) {
    uint32_t _addr = (uint32_t)addr;
    _addr -= (_addr % PAGING_PAGE_SIZE);
    return (void*)_addr;
}


void* paging_get_physical_address(uint32_t* directory, void* virt) {
    void* virt_addr_new = (void*)paging_align_to_lower_page(virt);
    void* difference    = (void*)((uint32_t)virt - (uint32_t)virt_addr_new);
    return (void*)((paging_get(directory, virt_addr_new) & 0xfffff000)
                   + difference);
}
