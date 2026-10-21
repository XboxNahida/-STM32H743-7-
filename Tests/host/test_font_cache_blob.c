#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "font_cache_blob.h"

typedef struct
{
    uint32_t value;
    const uint8_t *bytes;
    const uint16_t *words;
} sample_root_t;

static int expect(const char *name, int actual, int expected)
{
    if (actual == expected)
        return 0;
    printf("FAIL %s actual=%d expected=%d\n", name, actual, expected);
    return 1;
}

int main(void)
{
    static const uint8_t bytes[5] = {3U, 1U, 4U, 1U, 5U};
    static const uint16_t words[3] = {100U, 200U, 300U};
    sample_root_t root;
    font_cache_blob_segment_t segments[3];
    font_cache_blob_relocation_t relocations[2];
    font_cache_blob_plan_t plan;
    uint8_t package[512];
    uint8_t relocated[512];
    sample_root_t *loaded_root;
    void *segment_view = NULL;
    uint32_t segment_size = 0U;
    uintptr_t stored_pointer = 0U;
    uint32_t total_size;
    int failures = 0;

    root.value = 0x12345678UL;
    root.bytes = bytes;
    root.words = words;
    segments[0].source = &root;
    segments[0].size = sizeof(root);
    segments[0].alignment = sizeof(void *);
    segments[1].source = bytes;
    segments[1].size = sizeof(bytes);
    segments[1].alignment = 1U;
    segments[2].source = words;
    segments[2].size = sizeof(words);
    segments[2].alignment = sizeof(uint16_t);
    relocations[0].owner_segment = 0U;
    relocations[0].pointer_offset = (uint32_t)offsetof(sample_root_t, bytes);
    relocations[0].target_segment = 1U;
    relocations[1].owner_segment = 0U;
    relocations[1].pointer_offset = (uint32_t)offsetof(sample_root_t, words);
    relocations[1].target_segment = 2U;

    failures += expect(
        "plan_init",
        font_cache_blob_plan_init(&plan, segments, 3U, relocations, 2U,
                                  0x10203040UL, 0x55667788UL),
        FONT_CACHE_BLOB_OK);
    total_size = font_cache_blob_plan_size(&plan);
    if ((total_size == 0U) || (total_size > sizeof(package)))
    {
        printf("FAIL package_size size=%lu\n", (unsigned long)total_size);
        failures++;
    }
    memset(package, 0xA5, sizeof(package));
    failures += expect(
        "stream_first_half",
        font_cache_blob_plan_read(&plan, 0U, package, total_size / 2U),
        FONT_CACHE_BLOB_OK);
    failures += expect(
        "stream_second_half",
        font_cache_blob_plan_read(&plan, total_size / 2U,
                                  &package[total_size / 2U],
                                  total_size - total_size / 2U),
        FONT_CACHE_BLOB_OK);

    memcpy(&stored_pointer,
           &package[font_cache_blob_segment_offset(&plan, 0U) +
                    offsetof(sample_root_t, bytes)],
           sizeof(stored_pointer));
    if ((stored_pointer == (uintptr_t)bytes) ||
        (stored_pointer != font_cache_blob_segment_offset(&plan, 1U)))
    {
        printf("FAIL package_contains_relative_pointer\n");
        failures++;
    }

    memcpy(relocated, package, total_size);
    failures += expect(
        "relocate",
        font_cache_blob_relocate(relocated, total_size,
                                 0x10203040UL, 0x55667788UL),
        FONT_CACHE_BLOB_OK);
    loaded_root = (sample_root_t *)&relocated[
        font_cache_blob_segment_offset(&plan, 0U)];
    failures += expect(
        "segment_view",
        font_cache_blob_get_segment(relocated, total_size, 0U,
                                    &segment_view, &segment_size),
        FONT_CACHE_BLOB_OK);
    if ((segment_view != loaded_root) || (segment_size != sizeof(root)))
    {
        printf("FAIL segment_view_value\n");
        failures++;
    }
    failures += expect(
        "segment_view_out_of_range",
        font_cache_blob_get_segment(relocated, total_size, 3U,
                                    &segment_view, &segment_size),
        FONT_CACHE_BLOB_INVALID_ARG);
    if ((loaded_root->value != root.value) ||
        (loaded_root->bytes != &relocated[
             font_cache_blob_segment_offset(&plan, 1U)]) ||
        (loaded_root->words != (const uint16_t *)&relocated[
             font_cache_blob_segment_offset(&plan, 2U)]) ||
        (memcmp(loaded_root->bytes, bytes, sizeof(bytes)) != 0) ||
        (memcmp(loaded_root->words, words, sizeof(words)) != 0))
    {
        printf("FAIL relocated_graph\n");
        failures++;
    }

    memcpy(relocated, package, total_size);
    failures += expect(
        "abi_mismatch",
        font_cache_blob_relocate(relocated, total_size,
                                 0x10203041UL, 0x55667788UL),
        FONT_CACHE_BLOB_INCOMPATIBLE);
    failures += expect(
        "source_mismatch",
        font_cache_blob_relocate(relocated, total_size,
                                 0x10203040UL, 0x55667789UL),
        FONT_CACHE_BLOB_INCOMPATIBLE);
    memcpy(relocated, package, total_size);
    relocated[total_size - 1U] ^= 0x80U;
    failures += expect(
        "payload_crc",
        font_cache_blob_relocate(relocated, total_size,
                                 0x10203040UL, 0x55667788UL),
        FONT_CACHE_BLOB_CRC_ERROR);
    failures += expect(
        "invalid_segment",
        font_cache_blob_plan_read(&plan, total_size, package, 1U),
        FONT_CACHE_BLOB_INVALID_ARG);
    failures += expect(
        "zero_length_read",
        font_cache_blob_plan_read(&plan, total_size, NULL, 0U),
        FONT_CACHE_BLOB_OK);
    segments[0].size = 1U;
    failures += expect(
        "pointer_does_not_fit_owner",
        font_cache_blob_plan_init(&plan, segments, 3U, relocations, 2U,
                                  0x10203040UL, 0x55667788UL),
        FONT_CACHE_BLOB_INVALID_ARG);

    if (failures != 0)
    {
        printf("FAIL font_cache_blob failures=%d\n", failures);
        return 1;
    }
    printf("PASS font_cache_blob\n");
    return 0;
}
