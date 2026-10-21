#include "nand_cache_page.h"

#include <string.h>

#include "nand_page_ecc.h"

#define NAND_CACHE_PAGE_MAGIC0          0x46U
#define NAND_CACHE_PAGE_MAGIC1          0x43U
#define NAND_CACHE_PAGE_VERSION         1U
#define NAND_CACHE_PAGE_META_OFFSET     2U
#define NAND_CACHE_PAGE_LOGICAL_OFFSET  6U
#define NAND_CACHE_PAGE_LENGTH_OFFSET   10U
#define NAND_CACHE_PAGE_ECC_OFFSET      12U
#define NAND_CACHE_PAGE_CRC_OFFSET      14U
#define NAND_CACHE_PAGE_HEAD_CRC_OFFSET 18U
#define NAND_CACHE_PAGE_HEAD_CRC_LENGTH 16U

static uint32_t nand_cache_page_crc32(const uint8_t *data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFFUL;
    uint32_t index;

    for (index = 0U; index < length; index++)
    {
        uint32_t bit;

        crc ^= data[index];
        for (bit = 0U; bit < 8U; bit++)
        {
            uint32_t mask = 0U - (crc & 1U);
            crc = (crc >> 1) ^ (0xEDB88320UL & mask);
        }
    }
    return ~crc;
}

static void nand_cache_page_put_u16(uint8_t *buffer, uint16_t value)
{
    buffer[0] = (uint8_t)value;
    buffer[1] = (uint8_t)(value >> 8);
}

static void nand_cache_page_put_u32(uint8_t *buffer, uint32_t value)
{
    buffer[0] = (uint8_t)value;
    buffer[1] = (uint8_t)(value >> 8);
    buffer[2] = (uint8_t)(value >> 16);
    buffer[3] = (uint8_t)(value >> 24);
}

static uint16_t nand_cache_page_get_u16(const uint8_t *buffer)
{
    return (uint16_t)((uint16_t)buffer[0] |
                      ((uint16_t)buffer[1] << 8));
}

static uint32_t nand_cache_page_get_u32(const uint8_t *buffer)
{
    return (uint32_t)buffer[0] |
           ((uint32_t)buffer[1] << 8) |
           ((uint32_t)buffer[2] << 16) |
           ((uint32_t)buffer[3] << 24);
}

int nand_cache_page_encode(uint32_t logical_page,
                           const uint8_t *data,
                           uint32_t length,
                           uint8_t *main_area,
                           uint8_t *spare_area)
{
    uint16_t ecc;
    uint32_t data_crc;
    uint32_t header_crc;

    if ((data == NULL) || (main_area == NULL) || (spare_area == NULL) ||
        (length == 0U) || (length > NAND_CACHE_PAGE_MAIN_SIZE))
    {
        return NAND_CACHE_PAGE_INVALID_ARG;
    }

    memset(main_area, 0xFF, NAND_CACHE_PAGE_MAIN_SIZE);
    memcpy(main_area, data, length);
    memset(spare_area, 0xFF, NAND_CACHE_PAGE_SPARE_SIZE);

    if (nand_page_ecc_encode(main_area, NAND_CACHE_PAGE_MAIN_SIZE, &ecc) !=
        NAND_PAGE_ECC_OK)
    {
        return NAND_CACHE_PAGE_INVALID_ARG;
    }
    data_crc = nand_cache_page_crc32(main_area, NAND_CACHE_PAGE_MAIN_SIZE);

    spare_area[NAND_CACHE_PAGE_META_OFFSET] = NAND_CACHE_PAGE_MAGIC0;
    spare_area[NAND_CACHE_PAGE_META_OFFSET + 1U] = NAND_CACHE_PAGE_MAGIC1;
    spare_area[NAND_CACHE_PAGE_META_OFFSET + 2U] = NAND_CACHE_PAGE_VERSION;
    spare_area[NAND_CACHE_PAGE_META_OFFSET + 3U] = 0U;
    nand_cache_page_put_u32(&spare_area[NAND_CACHE_PAGE_LOGICAL_OFFSET],
                            logical_page);
    nand_cache_page_put_u16(&spare_area[NAND_CACHE_PAGE_LENGTH_OFFSET],
                            (uint16_t)length);
    nand_cache_page_put_u16(&spare_area[NAND_CACHE_PAGE_ECC_OFFSET], ecc);
    nand_cache_page_put_u32(&spare_area[NAND_CACHE_PAGE_CRC_OFFSET], data_crc);
    header_crc = nand_cache_page_crc32(
        &spare_area[NAND_CACHE_PAGE_META_OFFSET],
        NAND_CACHE_PAGE_HEAD_CRC_LENGTH);
    nand_cache_page_put_u32(&spare_area[NAND_CACHE_PAGE_HEAD_CRC_OFFSET],
                            header_crc);
    return NAND_CACHE_PAGE_OK;
}

