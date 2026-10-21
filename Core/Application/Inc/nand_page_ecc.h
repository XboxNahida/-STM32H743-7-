#ifndef NAND_PAGE_ECC_H
#define NAND_PAGE_ECC_H

#include <stdint.h>

#define NAND_PAGE_ECC_DATA_SIZE 2048U

typedef enum
{
    NAND_PAGE_ECC_OK = 0,
    NAND_PAGE_ECC_CORRECTED,
    NAND_PAGE_ECC_UNCORRECTABLE,
    NAND_PAGE_ECC_INVALID_ARG
} nand_page_ecc_result_t;

int nand_page_ecc_encode(const uint8_t *data,
                         uint32_t length,
                         uint16_t *ecc);
int nand_page_ecc_check_and_correct(uint8_t *data,
                                    uint32_t length,
                                    uint16_t stored_ecc);

#endif
