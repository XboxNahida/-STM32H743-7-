#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_text(const char *path)
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

static int expect_contains(const char *name, const char *text,
                           const char *needle)
{
    if ((text == NULL) || (strstr(text, needle) == NULL))
    {
        printf("FAIL %s missing=%s\n", name, needle);
        return 1;
    }
    return 0;
}

static int expect_not_contains(const char *name, const char *text,
                               const char *needle)
{
    if ((text != NULL) && (strstr(text, needle) != NULL))
    {
        printf("FAIL %s unexpected=%s\n", name, needle);
        return 1;
    }
    return 0;
}

static int expect_before(const char *name, const char *text,
                         const char *first, const char *second)
{
    const char *first_position;
    const char *second_position;

    if (text == NULL)
    {
        printf("FAIL %s missing_source\n", name);
        return 1;
    }
    first_position = strstr(text, first);
    second_position = strstr(text, second);
    if ((first_position == NULL) || (second_position == NULL) ||
        (first_position >= second_position))
    {
        printf("FAIL %s expected=%s before %s\n", name, first, second);
        return 1;
    }
    return 0;
}

int main(void)
{
    char *custom = read_text("lvgl_GUI/custom/custom.c");
    char *custom_h = read_text("lvgl_GUI/custom/custom.h");
    char *navigation = read_text(
        "lvgl_GUI/generated/setup_scr_screen_navigation_bar.c");
    char *camera = read_text("Core/Application/Src/camera_ui_bridge.c");
    char *settings_h = read_text("Core/Application/Inc/system_settings_service.h");
    char *settings_c = read_text("Core/Application/Src/system_settings_service.c");
    int failures = 0;

    failures += expect_contains("custom_settings_include", custom,
        "#include \"system_settings_service.h\"");
    failures += expect_contains("custom_restore", custom,
        "system_settings_apply_ui();");
    failures += expect_before("music_init_before_volume_restore", custom,
        "music_player_ui_init();", "system_settings_apply_ui();");
    failures += expect_contains("custom_brightness_update", custom,
        "system_settings_update_brightness");
    failures += expect_contains("custom_volume_update", custom,
        "system_settings_update_volume");
    failures += expect_contains("custom_volume_immediate_save", custom,
        "system_settings_request_save();");
    failures += expect_contains("navigation_sync_api", custom_h,
        "system_settings_sync_navigation_controls");
    failures += expect_contains("navigation_sync_reads_settings", custom,
        "system_settings_sync_navigation_controls");
    failures += expect_contains("navigation_syncs_volume_slider", custom,
        "settings.volume_percent, LV_ANIM_OFF");
    failures += expect_contains("navigation_calls_settings_sync", navigation,
        "system_settings_sync_navigation_controls(ui);");
    failures += expect_contains("camera_settings_include", camera,
        "#include \"system_settings_service.h\"");
    failures += expect_contains("camera_loads_settings", camera,
        "system_settings_get(&settings);");
    failures += expect_contains("camera_applies_preferences", camera,
        "camera_apply_preferences();");
    failures += expect_contains("camera_updates_settings", camera,
        "system_settings_update_camera");
    failures += expect_not_contains("header_has_no_piano", settings_h, "piano");
    failures += expect_not_contains("source_has_no_piano", settings_c, "piano");

    free(custom);
    free(custom_h);
    free(navigation);
    free(camera);
    free(settings_h);
    free(settings_c);
    if (failures != 0)
    {
        printf("FAIL system_settings_ui_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS system_settings_ui_contract\n");
    return 0;
}
