#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    long size;
    char *text;
    if (file == NULL || fseek(file, 0L, SEEK_END) != 0)
    {
        if (file != NULL) fclose(file);
        return NULL;
    }
    size = ftell(file);
    if (size < 0L || fseek(file, 0L, SEEK_SET) != 0)
    {
        fclose(file);
        return NULL;
    }
    text = (char *)malloc((size_t)size + 1U);
    if (text != NULL)
    {
        (void)fread(text, 1U, (size_t)size, file);
        text[size] = '\0';
    }
    fclose(file);
    return text;
}

static int require_text(const char *source, const char *text)
{
    if (strstr(source, text) == NULL)
    {
        printf("FAIL missing=%s\n", text);
        return 1;
    }
    return 0;
}

int main(void)
{
    char *bridge = read_file("Core/Application/Src/camera_ui_bridge.c");
    char *style = read_file("Core/Application/Src/camera_ui_style.h");
    char *setup = read_file("lvgl_GUI/generated/setup_scr_screen_camera.c");
    char *events = read_file("lvgl_GUI/generated/events_init.c");
    int failures = 0;
    if (bridge == NULL || style == NULL || setup == NULL || events == NULL)
    {
        printf("FAIL source_open\n");
        free(bridge);
        free(style);
        free(setup);
        free(events);
        return 1;
    }
    failures += require_text(bridge, "camera_ui_bridge_attach");
    failures += require_text(bridge, "camera_ui_bridge_poll");
    failures += require_text(bridge, "camera_ui_bridge_request_leave");
    failures += require_text(bridge, "#include \"music_player_service.h\"");
    failures += require_text(bridge, "music_player_suspend_for_camera()");
    failures += require_text(bridge,
                             "music_player_request_restore_after_camera()");
    failures += require_text(bridge, "LV_COLOR_FORMAT_RGB565");
    failures += require_text(bridge, "CAMERA_FLASH_MODE_OFF");
    failures += require_text(bridge, "CAMERA_FLASH_MODE_ON");
    failures += require_text(bridge, "CAMERA_FLASH_MODE_AUTO");
    failures += require_text(bridge, "ov5640_set_flash");
    failures += require_text(bridge, "600");
    failures += require_text(bridge, "400");
    failures += require_text(bridge, "lv_obj_add_flag");
    failures += require_text(bridge, "lv_color_hex(0x000000)");
    failures += require_text(style, "&lv_font_myyh_20");
    failures += require_text(style, "lv_color_hex(0x242424)");
    failures += require_text(style, "lv_color_hex(0x4A4A4A)");
    failures += require_text(style, "lv_color_hex(0x383838)");
    failures += require_text(bridge, "g_camera_shutter_outer");
    failures += require_text(bridge, "g_camera_shutter_inner");
    failures += require_text(style, "LV_RADIUS_CIRCLE");
    failures += require_text(bridge, "stage=%d result=%d");
    failures += require_text(bridge, "camera_resource_get_status");
    failures += require_text(setup, "camera_ui_bridge_attach");
    failures += require_text(events, "camera_ui_bridge_request_leave");
    {
        const char *suspend = strstr(bridge,
            "music_player_suspend_for_camera()");
        const char *start = strstr(bridge, "camera_preview_start()");
        const char *stop = strstr(bridge, "camera_preview_stop()");
        const char *restore = strstr(bridge,
            "music_player_request_restore_after_camera()");

        if ((suspend == NULL) || (start == NULL) || (suspend > start) ||
            (stop == NULL) || (restore == NULL) || (stop > restore))
        {
            printf("FAIL camera_music_lifecycle_order\n");
            failures++;
        }
    }
    if (strstr(bridge, "f_open") != NULL || strstr(bridge, "HAL_DCMI") != NULL)
    {
        printf("FAIL bridge_layer_violation\n");
        failures++;
    }
    if (strstr(bridge, "lv_font_myyh_16") != NULL ||
        strstr(bridge, "lv_color_hex(0x151515)") != NULL ||
        strstr(bridge, "lv_color_hex(0x191919)") != NULL)
    {
        printf("FAIL legacy_camera_style\n");
        failures++;
    }
    free(bridge);
    free(style);
    free(setup);
    free(events);
    if (failures == 0)
    {
        printf("PASS camera_ui_bridge_contract\n");
        return 0;
    }
    printf("FAIL camera_ui_bridge_contract failures=%d\n", failures);
    return 1;
}
