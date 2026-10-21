#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "font_cache_source.h"

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

static uint32_t add_table(uint8_t *buffer, uint32_t offset,
                          const char *label, uint32_t length, uint8_t seed)
{
    uint32_t index;

    put_u32(&buffer[offset], length);
    memcpy(&buffer[offset + 4U], label, 4U);
    for (index = 8U; index < length; index++)
        buffer[offset + index] = (uint8_t)(seed + index);
    return offset + length;
}

static uint32_t make_font(uint8_t *buffer, uint8_t seed)
{
    uint32_t offset;

    memset(buffer, 0xFF, 256U);
    put_u32(&buffer[0], 48U);
    memcpy(&buffer[4], "head", 4U);
    put_u32(&buffer[8], 1U);
    put_u16(&buffer[12], 4U);
    buffer[14] = seed;
    offset = 48U;
    offset = add_table(buffer, offset, "cmap", 24U, seed);
    offset = add_table(buffer, offset, "loca", 20U, (uint8_t)(seed + 1U));
    offset = add_table(buffer, offset, "glyf", 32U, (uint8_t)(seed + 2U));
    offset = add_table(buffer, offset, "kern", 16U, (uint8_t)(seed + 3U));
    return offset;
}

static int expect(const char *name, int actual, int expected)
{
    if (actual == expected)
        return 0;
    printf("FAIL %s actual=%d expected=%d\n", name, actual, expected);
    return 1;
}

int main(void)
{
    uint8_t font_a[256];
    uint8_t font_b[256];
    font_cache_source_t sources[2];
    uint32_t length_a;
    uint32_t length_b;
    uint32_t crc_first = 0U;
    uint32_t crc_second = 0U;
    uint32_t total = 0U;
    int failures = 0;

    length_a = make_font(font_a, 12U);
    length_b = make_font(font_b, 20U);
    sources[0].data = font_a;
    sources[0].capacity = sizeof(font_a);
    sources[1].data = font_b;
    sources[1].capacity = sizeof(font_b);
    failures += expect(
        "valid_collection",
        font_cache_source_collection_crc(sources, 2U, &crc_first, &total),
        FONT_CACHE_SOURCE_OK);
    if ((total != length_a + length_b) || (crc_first == 0U))
    {
        printf("FAIL valid_collection_values\n");
        failures++;
    }

    font_a[length_a + 10U] ^= 0x55U;
    failures += expect(
        "padding_ignored",
        font_cache_source_collection_crc(sources, 2U, &crc_second, &total),
        FONT_CACHE_SOURCE_OK);
    if (crc_second != crc_first)
    {
        printf("FAIL padding_changed_crc\n");
        failures++;
    }
    font_a[60] ^= 0x01U;
    failures += expect(
        "payload_changes_crc",
        font_cache_source_collection_crc(sources, 2U, &crc_second, &total),
        FONT_CACHE_SOURCE_OK);
    if (crc_second == crc_first)
    {
        printf("FAIL payload_crc_unchanged\n");
        failures++;
    }
    font_a[60] ^= 0x01U;

    font_b[52] = 'X';
    failures += expect(
        "label_damage",
        font_cache_source_collection_crc(sources, 2U, &crc_second, &total),
        FONT_CACHE_SOURCE_FORMAT_ERROR);
    font_b[52] = 'c';
    put_u32(&font_b[48], 250U);
    failures += expect(
        "table_length_overflow",
        font_cache_source_collection_crc(sources, 2U, &crc_second, &total),
        FONT_CACHE_SOURCE_BOUNDS_ERROR);

    if (failures != 0)
    {
        printf("FAIL font_cache_source failures=%d\n", failures);
        return 1;
    }
    printf("PASS font_cache_source\n");
    return 0;
}
