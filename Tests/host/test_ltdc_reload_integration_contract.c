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

static int require_text(const char *text, const char *needle,
                        const char *label)
{
    if (strstr(text, needle) == NULL)
    {
        printf("FAIL %s\n", label);
        return 1;
    }
    return 0;
}

int main(void)
{
    char *source = read_file("Core/Peripheral/Src/ltdc_video_output.c");
    int failures = 0;

    if (source == NULL)
    {
        printf("FAIL read ltdc_video_output.c\n");
        return 1;
    }
    failures += require_text(source, "uint32_t lvgl_restore_addr;",
                             "restore address stored");
    failures += require_text(source,
                             "g_ltdc_video.lvgl_restore_addr = disp_get_frame_address();",
                             "active LVGL frame captured");
    failures += require_text(source,
                             "ltdc_video_schedule_swap(g_ltdc_video.lvgl_restore_addr",
                             "video exits to captured frame");
    failures += require_text(source, "disp_on_ltdc_reload();",
                             "reload interrupt dispatched to LVGL");
    if (strstr(source,
               "ltdc_video_schedule_swap(SDRAM_LVGL_FRAME_ADDR") != NULL)
    {
        printf("FAIL fixed LVGL restore address removed\n");
        failures++;
    }
    free(source);
    if (failures == 0)
    {
        printf("PASS ltdc_reload_integration_contract\n");
    }
    return failures;
}
