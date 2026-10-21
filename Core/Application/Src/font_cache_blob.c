#include "font_cache_blob.h"

#include <stddef.h>
#include <string.h>

#define BLOB_MAGIC       0x4C424346UL
#define BLOB_VERSION     1U
#define BLOB_HEADER_SIZE 36U
#define BLOB_ENTRY_SIZE  8U

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

static int add_u32(uint32_t left, uint32_t right, uint32_t *result)
{
    if ((result == NULL) || (left > UINT32_MAX - right))
        return 0;
    *result = left + right;
    return 1;
}

static int align_u32(uint32_t value, uint32_t alignment, uint32_t *result)
{
    uint32_t mask;

    if ((alignment == 0U) || ((alignment & (alignment - 1U)) != 0U) ||
        (alignment > 64U))
        return 0;
    mask = alignment - 1U;
    if (value > UINT32_MAX - mask)
        return 0;
    *result = (value + mask) & ~mask;
    return 1;
}

static void copy_overlap(uint8_t *destination, uint32_t request_offset,
                         uint32_t request_length, uint32_t source_offset,
                         const uint8_t *source, uint32_t source_length)
{
    uint32_t start = (request_offset > source_offset) ? request_offset :
                                                       source_offset;
    uint32_t request_end = request_offset + request_length;
    uint32_t source_end = source_offset + source_length;
    uint32_t end = (request_end < source_end) ? request_end : source_end;

    if (start < end)
        memcpy(&destination[start - request_offset], &source[start - source_offset],
               end - start);
}

static void make_header(const font_cache_blob_plan_t *plan, uint8_t *header)
{
    memset(header, 0, BLOB_HEADER_SIZE);
    put_u32(&header[0], BLOB_MAGIC);
    put_u16(&header[4], BLOB_VERSION);
    put_u16(&header[6], BLOB_HEADER_SIZE);
    put_u32(&header[8], plan->abi_id);
    put_u32(&header[12], plan->source_crc);
    put_u32(&header[16], plan->total_size);
    put_u32(&header[20], plan->payload_crc);
    put_u16(&header[24], plan->segment_count);
    put_u16(&header[26], plan->relocation_count);
    header[28] = (uint8_t)sizeof(uintptr_t);
    put_u32(&header[32], ~crc_update(0xFFFFFFFFUL, header, 32U));
}

uint32_t font_cache_blob_plan_size(const font_cache_blob_plan_t *plan)
{
    return (plan != NULL) ? plan->total_size : 0U;
}

uint32_t font_cache_blob_segment_offset(const font_cache_blob_plan_t *plan,
                                        uint32_t segment)
{
    return ((plan != NULL) && (segment < plan->segment_count)) ?
           plan->segment_offsets[segment] : 0U;
}

int font_cache_blob_plan_read(const font_cache_blob_plan_t *plan,
                              uint32_t offset, uint8_t *destination,
                              uint32_t length)
{
    uint8_t entry[BLOB_ENTRY_SIZE];
    uint8_t header[BLOB_HEADER_SIZE];
    uint8_t pointer_bytes[sizeof(uintptr_t)];
    uint32_t index;
    uint32_t table_offset;
    uintptr_t relative;

    if ((plan == NULL) || ((destination == NULL) && (length != 0U)) ||
        (offset > plan->total_size) || (length > plan->total_size - offset))
        return FONT_CACHE_BLOB_INVALID_ARG;
    if (length != 0U)
        memset(destination, 0, length);
    make_header(plan, header);
    copy_overlap(destination, offset, length, 0U, header, sizeof(header));

    table_offset = BLOB_HEADER_SIZE;
    for (index = 0U; index < plan->segment_count; index++)
    {
        put_u32(&entry[0], plan->segment_offsets[index]);
        put_u32(&entry[4], plan->segments[index].size);
        copy_overlap(destination, offset, length, table_offset, entry,
                     sizeof(entry));
        table_offset += BLOB_ENTRY_SIZE;
    }
    for (index = 0U; index < plan->relocation_count; index++)
    {
        put_u32(&entry[0],
                plan->segment_offsets[plan->relocations[index].owner_segment] +
                plan->relocations[index].pointer_offset);
        put_u32(&entry[4],
                plan->segment_offsets[plan->relocations[index].target_segment]);
        copy_overlap(destination, offset, length, table_offset, entry,
                     sizeof(entry));
        table_offset += BLOB_ENTRY_SIZE;
    }
    for (index = 0U; index < plan->segment_count; index++)
        copy_overlap(destination, offset, length, plan->segment_offsets[index],
                     (const uint8_t *)plan->segments[index].source,
                     plan->segments[index].size);

    for (index = 0U; index < plan->relocation_count; index++)
    {
        uint32_t field =
            plan->segment_offsets[plan->relocations[index].owner_segment] +
            plan->relocations[index].pointer_offset;
        relative = plan->segment_offsets[plan->relocations[index].target_segment];
        memcpy(pointer_bytes, &relative, sizeof(relative));
        copy_overlap(destination, offset, length, field, pointer_bytes,
                     sizeof(pointer_bytes));
    }
    return FONT_CACHE_BLOB_OK;
}