int nand_cache_page_decode(uint32_t expected_logical_page,
                           uint8_t *main_area,
                           const uint8_t *spare_area,
                           uint32_t *valid_length)
{
    uint32_t stored_header_crc;
    uint32_t calculated_header_crc;
    uint32_t stored_data_crc;
    uint32_t calculated_data_crc;
    uint32_t logical_page;
    uint16_t stored_ecc;
    uint16_t length;
    int ecc_result;

    if ((main_area == NULL) || (spare_area == NULL) ||
        (valid_length == NULL))
    {
        return NAND_CACHE_PAGE_INVALID_ARG;
    }

    stored_header_crc = nand_cache_page_get_u32(
        &spare_area[NAND_CACHE_PAGE_HEAD_CRC_OFFSET]);
    calculated_header_crc = nand_cache_page_crc32(
        &spare_area[NAND_CACHE_PAGE_META_OFFSET],
        NAND_CACHE_PAGE_HEAD_CRC_LENGTH);
    if ((stored_header_crc != calculated_header_crc) ||
        (spare_area[NAND_CACHE_PAGE_META_OFFSET] != NAND_CACHE_PAGE_MAGIC0) ||
        (spare_area[NAND_CACHE_PAGE_META_OFFSET + 1U] !=
         NAND_CACHE_PAGE_MAGIC1) ||
        (spare_area[NAND_CACHE_PAGE_META_OFFSET + 2U] !=
         NAND_CACHE_PAGE_VERSION))
    {
        return NAND_CACHE_PAGE_METADATA_ERROR;
    }

    logical_page = nand_cache_page_get_u32(
        &spare_area[NAND_CACHE_PAGE_LOGICAL_OFFSET]);
    length = nand_cache_page_get_u16(
        &spare_area[NAND_CACHE_PAGE_LENGTH_OFFSET]);
    if ((logical_page != expected_logical_page) ||
        (length == 0U) || (length > NAND_CACHE_PAGE_MAIN_SIZE))
    {
        return NAND_CACHE_PAGE_METADATA_ERROR;
    }

    stored_ecc = nand_cache_page_get_u16(
        &spare_area[NAND_CACHE_PAGE_ECC_OFFSET]);
    ecc_result = nand_page_ecc_check_and_correct(
        main_area, NAND_CACHE_PAGE_MAIN_SIZE, stored_ecc);
    if (ecc_result == NAND_PAGE_ECC_UNCORRECTABLE)
    {
        return NAND_CACHE_PAGE_UNCORRECTABLE;
    }
    if ((ecc_result != NAND_PAGE_ECC_OK) &&
        (ecc_result != NAND_PAGE_ECC_CORRECTED))
    {
        return NAND_CACHE_PAGE_INVALID_ARG;
    }

    stored_data_crc = nand_cache_page_get_u32(
        &spare_area[NAND_CACHE_PAGE_CRC_OFFSET]);
    calculated_data_crc = nand_cache_page_crc32(main_area,
                                                 NAND_CACHE_PAGE_MAIN_SIZE);
    if (stored_data_crc != calculated_data_crc)
    {
        return NAND_CACHE_PAGE_CRC_ERROR;
    }

    *valid_length = length;
    return (ecc_result == NAND_PAGE_ECC_CORRECTED) ?
           NAND_CACHE_PAGE_CORRECTED : NAND_CACHE_PAGE_OK;
}
