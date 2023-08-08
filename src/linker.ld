ENTRY(_start)
OUTPUT_FORMAT(binary)

SECTIONS 
{
    /* start at 0x100000 */
    . = 1M;
    .text : 
    {
        *(.text)
    }

    .rodata :
    {
        *(.rodata)
    }

    .data :
    {
        *(.data)
    }

    .bss :
    {
        *(COMMON)
        *(.bss)

    }
}