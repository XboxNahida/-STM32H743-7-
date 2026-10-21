#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_source(const char *path)
{
    FILE *file = fopen(path, "rb");
    long length;
    char *source;

    if (file == NULL)
        return NULL;
    if ((fseek(file, 0L, SEEK_END) != 0) ||
        ((length = ftell(file)) < 0L) ||
        (fseek(file, 0L, SEEK_SET) != 0))
    {
        fclose(file);
        return NULL;
    }
    source = malloc((size_t)length + 1U);
    if (source == NULL)
    {
        fclose(file);
        return NULL;
    }
    if (fread(source, 1U, (size_t)length, file) != (size_t)length)
    {
        free(source);
        fclose(file);
        return NULL;
    }
    source[length] = '\0';
    fclose(file);
    return source;
}

static int require_text(const char *source, const char *needle,
                        const char *name)
{
    if (strstr(source, needle) != NULL)
        return 0;
    printf("FAIL %s\n", name);
    return 1;
}

static int require_order(const char *source, const char *first,
                         const char *second, const char *name)
{
    const char *left = strstr(source, first);
    const char *right = strstr(source, second);

    if ((left != NULL) && (right != NULL) && (left < right))
        return 0;
    printf("FAIL %s\n", name);
    return 1;
}

int main(void)
{
    char *main_source = read_source("Core/Src/main.c");
    char *nor_header = read_source("Core/Peripheral/Inc/norflash.h");
    char *nor_source = read_source("Core/Peripheral/Src/norflash.c");
    const char *startup_before_fonts;
    const char *cache_load;
    int failures = 0;

    if ((main_source == NULL) || (nor_header == NULL) ||
        (nor_source == NULL))
    {
        printf("FAIL read_startup_sources\n");
        free(main_source);
        free(nor_header);
        free(nor_source);
        return 1;
    }
    failures += require_text(main_source, "#include \"font_cache_service.h\"",
                             "main_include");
    failures += require_text(main_source, "font_cache_service_init();",
                             "service_init");
    failures += require_text(main_source, "font_cache_service_try_load()",
                             "try_load");
    failures += require_order(main_source, "norflash_preload_images();",
                              "font_cache_service_try_load()",
                              "cache_after_images");
    failures += require_order(main_source, "font_cache_service_try_load()",
                              "boot_progress_load_font(\"Font 12\", norflash_init_font12,",
                              "cache_before_parse");
    failures += require_text(main_source,
                             "boot_progress_set_current(\"Font cache\",",
                             "cache_visible_stage");
    startup_before_fonts = strstr(main_source, "diagnostics_init();");
    cache_load = (startup_before_fonts != NULL) ?
                 strstr(startup_before_fonts,
                        "font_cache_service_try_load()") : NULL;
    if ((startup_before_fonts == NULL) || (cache_load == NULL) ||
        ((strstr(startup_before_fonts, "\\xE") != NULL) &&
         (strstr(startup_before_fonts, "\\xE") < cache_load)))
    {
        printf("FAIL startup_before_fonts_is_english\n");
        failures++;
    }
    failures += require_text(main_source, "\"Camera service\"",
                             "english_camera_stage");
    failures += require_text(main_source, "\"Peripheral probe\"",
                             "english_probe_stage");
    failures += require_text(main_source, "\"Image resources\"",
                             "english_image_stage");
    failures += require_order(main_source,
                              "boot_progress_set_current(\"Font cache\",",
                              "font_cache_service_try_load()",
                              "cache_stage_before_load");
    failures += require_text(main_source,
                             "#define FONT_CACHE_PROGRESS_STEP_PERCENT 5U",
                             "cache_progress_step");
    failures += require_text(main_source,
                             "#define FONT_CACHE_PROGRESS_REFRESH_MS 500U",
                             "cache_progress_time_throttle");
    failures += require_text(main_source,
                             "boot_progress_font_cache_callback",
                             "cache_progress_callback");
    failures += require_order(
        main_source,
        "font_cache_service_set_load_progress_callback(\n"
        "      boot_progress_font_cache_callback, NULL);",
        "font_cache_service_try_load()",
        "cache_progress_registered_before_load");
    failures += require_order(
        main_source, "font_cache_service_try_load()",
        "font_cache_service_set_load_progress_callback(NULL, NULL);",
        "cache_progress_cleared_after_load");
    failures += require_text(main_source,
                             "boot_progress_ui_set_font(&lv_font_myyh_20);",
                             "cache_switches_font20");
    failures += require_text(main_source, "if (font_cache_hit != 0U)",
                             "cache_hit_branch");
    failures += require_text(main_source, "norflash_set_fonts_ready(6U);",
                             "cache_ready_count");
    failures += require_text(main_source, "font_cache_service_rebuild();",
                             "fallback_rebuild");
    failures += require_text(main_source,
                             "stage=%u target=%lu io_result=%ld nand_status=0x%02X status_valid=%u ready=%u",
                             "font_cache_boot_diagnostics");
    if (strstr(main_source, "nand_readonly_init();") != NULL)
    {
        printf("FAIL duplicate_nand_init\n");
        failures++;
    }
    failures += require_text(nor_header,
                             "void     norflash_set_fonts_ready(uint8_t count);",
                             "ready_api");
    failures += require_text(nor_source,
                             "void norflash_set_fonts_ready(uint8_t count)",
                             "ready_implementation");
    failures += require_text(nor_source,
                             "g_boot_status.fonts_ready =",
                             "ready_assignment");
    free(main_source);
    free(nor_header);
    free(nor_source);
    if (failures != 0)
    {
        printf("FAIL font_cache_startup_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS font_cache_startup_contract\n");
    return 0;
}
