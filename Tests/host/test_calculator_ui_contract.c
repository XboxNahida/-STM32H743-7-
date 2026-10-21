#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_source(const char *path)
{
    FILE *file = fopen(path, "rb");
    long size;
    char *content;

    if (file == NULL) return NULL;
    if ((fseek(file, 0L, SEEK_END) != 0) ||
        ((size = ftell(file)) < 0L) ||
        (fseek(file, 0L, SEEK_SET) != 0))
    {
        fclose(file);
        return NULL;
    }
    content = (char *)malloc((size_t)size + 1U);
    if (content == NULL)
    {
        fclose(file);
        return NULL;
    }
    if (fread(content, 1U, (size_t)size, file) != (size_t)size)
    {
        free(content);
        fclose(file);
        return NULL;
    }
    content[size] = '\0';
    fclose(file);
    return content;
}

static int expect_contains(const char *name, const char *source,
                           const char *needle)
{
    if ((source == NULL) || (strstr(source, needle) == NULL))
    {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

static int expect_not_contains(const char *name, const char *source,
                               const char *needle)
{
    if ((source == NULL) || (strstr(source, needle) != NULL))
    {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

int main(void)
{
    char *source = read_source(
        "lvgl_GUI/generated/setup_scr_screen_calculator.c");
    char *project = read_source("MDK-ARM/H743_LCD_Keil5.uvprojx");
    int failures = 0;

    failures += expect_contains("controller_include", source,
                                "calculator_controller.h");
    failures += expect_contains("button_event", source,
                                "screen_calculator_button_event_handler");
    failures += expect_contains("selected_button", source,
                                "lv_buttonmatrix_get_selected_button");
    failures += expect_contains("selected_text", source,
                                "lv_buttonmatrix_get_button_text");
    failures += expect_contains("controller_press", source,
                                "calculator_controller_press(");
    failures += expect_contains("expression_sync", source,
                                "g_calculator.expression");
    failures += expect_contains("result_sync", source,
                                "g_calculator.result_text");
    failures += expect_contains("dynamic_map", source,
                                "calculator_update_button_map");
    failures += expect_not_contains("no_keyboard_callback", source,
                                    "ta_event_cb");
    failures += expect_contains("no_textarea_click", source,
                                "lv_obj_remove_flag(textarea, LV_OBJ_FLAG_CLICKABLE);");
    failures += expect_contains("no_textarea_focus", source,
                                "lv_obj_remove_flag(textarea, LV_OBJ_FLAG_CLICK_FOCUSABLE);");
    failures += expect_contains("cursor_click_disabled", source,
                                "lv_textarea_set_cursor_click_pos");
    failures += expect_contains("keil_engine", project,
                                "calculator_engine.c");
    failures += expect_contains("keil_controller", project,
                                "calculator_controller.c");

    free(source);
    free(project);
    if (failures != 0)
    {
        printf("FAIL calculator_ui_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS calculator_ui_contract\n");
    return 0;
}
