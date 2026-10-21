#include "nand_cache_slot.h"

#include <stddef.h>

int nand_cache_slot_page_address(const nand_cache_layout_t *layout,
                                 uint8_t slot,
                                 uint32_t logical_page,
                                 uint32_t *physical_page)
{
    uint32_t block_index;
    uint32_t page_in_block;

    if ((layout == NULL) || (physical_page == NULL) ||
        (slot >= NAND_CACHE_SLOT_COUNT) ||
        (logical_page >= NAND_CACHE_SLOT_PAGE_COUNT))
    {
        return NAND_CACHE_SLOT_INVALID_ARG;
    }
    block_index = logical_page / NAND_CACHE_SLOT_PAGES_PER_BLOCK;
    page_in_block = logical_page % NAND_CACHE_SLOT_PAGES_PER_BLOCK;
    *physical_page =
        layout->slot_blocks[slot][block_index] *
        NAND_CACHE_SLOT_PAGES_PER_BLOCK + page_in_block;
    return NAND_CACHE_SLOT_OK;
}

int nand_cache_slot_metadata_page_address(const nand_cache_layout_t *layout,
                                          uint32_t copy,
                                          uint32_t *physical_page)
{
    if ((layout == NULL) || (physical_page == NULL) ||
        (copy >= NAND_CACHE_METADATA_BLOCKS))
    {
        return NAND_CACHE_SLOT_INVALID_ARG;
    }
    *physical_page = layout->metadata_blocks[copy] *
                     NAND_CACHE_SLOT_PAGES_PER_BLOCK;
    return NAND_CACHE_SLOT_OK;
}
