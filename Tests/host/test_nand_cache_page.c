#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nand_cache_page.h"

int main(void)
{
    uint8_t input[777];
    uint8_t main_area[NAND_CACHE_PAGE_MAIN_SIZE];
    uint8_t spare[NAND_CACHE_PAGE_SPARE_SIZE];
    uint8_t original_main[NAND_CACHE_PAGE_MAIN_SIZE];
    uint32_t valid_length = 0U;
    uint32_t index;
    int failures = 0;

    for (index = 0U; index < sizeof(input); index++)
        input[index] = (uint8_t)(index * 19U);

    if ((nand_cache_page_encode(1234U, input, sizeof(input),
                                 main_area, spare) != NAND_CACHE_PAGE_OK) ||
        (spare[0] != 0xFFU) || (spare[1] != 0xFFU))
    {
        printf("FAIL encode_and_marker_reserve\n");
        failures++;
    }
    memcpy(original_main, main_area, sizeof(main_area));
    if ((memcmp(main_area, input, sizeof(input)) != 0) ||
        (main_area[sizeof(input)] != 0xFFU) ||
        (nand_cache_page_decode(1234U, main_area, spare, &valid_length) !=
         NAND_CACHE_PAGE_OK) ||
        (valid_length != sizeof(input)))
    {
        printf("FAIL clean_roundtrip\n");
        failures++;
    }

    memcpy(main_area, original_main, sizeof(main_area));
    main_area[500] ^= 0x08U;
    if ((nand_cache_page_decode(1234U, main_area, spare, &valid_length) !=
         NAND_CACHE_PAGE_CORRECTED) ||
        (memcmp(main_area, original_main, sizeof(main_area)) != 0))
    {
        printf("FAIL corrected_roundtrip\n");
        failures++;
    }

    memcpy(main_area, original_main, sizeof(main_area));
    main_area[10] ^= 0x01U;
    main_area[20] ^= 0x02U;
    if (nand_cache_page_decode(1234U, main_area, spare, &valid_length) !=
        NAND_CACHE_PAGE_UNCORRECTABLE)
    {
        printf("FAIL double_bit_rejected\n");
        failures++;
    }

    memcpy(main_area, original_main, sizeof(main_area));
    main_area[10] ^= 0x01U;
    main_area[20] ^= 0x02U;
    main_area[30] ^= 0x04U;
    if (nand_cache_page_decode(1234U, main_area, spare, &valid_length) ==
        NAND_CACHE_PAGE_OK)
    {
        printf("FAIL crc_rejects_complex_damage\n");
        failures++;
    }

    memcpy(main_area, original_main, sizeof(main_area));
    spare[6] ^= 0x80U;
    if (nand_cache_page_decode(1234U, main_area, spare, &valid_length) !=
        NAND_CACHE_PAGE_METADATA_ERROR)
    {
        printf("FAIL metadata_crc\n");
        failures++;
    }

    if ((nand_cache_page_encode(0U, input, 2049U, main_area, spare) !=
         NAND_CACHE_PAGE_INVALID_ARG) ||
        (nand_cache_page_encode(0U, NULL, 1U, main_area, spare) !=
         NAND_CACHE_PAGE_INVALID_ARG) ||
        (nand_cache_page_decode(0U, NULL, spare, &valid_length) !=
         NAND_CACHE_PAGE_INVALID_ARG))
    {
        printf("FAIL invalid_args\n");
        failures++;
    }

    if (failures != 0)
    {
        printf("FAIL nand_cache_page failures=%d\n", failures);
        return 1;
    }
    printf("PASS nand_cache_page\n");
    return 0;
}
