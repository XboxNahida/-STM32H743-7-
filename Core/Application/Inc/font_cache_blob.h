#ifndef FONT_CACHE_BLOB_H
#define FONT_CACHE_BLOB_H

#include <stdint.h>

#define FONT_CACHE_BLOB_MAX_SEGMENTS    192U
#define FONT_CACHE_BLOB_MAX_RELOCATIONS 192U

typedef enum
{
    FONT_CACHE_BLOB_OK = 0,
    FONT_CACHE_BLOB_INVALID_ARG,
    FONT_CACHE_BLOB_TOO_COMPLEX,
    FONT_CACHE_BLOB_OVERFLOW,
    FONT_CACHE_BLOB_FORMAT_ERROR,
    FONT_CACHE_BLOB_INCOMPATIBLE,
    FONT_CACHE_BLOB_CRC_ERROR
} font_cache_blob_result_t;

typedef struct
{
    const void *source;
    uint32_t size;
    uint32_t alignment;
} font_cache_blob_segment_t;

typedef struct
{
    uint16_t owner_segment;
    uint32_t pointer_offset;
    uint16_t target_segment;
} font_cache_blob_relocation_t;

typedef struct
{
    font_cache_blob_segment_t segments[FONT_CACHE_BLOB_MAX_SEGMENTS];
    font_cache_blob_relocation_t relocations[FONT_CACHE_BLOB_MAX_RELOCATIONS];
    uint32_t segment_offsets[FONT_CACHE_BLOB_MAX_SEGMENTS];
    uint32_t abi_id;
    uint32_t source_crc;
    uint32_t total_size;
    uint32_t payload_crc;
    uint16_t segment_count;
    uint16_t relocation_count;
    uint32_t data_start;
} font_cache_blob_plan_t;

int font_cache_blob_plan_init(
    font_cache_blob_plan_t *plan,
    const font_cache_blob_segment_t *segments,
    uint32_t segment_count,
    const font_cache_blob_relocation_t *relocations,
    uint32_t relocation_count,
    uint32_t abi_id,
    uint32_t source_crc);
uint32_t font_cache_blob_plan_size(const font_cache_blob_plan_t *plan);
uint32_t font_cache_blob_segment_offset(const font_cache_blob_plan_t *plan,
                                        uint32_t segment);
int font_cache_blob_plan_read(const font_cache_blob_plan_t *plan,
                              uint32_t offset,
                              uint8_t *destination,
                              uint32_t length);
int font_cache_blob_relocate(uint8_t *package,
                             uint32_t package_size,
                             uint32_t expected_abi,
                             uint32_t expected_source_crc);
int font_cache_blob_get_segment(uint8_t *package,
                                uint32_t package_size,
                                uint32_t segment,
                                void **segment_data,
                                uint32_t *segment_size);

#endif
