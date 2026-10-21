#ifndef FONT_CACHE_LVGL_H
#define FONT_CACHE_LVGL_H

#include <stdint.h>

#include "font_cache_blob.h"
#include "lvgl.h"

#define FONT_CACHE_LVGL_MAX_FONTS 6U

typedef enum
{
    FONT_CACHE_LVGL_OK = 0,
    FONT_CACHE_LVGL_INVALID_ARG,
    FONT_CACHE_LVGL_TOO_COMPLEX,
    FONT_CACHE_LVGL_ALLOCATION_UNKNOWN,
    FONT_CACHE_LVGL_FORMAT_ERROR,
    FONT_CACHE_LVGL_INCOMPATIBLE
} font_cache_lvgl_result_t;

typedef uint32_t (*font_cache_lvgl_allocation_size_fn)(const void *pointer,
                                                       void *context);

typedef struct
{
    uint32_t magic;
    uint16_t version;
    uint16_t font_count;
    uint16_t root_segments[FONT_CACHE_LVGL_MAX_FONTS];
} font_cache_lvgl_manifest_t;

typedef struct
{
    font_cache_blob_plan_t blob;
    font_cache_lvgl_manifest_t manifest;
    lv_font_t font_copies[FONT_CACHE_LVGL_MAX_FONTS];
} font_cache_lvgl_plan_t;

uint32_t font_cache_lvgl_abi_id(void);
int font_cache_lvgl_plan_init(font_cache_lvgl_plan_t *plan,
                              const lv_font_t *const *fonts,
                              uint32_t font_count,
                              font_cache_lvgl_allocation_size_fn size_fn,
                              void *size_context,
                              uint32_t source_crc);
int font_cache_lvgl_bind(uint8_t *package,
                         uint32_t package_size,
                         lv_font_t *const *destinations,
                         uint32_t destination_count,
                         uint32_t expected_source_crc);

#endif
