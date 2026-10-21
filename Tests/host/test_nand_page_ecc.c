#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nand_page_ecc.h"

static void fill_page(uint8_t *page)
{
    uint32_t index;

    for (index = 0U; index < NAND_PAGE_ECC_DATA_SIZE; index++)
    {
        page[index] = (uint8_t)((index * 37U) ^ (index >> 3));
    }
}

int main(void)
{
    uint8_t original[NAND_PAGE_ECC_DATA_SIZE];
    uint8_t working[NAND_PAGE_ECC_DATA_SIZE];
    uint16_t ecc = 0U;
    int failures = 0;

    fill_page(original);
    memcpy(working, original, sizeof(working));
    if ((nand_page_ecc_encode(original, sizeof(original), &ecc) !=
         NAND_PAGE_ECC_OK) ||
        (nand_page_ecc_check_and_correct(working, sizeof(working), ecc) !=
         NAND_PAGE_ECC_OK) ||
        (memcmp(working, original, sizeof(working)) != 0))
    {
        printf("FAIL clean_page\n");
        failures++;
    }

    memcpy(working, original, sizeof(working));
    working[0] ^= 0x01U;
    if ((nand_page_ecc_check_and_correct(working, sizeof(working), ecc) !=
         NAND_PAGE_ECC_CORRECTED) ||
        (memcmp(working, original, sizeof(working)) != 0))
    {
        printf("FAIL correct_first_bit\n");
        failures++;
    }

    memcpy(working, original, sizeof(working));
    working[1023] ^= 0x20U;
    if ((nand_page_ecc_check_and_correct(working, sizeof(working), ecc) !=
         NAND_PAGE_ECC_CORRECTED) ||
        (memcmp(working, original, sizeof(working)) != 0))
    {
        printf("FAIL correct_middle_bit\n");
        failures++;
    }

    memcpy(working, original, sizeof(working));
    working[2047] ^= 0x80U;
    if ((nand_page_ecc_check_and_correct(working, sizeof(working), ecc) !=
         NAND_PAGE_ECC_CORRECTED) ||
        (memcmp(working, original, sizeof(working)) != 0))
    {
        printf("FAIL correct_last_bit\n");
        failures++;
    }

    memcpy(working, original, sizeof(working));
    working[10] ^= 0x04U;
    working[900] ^= 0x40U;
    if (nand_page_ecc_check_and_correct(working, sizeof(working), ecc) !=
        NAND_PAGE_ECC_UNCORRECTABLE)
    {
        printf("FAIL detect_double_bit\n");
        failures++;
    }

    memcpy(working, original, sizeof(working));
    if ((nand_page_ecc_check_and_correct(working, sizeof(working),
                                          (uint16_t)(ecc ^ 0x0001U)) !=
         NAND_PAGE_ECC_CORRECTED) ||
        (memcmp(working, original, sizeof(working)) != 0))
    {
        printf("FAIL parity_bit_error\n");
        failures++;
    }

    memcpy(working, original, sizeof(working));
    if ((nand_page_ecc_check_and_correct(working, sizeof(working),
                                          (uint16_t)(ecc ^ 0x8000U)) !=
         NAND_PAGE_ECC_CORRECTED) ||
        (memcmp(working, original, sizeof(working)) != 0))
    {
        printf("FAIL overall_bit_error\n");
        failures++;
    }

    if ((nand_page_ecc_encode(NULL, sizeof(original), &ecc) !=
         NAND_PAGE_ECC_INVALID_ARG) ||
        (nand_page_ecc_encode(original, sizeof(original) - 1U, &ecc) !=
         NAND_PAGE_ECC_INVALID_ARG) ||
        (nand_page_ecc_encode(original, sizeof(original), NULL) !=
         NAND_PAGE_ECC_INVALID_ARG) ||
        (nand_page_ecc_check_and_correct(NULL, sizeof(original), ecc) !=
         NAND_PAGE_ECC_INVALID_ARG))
    {
        printf("FAIL invalid_args\n");
        failures++;
    }

    if (failures != 0)
    {
        printf("FAIL nand_page_ecc failures=%d\n", failures);
        return 1;
    }
    printf("PASS nand_page_ecc\n");
    return 0;
}
