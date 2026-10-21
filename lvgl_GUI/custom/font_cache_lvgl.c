#include "font_cache_lvgl.h"

#include <stddef.h>
#include <string.h>
#include "src/font/fmt_txt/lv_font_fmt_txt.h"
#define FONT_CACHE_LVGL_MAGIC   0x544E464CUL
#define FONT_CACHE_LVGL_VERSION 1U

typedef struct
{
    font_cache_blob_segment_t segments[FONT_CACHE_BLOB_MAX_SEGMENTS];
    font_cache_blob_relocation_t relocations[FONT_CACHE_BLOB_MAX_RELOCATIONS];
    uint16_t segment_count;
    uint16_t relocation_count;
} lvgl_builder_t;

static uint32_t hash_value(uint32_t hash, uint32_t value)
{
    uint32_t index;
    for (index = 0U; index < 4U; index++)
    {
        hash ^= (uint8_t)(value >> (index * 8U));
        hash *= 16777619UL;
    }
    return hash;
}

uint32_t font_cache_lvgl_abi_id(void)
{
    uint32_t hash = 2166136261UL;

    hash = hash_value(hash, (uint32_t)LVGL_VERSION_MAJOR);
    hash = hash_value(hash, (uint32_t)LVGL_VERSION_MINOR);
    hash = hash_value(hash, (uint32_t)LVGL_VERSION_PATCH);
    hash = hash_value(hash, (uint32_t)sizeof(void *));
    hash = hash_value(hash, (uint32_t)sizeof(lv_font_t));
    hash = hash_value(hash, (uint32_t)sizeof(lv_font_fmt_txt_dsc_t));
    hash = hash_value(hash, (uint32_t)sizeof(lv_font_fmt_txt_cmap_t));
    hash = hash_value(hash, (uint32_t)sizeof(lv_font_fmt_txt_glyph_dsc_t));
    hash = hash_value(hash, (uint32_t)sizeof(lv_font_fmt_txt_kern_pair_t));
    hash = hash_value(hash, (uint32_t)sizeof(lv_font_fmt_txt_kern_classes_t));
    hash = hash_value(hash, (uint32_t)offsetof(lv_font_t, dsc));
    hash = hash_value(hash,
                      (uint32_t)offsetof(lv_font_fmt_txt_dsc_t, cmaps));
    return hash;
}

static int add_segment(lvgl_builder_t *builder, const void *source,
                       uint32_t size, uint32_t alignment, uint16_t *index)
{
    uint16_t current;

    if ((source == NULL) || (size == 0U) ||
        (builder->segment_count >= FONT_CACHE_BLOB_MAX_SEGMENTS))
        return FONT_CACHE_LVGL_TOO_COMPLEX;
    current = builder->segment_count++;
    builder->segments[current].source = source;
    builder->segments[current].size = size;
    builder->segments[current].alignment = alignment;
    *index = current;
    return FONT_CACHE_LVGL_OK;
}

static int add_relocation(lvgl_builder_t *builder, uint16_t owner,
                          uint32_t field, uint16_t target)
{
    font_cache_blob_relocation_t *relocation;

    if (builder->relocation_count >= FONT_CACHE_BLOB_MAX_RELOCATIONS)
        return FONT_CACHE_LVGL_TOO_COMPLEX;
    relocation = &builder->relocations[builder->relocation_count++];
    relocation->owner_segment = owner;
    relocation->pointer_offset = field;
    relocation->target_segment = target;
    return FONT_CACHE_LVGL_OK;
}

static int add_sized_segment(lvgl_builder_t *builder, const void *source,
                             font_cache_lvgl_allocation_size_fn size_fn,
                             void *context, uint16_t *index)
{
    uint32_t size;

    if (source == NULL)
        return FONT_CACHE_LVGL_INVALID_ARG;
    size = size_fn(source, context);
    if (size == 0U)
        return FONT_CACHE_LVGL_ALLOCATION_UNKNOWN;
    return add_segment(builder, source, size, sizeof(void *), index);
}

