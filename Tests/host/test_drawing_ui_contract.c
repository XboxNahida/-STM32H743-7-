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
    if ((source != NULL) && (strstr(source, needle) != NULL))
    {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

static unsigned int count_occurrences(const char *source, const char *needle)
{
    unsigned int count = 0U;
    size_t length = strlen(needle);

    if ((source == NULL) || (length == 0U)) return 0U;
    while ((source = strstr(source, needle)) != NULL)
    {
        count++;
        source += length;
    }
    return count;
}

int main(void)
{
    char *source = read_source("lvgl_GUI/custom/drawing_app_ui.c");
    char *header = read_source("lvgl_GUI/custom/drawing_app_ui.h");
    char *setup = read_source(
        "lvgl_GUI/generated/setup_scr_screen_drawing.c");
    char *project = read_source("MDK-ARM/H743_LCD_Keil5.uvprojx");
    int failures = 0;

    failures += expect_contains("history_include", source,
                                "drawing_history.h");
    failures += expect_contains("canvas_rgb565", source,
                                "LV_COLOR_FORMAT_RGB565");
    failures += expect_contains("sdram_heap", source, "lv_malloc(");
    failures += expect_contains("buffer_release", source, "lv_free(");
    failures += expect_contains("canvas_delete_first", source,
                                "lv_obj_delete(session->canvas)");
    failures += expect_contains("press_begin", source, "LV_EVENT_PRESSED");
    failures += expect_contains("press_append", source,
                                "LV_EVENT_PRESSING");
    failures += expect_contains("press_end", source, "LV_EVENT_RELEASED");
    failures += expect_contains("lost_end", source,
                                "LV_EVENT_PRESS_LOST");
    failures += expect_contains("line_draw", source, "lv_draw_line(");
    failures += expect_contains("canvas_layer", source,
                                "lv_canvas_init_layer");
    failures += expect_contains("pen_mode", source,
                                "screen_drawing_pen_btn");
    failures += expect_contains("eraser_mode", source,
                                "screen_drawing_eraser_btn");
    failures += expect_contains("width_slider", source,
                                "screen_drawing_width_slider");
    failures += expect_contains("undo", source, "drawing_history_undo");
    failures += expect_contains("redo", source, "drawing_history_redo");
    failures += expect_contains("clear", source, "drawing_history_clear");
    failures += expect_contains("swatches", source,
                                "DRAWING_COLOR_COUNT");
    failures += expect_not_contains("no_direct_fatfs", source, "f_open(");
    failures += expect_contains("bmp_writer", source,
                                "drawing_bmp_writer.h");
    failures += expect_contains("save_next", source,
                                "drawing_bmp_save_next(");
    failures += expect_contains("save_button", source,
                                "screen_drawing_save_btn");
    failures += expect_contains("save_deferred", source, "lv_async_call(");
    failures += expect_contains("save_cancel", source,
                                "lv_async_call_cancel(");
    failures += expect_contains("save_status", source, "status_label");
    failures += expect_contains("public_attach", header,
                                "drawing_app_ui_attach");
    failures += expect_contains("canvas_widget", setup,
                                "lv_canvas_create");
    failures += expect_contains("attach_call", setup,
                                "drawing_app_ui_attach(ui, swatches)");
    if (count_occurrences(project,
                          "<FileName>drawing_history.c</FileName>") != 1U)
    {
        printf("FAIL keil_history_unique\n");
        failures++;
    }
    if (count_occurrences(project,
                          "<FileName>drawing_bmp_writer.c</FileName>") != 1U)
    {
        printf("FAIL keil_bmp_unique\n");
        failures++;
    }
    if (count_occurrences(project,
                          "<FileName>drawing_app_ui.c</FileName>") != 1U)
    {
        printf("FAIL keil_ui_unique\n");
        failures++;
    }

    free(source);
    free(header);
    free(setup);
    free(project);
    if (failures != 0)
    {
        printf("FAIL drawing_ui_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS drawing_ui_contract\n");
    return 0;
}
