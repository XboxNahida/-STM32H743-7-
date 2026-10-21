#ifndef NAND_CACHE_METADATA_H
#define NAND_CACHE_METADATA_H

#include <stdint.h>

#define NAND_CACHE_METADATA_RECORD_SIZE 64U

typedef enum
{
    NAND_CACHE_METADATA_OK = 0,
    NAND_CACHE_METADATA_INVALID_ARG,
    NAND_CACHE_METADATA_FORMAT_ERROR,
    NAND_CACHE_METADATA_NOT_COMMITTED,
    NAND_CACHE_METADATA_CRC_ERROR,
    NAND_CACHE_METADATA_NO_VALID_RECORD
} nand_cache_metadata_result_t;

typedef struct
{
    uint8_t slot;
    uint32_t sequence;
    uint32_t data_length;
    uint32_t page_count;
    uint32_t data_crc;
} nand_cache_metadata_t;

int nand_cache_metadata_encode(const nand_cache_metadata_t *metadata,
                               uint8_t committed,
                               uint8_t *record,
                               uint32_t record_size);
int nand_cache_metadata_decode(const uint8_t *record,
                               uint32_t record_size,
                               nand_cache_metadata_t *metadata);
int nand_cache_metadata_select(const nand_cache_metadata_t *candidates,
                               const uint8_t *valid,
                               uint32_t count,
                               uint32_t *selected_index);

#endif
