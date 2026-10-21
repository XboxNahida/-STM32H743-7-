#include "font_cache_lvgl.h"

#include "src/font/fmt_txt/lv_font_fmt_txt.h"

#define FONT_CACHE_LVGL_MAGIC   0x544E464CUL
#define FONT_CACHE_LVGL_VERSION 1U

static void cache_release_glyph(const lv_font_t *font,
                                lv_font_glyph_dsc_t *glyph_dsc)
{
    (void)font;
    (void)glyph_dsc;
}

int font_cache_lvgl_bind(uint8_t *package, uint32_t package_size,
                         lv_font_t *const *destinations,
                         uint32_t destination_count,
                         uint32_t expected_source_crc)
{
    font_cache_lvgl_manifest_t *manifest;
    lv_font_t *cached;
    void *segment;
    uint32_t segment_size;
    uint32_t index;
    int result;

    if ((package == NULL) || (destinations == NULL) ||
        (destination_count == 0U) ||
        (destination_count > FONT_CACHE_LVGL_MAX_FONTS))
        return FONT_CACHE_LVGL_INVALID_ARG;
    result = font_cache_blob_relocate(package, package_size,
                                      font_cache_lvgl_abi_id(),
                                      expected_source_crc);
    if (result == FONT_CACHE_BLOB_INCOMPATIBLE)
        return FONT_CACHE_LVGL_INCOMPATIBLE;
    if (result != FONT_CACHE_BLOB_OK)
        return FONT_CACHE_LVGL_FORMAT_ERROR;
    if (font_cache_blob_get_segment(package, package_size, 0U, &segment,
                                    &segment_size) != FONT_CACHE_BLOB_OK)
        return FONT_CACHE_LVGL_FORMAT_ERROR;
    manifest = segment;
    if ((segment_size != sizeof(*manifest)) ||
        (manifest->magic != FONT_CACHE_LVGL_MAGIC) ||
        (manifest->version != FONT_CACHE_LVGL_VERSION) ||
        (manifest->font_count != destination_count))
        return FONT_CACHE_LVGL_FORMAT_ERROR;
    for (index = 0U; index < destination_count; index++)
    {
        if ((destinations[index] == NULL) ||
            (font_cache_blob_get_segment(
                 package, package_size, manifest->root_segments[index],
                 &segment, &segment_size) != FONT_CACHE_BLOB_OK) ||
            (segment_size != sizeof(lv_font_t)))
            return FONT_CACHE_LVGL_FORMAT_ERROR;
        cached = segment;
        cached->get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt;
        cached->get_glyph_bitmap = lv_font_get_bitmap_fmt_txt;
        cached->release_glyph = cache_release_glyph;
        *destinations[index] = *cached;
    }
    return FONT_CACHE_LVGL_OK;
}