int font_cache_blob_plan_init(
    font_cache_blob_plan_t *plan,
    const font_cache_blob_segment_t *segments, uint32_t segment_count,
    const font_cache_blob_relocation_t *relocations, uint32_t relocation_count,
    uint32_t abi_id, uint32_t source_crc)
{
    uint8_t buffer[256];
    uint32_t index;
    uint32_t cursor;
    uint32_t offset;
    uint32_t chunk;
    uint32_t crc = 0xFFFFFFFFUL;

    if ((plan == NULL) || (segments == NULL) || (segment_count == 0U) ||
        (segment_count > FONT_CACHE_BLOB_MAX_SEGMENTS) ||
        (relocation_count > FONT_CACHE_BLOB_MAX_RELOCATIONS) ||
        ((relocation_count != 0U) && (relocations == NULL)))
        return FONT_CACHE_BLOB_INVALID_ARG;
    memset(plan, 0, sizeof(*plan));
    plan->segment_count = (uint16_t)segment_count;
    plan->relocation_count = (uint16_t)relocation_count;
    plan->abi_id = abi_id;
    plan->source_crc = source_crc;
    memcpy(plan->segments, segments, segment_count * sizeof(segments[0]));
    if (relocation_count != 0U)
        memcpy(plan->relocations, relocations,
               relocation_count * sizeof(relocations[0]));

    cursor = BLOB_HEADER_SIZE +
             (segment_count + relocation_count) * BLOB_ENTRY_SIZE;
    if (align_u32(cursor, (uint32_t)sizeof(uintptr_t), &cursor) == 0)
        return FONT_CACHE_BLOB_OVERFLOW;
    plan->data_start = cursor;
    for (index = 0U; index < segment_count; index++)
    {
        if ((segments[index].source == NULL) || (segments[index].size == 0U) ||
            (align_u32(cursor, segments[index].alignment, &cursor) == 0))
            return FONT_CACHE_BLOB_INVALID_ARG;
        plan->segment_offsets[index] = cursor;
        if (add_u32(cursor, segments[index].size, &cursor) == 0)
            return FONT_CACHE_BLOB_OVERFLOW;
    }
    for (index = 0U; index < relocation_count; index++)
    {
        const font_cache_blob_relocation_t *relocation = &relocations[index];
        if ((relocation->owner_segment >= segment_count) ||
            (relocation->target_segment >= segment_count) ||
            (segments[relocation->owner_segment].size < sizeof(uintptr_t)) ||
            (relocation->pointer_offset >
             segments[relocation->owner_segment].size - sizeof(uintptr_t)))
            return FONT_CACHE_BLOB_INVALID_ARG;
    }
    plan->total_size = cursor;
    for (offset = BLOB_HEADER_SIZE; offset < plan->total_size; offset += chunk)
    {
        chunk = plan->total_size - offset;
        if (chunk > sizeof(buffer))
            chunk = sizeof(buffer);
        font_cache_blob_plan_read(plan, offset, buffer, chunk);
        crc = crc_update(crc, buffer, chunk);
    }
    plan->payload_crc = ~crc;
    return FONT_CACHE_BLOB_OK;
}
