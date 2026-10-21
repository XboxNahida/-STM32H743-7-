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
    char *command_source = read_source("Core/Peripheral/Src/command_parser.c");
    char *norflash_source = read_source("Core/Peripheral/Src/norflash.c");
    char *norflash_header = read_source("Core/Peripheral/Inc/norflash.h");
    int failures = 0;

    if ((command_source == NULL) || (norflash_source == NULL) ||
        (norflash_header == NULL))
    {
        free(command_source);
        free(norflash_source);
        free(norflash_header);
        printf("FAIL source_open\n");
        return 1;
    }

    failures += require_text(command_source, "norflash_include",
                             "#include \"norflash.h\"");
    failures += require_text(command_source, "qspi_handler",
                             "static int command_qspi_handler(");
    failures += require_text(command_source, "confirmation_required",
                             "strcmp(argv[2], \"CONFIRM\") == 0");
    failures += require_text(command_source, "asset_erase_called",
                             "norflash_erase_assets()");
    failures += require_text(command_source, "erase_success_result",
                             "OK qspi action=erase target=assets result=0");
    failures += require_text(command_source, "qspi_usage",
                             "INVALID_ARG usage=qspi erase assets CONFIRM");
    failures += require_text(command_source, "qspi_registration",
                             "{\"qspi\", \"qspi erase assets CONFIRM");

    failures += require_text(norflash_header, "erase_assets_api",
                             "uint8_t  norflash_erase_assets(void);");
    failures += require_text(norflash_source, "erase_assets_implementation",
                             "uint8_t norflash_erase_assets(void)");
    failures += require_text(norflash_source, "exit_memmap_before_erase",
                             "norflash_exit_memmap();");
    failures += require_text(norflash_source, "restore_memmap_after_erase",
                             "norflash_enter_memmap();");
    failures += require_text(norflash_source, "whole_range_erased_check",
                             "norflash_verify_erased_range(");
    failures += require_text(norflash_source, "wallpaper_range",
                             "QSPI_WALLPAPER_OFFSET, QSPI_WALLPAPER_SIZE");
    failures += require_text(norflash_source, "img1_range",
                             "QSPI_IMG1_OFFSET, QSPI_IMG1_SIZE");
    failures += require_text(norflash_source, "img2_range",
                             "QSPI_IMG2_OFFSET, QSPI_IMG2_SIZE");
    failures += require_text(norflash_source, "img3_range",
                             "QSPI_IMG3_OFFSET, QSPI_IMG3_SIZE");
    failures += forbid_text(norflash_source, "asset_erase_is_not_chip_erase",
                            "norflash_erase_chip();");

    free(command_source);
    free(norflash_source);
    free(norflash_header);
    if (failures != 0)
    {
        printf("FAIL qspi_command_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS qspi_command_contract\n");
    return 0;
}
