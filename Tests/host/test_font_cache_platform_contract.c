#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    long length;
    char *text;

    if (file == NULL)
        return NULL;
    if ((fseek(file, 0, SEEK_END) != 0) || ((length = ftell(file)) < 0) ||
        (fseek(file, 0, SEEK_SET) != 0))
    {
        fclose(file);
        return NULL;
    }
    text = malloc((size_t)length + 1U);
    if ((text == NULL) ||
        (fread(text, 1U, (size_t)length, file) != (size_t)length))
    {
        free(text);
        fclose(file);
        return NULL;
    }
    text[length] = '\0';
    fclose(file);
    return text;
}

static int require_text(const char *source, const char *needle,
                        const char *name)
{
    if ((source != NULL) && (strstr(source, needle) != NULL))
        return 0;
    printf("FAIL %s\n", name);
    return 1;
}

int main(void)
{
    const char *path = "Core/Application/Src/font_cache_service_platform.c";
    char *source = read_file(path);
    int failures = 0;

    failures += require_text(source, "nand_readonly_probe(", "nand_probe");
    failures += require_text(source, "nand_cache_layout_build(", "bad_block_layout");
    failures += require_text(source, "NAND_CACHE_PAGE_MAIN_SIZE +",
                             "combined_page_buffer");
    failures += require_text(source, "nand_cache_io_program_page(",
                             "raw_page_program");
    failures += require_text(source, "nand_readonly_read_page(",
                             "raw_page_read");
    failures += require_text(source, "font_cache_source_collection_crc(",
                             "source_collection_crc");
    failures += require_text(source, "QSPI_FONT12_OFFSET", "font12_source");
    failures += require_text(source, "QSPI_FONT30_OFFSET", "font30_source");
    failures += require_text(source, "lv_tlsf_block_size(",
                             "allocation_block_size");
    failures += require_text(source, "lv_malloc(", "final_arena_allocate");
    failures += require_text(source, "lv_free(", "failed_arena_free");
    failures += require_text(source, "font_cache_service_progress_fn progress",
                             "load_progress_callback");
    failures += require_text(source, "nand_cache_store_load_progress(",
                             "slot_load_progress");
    failures += require_text(source, "nand_cache_store_write_stream(",
                             "stream_rebuild");
    failures += require_text(source, "font_cache_lvgl_plan_init(",
                             "lvgl_graph_plan");
    failures += require_text(source, "font_cache_lvgl_bind(",
                             "lvgl_graph_bind");
    failures += require_text(source, "nand_cache_io_get_diagnostics(",
                             "nand_io_diagnostics");
    failures += require_text(source,
                             "font_cache_service_platform_get_rebuild_diagnostics",
                             "platform_rebuild_diagnostics");
    failures += require_text(source, "&lv_font_myyh_12", "font12_publish");
    failures += require_text(source, "&lv_font_myyh_30", "font30_publish");

    free(source);
    if (failures != 0)
    {
        printf("FAIL font_cache_platform_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS font_cache_platform_contract\n");
    return 0;
}
