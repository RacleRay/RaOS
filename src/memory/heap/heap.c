#include "heap.h"
#include "../../status.h"
#include "../memory.h"
#include <stdint.h>



/**
 * @brief check address alignment to RAOS_HEAP_BLOCK_SIZE.
 *
 * @param ptr
 * @return int
 */
static int heap_validate_alignment(void* ptr) {
    return ((unsigned int)ptr % RAOS_HEAP_BLOCK_SIZE) == 0;
}


/**
 * @brief check if the table has the right arguments satisfied the address
 * betweem ptr to end.
 *
 * @param ptr pointer to heap data pool
 * @param end  end pointer to heap data pool
 * @param table table structure to check.
 * @return int
 */
static int heap_validate_table(void* ptr, void* end, struct heap_table* table) {
    int res = 0;

    size_t table_size   = (size_t)(end - ptr);
    size_t total_blocks = table_size / RAOS_HEAP_BLOCK_SIZE;

    if (table->total != total_blocks) {
        res = -EINVARG;
        goto out;
    }

out:
    return res;
}

/**
 * @brief initialize the input heap.
 *
 * @param heap heap to be initialized
 * @param ptr  where heap start
 * @param end  where heap end
 * @param table providing a valid heap table.
 * @return int
 */
int heap_create(struct heap* heap, void* ptr, void* end,
                struct heap_table* table) {
    int res = 0;

    if (!heap_validate_alignment(ptr) || !heap_validate_alignment(end)) {
        res = -EINVARG;
        goto out;  // error handling in C.
    }

    memset(heap, 0, sizeof(struct heap));
    heap->saddr = ptr;
    heap->table = table;

    res = heap_validate_table(ptr, end, table);
    if (res < 0) {
        goto out;
    }

    // each entry byte represent a memory block.
    size_t table_size = sizeof(HEAP_BLOCK_TABLE_ENTRY) * table->total;
    memset(table->entries, HEAP_BLOCK_TABLE_ENTRY_FREE, table_size);

out:
    return res;
}


// ========================================================================

/**
 * @brief Round val to an integer multiple of 4096
 *
 * @param val bytes caller malloced
 * @return uint32_t
 */
static uint32_t heap_round_value_to_upper(uint32_t val) {
    if ((val % RAOS_HEAP_BLOCK_SIZE) == 0) {
        return val;
    }

    val = (val - (val % RAOS_HEAP_BLOCK_SIZE));  // lower bound
    return (val + RAOS_HEAP_BLOCK_SIZE);
}


// get the lower 4-bits entry type.
static uint8_t heap_get_entry_type(HEAP_BLOCK_TABLE_ENTRY entry) {
    return entry & 0x0f;
}


/**
 * @brief Get the first valid heap memory block.
 *
 * @param heap heap struct pointer
 * @param total_blocks total blocks to malloc.
 * @return int
 */
int heap_get_start_block(struct heap* heap, uint32_t total_blocks) {
    struct heap_table* table = heap->table;  // entry table

    int bc = 0;   // current block index
    int bs = -1;  // the start block index

    for (size_t i = 0; i < table->total; ++i) {
        if (heap_get_entry_type(table->entries[i])
            != HEAP_BLOCK_TABLE_ENTRY_FREE) {
            bc = 0;  // reset
            bs = -1;
            continue;
        }

        // the first block
        if (bs == -1) {
            bs = i;
        }
        bc++;
        if (bc == total_blocks) {
            break;
        }
    }

    if (bs == -1) {
        return -ENOMEM;
    }

    return bs;
}


/**
 * @brief Get the start address of block_idx block.
 *
 * @param heap
 * @param block_idx
 * @return void*
 */
void* heap_block_to_address(struct heap* heap, int block_idx) {
    return heap->saddr + (block_idx * RAOS_HEAP_BLOCK_SIZE);
}


void heap_mark_blocks_taken(struct heap* heap, int start_block,
                            int total_block) {
    int end_block = (start_block + total_block) - 1;

    HEAP_BLOCK_TABLE_ENTRY entry =
        HEAP_BLOCK_TABLE_ENTRY_TAKEN | HEAP_BLOCK_IS_FIRST;
    if (total_block > 1) {
        entry |= HEAP_BLOCK_HAS_NEXT;
    }

    for (int i = start_block; i <= end_block; ++i) {
        heap->table->entries[i] = entry;
        entry                   = HEAP_BLOCK_TABLE_ENTRY_TAKEN;
        if (i != end_block - 1) {  // will be set at next loop
            entry |= HEAP_BLOCK_HAS_NEXT;
        }
    }
}


/**
 * @brief Try to get total_blocks memory.
 *
 * @param heap
 * @param total_blocks number of blocks(4096 bytes each) we need.
 * @return void*
 */
void* heap_malloc_blocks(struct heap* heap, uint32_t total_blocks) {
    void* address = NULL;

    int start_block = heap_get_start_block(heap, total_blocks);
    if (start_block < 0) {
        goto out;
    }

    address = heap_block_to_address(heap, start_block);

    heap_mark_blocks_taken(heap, start_block, total_blocks);

out:
    return address;
}


/**
 * @brief Malloc size of bytes.
 *
 * @param heap heap structure.
 * @param size number of bytes
 * @return void* the start pointer of malloced memory.
 */
void* heap_malloc(struct heap* heap, size_t size) {
    size_t   aligned_size = heap_round_value_to_upper(size);
    uint32_t total_blocks = aligned_size / RAOS_HEAP_BLOCK_SIZE;
    return heap_malloc_blocks(heap, total_blocks);
}


/**
 * @brief Heap address to the corresponding block index.
 *
 * @param heap
 * @param address
 * @return int
 */
int heap_address_to_block(struct heap* heap, void* address) {
    return ((int)(address - heap->saddr)) / RAOS_HEAP_BLOCK_SIZE;
}


void heap_mark_block_free(struct heap* heap, int start_block) {
    struct heap_table* table = heap->table;

    for (int i = start_block; i < (int)table->total; ++i) {
        HEAP_BLOCK_TABLE_ENTRY entry = table->entries[i];
        table->entries[i]            = HEAP_BLOCK_TABLE_ENTRY_FREE;
        if (!(entry & HEAP_BLOCK_HAS_NEXT)) {
            break;
        }
    }
}


/**
 * @brief Free the serial blocks starting from ptr, which ended with 0x01 byte
 * entry.
 *
 * @param heap heap structure.
 * @param ptr starting address of memory block to free.
 */
void heap_free(struct heap* heap, void* ptr) {
    int start_block = heap_address_to_block(heap, ptr);
    heap_mark_block_free(heap, start_block);
}