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

static int require_text(const char *source, const char *name,
                        const char *text)
{
    if (strstr(source, text) == NULL)
    {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

static int forbid_text(const char *source, const char *name,
                       const char *text)
{
    if (strstr(source, text) != NULL)
    {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

int main(void)
{
    char *source = read_source("Core/Peripheral/Src/norflash.c");
    int failures = 0;

    if (source == NULL)
    {
        printf("FAIL source_open\n");
        return 1;
    }

    failures += require_text(source, "crc_update_helper",
                             "static uint32_t norflash_crc32_update(");
    failures += require_text(source, "qspi_full_crc_helper",
                             "static uint32_t norflash_read_crc32(");
    failures += require_text(source, "validated_asset_copy",
                             "static uint8_t sd_to_qspi_asset(");
    failures += require_text(source, "exact_source_size",
                             "file_size == expected_size");
    failures += require_text(source, "legacy_wrapped_size",
                             "file_size == (expected_size + 7U)");
    failures += require_text(source, "legacy_payload_offset",
                             "source_offset = 4U;");
    failures += require_text(source, "legacy_payload_seek",
                             "f_lseek(&fil, source_offset)");
    failures += require_text(source, "legacy_source_diagnostic",
                             "source=legacy_wrapped prefix=4 suffix=3");
    failures += require_text(source, "source_crc_checked",
                             "source_crc != expected_crc");
    failures += require_text(source, "exit_memmap_before_asset_write",
                             "norflash_exit_memmap();");
    failures += require_text(source, "whole_qspi_readback",
                             "actual_crc = norflash_read_crc32(");
    failures += require_text(source, "asset_hit_log",
                             "[BOOT] asset_cache HIT item=%s");
    failures += require_text(source, "asset_miss_log",
                             "[BOOT] asset_cache MISS item=%s");
    failures += require_text(source, "asset_built_log",
                             "[BOOT] asset_cache BUILT item=%s");
    failures += forbid_text(source, "no_erased_prefix_heuristic",
                            "norflash_is_erased(");

    free(source);
    if (failures != 0)
    {
        printf("FAIL qspi_asset_recovery_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS qspi_asset_recovery_contract\n");
    return 0;
}
