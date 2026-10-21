#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_source(const char *path)
{
    FILE *file = fopen(path, "rb");
    long size;
    char *text;

    if (file == NULL)
    {
        return NULL;
    }
    if ((fseek(file, 0L, SEEK_END) != 0) ||
        ((size = ftell(file)) < 0L) ||
        (fseek(file, 0L, SEEK_SET) != 0))
    {
        fclose(file);
        return NULL;
    }
    text = (char *)malloc((size_t)size + 1U);
    if (text == NULL)
    {
        fclose(file);
        return NULL;
    }
    if (fread(text, 1U, (size_t)size, file) != (size_t)size)
    {
        free(text);
        fclose(file);
        return NULL;
    }
    text[size] = '\0';
    fclose(file);
    return text;
}

static unsigned int count_text(const char *source, const char *needle)
{
    unsigned int count = 0U;
    size_t length = strlen(needle);
    const char *cursor = source;

    while ((cursor = strstr(cursor, needle)) != NULL)
    {
        count++;
        cursor += length;
    }
    return count;
}

int main(void)
{
    char *project = read_source("MDK-ARM/H743_LCD_Keil5.uvprojx");
    int failures = 0;

    if (project == NULL)
    {
        printf("FAIL read_project\n");
        return 1;
    }
    if (count_text(project, "<FileName>audio_recording_service.c</FileName>") != 1U)
    {
        printf("FAIL recording_source_entry\n");
        failures++;
    }
    if (count_text(project,
                   "<FilePath>..\\Core\\Peripheral\\Src\\audio_recording_service.c</FilePath>") != 1U)
    {
        printf("FAIL recording_source_path\n");
        failures++;
    }
    free(project);
    if (failures != 0)
    {
        printf("FAIL audio_record_keil_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS audio_record_keil_contract\n");
    return 0;
}
