#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    char *data;
    long size;
    if (file == NULL || fseek(file, 0L, SEEK_END) != 0 ||
        (size = ftell(file)) < 0L || fseek(file, 0L, SEEK_SET) != 0)
    {
        if (file != NULL) fclose(file);
        return NULL;
    }
    data = (char *)malloc((size_t)size + 1U);
    if (data == NULL || fread(data, 1U, (size_t)size, file) != (size_t)size)
    {
        free(data); fclose(file); return NULL;
    }
    data[size] = '\0';
    fclose(file);
    return data;
}

static int require_text(const char *source, const char *needle,
                        const char *name)
{
    if (source == NULL || strstr(source, needle) == NULL)
    {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

static int require_absent(const char *source, const char *needle,
                          const char *name)
{
    if ((source != NULL) && (strstr(source, needle) != NULL))
    {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

static int require_count(const char *source, const char *needle,
                         unsigned int expected, const char *name)
{
    unsigned int count = 0U;
    size_t length = strlen(needle);
    while ((source = strstr(source, needle)) != NULL)
    {
        ++count;
        source += length;
    }
    if (count == expected) return 0;
    printf("FAIL %s count=%u expected=%u\n", name, count, expected);
    return 1;
}

static int require_before(const char *source, const char *first,
                          const char *second, const char *name)
{
    const char *first_at = (source != NULL) ? strstr(source, first) : NULL;
    const char *second_at = (source != NULL) ? strstr(source, second) : NULL;

    if ((first_at != NULL) && (second_at != NULL) &&
        (first_at < second_at)) return 0;
    printf("FAIL %s\n", name);
    return 1;
}

int main(void)
{
    char *source = read_file("lvgl_GUI/custom/nes_player_ui.c");
    char *layout = read_file("lvgl_GUI/custom/nes_player_ui_layout.h");
    char *custom = read_file("lvgl_GUI/custom/custom.c");
    char *events = read_file("lvgl_GUI/generated/events_init.c");
    int failures = 0;
    if ((source == NULL) || (layout == NULL))
    {
        printf("FAIL production_missing\n");
        return 1;
    }
    failures += require_text(source, "0:/SYSTEM/NES", "rom directory");
    failures += require_text(source, "lv_font_myyh_20", "20px font");
    failures += require_text(source, "NES_GAME_WIDTH 512", "game width");
    failures += require_text(source, "NES_GAME_HEIGHT 480", "game height");
    failures += require_text(source, "nes_rom_catalog_scan_page", "paging");
    failures += require_text(source, "nes_ui_add_pagination",
                             "persistent pagination controls");
    failures += require_text(source, "status->total_count",
                             "catalog total used for page count");
    failures += require_text(source, "lv_label_set_text_fmt",
                             "current and total page label");
    failures += require_text(source, "LV_STATE_DISABLED",
                             "edge page buttons disabled");
    failures += require_text(source, "lv_port_indev_get_touch_snapshot",
                             "five point snapshot");
    failures += require_text(source, "nes_player_controls_update",
                             "extended controls");
    failures += require_text(source, "#include \"nes_controller_port.h\"",
                             "infrared controller include");
    failures += require_text(source, "nes_controller_port_init();",
                             "infrared controller init");
    failures += require_text(source, "nes_controller_port_poll(",
                             "infrared controller poll");
    failures += require_text(source, "nes_controller_port_take_pause_toggle",
                             "infrared pause event");
    failures += require_text(source, "nes_runtime_set_paused",
                             "runtime pause");
    failures += require_text(source, "NES_RUNTIME_PAUSED", "paused state");
    failures += require_text(source, "lv_tick_get", "turbo tick");
    failures += require_text(source, "nes_runtime_start", "runtime start");
    failures += require_text(source, "nes_runtime_stop", "runtime stop");
    failures += require_text(source, "#include \"music_player_service.h\"",
                             "music handoff include");
    failures += require_before(source, "(void)music_player_stop();",
                               "start_result = nes_runtime_start(",
                               "music stopped before NES start");
    failures += require_text(source, "int start_result = NES_RUNTIME_OK;",
                             "start result preserved");
    failures += require_text(source,
                             "start_result = nes_runtime_start(",
                             "start result captured");
    failures += require_text(source,
                             "nes_ui_handle_failure(start_result);",
                             "start failure recovery");
    failures += require_text(source,
                             "runtime.state == NES_RUNTIME_ERROR",
                             "runtime failure recovery");
    failures += require_text(source,
                             "nes_ui_handle_failure(runtime.result);",
                             "runtime result preserved");
    failures += require_text(source, "NES_RUNTIME_UNSUPPORTED_ROM",
                             "unsupported mapper message");
    failures += require_text(source, "NES_RUNTIME_NO_MEMORY",
                             "no memory message");
    failures += require_text(source, "NES_RUNTIME_STORAGE_ERROR",
                             "storage error message");
    failures += require_text(source, "NES_RUNTIME_INVALID_ROM",
                             "invalid rom message");
    failures += require_text(source, "NES_RUNTIME_CORE_ERROR",
                             "core error message");
    failures += require_text(source, "nes_video_get_frame", "video frame");
    failures += require_text(source, "nes_video_mark_consumed", "consume frame");
    failures += require_text(source, "lv_image_set_scale(g_nes_ui.image, 256U)",
                             "native size image");
    failures += require_text(source, "LV_OBJ_FLAG_SCROLLABLE",
                             "scroll disabled");
    failures += require_text(source, "LV_SCROLLBAR_MODE_OFF",
                             "scrollbar disabled");
    failures += require_text(source, "lv_obj_set_style_pad_all",
                             "panel padding removed");
    failures += require_text(source, "lv_obj_set_style_border_width",
                             "panel border removed");
    failures += require_count(source, "nes_ui_lock_view(", 4U,
                              "all three views locked");
    failures += require_text(layout, "NES_PLAYER_CONTROL_COUNT",
                             "fifteen controls");
    failures += require_text(layout,
        "\\xE5\\xB7\\xA6\\xE4\\xB8\\x8A", "up left button");
    failures += require_text(layout,
        "\\xE5\\x8F\\xB3\\xE4\\xB8\\x8A", "up right button");
    failures += require_text(layout,
        "\\xE5\\xB7\\xA6\\xE4\\xB8\\x8B", "down left button");
    failures += require_text(layout,
        "\\xE5\\x8F\\xB3\\xE4\\xB8\\x8B", "down right button");
    failures += require_text(layout, "\"X\"", "X button");
    failures += require_text(layout, "\"Y\"", "Y button");
    failures += require_count(layout,
        "\\xE6\\x9A\\x82\\xE5\\x81\\x9C", 2U,
        "pause button and help");
    failures += require_absent(layout, "\"OK\"", "OK button removed");
    failures += require_text(layout, "nes_ui_create_help",
                             "top right help");
    failures += require_text(layout, "lv_obj_set_pos(label, 774, 68)",
                             "help moved below pause");
    failures += require_text(layout, "lv_obj_set_size(label, 240, 78)",
                             "three line help height");
    failures += require_text(layout,
        "\\xE8\\xBF\\x9E\\xE5\\x8F\\x91", "turbo help");
    failures += require_text(layout,
        "\\xE6\\x9A\\x82\\xE5\\x81\\x9C/"
        "\\xE7\\xBB\\xA7\\xE7\\xBB\\xAD", "pause help");
    failures += require_text(layout,
        "\"\\xE9\\x80\\x89\\xE6\\x8B\\xA9\",",
        "select chinese");
    failures += require_text(layout,
        "\"\\xE5\\xBC\\x80\\xE5\\xA7\\x8B\"",
        "start chinese");
    failures += require_text(custom, "nes_player_ui_init", "ui init");
    failures += require_text(custom, "nes_player_ui_poll", "ui poll");
    failures += require_text(events, "nes_player_ui_request_open",
                             "desktop open");
    failures += require_text(events, "DeskTop_img_nes",
                             "desktop binding");
    free(source);
    free(layout);
    free(custom);
    free(events);
    if (failures == 0) printf("PASS nes_ui_contract\n");
    return failures;
}
