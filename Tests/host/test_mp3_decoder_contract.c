#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_source(const char *path)
{
    FILE *file = fopen(path, "rb");
    long size;
    char *source;

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
    source = (char *)malloc((size_t)size + 1U);
    if (source == NULL)
    {
        fclose(file);
        return NULL;
    }
    if (fread(source, 1U, (size_t)size, file) != (size_t)size)
    {
        free(source);
        fclose(file);
        return NULL;
    }
    source[size] = '\0';
    fclose(file);
    return source;
}

int main(void)
{
    char *source = read_source("Core/Peripheral/Src/mp3_decoder_service.c");
    const char *stereo_branch;
    int failures = 0;

    if (source == NULL)
    {
        printf("FAIL read_mp3_source\n");
        return 1;
    }
    stereo_branch = strstr(
        source,
        "uint32_t output_samples = (uint32_t)frame_info.outputSamps;");
    if (stereo_branch == NULL)
    {
        printf("FAIL stereo_branch_missing\n");
        failures++;
    }
    else if (strstr(stereo_branch, "outputSamps * 2U") != NULL)
    {
        printf("FAIL stereo_samples_multiply\n");
        failures++;
    }
    if (strstr(source, "frame_info.nChans == 1") == NULL)
    {
        printf("FAIL mono_branch_missing\n");
        failures++;
    }
    free(source);
    if (failures != 0)
    {
        printf("FAIL mp3_decoder_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS mp3_decoder_contract\n");
    return 0;
}
