#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    long length;
    char *text;

    if ((file == NULL) || (fseek(file, 0L, SEEK_END) != 0) ||
        ((length = ftell(file)) < 0L) || (fseek(file, 0L, SEEK_SET) != 0))
    {
        if (file != NULL) fclose(file);
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

static int require_text(const char *text, const char *needle, const char *label)
{
    if (strstr(text, needle) == NULL)
    {
        printf("FAIL %s\n", label);
        return 1;
    }
    return 0;
}

static int require_absent(const char *text, const char *needle, const char *label)
{
    if (strstr(text, needle) != NULL)
    {
        printf("FAIL %s\n", label);
        return 1;
    }
    return 0;
}

int main(void)
{
    char *source = read_file("Core/Peripheral/Src/jpegcodec.c");
    char *header = read_file("Core/Peripheral/Inc/jpegcodec.h");
    char *fmc = read_file("Core/Src/fmc.c");
    char *main_source = read_file("Core/Src/main.c");
    int failures = 0;

    if ((source == NULL) || (header == NULL) || (fmc == NULL) ||
        (main_source == NULL))
    {
        printf("FAIL read bandwidth sources\n");
        free(source);
        free(header);
        free(fmc);
        free(main_source);
        return 1;
    }

    failures += require_absent(header, "JPEG_DMA2D_DEAD_TIME",
                               "DMA2D dead time experiment reverted");
    failures += require_absent(source, "DMA2D_AMTCR_DT_Pos",
                               "DMA2D dead time register is untouched");
    failures += require_text(fmc, "SDRAM_MODEREG_BURST_LENGTH_1",
                             "failed SDRAM burst experiment reverted");
    failures += require_text(main_source, "PeriphClkInitStruct.PLL3.PLL3R = 15",
                             "LTDC pixel clock reduced independently");

    free(source);
    free(header);
    free(fmc);
    free(main_source);
    if (failures == 0)
    {
        printf("PASS dma2d_bandwidth_contract\n");
    }
    return failures;
}
