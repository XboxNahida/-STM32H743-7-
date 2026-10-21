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

int main(void)
{
    char *source = read_source("Core/Peripheral/Src/command_parser.c");
    char *header = read_source("Core/Peripheral/Inc/command_parser.h");
    int failures = 0;

    if ((source == NULL) || (header == NULL))
    {
        printf("FAIL read_command_parser_files\n");
        free(source);
        free(header);
        return 1;
    }
    failures += require_text(source, "#include \"font_cache_service.h\"",
                             "font_cache_include");
    failures += require_text(source, "command_font_cache_handler",
                             "font_cache_handler");
    failures += require_text(source, "font_cache_service_get_status",
                             "font_cache_status");
    failures += require_text(source, "font_cache_service_rebuild()",
                             "font_cache_rebuild");
    failures += require_text(source, "font_cache_service_invalidate()",
                             "font_cache_invalidate");
    failures += require_text(source,
                             "font_cache status state=%u slot=%u sequence=%lu",
                             "font_cache_status_output");
    failures += require_text(source,
                             "stage=%u target=%lu io_result=%ld nand_status=0x%02X status_valid=%u ready=%u",
                             "font_cache_diagnostics_output");
    failures += require_text(source,
                             "font_cache status|rebuild|invalidate",
                             "font_cache_usage");
    failures += require_text(source, "{\"font_cache\"",
                             "font_cache_registration");
    failures += require_text(header, "#define COMMAND_MAX_ENTRIES     40U",
                             "command_capacity");
    free(source);
    free(header);
    if (failures != 0)
    {
        printf("FAIL font_cache_command_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS font_cache_command_contract\n");
    return 0;
}
