#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_source(const char *path)
{
    FILE *file = fopen(path, "rb");
    long length;
    char *text;

    if (file == NULL) return NULL;
    if ((fseek(file, 0L, SEEK_END) != 0) ||
        ((length = ftell(file)) < 0L) ||
        (fseek(file, 0L, SEEK_SET) != 0))
    {
        fclose(file);
        return NULL;
    }
    text = (char *)malloc((size_t)length + 1U);
    if (text == NULL)
    {
        fclose(file);
        return NULL;
    }
    if (fread(text, 1U, (size_t)length, file) != (size_t)length)
    {
        free(text);
        fclose(file);
        return NULL;
    }
    text[length] = '\0';
    fclose(file);
    return text;
}

static int expect_true(const char *name, int condition)
{
    if (!condition)
    {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

int main(void)
{
    char *source = read_source("lvgl_GUI/custom/image_viewer_ui.c");
    int failures = 0;

    failures += expect_true("read_image_viewer_ui_source", source != NULL);
    if (source == NULL) return 1;

    /* The viewer must not animate in before its first decoded frame exists. */
    failures += expect_true("image_viewer_entry_has_no_screen_animation",
        strstr(source, "LV_SCR_LOAD_ANIM_NONE") != NULL);
    failures += expect_true("image_viewer_entry_does_not_use_over_top",
        strstr(source, "LV_SCR_LOAD_ANIM_OVER_TOP") == NULL);

    free(source);
    if (failures != 0) return 1;
    printf("PASS image_viewer_transition_contract\n");
    return 0;
}
