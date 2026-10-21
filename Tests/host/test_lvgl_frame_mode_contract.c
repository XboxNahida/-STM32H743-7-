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

int main(void)
{
    char *config = read_file("lvgl_GUI/custom/lv_conf_ext.h");
    int failed = 0;

    if (config == NULL)
    {
        printf("FAIL read lv_conf_ext.h\n");
        return 1;
    }
    if (strstr(config, "#define LV_USE_LTDC_FULL_FRAME_BUFFER  1") == NULL)
    {
        printf("FAIL full frame mode defaults enabled\n");
        failed = 1;
    }
    free(config);
    if (failed == 0)
    {
        printf("PASS lvgl_frame_mode_contract\n");
    }
    return failed;
}
