#ifndef NAND_CACHE_PAGE_H
#define NAND_CACHE_PAGE_H

#include <stdint.h>

#define NAND_CACHE_PAGE_MAIN_SIZE  2048U
#define NAND_CACHE_PAGE_SPARE_SIZE 64U

typedef enum
{
    NAND_CACHE_PAGE_OK = 0,
    NAND_CACHE_PAGE_CORRECTED,
    NAND_CACHE_PAGE_INVALID_ARG,
    NAND_CACHE_PAGE_METADATA_ERROR,
    NAND_CACHE_PAGE_UNCORRECTABLE,
    NAND_CACHE_PAGE_CRC_ERROR
} nand_cache_page_result_t;

int nand_cache_page_encode(uint32_t logical_page,
                           const uint8_t *data,
                           uint32_t length,
                           uint8_t *main_area,
                           uint8_t *spare_area);
int nand_cache_page_decode(uint32_t expected_logical_page,
                           uint8_t *main_area,
                           const uint8_t *spare_area,
                           uint32_t *valid_length);

#endif
