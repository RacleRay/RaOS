#ifndef _CONFIG_H
#define _CONFIG_H

#define RAOS_TOTAL_INTERRUPTS 512

// define in kernel.asm CODE_SEG, DATA_SEG
#define KERNEL_CODE_SELECTOR 0x08
#define KERNEL_DATA_SELECTOR 0x10

// Naive implementation. 100M size of kernel heap.
#define RAOS_HEAP_SIZE_BYTES 104857600
#define RAOS_HEAP_BLOCK_SIZE 4096

#define RAOS_HEAP_ADDRESS 0x01000000
#define RAOS_HEAP_TABLE_ADDRESS 0x00007E00

#define RAOS_SECTOR_SIZE 512

#define RAOS_MAX_FILESYSTEMS 16
#define RAOS_MAX_FILE_DESCRIPTORS 512

#define RAOS_MAX_PATH 128

#endif