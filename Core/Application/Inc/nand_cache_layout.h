#ifndef NAND_CACHE_LAYOUT_H
#define NAND_CACHE_LAYOUT_H

#include <stdint.h>

#define NAND_CACHE_SLOT_COUNT       2U
#define NAND_CACHE_SLOT_BLOCKS      192U
#define NAND_CACHE_METADATA_BLOCKS  4U

typedef enum
{
    NAND_CACHE_LAYOUT_OK = 0,
    NAND_CACHE_LAYOUT_INVALID_ARG,
    NAND_CACHE_LAYOUT_UNSUPPORTED_GEOMETRY,
    NAND_CACHE_LAYOUT_SCAN_ERROR,
    NAND_CACHE_LAYOUT_NO_SPACE
} nand_cache_layout_result_t;

typedef int (*nand_cache_bad_block_fn)(uint32_t block, void *context);

typedef struct
{
    uint32_t slot_blocks[NAND_CACHE_SLOT_COUNT][NAND_CACHE_SLOT_BLOCKS];
    uint32_t metadata_blocks[NAND_CACHE_METADATA_BLOCKS];
    uint32_t scanned_blocks;
    uint32_t good_blocks;
    uint32_t bad_blocks;
} nand_cache_layout_t;

int nand_cache_layout_build(uint32_t page_main_size,
                            uint32_t pages_per_block,
                            uint32_t block_count,
                            nand_cache_bad_block_fn is_bad,
                            void *context,
                            nand_cache_layout_t *layout);

#endif
