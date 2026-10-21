#include "notes_storage_codec.h"

#include <stddef.h>
#include <string.h>

#include "notes_utf8_internal.h"

static uint32_t notes_storage_utf8_prefix(const uint8_t *raw,
                                          uint32_t raw_bytes,
                                          uint32_t source_bytes,
                                          uint8_t *is_utf8)
{
    uint32_t trim;

    if (notes_utf8_validate(raw, raw_bytes))
    {
        *is_utf8 = 1U;
        return raw_bytes;
    }
    if (source_bytes <= raw_bytes)
    {
        *is_utf8 = 0U;
        return raw_bytes;
    }
    for (trim = 1U; (trim <= 3U) && (trim <= raw_bytes); trim++)
    {
        if (notes_utf8_validate(raw, raw_bytes - trim))
        {
            *is_utf8 = 1U;
            return raw_bytes - trim;
        }
    }
    *is_utf8 = 0U;
    return raw_bytes;
}

static uint32_t notes_storage_encode_utf8(uint16_t unicode, uint8_t *output)
{
    if (unicode < 0x80U)
    {
        output[0] = (uint8_t)unicode;
        return 1U;
    }
    if (unicode < 0x800U)
    {
        output[0] = (uint8_t)(0xC0U | (unicode >> 6));
        output[1] = (uint8_t)(0x80U | (unicode & 0x3FU));
        return 2U;
    }
    output[0] = (uint8_t)(0xE0U | (unicode >> 12));
    output[1] = (uint8_t)(0x80U | ((unicode >> 6) & 0x3FU));
    output[2] = (uint8_t)(0x80U | (unicode & 0x3FU));
    return 3U;
}

notes_storage_result_t notes_storage_decode_text(
    const uint8_t *raw, uint32_t raw_bytes, uint32_t source_bytes,
    uint8_t *output, uint32_t output_capacity,
    notes_storage_oem2uni_fn oem2uni,
    uint32_t *output_bytes, uint8_t *truncated)
{
    uint32_t input_offset = 0U;
    uint32_t output_offset = 0U;
    uint32_t limit;
    uint8_t is_utf8;

    if ((raw == NULL) || (output == NULL) || (output_bytes == NULL) ||
        (truncated == NULL) || (oem2uni == NULL) ||
        (output_capacity == 0U))
    {
        return NOTES_STORAGE_INVALID_ARG;
    }
    limit = output_capacity - 1U;
    if (limit > NOTES_DOCUMENT_MAX_BYTES)
    {
        limit = NOTES_DOCUMENT_MAX_BYTES;
    }
    raw_bytes = notes_storage_utf8_prefix(raw, raw_bytes, source_bytes,
                                          &is_utf8);
    *truncated = (source_bytes > raw_bytes) ? 1U : 0U;
    if (is_utf8)
    {
        uint32_t copy_bytes = raw_bytes;
        if (copy_bytes > limit)
        {
            copy_bytes = notes_utf8_align_down(raw, limit);
            *truncated = 1U;
        }
        if (copy_bytes != 0U)
        {
            memcpy(output, raw, copy_bytes);
        }
        output[copy_bytes] = 0U;
        *output_bytes = copy_bytes;
        return NOTES_STORAGE_OK;
    }
    while (input_offset < raw_bytes)
    {
        uint8_t encoded[3];
        uint32_t encoded_bytes;

        if (raw[input_offset] < 0x80U)
        {
            encoded[0] = raw[input_offset];
            encoded_bytes = 1U;
            input_offset++;
        }
        else if ((input_offset + 1U) < raw_bytes)
        {
            uint16_t oem = (uint16_t)(((uint16_t)raw[input_offset] << 8) |
                                      raw[input_offset + 1U]);
            uint16_t unicode = oem2uni(oem);
            if (unicode == 0U)
            {
                unicode = (uint16_t)'?';
            }
            encoded_bytes = notes_storage_encode_utf8(unicode, encoded);
            input_offset += 2U;
        }
        else
        {
            encoded[0] = (uint8_t)'?';
            encoded_bytes = 1U;
            input_offset++;
        }
        if (encoded_bytes > (limit - output_offset))
        {
            *truncated = 1U;
            break;
        }
        memcpy(output + output_offset, encoded, encoded_bytes);
        output_offset += encoded_bytes;
    }
    if (input_offset < raw_bytes)
    {
        *truncated = 1U;
    }
    output[output_offset] = 0U;
    *output_bytes = output_offset;
    return NOTES_STORAGE_OK;
}
