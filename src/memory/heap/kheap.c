#include "kheap.h"
#include "../../kernel.h"
#include "../memory.h"
#include "heap.h"

struct heap kernel_heap;
struct heap_table kernel_heap_table;

void kheap_init() {
  uint32_t total_table_entries = RAOS_HEAP_SIZE_BYTES / RAOS_HEAP_BLOCK_SIZE;

  //   kernel_heap_table.entries = (void *)0x00; // TODO: modify later
  kernel_heap_table.entries =
      (HEAP_BLOCK_TABLE_ENTRY *)(RAOS_HEAP_TABLE_ADDRESS);
  kernel_heap_table.total = total_table_entries;

  void *end = (void *)(RAOS_HEAP_ADDRESS + RAOS_HEAP_SIZE_BYTES);
  int res = heap_create(&kernel_heap, (void *)(RAOS_HEAP_ADDRESS), end,
                        &kernel_heap_table);

  if (res < 0) {
    print("Failed to create heap\n");
  }
}

void *kmalloc(size_t size) { return heap_malloc(&kernel_heap, size); }

void kfree(void *ptr) { heap_free(&kernel_heap, ptr); }

void *kzalloc(size_t size) {
  void *ptr = kmalloc(size);
  if (NULL == ptr) {
    return NULL;
  }
  memset(ptr, 0x00, size);
  return ptr;
}