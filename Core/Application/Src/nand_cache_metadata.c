#include "nand_cache_metadata.h"

#include <string.h>

#define NAND_CACHE_METADATA_MAGIC       0x544D4346UL
#define NAND_CACHE_METADATA_VERSION     1U
#define NAND_CACHE_METADATA_COMMITTED   0xA5U
#define NAND_CACHE_METADATA_SLOT_COUNT  2U
#define NAND_CACHE_METADATA_PAGE_SIZE   2048U
#define NAND_CACHE_METADATA_SLOT_BYTES  (24UL * 1024UL * 1024UL)
#define NAND_CACHE_METADATA_CRC_OFFSET  28U

static uint32_t metadata_crc32(const uint8_t *data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFFUL;
    uint32_t byte_index;
    uint32_t bit_index;

    for (byte_index = 0U; byte_index < length; byte_index++)
    {
        crc ^= data[byte_index];
        for (bit_index = 0U; bit_index < 8U; bit_index++)
        {
            uint32_t mask = (uint32_t)(0U - (crc & 1U));
            crc = (crc >> 1U) ^ (0xEDB88320UL & mask);
        }
    }
    return ~crc;
}

static void put_u16(uint8_t *buffer, uint16_t value)
{
    buffer[0] = (uint8_t)value;
    buffer[1] = (uint8_t)(value >> 8U);
}

static void put_u32(uint8_t *buffer, uint32_t value)
{
    buffer[0] = (uint8_t)value;
    buffer[1] = (uint8_t)(value >> 8U);
    buffer[2] = (uint8_t)(value >> 16U);
    buffer[3] = (uint8_t)(value >> 24U);
}

static uint16_t get_u16(const uint8_t *buffer)
{
    return (uint16_t)((uint16_t)buffer[0] |
                      ((uint16_t)buffer[1] << 8U));
}

static uint32_t get_u32(const uint8_t *buffer)
{
    return (uint32_t)buffer[0] |
           ((uint32_t)buffer[1] << 8U) |
           ((uint32_t)buffer[2] << 16U) |
           ((uint32_t)buffer[3] << 24U);
}

static int metadata_values_valid(const nand_cache_metadata_t *metadata)
{
    uint32_t expected_pages;

    if ((metadata == NULL) ||
        (metadata->slot >= NAND_CACHE_METADATA_SLOT_COUNT) ||
        (metadata->data_length == 0U) ||
        (metadata->data_length > NAND_CACHE_METADATA_SLOT_BYTES))
    {
        return 0;
    }
    expected_pages = (metadata->data_length +
                      (NAND_CACHE_METADATA_PAGE_SIZE - 1U)) /
                     NAND_CACHE_METADATA_PAGE_SIZE;
    return (metadata->page_count == expected_pages) ? 1 : 0;
}

int nand_cache_metadata_encode(const nand_cache_metadata_t *metadata,
                               uint8_t committed,
                               uint8_t *record,
                               uint32_t record_size)
{
    uint32_t crc;

    if ((record == NULL) ||
        (record_size != NAND_CACHE_METADATA_RECORD_SIZE) ||
        (metadata_values_valid(metadata) == 0))
    {
        return NAND_CACHE_METADATA_INVALID_ARG;
    }

    memset(record, 0xFF, record_size);
    put_u32(&record[0], NAND_CACHE_METADATA_MAGIC);
    put_u16(&record[4], NAND_CACHE_METADATA_VERSION);
    put_u16(&record[6], NAND_CACHE_METADATA_RECORD_SIZE);
    put_u32(&record[8], metadata->sequence);
    record[12] = metadata->slot;
    record[13] = (committed != 0U) ? NAND_CACHE_METADATA_COMMITTED : 0xFFU;
    put_u16(&record[14], 0U);
    put_u32(&record[16], metadata->data_length);
    put_u32(&record[20], metadata->page_count);
    put_u32(&record[24], metadata->data_crc);
    crc = metadata_crc32(record, NAND_CACHE_METADATA_CRC_OFFSET);
    put_u32(&record[NAND_CACHE_METADATA_CRC_OFFSET], crc);
    return NAND_CACHE_METADATA_OK;
}

int nand_cache_metadata_decode(const uint8_t *record,
                               uint32_t record_size,
                               nand_cache_metadata_t *metadata)
{
    nand_cache_metadata_t decoded;

    if ((record == NULL) || (metadata == NULL) ||
        (record_size != NAND_CACHE_METADATA_RECORD_SIZE))
    {
        return NAND_CACHE_METADATA_INVALID_ARG;
    }
    if ((get_u32(&record[0]) != NAND_CACHE_METADATA_MAGIC) ||
        (get_u16(&record[4]) != NAND_CACHE_METADATA_VERSION) ||
        (get_u16(&record[6]) != NAND_CACHE_METADATA_RECORD_SIZE))
    {
        return NAND_CACHE_METADATA_FORMAT_ERROR;
    }
    if (record[13] != NAND_CACHE_METADATA_COMMITTED)
    {
        return NAND_CACHE_METADATA_NOT_COMMITTED;
    }
    if (get_u32(&record[NAND_CACHE_METADATA_CRC_OFFSET]) !=
        metadata_crc32(record, NAND_CACHE_METADATA_CRC_OFFSET))
    {
        return NAND_CACHE_METADATA_CRC_ERROR;
    }

    decoded.sequence = get_u32(&record[8]);
    decoded.slot = record[12];
    decoded.data_length = get_u32(&record[16]);
    decoded.page_count = get_u32(&record[20]);
    decoded.data_crc = get_u32(&record[24]);
    if (metadata_values_valid(&decoded) == 0)
    {
        return NAND_CACHE_METADATA_FORMAT_ERROR;
    }
    *metadata = decoded;
    return NAND_CACHE_METADATA_OK;
}

static int sequence_is_newer(uint32_t candidate, uint32_t reference)
{
    uint32_t difference = candidate - reference;

    return ((difference != 0U) && (difference < 0x80000000UL)) ? 1 : 0;
}

int nand_cache_metadata_select(const nand_cache_metadata_t *candidates,
                               const uint8_t *valid,
                               uint32_t count,
                               uint32_t *selected_index)
{
    uint32_t index;
    uint32_t selected = 0U;
    uint8_t found = 0U;

    if ((candidates == NULL) || (valid == NULL) ||
        (count == 0U) || (selected_index == NULL))
    {
        return NAND_CACHE_METADATA_INVALID_ARG;
    }
    for (index = 0U; index < count; index++)
    {
        if ((valid[index] != 0U) &&
            ((found == 0U) ||
             (sequence_is_newer(candidates[index].sequence,
                                candidates[selected].sequence) != 0)))
        {
            selected = index;
            found = 1U;
        }
    }
    if (found == 0U)
    {
        return NAND_CACHE_METADATA_NO_VALID_RECORD;
    }
    *selected_index = selected;
    return NAND_CACHE_METADATA_OK;
}
