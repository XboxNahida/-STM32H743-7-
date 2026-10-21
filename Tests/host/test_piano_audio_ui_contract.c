#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_source(const char *path)
{
    FILE *file = fopen(path, "rb");
    long size;
    char *content;

    if (file == NULL) return NULL;
    if (fseek(file, 0L, SEEK_END) != 0)
    {
        fclose(file);
        return NULL;
    }
    size = ftell(file);
    if ((size < 0L) || (fseek(file, 0L, SEEK_SET) != 0))
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

static int expect_contains(const char *name, const char *text,
                           const char *needle)
{
    if ((text == NULL) || (strstr(text, needle) == NULL))
    {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

int main(void)
{
    char *port_header = read_source("lvgl/porting/lv_port_indev.h");
    char *port_source = read_source("lvgl/porting/lv_port_indev.c");
    char *piano = read_source("lvgl_GUI/generated/setup_scr_screen_piano.c");
    char *project = read_source("MDK-ARM/H743_LCD_Keil5.uvprojx");
    int failures = 0;

    failures += expect_contains("snapshot_type", port_header,
                                "lv_port_touch_snapshot_t");
    failures += expect_contains("snapshot_api", port_header,
                                "lv_port_indev_get_touch_snapshot");
    failures += expect_contains("snapshot_maps_coordinates", port_source,
                                "lv_port_indev_get_touch_snapshot");
    failures += expect_contains("piano_audio_include", piano,
                                "piano_audio_service.h");
    failures += expect_contains("piano_mapper_include", piano,
                                "piano_touch_mapper.h");
    failures += expect_contains("piano_snapshot_include", piano,
                                "lv_port_indev.h");
    failures += expect_contains("piano_stops_music", piano,
                                "music_player_stop()");
    failures += expect_contains("piano_audio_start", piano,
                                "piano_audio_start()");
    failures += expect_contains("piano_maps_snapshot", piano,
                                "piano_touch_map(");
    failures += expect_contains("piano_applies_mask", piano,
                                "piano_audio_apply_note_mask(");
    failures += expect_contains("piano_timer", piano,
                                "lv_timer_create(");
    failures += expect_contains("piano_delete_stops", piano,
                                "piano_audio_stop()");
    failures += expect_contains("piano_tune_down_event", piano,
                                "screen_piano_tune_down_event_handler");
    failures += expect_contains("piano_tune_up_event", piano,
                                "screen_piano_tune_up_event_handler");
    failures += expect_contains("piano_sustain_switch", piano,
                                "lv_switch_create(");
    failures += expect_contains("piano_sustain_event", piano,
                                "screen_piano_sustain_event_handler");
    failures += expect_contains("piano_sustain_service", piano,
                                "piano_audio_set_sustain(");
    failures += expect_contains("piano_sustain_font", piano,
                                "&lv_font_myyh_20");
    failures += expect_contains("keil_piano_synth", project,
                                "piano_synth.c");
    failures += expect_contains("keil_piano_audio", project,
                                "piano_audio_service.c");
    failures += expect_contains("keil_piano_mapper", project,
                                "piano_touch_mapper.c");

    free(port_header);
    free(port_source);
    free(piano);
    free(project);
    if (failures != 0)
    {
        printf("FAIL piano_audio_ui_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS piano_audio_ui_contract\n");
    return 0;
}
