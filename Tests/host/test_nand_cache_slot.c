#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nand_cache_slot.h"

static int expect_slot_page(const char *name, const nand_cache_layout_t *layout,
                            uint8_t slot, uint32_t logical_page,
                            int expected_result, uint32_t expected_page)
{
    uint32_t actual = 0U;
    int result = nand_cache_slot_page_address(layout, slot, logical_page,
                                              &actual);

    if ((result == expected_result) &&
        ((result != NAND_CACHE_SLOT_OK) || (actual == expected_page)))
    {
        return 0;
    }
    printf("FAIL %s result=%d page=%lu\n", name, result,
           (unsigned long)actual);
    return 1;
}

static int expect_metadata_page(const char *name,
                                const nand_cache_layout_t *layout,
                                uint32_t copy, int expected_result,
                                uint32_t expected_page)
{
    uint32_t actual = 0U;
    int result = nand_cache_slot_metadata_page_address(layout, copy, &actual);

    if ((result == expected_result) &&
        ((result != NAND_CACHE_SLOT_OK) || (actual == expected_page)))
    {
        return 0;
    }
    printf("FAIL %s result=%d page=%lu\n", name, result,
           (unsigned long)actual);
    return 1;
}

int main(void)
{
    nand_cache_layout_t layout;
    uint32_t index;
    int failures = 0;

    memset(&layout, 0, sizeof(layout));
    for (index = 0U; index < NAND_CACHE_SLOT_BLOCKS; index++)
    {
        layout.slot_blocks[0][index] = 10U + index * 2U;
        layout.slot_blocks[1][index] = 500U + index * 3U;
    }
    for (index = 0U; index < NAND_CACHE_METADATA_BLOCKS; index++)
    {
        layout.metadata_blocks[index] = 2000U + index * 5U;
    }

    failures += expect_slot_page("slot_first", &layout, 0U, 0U,
                                 NAND_CACHE_SLOT_OK, 10U * 64U);
    failures += expect_slot_page("slot_block_end", &layout, 0U, 63U,
                                 NAND_CACHE_SLOT_OK, 10U * 64U + 63U);
    failures += expect_slot_page("slot_next_block", &layout, 0U, 64U,
                                 NAND_CACHE_SLOT_OK, 12U * 64U);
    failures += expect_slot_page(
        "slot_b_last", &layout, 1U, NAND_CACHE_SLOT_PAGE_COUNT - 1U,
        NAND_CACHE_SLOT_OK, (500U + 191U * 3U) * 64U + 63U);
    failures += expect_metadata_page("metadata_copy", &layout, 2U,
                                     NAND_CACHE_SLOT_OK, 2010U * 64U);

    failures += expect_slot_page("invalid_slot", &layout, 2U, 0U,
                                 NAND_CACHE_SLOT_INVALID_ARG, 0U);
    failures += expect_slot_page("invalid_page", &layout, 0U,
                                 NAND_CACHE_SLOT_PAGE_COUNT,
                                 NAND_CACHE_SLOT_INVALID_ARG, 0U);
    failures += expect_metadata_page("invalid_metadata_copy", &layout,
                                     NAND_CACHE_METADATA_BLOCKS,
                                     NAND_CACHE_SLOT_INVALID_ARG, 0U);
    failures += expect_slot_page("null_layout", NULL, 0U, 0U,
                                 NAND_CACHE_SLOT_INVALID_ARG, 0U);

    if (failures != 0)
    {
        printf("FAIL nand_cache_slot failures=%d\n", failures);
        return 1;
    }
    printf("PASS nand_cache_slot\n");
    return 0;
}
