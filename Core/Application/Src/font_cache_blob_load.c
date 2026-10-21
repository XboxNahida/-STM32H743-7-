#include "font_cache_blob.h"

#include <stddef.h>
#include <string.h>

#define BLOB_MAGIC       0x4C424346UL
#define BLOB_VERSION     1U
#define BLOB_HEADER_SIZE 36U
#define BLOB_ENTRY_SIZE  8U

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

static int offset_in_segments(const uint8_t *package, uint16_t count,
                              uint32_t offset, uint32_t length)
{
    uint32_t index;

    for (index = 0U; index < count; index++)
    {
        uint32_t start = get_u32(&package[BLOB_HEADER_SIZE + index * 8U]);
        uint32_t size = get_u32(&package[BLOB_HEADER_SIZE + index * 8U + 4U]);
        if ((offset >= start) && (length <= size) &&
            (offset - start <= size - length))
            return 1;
    }
    return 0;
}

int font_cache_blob_relocate(uint8_t *package, uint32_t package_size,
                             uint32_t expected_abi,
                             uint32_t expected_source_crc)
{
    uint16_t segment_count;
    uint16_t relocation_count;
    uint32_t total_size;
    uint32_t table_offset;
    uint32_t index;
    uint32_t field;
    uint32_t target;
    uint32_t crc;
    uintptr_t relative;
    uintptr_t absolute;

    if ((package == NULL) || (package_size < BLOB_HEADER_SIZE))
        return FONT_CACHE_BLOB_INVALID_ARG;
    if ((get_u32(&package[0]) != BLOB_MAGIC) ||
        (get_u16(&package[4]) != BLOB_VERSION) ||
        (get_u16(&package[6]) != BLOB_HEADER_SIZE) ||
        (get_u32(&package[32]) !=
         ~crc_update(0xFFFFFFFFUL, package, 32U)))
        return FONT_CACHE_BLOB_FORMAT_ERROR;
    if ((get_u32(&package[8]) != expected_abi) ||
        (get_u32(&package[12]) != expected_source_crc) ||
        (package[28] != sizeof(uintptr_t)))
        return FONT_CACHE_BLOB_INCOMPATIBLE;
    total_size = get_u32(&package[16]);
    segment_count = get_u16(&package[24]);
    relocation_count = get_u16(&package[26]);
    if ((total_size != package_size) || (segment_count == 0U) ||
        (segment_count > FONT_CACHE_BLOB_MAX_SEGMENTS) ||
        (relocation_count > FONT_CACHE_BLOB_MAX_RELOCATIONS) ||
        ((uint32_t)(segment_count + relocation_count) >
         (total_size - BLOB_HEADER_SIZE) / BLOB_ENTRY_SIZE))
        return FONT_CACHE_BLOB_FORMAT_ERROR;
    crc = ~crc_update(0xFFFFFFFFUL, &package[BLOB_HEADER_SIZE],
                      total_size - BLOB_HEADER_SIZE);
    if (crc != get_u32(&package[20]))
        return FONT_CACHE_BLOB_CRC_ERROR;

    table_offset = BLOB_HEADER_SIZE + segment_count * BLOB_ENTRY_SIZE;
    for (index = 0U; index < relocation_count; index++)
    {
        field = get_u32(&package[table_offset + index * 8U]);
        target = get_u32(&package[table_offset + index * 8U + 4U]);
        if ((offset_in_segments(package, segment_count, field,
                                sizeof(uintptr_t)) == 0) ||
            (offset_in_segments(package, segment_count, target, 1U) == 0))
            return FONT_CACHE_BLOB_FORMAT_ERROR;
        memcpy(&relative, &package[field], sizeof(relative));
        if (relative != (uintptr_t)target)
            return FONT_CACHE_BLOB_FORMAT_ERROR;
        absolute = (uintptr_t)&package[target];
        memcpy(&package[field], &absolute, sizeof(absolute));
    }
    return FONT_CACHE_BLOB_OK;
}

int font_cache_blob_get_segment(uint8_t *package, uint32_t package_size,
                                uint32_t segment, void **segment_data,
                                uint32_t *segment_size)
{
    uint16_t segment_count;
    uint32_t offset;
    uint32_t size;

    if ((package == NULL) || (segment_data == NULL) ||
        (segment_size == NULL) || (package_size < BLOB_HEADER_SIZE) ||
        (get_u32(&package[0]) != BLOB_MAGIC) ||
        (get_u16(&package[4]) != BLOB_VERSION) ||
        (get_u16(&package[6]) != BLOB_HEADER_SIZE) ||
        (get_u32(&package[16]) != package_size) ||
        (get_u32(&package[32]) !=
         ~crc_update(0xFFFFFFFFUL, package, 32U)))
        return FONT_CACHE_BLOB_INVALID_ARG;
    segment_count = get_u16(&package[24]);
    if ((segment >= segment_count) ||
        (segment_count > FONT_CACHE_BLOB_MAX_SEGMENTS) ||
        (segment_count > (package_size - BLOB_HEADER_SIZE) / BLOB_ENTRY_SIZE))
        return FONT_CACHE_BLOB_INVALID_ARG;
    offset = get_u32(&package[BLOB_HEADER_SIZE + segment * BLOB_ENTRY_SIZE]);
    size = get_u32(&package[BLOB_HEADER_SIZE + segment * BLOB_ENTRY_SIZE + 4U]);
    if ((offset > package_size) || (size > package_size - offset))
        return FONT_CACHE_BLOB_FORMAT_ERROR;
    *segment_data = &package[offset];
    *segment_size = size;
    return FONT_CACHE_BLOB_OK;
}
