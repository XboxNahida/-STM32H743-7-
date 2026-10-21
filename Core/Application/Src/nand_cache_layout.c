#include "nand_cache_layout.h"

#include <string.h>

#define NAND_CACHE_REQUIRED_GOOD_BLOCKS \
    ((NAND_CACHE_SLOT_COUNT * NAND_CACHE_SLOT_BLOCKS) + \
     NAND_CACHE_METADATA_BLOCKS)

int nand_cache_layout_build(uint32_t page_main_size,
                            uint32_t pages_per_block,
                            uint32_t block_count,
                            nand_cache_bad_block_fn is_bad,
                            void *context,
                            nand_cache_layout_t *layout)
{
    uint32_t block;
    uint32_t good_index = 0U;

    if ((is_bad == NULL) || (layout == NULL))
    {
        return NAND_CACHE_LAYOUT_INVALID_ARG;
    }
    if ((page_main_size != 2048U) || (pages_per_block != 64U))
    {
        return NAND_CACHE_LAYOUT_UNSUPPORTED_GEOMETRY;
    }

    memset(layout, 0, sizeof(*layout));
    for (block = 0U; block < block_count; block++)
    {
        int bad_result = is_bad(block, context);

        layout->scanned_blocks = block + 1U;
        if (bad_result < 0)
        {
            return NAND_CACHE_LAYOUT_SCAN_ERROR;
        }
        if (bad_result != 0)
        {
            layout->bad_blocks++;
            continue;
        }

        if (good_index < NAND_CACHE_SLOT_BLOCKS)
        {
            layout->slot_blocks[0][good_index] = block;
        }
        else if (good_index <
                 (NAND_CACHE_SLOT_COUNT * NAND_CACHE_SLOT_BLOCKS))
        {
            layout->slot_blocks[1][good_index - NAND_CACHE_SLOT_BLOCKS] =
                block;
        }
        else
        {
            layout->metadata_blocks[
                good_index -
                (NAND_CACHE_SLOT_COUNT * NAND_CACHE_SLOT_BLOCKS)] = block;
        }

        good_index++;
        layout->good_blocks = good_index;
        if (good_index == NAND_CACHE_REQUIRED_GOOD_BLOCKS)
        {
            return NAND_CACHE_LAYOUT_OK;
        }
    }

    return NAND_CACHE_LAYOUT_NO_SPACE;
}
