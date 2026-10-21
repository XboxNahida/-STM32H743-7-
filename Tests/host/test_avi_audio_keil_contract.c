#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_source(const char *path)
{
    FILE *file = fopen(path, "rb");
    long length;
    char *text;

    if (file == NULL) return NULL;
    fseek(file, 0L, SEEK_END);
    length = ftell(file);
    fseek(file, 0L, SEEK_SET);
    if (length < 0L)
    {
        fclose(file);
        return NULL;
    }
    text = (char *)malloc((size_t)length + 1U);
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

static unsigned int count_text(const char *text, const char *needle)
{
    unsigned int count = 0U;
    size_t length = strlen(needle);

    while ((text = strstr(text, needle)) != NULL)
    {
        count++;
        text += length;
    }
    return count;
}

int main(void)
{
    char *project = read_source("MDK-ARM/H743_LCD_Keil5.uvprojx");
    char *decoder = read_source("Core/Peripheral/Src/avi_audio_decoder.c");
    int failures = 0;

    if ((project == NULL) || (decoder == NULL))
    {
        printf("FAIL avi_audio_keil_read\n");
        free(project);
        free(decoder);
        return 1;
    }
    if ((count_text(project, "<FileName>avi_audio_decoder.c</FileName>") != 1U) ||
        (count_text(project,
            "<FilePath>..\\Core\\Peripheral\\Src\\avi_audio_decoder.c</FilePath>") != 1U))
    {
        printf("FAIL avi_audio_decoder_project_entry\n");
        failures++;
    }
    if ((strstr(decoder, "section(\".audio_dma\")") == NULL) ||
        (strstr(decoder, "AVI_AUDIO_DECODER_INPUT_CAPACITY") == NULL))
    {
        printf("FAIL avi_audio_decoder_axi_buffer\n");
        failures++;
    }
    free(project);
    free(decoder);
    if (failures != 0)
    {
        printf("FAIL avi_audio_keil_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS avi_audio_keil_contract\n");
    return 0;
}
