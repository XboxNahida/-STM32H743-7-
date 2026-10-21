#include "nand_page_ecc.h"

#include <stddef.h>

static uint8_t nand_page_ecc_is_power_of_two(uint32_t value)
{
    return ((value != 0U) && ((value & (value - 1U)) == 0U)) ? 1U : 0U;
}

static uint8_t nand_page_ecc_parity16(uint16_t value)
{
    uint8_t parity = 0U;

    while (value != 0U)
    {
        parity ^= (uint8_t)(value & 1U);
        value >>= 1;
    }
    return parity;
}

static void nand_page_ecc_calculate(const uint8_t *data,
                                    uint16_t *position_parity,
                                    uint8_t *data_parity)
{
    uint32_t data_bit;
    uint32_t code_position = 1U;
    uint16_t positions = 0U;
    uint8_t parity = 0U;

    for (data_bit = 0U;
         data_bit < (NAND_PAGE_ECC_DATA_SIZE * 8U);
         data_bit++)
    {
        while (nand_page_ecc_is_power_of_two(code_position) != 0U)
        {
            code_position++;
        }
        if ((data[data_bit >> 3] &
             (uint8_t)(1U << (data_bit & 7U))) != 0U)
        {
            positions ^= (uint16_t)code_position;
            parity ^= 1U;
        }
        code_position++;
    }

    *position_parity = positions;
    *data_parity = parity;
}

int nand_page_ecc_encode(const uint8_t *data,
                         uint32_t length,
                         uint16_t *ecc)
{
    uint16_t position_parity;
    uint8_t data_parity;
    uint8_t overall;

    if ((data == NULL) || (ecc == NULL) ||
        (length != NAND_PAGE_ECC_DATA_SIZE))
    {
        return NAND_PAGE_ECC_INVALID_ARG;
    }

    nand_page_ecc_calculate(data, &position_parity, &data_parity);
    overall = data_parity ^ nand_page_ecc_parity16(position_parity);
    *ecc = (uint16_t)(position_parity | ((uint16_t)overall << 15));
    return NAND_PAGE_ECC_OK;
}

int nand_page_ecc_check_and_correct(uint8_t *data,
                                    uint32_t length,
                                    uint16_t stored_ecc)
{
    uint16_t calculated_positions;
    uint16_t stored_positions;
    uint16_t syndrome;
    uint8_t calculated_data_parity;
    uint8_t overall_check;
    uint32_t parity_positions;
    uint32_t power;
    uint32_t data_bit;

    if ((data == NULL) || (length != NAND_PAGE_ECC_DATA_SIZE))
    {
        return NAND_PAGE_ECC_INVALID_ARG;
    }

    nand_page_ecc_calculate(data, &calculated_positions,
                            &calculated_data_parity);
    stored_positions = (uint16_t)(stored_ecc & 0x7FFFU);
    syndrome = (uint16_t)(stored_positions ^ calculated_positions);
    overall_check = calculated_data_parity ^
                    nand_page_ecc_parity16(stored_positions) ^
                    (uint8_t)(stored_ecc >> 15);

    if (syndrome == 0U)
    {
        return (overall_check == 0U) ?
               NAND_PAGE_ECC_OK : NAND_PAGE_ECC_CORRECTED;
    }
    if (overall_check == 0U)
    {
        return NAND_PAGE_ECC_UNCORRECTABLE;
    }
    if (nand_page_ecc_is_power_of_two(syndrome) != 0U)
    {
        return NAND_PAGE_ECC_CORRECTED;
    }

    parity_positions = 0U;
    for (power = 1U; power <= syndrome; power <<= 1)
    {
        parity_positions++;
    }
    if ((uint32_t)syndrome <= parity_positions)
    {
        return NAND_PAGE_ECC_UNCORRECTABLE;
    }
    data_bit = (uint32_t)syndrome - parity_positions - 1U;
    if (data_bit >= (NAND_PAGE_ECC_DATA_SIZE * 8U))
    {
        return NAND_PAGE_ECC_UNCORRECTABLE;
    }

    data[data_bit >> 3] ^= (uint8_t)(1U << (data_bit & 7U));
    return NAND_PAGE_ECC_CORRECTED;
}
