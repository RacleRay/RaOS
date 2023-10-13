#include "gdt.h"
#include "../kernel.h"


static void encode_gdt_entry(uint8_t*              gdt_entry,
                             struct gdt_structured source) {
    if ((source.limit > 65535) && ((source.limit & 0xFFF) != 0xFFF)) {
        panic("encode_gdt_entry: Invalid argument\n");
    }

    gdt_entry[6] = 0x40;
    if (source.limit > 65535) {
        source.limit = source.limit >> 12;
        gdt_entry[6]    = 0xC0;
    }

    // Encodes the limit
    gdt_entry[0] = source.limit & 0xFF;
    gdt_entry[1] = (source.limit >> 8) & 0xFF;
    gdt_entry[6] |= (source.limit >> 16) & 0x0F;

    // Encode the base
    gdt_entry[2] = source.base & 0xFF;
    gdt_entry[3] = (source.base >> 8) & 0xFF;
    gdt_entry[4] = (source.base >> 16) & 0xFF;
    gdt_entry[7] = (source.base >> 24) & 0xFF;

    // Set the type
    gdt_entry[5] = source.type;
}


/**
 * @brief Encode the gdt structure to a kernel readable gdt format structure.
 *
 * @param gdt
 * @param structured_gdt
 * @param total_entires
 */
void gdt_structured_to_gdt(struct gdt*            gdt,
                           struct gdt_structured* structured_gdt,
                           int                    total_entires) {
    for (int i = 0; i < total_entires; i++) {
        encode_gdt_entry((uint8_t*)&gdt[i], structured_gdt[i]);
    }
}