static int add_cmaps(lvgl_builder_t *builder,
                     const lv_font_fmt_txt_dsc_t *dsc, uint16_t dsc_segment)
{
    uint16_t cmaps_segment;
    uint16_t list_segment;
    uint32_t index;
    uint32_t field;
    uint32_t size;
    int result;

    if ((dsc->cmap_num == 0U) || (dsc->cmaps == NULL))
        return FONT_CACHE_LVGL_FORMAT_ERROR;
    result = add_segment(builder, dsc->cmaps,
                         dsc->cmap_num * sizeof(dsc->cmaps[0]),
                         sizeof(void *), &cmaps_segment);
    if (result != FONT_CACHE_LVGL_OK)
        return result;
    add_relocation(builder, dsc_segment,
                   offsetof(lv_font_fmt_txt_dsc_t, cmaps), cmaps_segment);
    for (index = 0U; index < dsc->cmap_num; index++)
    {
        const lv_font_fmt_txt_cmap_t *cmap = &dsc->cmaps[index];
        field = index * sizeof(*cmap);
        if (cmap->unicode_list != NULL)
        {
            size = (uint32_t)cmap->list_length * sizeof(uint16_t);
            result = add_segment(builder, cmap->unicode_list, size,
                                 sizeof(uint16_t), &list_segment);
            if (result != FONT_CACHE_LVGL_OK)
                return result;
            add_relocation(builder, cmaps_segment,
                           field + offsetof(lv_font_fmt_txt_cmap_t,
                                            unicode_list), list_segment);
        }
        if (cmap->glyph_id_ofs_list != NULL)
        {
            size = (uint32_t)cmap->list_length *
                   ((cmap->type == LV_FONT_FMT_TXT_CMAP_FORMAT0_FULL) ?
                    sizeof(uint8_t) : sizeof(uint16_t));
            result = add_segment(builder, cmap->glyph_id_ofs_list, size,
                                 sizeof(uint16_t), &list_segment);
            if (result != FONT_CACHE_LVGL_OK)
                return result;
            add_relocation(builder, cmaps_segment,
                           field + offsetof(lv_font_fmt_txt_cmap_t,
                                            glyph_id_ofs_list), list_segment);
        }
    }
    return FONT_CACHE_LVGL_OK;
}

static int add_kern(lvgl_builder_t *builder,
                    const lv_font_fmt_txt_dsc_t *dsc, uint16_t dsc_segment,
                    font_cache_lvgl_allocation_size_fn size_fn, void *context)
{
    uint16_t kern_segment;
    uint16_t data_segment;
    uint32_t size;
    int result;

    if (dsc->kern_dsc == NULL)
        return FONT_CACHE_LVGL_OK;
    if (dsc->kern_classes == 0U)
    {
        const lv_font_fmt_txt_kern_pair_t *kern = dsc->kern_dsc;
        result = add_segment(builder, kern, sizeof(*kern), sizeof(void *),
                             &kern_segment);
        if (result != FONT_CACHE_LVGL_OK)
            return result;
        size = kern->pair_cnt * 2U *
               ((kern->glyph_ids_size == 0U) ? 1U : 2U);
        result = add_segment(builder, kern->glyph_ids, size, 2U, &data_segment);
        if (result != FONT_CACHE_LVGL_OK)
            return result;
        add_relocation(builder, kern_segment,
                       offsetof(lv_font_fmt_txt_kern_pair_t, glyph_ids),
                       data_segment);
        result = add_segment(builder, kern->values, kern->pair_cnt, 1U,
                             &data_segment);
        if (result != FONT_CACHE_LVGL_OK)
            return result;
        add_relocation(builder, kern_segment,
                       offsetof(lv_font_fmt_txt_kern_pair_t, values),
                       data_segment);
    }
    else
    {
        const lv_font_fmt_txt_kern_classes_t *kern = dsc->kern_dsc;
        result = add_segment(builder, kern, sizeof(*kern), sizeof(void *),
                             &kern_segment);
        if (result != FONT_CACHE_LVGL_OK)
            return result;
        result = add_sized_segment(builder, kern->left_class_mapping,
                                   size_fn, context, &data_segment);
        if (result != FONT_CACHE_LVGL_OK)
            return result;
        add_relocation(builder, kern_segment,
                       offsetof(lv_font_fmt_txt_kern_classes_t,
                                left_class_mapping), data_segment);
        result = add_sized_segment(builder, kern->right_class_mapping,
                                   size_fn, context, &data_segment);
        if (result != FONT_CACHE_LVGL_OK)
            return result;
        add_relocation(builder, kern_segment,
                       offsetof(lv_font_fmt_txt_kern_classes_t,
                                right_class_mapping), data_segment);
        size = (uint32_t)kern->left_class_cnt * kern->right_class_cnt;
        result = add_segment(builder, kern->class_pair_values, size, 1U,
                             &data_segment);
        if (result != FONT_CACHE_LVGL_OK)
            return result;
        add_relocation(builder, kern_segment,
                       offsetof(lv_font_fmt_txt_kern_classes_t,
                                class_pair_values), data_segment);
    }
    add_relocation(builder, dsc_segment,
                   offsetof(lv_font_fmt_txt_dsc_t, kern_dsc), kern_segment);
    return FONT_CACHE_LVGL_OK;
}

