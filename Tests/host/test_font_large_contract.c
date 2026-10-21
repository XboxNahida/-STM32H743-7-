#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ONE_MIB (1024UL * 1024UL)
#define CJK_FIRST 0x4E00UL
#define CJK_LAST 0x9FFFUL
#define CJK_COUNT (CJK_LAST - CJK_FIRST + 1UL)

static uint16_t read_u16_le(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static uint32_t read_u32_le(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

static uint8_t *read_file(const char *path, size_t *length)
{
    FILE *file;
    long file_length;
    uint8_t *data;

    if ((path == NULL) || (length == NULL))
    {
        return NULL;
    }

    file = fopen(path, "rb");
    if (file == NULL)
    {
        return NULL;
    }
    if ((fseek(file, 0L, SEEK_END) != 0) ||
        ((file_length = ftell(file)) < 0L) ||
        (fseek(file, 0L, SEEK_SET) != 0))
    {
        (void)fclose(file);
        return NULL;
    }

    data = (uint8_t *)malloc((size_t)file_length + 1U);
    if (data == NULL)
    {
        (void)fclose(file);
        return NULL;
    }
    if (fread(data, 1U, (size_t)file_length, file) != (size_t)file_length)
    {
        free(data);
        (void)fclose(file);
        return NULL;
    }
    data[file_length] = 0U;
    *length = (size_t)file_length;
    (void)fclose(file);
    return data;
}

static int expect_true(const char *name, int condition)
{
    if (condition == 0)
    {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

int main(void)
{
    uint8_t *config;
    uint8_t *font;
    size_t config_length = 0U;
    size_t font_length = 0U;
    uint32_t head_length;
    uint32_t cmap_length;
    uint32_t cmap_count;
    uint32_t loca_offset;
    uint32_t loca_length;
    uint32_t glyf_offset;
    uint32_t glyf_length;
    uint32_t cjk_covered = 0U;
    uint32_t index;
    int failures = 0;

    config = read_file("lvgl/lv_conf.h", &config_length);
    font = read_file("Tools/bin_files/lv_font_myh_ui_20.bin", &font_length);
    failures += expect_true("read_lv_conf", config != NULL);
    failures += expect_true("read_font20_bin", font != NULL);
    if ((config == NULL) || (font == NULL))
    {
        free(config);
        free(font);
        return 1;
    }

    failures += expect_true("large_font_mode_enabled",
                            strstr((const char *)config,
                                   "#define LV_FONT_FMT_TXT_LARGE 1") != NULL);
    failures += expect_true("font20_file_exceeds_small_limit",
                            font_length > ONE_MIB);
    failures += expect_true("font20_header_available", font_length >= 48U);
    if (font_length < 48U)
    {
        free(config);
        free(font);
        return 1;
    }

    head_length = read_u32_le(font);
    failures += expect_true("font20_head_label",
                            memcmp(font + 4U, "head", 4U) == 0);
    failures += expect_true("font20_pixel_size",
                            read_u16_le(font + 14U) == 20U);
    failures += expect_true("font20_head_bounds",
                            (head_length >= 32U) &&
                            ((size_t)head_length + 12U <= font_length));
    if ((head_length < 32U) || ((size_t)head_length + 12U > font_length))
    {
        free(config);
        free(font);
        return 1;
    }

    cmap_length = read_u32_le(font + head_length);
    cmap_count = read_u32_le(font + head_length + 8U);
    failures += expect_true("font20_cmap_label",
                            memcmp(font + head_length + 4U, "cmap", 4U) == 0);
    failures += expect_true("font20_cmap_bounds",
                            ((size_t)head_length + cmap_length <= font_length) &&
                            ((size_t)head_length + 12U +
                             ((size_t)cmap_count * 16U) <= font_length));
    if (((size_t)head_length + cmap_length > font_length) ||
        ((size_t)head_length + 12U + ((size_t)cmap_count * 16U) > font_length))
    {
        free(config);
        free(font);
        return 1;
    }

    for (index = 0U; index < cmap_count; index++)
    {
        const uint8_t *entry = font + head_length + 12U + (index * 16U);
        uint32_t range_start = read_u32_le(entry + 4U);
        uint32_t range_length = read_u16_le(entry + 8U);
        uint32_t range_end = range_start + range_length - 1U;
        uint32_t overlap_start;
        uint32_t overlap_end;

        if ((range_length == 0U) || (range_end < CJK_FIRST) ||
            (range_start > CJK_LAST))
        {
            continue;
        }
        overlap_start = (range_start > CJK_FIRST) ? range_start : CJK_FIRST;
        overlap_end = (range_end < CJK_LAST) ? range_end : CJK_LAST;
        cjk_covered += overlap_end - overlap_start + 1U;
    }
    failures += expect_true("font20_full_basic_cjk", cjk_covered == CJK_COUNT);

    loca_offset = head_length + cmap_length;
    failures += expect_true("font20_loca_bounds",
                            (size_t)loca_offset + 12U <= font_length);
    if ((size_t)loca_offset + 12U > font_length)
    {
        free(config);
        free(font);
        return 1;
    }
    loca_length = read_u32_le(font + loca_offset);
    glyf_offset = loca_offset + loca_length;
    failures += expect_true("font20_glyf_bounds",
                            (size_t)glyf_offset + 8U <= font_length);
    if ((size_t)glyf_offset + 8U > font_length)
    {
        free(config);
        free(font);
        return 1;
    }
    glyf_length = read_u32_le(font + glyf_offset);
    failures += expect_true("font20_glyph_table_exceeds_small_limit",
                            glyf_length > ONE_MIB);
    failures += expect_true("font20_glyph_table_bounds",
                            (size_t)glyf_offset + glyf_length <= font_length);

    free(config);
    free(font);
    if (failures == 0)
    {
        printf("PASS font_large_contract cjk=%lu glyf=%lu\n",
               (unsigned long)cjk_covered,
               (unsigned long)glyf_length);
        return 0;
    }
    printf("FAIL font_large_contract failures=%d\n", failures);
    return 1;
}
