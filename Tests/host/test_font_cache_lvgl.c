#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "font_cache_lvgl.h"
#include "src/font/fmt_txt/lv_font_fmt_txt.h"

typedef struct
{
    const void *pointer;
    uint32_t size;
} allocation_t;

static allocation_t g_allocations[4];

uint32_t fake_allocation_size(const void *pointer, void *context)
{
    uint32_t index;
    (void)context;

    for (index = 0U; index < 4U; index++)
    {
        if (g_allocations[index].pointer == pointer)
            return g_allocations[index].size;
    }
    return 0U;
}

bool lv_font_get_glyph_dsc_fmt_txt(const lv_font_t *font,
                                   lv_font_glyph_dsc_t *dsc_out,
                                   uint32_t letter, uint32_t letter_next)
{
    (void)font;
    (void)dsc_out;
    (void)letter;
    (void)letter_next;
    return true;
}

const void *lv_font_get_bitmap_fmt_txt(lv_font_glyph_dsc_t *glyph_dsc,
                                       lv_draw_buf_t *draw_buf)
{
    (void)glyph_dsc;
    (void)draw_buf;
    return NULL;
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
    uint8_t bitmap[23];
    lv_font_fmt_txt_glyph_dsc_t glyphs[4];
    uint16_t unicode_list[3] = {0U, 5U, 9U};
    uint16_t glyph_offsets[3] = {0U, 1U, 2U};
    uint16_t kern_ids[4] = {1U, 2U, 2U, 3U};
    int8_t kern_values[2] = {-2, 1};
    lv_font_fmt_txt_cmap_t cmap;
    lv_font_fmt_txt_kern_pair_t kern;
    lv_font_fmt_txt_dsc_t dsc;
    lv_font_t source;
    lv_font_t destination;
    const lv_font_t *sources[1];
    lv_font_t *destinations[1];
    font_cache_lvgl_plan_t plan;
    uint8_t package[2048];
    uint32_t package_size;
    uint32_t root_offset;
    lv_font_t *stored_root;
    const lv_font_fmt_txt_dsc_t *loaded_dsc;
    int failures = 0;

    if (FONT_CACHE_BLOB_MAX_SEGMENTS < 175U)
    {
        printf("FAIL real_font_segment_capacity actual=%u\n",
               (unsigned int)FONT_CACHE_BLOB_MAX_SEGMENTS);
        failures++;
    }

    memset(bitmap, 0x5A, sizeof(bitmap));
    memset(glyphs, 0, sizeof(glyphs));
    memset(&cmap, 0, sizeof(cmap));
    cmap.range_start = 0x4E00U;
    cmap.range_length = 10U;
    cmap.glyph_id_start = 1U;
    cmap.unicode_list = unicode_list;
    cmap.glyph_id_ofs_list = glyph_offsets;
    cmap.list_length = 3U;
    cmap.type = LV_FONT_FMT_TXT_CMAP_SPARSE_FULL;
    memset(&kern, 0, sizeof(kern));
    kern.glyph_ids = kern_ids;
    kern.values = kern_values;
    kern.pair_cnt = 2U;
    kern.glyph_ids_size = 1U;
    memset(&dsc, 0, sizeof(dsc));
    dsc.glyph_bitmap = bitmap;
    dsc.glyph_dsc = glyphs;
    dsc.cmaps = &cmap;
    dsc.kern_dsc = &kern;
    dsc.kern_scale = 16U;
    dsc.cmap_num = 1U;
    dsc.bpp = 4U;
    dsc.kern_classes = 0U;
    memset(&source, 0, sizeof(source));
    source.dsc = &dsc;
    source.get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt;
    source.get_glyph_bitmap = lv_font_get_bitmap_fmt_txt;
    source.line_height = 20;
    source.base_line = 4;
    source.underline_position = -2;
    source.underline_thickness = 1;
    sources[0] = &source;
    destinations[0] = &destination;
    g_allocations[0].pointer = bitmap;
    g_allocations[0].size = sizeof(bitmap);
    g_allocations[1].pointer = glyphs;
    g_allocations[1].size = sizeof(glyphs);

    failures += expect(
        "plan",
        font_cache_lvgl_plan_init(&plan, sources, 1U,
                                  fake_allocation_size, NULL, 0xA1B2C3D4UL),
        FONT_CACHE_LVGL_OK);
    package_size = font_cache_blob_plan_size(&plan.blob);
    if ((package_size == 0U) || (package_size > sizeof(package)))
    {
        printf("FAIL package_size size=%lu\n", (unsigned long)package_size);
        failures++;
    }
    failures += expect(
        "read_package",
        font_cache_blob_plan_read(&plan.blob, 0U, package, package_size),
        FONT_CACHE_BLOB_OK);
    root_offset = font_cache_blob_segment_offset(&plan.blob, 1U);
    stored_root = (lv_font_t *)&package[root_offset];
    if ((stored_root->get_glyph_dsc != NULL) ||
        (stored_root->get_glyph_bitmap != NULL) ||
        (stored_root->release_glyph != NULL) ||
        (stored_root->fallback != NULL) || (stored_root->user_data != NULL))
    {
        printf("FAIL absolute_runtime_pointer_in_package\n");
        failures++;
    }

    memset(&destination, 0, sizeof(destination));
    failures += expect(
        "bind",
        font_cache_lvgl_bind(package, package_size, destinations, 1U,
                             0xA1B2C3D4UL),
        FONT_CACHE_LVGL_OK);
    loaded_dsc = (const lv_font_fmt_txt_dsc_t *)destination.dsc;
    if ((destination.line_height != 20) || (destination.base_line != 4) ||
        (destination.get_glyph_dsc != lv_font_get_glyph_dsc_fmt_txt) ||
        (destination.get_glyph_bitmap != lv_font_get_bitmap_fmt_txt) ||
        (destination.release_glyph == NULL) || (loaded_dsc == NULL) ||
        (loaded_dsc->cmap_num != 1U) ||
        (loaded_dsc->cmaps[0].unicode_list[2] != 9U) ||
        (((const uint16_t *)loaded_dsc->cmaps[0].glyph_id_ofs_list)[1] != 1U) ||
        (loaded_dsc->glyph_bitmap[5] != 0x5AU) ||
        (((const lv_font_fmt_txt_kern_pair_t *)loaded_dsc->kern_dsc)
             ->values[0] != -2))
    {
        printf("FAIL bound_font_graph\n");
        failures++;
    }
    failures += expect(
        "source_crc_mismatch",
        font_cache_lvgl_bind(package, package_size, destinations, 1U,
                             0xA1B2C3D5UL),
        FONT_CACHE_LVGL_INCOMPATIBLE);

    if (failures != 0)
    {
        printf("FAIL font_cache_lvgl failures=%d\n", failures);
        return 1;
    }
    printf("PASS font_cache_lvgl\n");
    return 0;
}
