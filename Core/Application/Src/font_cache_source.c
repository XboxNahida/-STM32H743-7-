#include "font_cache_source.h"

#include <string.h>

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

static uint32_t crc_update(uint32_t crc, const uint8_t *data, uint32_t length)
{
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
    return crc;
}

static int font_length(const font_cache_source_t *source, uint32_t *length)
{
    static const char labels[4][4] =
    {
        {'c', 'm', 'a', 'p'},
        {'l', 'o', 'c', 'a'},
        {'g', 'l', 'y', 'f'},
        {'k', 'e', 'r', 'n'}
    };
    uint32_t offset;
    uint32_t table_length;
    uint32_t table_index;
    uint16_t table_count;

    if ((source == NULL) || (source->data == NULL) ||
        (source->capacity < 48U) || (length == NULL))
        return FONT_CACHE_SOURCE_INVALID_ARG;
    if (memcmp(&source->data[4], "head", 4U) != 0)
        return FONT_CACHE_SOURCE_FORMAT_ERROR;
    offset = get_u32(&source->data[0]);
    table_count = get_u16(&source->data[12]);
    if ((offset < 48U) || (offset > source->capacity) ||
        ((table_count != 3U) && (table_count != 4U)))
        return FONT_CACHE_SOURCE_FORMAT_ERROR;

    for (table_index = 0U; table_index < table_count; table_index++)
    {
        if ((offset > source->capacity) ||
            (source->capacity - offset < 8U))
            return FONT_CACHE_SOURCE_BOUNDS_ERROR;
        table_length = get_u32(&source->data[offset]);
        if (memcmp(&source->data[offset + 4U], labels[table_index], 4U) != 0)
            return FONT_CACHE_SOURCE_FORMAT_ERROR;
        if ((table_length < 8U) ||
            (table_length > source->capacity - offset))
            return FONT_CACHE_SOURCE_BOUNDS_ERROR;
        offset += table_length;
    }
    *length = offset;
    return FONT_CACHE_SOURCE_OK;
}

int font_cache_source_collection_crc(const font_cache_source_t *sources,
                                     uint32_t source_count,
                                     uint32_t *collection_crc,
                                     uint32_t *total_length)
{
    uint8_t encoded_length[4];
    uint32_t index;
    uint32_t length;
    uint32_t total = 0U;
    uint32_t crc = 0xFFFFFFFFUL;
    int result;

    if ((sources == NULL) || (source_count == 0U) ||
        (collection_crc == NULL) || (total_length == NULL))
        return FONT_CACHE_SOURCE_INVALID_ARG;
    for (index = 0U; index < source_count; index++)
    {
        result = font_length(&sources[index], &length);
        if (result != FONT_CACHE_SOURCE_OK)
            return result;
        if (total > UINT32_MAX - length)
            return FONT_CACHE_SOURCE_BOUNDS_ERROR;
        encoded_length[0] = (uint8_t)length;
        encoded_length[1] = (uint8_t)(length >> 8U);
        encoded_length[2] = (uint8_t)(length >> 16U);
        encoded_length[3] = (uint8_t)(length >> 24U);
        crc = crc_update(crc, encoded_length, sizeof(encoded_length));
        crc = crc_update(crc, sources[index].data, length);
        total += length;
    }
    *collection_crc = ~crc;
    *total_length = total;
    return FONT_CACHE_SOURCE_OK;
}
