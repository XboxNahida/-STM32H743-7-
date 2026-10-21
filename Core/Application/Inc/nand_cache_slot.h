#ifndef NAND_CACHE_SLOT_H
#define NAND_CACHE_SLOT_H

#include <stdint.h>

#include "nand_cache_layout.h"

#define NAND_CACHE_SLOT_PAGES_PER_BLOCK 64U
#define NAND_CACHE_SLOT_PAGE_COUNT \
    (NAND_CACHE_SLOT_BLOCKS * NAND_CACHE_SLOT_PAGES_PER_BLOCK)

typedef enum
{
    NAND_CACHE_SLOT_OK = 0,
    NAND_CACHE_SLOT_INVALID_ARG
} nand_cache_slot_result_t;

int nand_cache_slot_page_address(const nand_cache_layout_t *layout,
                                 uint8_t slot,
                                 uint32_t logical_page,
                                 uint32_t *physical_page);
int nand_cache_slot_metadata_page_address(const nand_cache_layout_t *layout,
                                          uint32_t copy,
                                          uint32_t *physical_page);

#endif
