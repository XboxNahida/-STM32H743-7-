#ifndef FONT_CACHE_SOURCE_H
#define FONT_CACHE_SOURCE_H

#include <stdint.h>

typedef enum
{
    FONT_CACHE_SOURCE_OK = 0,
    FONT_CACHE_SOURCE_INVALID_ARG,
    FONT_CACHE_SOURCE_FORMAT_ERROR,
    FONT_CACHE_SOURCE_BOUNDS_ERROR
} font_cache_source_result_t;

typedef struct
{
    const uint8_t *data;
    uint32_t capacity;
} font_cache_source_t;

int font_cache_source_collection_crc(const font_cache_source_t *sources,
                                     uint32_t source_count,
                                     uint32_t *collection_crc,
                                     uint32_t *total_length);

#endif