static int add_font_data(lvgl_builder_t *builder, const lv_font_t *font,
                         uint16_t root_segment,
                         font_cache_lvgl_allocation_size_fn size_fn,
                         void *context)
{
    const lv_font_fmt_txt_dsc_t *dsc;
    uint16_t dsc_segment;
    uint16_t data_segment;
    int result;

    if ((font == NULL) || (font->dsc == NULL))
        return FONT_CACHE_LVGL_INVALID_ARG;
    dsc = font->dsc;
    result = add_segment(builder, dsc, sizeof(*dsc), sizeof(void *),
                         &dsc_segment);
    if (result != FONT_CACHE_LVGL_OK)
        return result;
    add_relocation(builder, root_segment, offsetof(lv_font_t, dsc),
                   dsc_segment);
    result = add_sized_segment(builder, dsc->glyph_bitmap, size_fn, context,
                               &data_segment);
    if (result != FONT_CACHE_LVGL_OK)
        return result;
    add_relocation(builder, dsc_segment,
                   offsetof(lv_font_fmt_txt_dsc_t, glyph_bitmap), data_segment);
    result = add_sized_segment(builder, dsc->glyph_dsc, size_fn, context,
                               &data_segment);
    if (result != FONT_CACHE_LVGL_OK)
        return result;
    add_relocation(builder, dsc_segment,
                   offsetof(lv_font_fmt_txt_dsc_t, glyph_dsc), data_segment);
    result = add_cmaps(builder, dsc, dsc_segment);
    if (result != FONT_CACHE_LVGL_OK)
        return result;
    return add_kern(builder, dsc, dsc_segment, size_fn, context);
}

int font_cache_lvgl_plan_init(font_cache_lvgl_plan_t *plan,
                              const lv_font_t *const *fonts,
                              uint32_t font_count,
                              font_cache_lvgl_allocation_size_fn size_fn,
                              void *size_context, uint32_t source_crc)
{
    lvgl_builder_t builder;
    uint16_t root_segment;
    uint32_t index;
    int result;

    if ((plan == NULL) || (fonts == NULL) || (size_fn == NULL) ||
        (font_count == 0U) || (font_count > FONT_CACHE_LVGL_MAX_FONTS))
        return FONT_CACHE_LVGL_INVALID_ARG;
    memset(plan, 0, sizeof(*plan));
    memset(&builder, 0, sizeof(builder));
    plan->manifest.magic = FONT_CACHE_LVGL_MAGIC;
    plan->manifest.version = FONT_CACHE_LVGL_VERSION;
    plan->manifest.font_count = (uint16_t)font_count;
    add_segment(&builder, &plan->manifest, sizeof(plan->manifest), 4U,
                &root_segment);
    for (index = 0U; index < font_count; index++)
    {
        if (fonts[index] == NULL)
            return FONT_CACHE_LVGL_INVALID_ARG;
        plan->font_copies[index] = *fonts[index];
        plan->font_copies[index].fallback = NULL;
        plan->font_copies[index].user_data = NULL;
        plan->font_copies[index].get_glyph_dsc = NULL;
        plan->font_copies[index].get_glyph_bitmap = NULL;
        plan->font_copies[index].release_glyph = NULL;
        result = add_segment(&builder, &plan->font_copies[index],
                             sizeof(lv_font_t), sizeof(void *), &root_segment);
        if (result != FONT_CACHE_LVGL_OK)
            return result;
        plan->manifest.root_segments[index] = root_segment;
    }
    for (index = 0U; index < font_count; index++)
    {
        result = add_font_data(&builder, fonts[index],
                               plan->manifest.root_segments[index], size_fn,
                               size_context);
        if (result != FONT_CACHE_LVGL_OK)
            return result;
    }
    result = font_cache_blob_plan_init(
        &plan->blob, builder.segments, builder.segment_count,
        builder.relocations, builder.relocation_count,
        font_cache_lvgl_abi_id(), source_crc);
    return (result == FONT_CACHE_BLOB_OK) ? FONT_CACHE_LVGL_OK :
                                           FONT_CACHE_LVGL_TOO_COMPLEX;
}
