#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_project(void)
{
    FILE *file = fopen("MDK-ARM/H743_LCD_Keil5.uvprojx", "rb");
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

static unsigned int count_text(const char *source, const char *needle)
{
    unsigned int count = 0U;
    size_t length = strlen(needle);

    while ((source = strstr(source, needle)) != NULL)
    {
        count++;
        source += length;
    }
    return count;
}

int main(void)
{
    static const char *const paths[] = {
        "..\\Core\\Application\\Src\\font_cache_blob.c",
        "..\\Core\\Application\\Src\\font_cache_blob_load.c",
        "..\\Core\\Application\\Src\\font_cache_service.c",
        "..\\Core\\Application\\Src\\font_cache_service_platform.c",
        "..\\Core\\Application\\Src\\font_cache_source.c",
        "..\\Core\\Application\\Src\\nand_cache_layout.c",
        "..\\Core\\Application\\Src\\nand_cache_metadata.c",
        "..\\Core\\Application\\Src\\nand_cache_page.c",
        "..\\Core\\Application\\Src\\nand_cache_slot.c",
        "..\\Core\\Application\\Src\\nand_cache_store.c",
        "..\\Core\\Application\\Src\\nand_cache_store_stream.c",
        "..\\Core\\Application\\Src\\nand_page_ecc.c",
        "..\\Core\\Peripheral\\Src\\nand_cache_io.c",
        "..\\lvgl_GUI\\custom\\font_cache_lvgl.c",
        "..\\lvgl_GUI\\custom\\font_cache_lvgl_load.c"
    };
    char *project = read_project();
    unsigned int index;
    int failures = 0;

    if (project == NULL)
    {
        printf("FAIL read_project\n");
        return 1;
    }
    for (index = 0U; index < sizeof(paths) / sizeof(paths[0]); index++)
    {
        unsigned int count = count_text(project, paths[index]);
        if (count != 1U)
        {
            printf("FAIL project_path index=%u count=%u\n", index, count);
            failures++;
        }
    }
    free(project);
    if (failures != 0)
    {
        printf("FAIL font_cache_keil_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS font_cache_keil_contract\n");
    return 0;
}
