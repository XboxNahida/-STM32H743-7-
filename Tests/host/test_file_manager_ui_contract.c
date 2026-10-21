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
    if (text == NULL) { fclose(file); return NULL; }
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

static int expect_source(const char *name, const char *source,
                         const char *needle)
{
    if (strstr(source, needle) != NULL) return 0;
    printf("FAIL %s\n", name);
    return 1;
}

static int expect_not_source(const char *name, const char *source,
                             const char *needle)
{
    if (strstr(source, needle) == NULL) return 0;
    printf("FAIL %s\n", name);
    return 1;
}

static int expect_count(const char *name, const char *source,
                        const char *needle, size_t expected)
{
    const char *cursor = source;
    size_t count = 0U;

    while ((cursor = strstr(cursor, needle)) != NULL)
    {
        count++;
        cursor += strlen(needle);
    }
    if (count == expected) return 0;
    printf("FAIL %s expected=%u actual=%u\n", name,
           (unsigned int)expected, (unsigned int)count);
    return 1;
}

int main(void)
{
    char *source = read_source("lvgl_GUI/custom/file_manager_ui.c");
    int failures = 0;

    if (source == NULL)
    {
        printf("FAIL read_file_manager_ui\n");
        return 1;
    }
    failures += expect_source("readonly_browser_entry", source,
        "file_ui_create_readonly(parent)");
    failures += expect_source("background_task_poll", source,
        "(void)file_manager_poll()");
    failures += expect_source("delete_cancel_message", source,
        "删除阶段不能取消");
    failures += expect_source("navigation_parent_valid", source,
        "!lv_obj_is_valid(navigation_parent)");
    failures += expect_source("navigation_indicator_owned", source,
        "lv_obj_get_parent(g_navigation_indicator) != navigation_parent");
    failures += expect_source("completion_refresh", source,
        "g_file_ui.refresh_pending = 1U");
    failures += expect_source("previous_page_disabled_on_first_page", source,
        "if (g_file_ui.offset == 0U)");
    failures += expect_source("next_page_disabled_without_more", source,
        "if (g_file_ui.page.has_more == 0U)");
    failures += expect_source("page_button_disabled_state", source,
        "lv_obj_add_state(g_file_ui.next_button, LV_STATE_DISABLED)");
    failures += expect_source("selected_row_highlight", source,
        "selected ? 0xcde9ec : 0xffffff");
    failures += expect_source("list_vertical_scroll_only", source,
        "lv_obj_set_scroll_dir(g_file_ui.list, LV_DIR_VER)");
    failures += expect_source("readonly_outer_scroll_locked", source,
        "lv_obj_remove_flag(outer, LV_OBJ_FLAG_SCROLLABLE)");
    failures += expect_source("back_button_below_status_bar", source,
        "\"返回\", 12, 50, 86, 40");
    failures += expect_count("single_fullscreen_list_object", source,
        "g_file_ui.list = lv_obj_create(g_file_ui.screen);", 1U);
    failures += expect_source("light_screen_background", source,
        "lv_color_hex(0xf1f6f7)");
    failures += expect_source("focused_button_keeps_command_color", source,
        "LV_PART_MAIN | LV_STATE_FOCUSED");
    failures += expect_source("file_screen_status_bar", source,
        "file_ui_create_status_bar(g_file_ui.screen)");
    failures += expect_source("status_bar_clock_reads_rtc", source,
        "HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN)");
    failures += expect_source("status_bar_clock_hours_minutes", source,
        "\"%02u:%02u\"");
    failures += expect_not_source("status_bar_clock_hides_seconds", source,
        "\"%02u:%02u:%02u\"");
    failures += expect_source("status_bar_connectivity_icons", source,
        "LV_SYMBOL_WIFI");

    free(source);
    if (failures != 0) return 1;
    printf("PASS file_manager_ui_contract\n");
    return 0;
}
