#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_source(const char *path)
{
    FILE *file = fopen(path, "rb");
    long size;
    char *data;

    if (file == NULL)
    {
        return NULL;
    }
    if ((fseek(file, 0L, SEEK_END) != 0) ||
        ((size = ftell(file)) < 0L) ||
        (fseek(file, 0L, SEEK_SET) != 0))
    {
        (void)fclose(file);
        return NULL;
    }
    data = (char *)malloc((size_t)size + 1U);
    if (data == NULL)
    {
        (void)fclose(file);
        return NULL;
    }
    if (fread(data, 1U, (size_t)size, file) != (size_t)size)
    {
        free(data);
        (void)fclose(file);
        return NULL;
    }
    data[size] = '\0';
    (void)fclose(file);
    return data;
}

static int expect_contains(const char *text, const char *name,
                           const char *needle)
{
    if (strstr(text, needle) == NULL)
    {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

static unsigned int count_text(const char *text, const char *needle)
{
    unsigned int count = 0U;
    size_t needle_length = strlen(needle);

    while ((text = strstr(text, needle)) != NULL)
    {
        count++;
        text += needle_length;
    }
    return count;
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
    static const char *const header_contract[] =
    {
        "NORFLASH_PROGRESS_ERASE = 0", "NORFLASH_PROGRESS_COPY",
        "NORFLASH_PROGRESS_VERIFY", "NORFLASH_PROGRESS_LOAD",
        "NORFLASH_PROGRESS_DONE", "NORFLASH_PROGRESS_ERROR",
        "const char *item;", "norflash_progress_action_t action;",
        "uint32_t processed;", "uint32_t total;", "uint8_t percent;",
        "typedef void (*norflash_progress_callback_t)(const norflash_progress_t *progress,",
        "void norflash_set_progress_callback(norflash_progress_callback_t callback,"
    };
    static const char *const source_contract[] =
    {
        "static norflash_progress_callback_t g_progress_callback;", "static void *g_progress_context;",
        "void norflash_set_progress_callback(norflash_progress_callback_t callback,",
        "static void norflash_report_progress(const char *item,",
        "((uint64_t)processed * 100U) / total",
        "progress.percent = 100U;",
        "NORFLASH_PROGRESS_ERASE", "NORFLASH_PROGRESS_COPY",
        "NORFLASH_PROGRESS_VERIFY", "NORFLASH_PROGRESS_LOAD",
        "NORFLASH_PROGRESS_DONE", "NORFLASH_PROGRESS_ERROR",
        "norflash_erase_sector(start_sector + s);",
        "res = f_read(&fil, buf, 256, &br);",
        "norflash_read(vbuf, qspi_offset, 256);",
        "norflash_read(chunk_data, qspi_offset + offset, (uint16_t)chunk);",
        "\"wallpaper\"", "\"img1\"", "\"img2\"", "\"img3\"",
        "\"font12\"", "\"font14\"", "\"font16\"",
        "\"font18\"", "\"font20\"", "\"font30\""
    };
    static const char *const preserved_header[] =
    {
        "#define QSPI_WALLPAPER_OFFSET   0x00000000UL", "#define QSPI_IMG1_OFFSET         0x00140000UL",
        "#define QSPI_IMG2_OFFSET         0x00160000UL", "#define QSPI_IMG3_OFFSET         0x00220000UL",
        "#define QSPI_FONT12_OFFSET       0x00300000UL", "#define QSPI_FONT20_OFFSET       0x00B00000UL",
        "#define QSPI_FONT30_OFFSET       0x01100000UL"
    };
    static const char *const preserved_source[] =
    {
        "saddr *= 4096;", "uint8_t buf[256];", "sectors = (total + 4095) / 4096;",
        "0:/SYSTEM/IMG/_PPC_lock_RGB565_1024x600.bin",
        "0:/SYSTEM/IMG/_1_RGB565_285x107.bin",
        "0:/SYSTEM/IMG/_2_RGB565_700x370.bin",
        "0:/SYSTEM/IMG/_3_RGB565_600x220.bin",
        "0:/SYSTEM/FONT/lv_font_myh_ui_20.bin",
        "if (vbuf[i] != buf[i])"
    };
    char *header = read_source("Core/Peripheral/Inc/norflash.h");
    char *source = read_source("Core/Peripheral/Src/norflash.c");
    unsigned int index;
    int failures = 0;

    if ((header == NULL) || (source == NULL))
    {
        free(header);
        free(source);
        printf("FAIL source_open\n");
        return 1;
    }
    for (index = 0U;
         index < (sizeof(header_contract) / sizeof(header_contract[0]));
         index++)
    {
        failures += expect_contains(header, "header_contract",
                                    header_contract[index]);
    }
    for (index = 0U;
         index < (sizeof(source_contract) / sizeof(source_contract[0]));
         index++)
    {
        failures += expect_contains(source, "source_contract",
                                    source_contract[index]);
    }
    for (index = 0U;
         index < (sizeof(preserved_header) / sizeof(preserved_header[0]));
         index++)
    {
        failures += expect_contains(header, "preserved_header",
                                    preserved_header[index]);
    }
    for (index = 0U;
         index < (sizeof(preserved_source) / sizeof(preserved_source[0]));
         index++)
    {
        failures += expect_contains(source, "preserved_source",
                                    preserved_source[index]);
    }
    failures += expect_true(
        "font_fallback_exits_memmap",
        count_text(source,
                   "norflash_exit_memmap();\n    copy_failed = sd_to_qspi_one") == 6U);
    failures += expect_contains(source, "bounded_memmap_abort_helper",
                                "static void norflash_abort_memmap(void)");
    failures += expect_true("memmap_abort_used_for_enter_and_exit",
                            count_text(source, "norflash_abort_memmap();") == 2U);

    free(header);
    free(source);
    if (failures == 0)
    {
        printf("PASS norflash_progress_contract\n");
        return 0;
    }
    printf("FAIL norflash_progress_contract failures=%d\n", failures);
    return 1;
}
