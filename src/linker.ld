ENTRY(_start)
OUTPUT_FORMAT(binary)

SECTIONS 
{
    /* start at 0x100000 */
    . = 1M;
    .text : ALIGN(4096)
    {
        *(.text)
    }

    .rodata : ALIGN(4096)
    {
        *(.rodata)
    }

    .data : ALIGN(4096)
    {
        *(.data)
    }

    .bss : ALIGN(4096)
    {
        *(COMMON)
        *(.bss)
    }

    /* Add at the end, in case of damaging the C objects alignment structure */
    .asm : ALIGN(4096)
    {
        *(.asm)
    }
}