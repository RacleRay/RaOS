#include "kheap.h"
#include "heap.h"
#include "../../kernel.h"
#include <stdint.h>

struct heap kernel_heap;
struct heap_table kernel_heap_table;

void kheap_init() {
  uint32_t total_table_entries = RAOS_HEAP_SIZE_BYTES / RAOS_HEAP_BLOCK_SIZE;

  //   kernel_heap_table.entries = (void *)0x00; // TODO: modify later
  kernel_heap_table.entries = (HEAP_BLOCK_TABLE_ENTRY *)(RAOS_HEAP_TABLE_ADDRESS);
  kernel_heap_table.total = total_table_entries;

  void* end = (void*)(RAOS_HEAP_ADDRESS + RAOS_HEAP_SIZE_BYTES);
  int res = head_create(&kernel_heap, (void*)(RAOS_HEAP_ADDRESS), end, &kernel_heap_table);
  
  if (res < 0) {
    print("Failed to create heap\n");
  }
}