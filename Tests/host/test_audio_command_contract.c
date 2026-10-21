#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_source(const char *path)
{
    FILE *file;
    long size;
    char *text;

    file = fopen(path, "rb");
    if (file == NULL)
    {
        return NULL;
    }
    if ((fseek(file, 0L, SEEK_END) != 0) ||
        ((size = ftell(file)) < 0L) ||
        (fseek(file, 0L, SEEK_SET) != 0))
    {
        fclose(file);
        return NULL;
    }
    text = (char *)malloc((size_t)size + 1U);
    if (text == NULL)
    {
        fclose(file);
        return NULL;
    }
    if (fread(text, 1U, (size_t)size, file) != (size_t)size)
    {
        free(text);
        fclose(file);
        return NULL;
    }
    text[size] = '\0';
    fclose(file);
    return text;
}

static int expect_true(const char *name, int condition)
{
    if (condition != 0)
    {
        return 0;
    }
    printf("FAIL %s\n", name);
    return 1;
}

static unsigned int count_text(const char *source, const char *needle)
{
    unsigned int count = 0U;
    size_t length = strlen(needle);
    const char *cursor = source;

    while ((cursor = strstr(cursor, needle)) != NULL)
    {
        count++;
        cursor += length;
    }
    return count;
}

int main(void)
{
    char *command = read_source("Core/Peripheral/Src/command_parser.c");
    char *custom_header = read_source("lvgl_GUI/custom/custom.h");
    char *custom_source = read_source("lvgl_GUI/custom/custom.c");
    char *events = read_source("lvgl_GUI/generated/events_init.c");
    int failures = 0;

    failures += expect_true("read_command", command != NULL);
    failures += expect_true("read_custom_header", custom_header != NULL);
    failures += expect_true("read_custom_source", custom_source != NULL);
    failures += expect_true("read_events", events != NULL);
    if (failures != 0)
    {
        free(command);
        free(custom_header);
        free(custom_source);
        free(events);
        return 1;
    }

    failures += expect_true(
        "audio_service_header",
        strstr(command, "#include \"audio_playback_service.h\"") != NULL);
    failures += expect_true(
        "music_service_header",
        strstr(command, "#include \"music_player_service.h\"") != NULL);
    failures += expect_true(
        "es8388_service_header",
        strstr(command, "#include \"es8388_service.h\"") != NULL);
    failures += expect_true(
        "audio_handler",
        strstr(command, "static int command_audio_handler") != NULL);
    failures += expect_true(
        "audio_status_action",
        strstr(command, "audio_print_status();") != NULL);
    failures += expect_true(
        "audio_probe_action",
        strstr(command, "es8388_probe_readonly(10U)") != NULL);
    failures += expect_true(
        "audio_play_forwards_joined_path",
        strstr(command, "audio_play(path)") != NULL);
    failures += expect_true(
        "audio_stop_action",
        strstr(command, "music_player_stop()") != NULL);
    failures += expect_true(
        "audio_volume_range",
        strstr(command, "command_parse_uint(argv[1], 0U, 100U, &volume)") != NULL);
    failures += expect_true(
        "audio_volume_action",
        strstr(command, "audio_set_volume_percent((uint8_t)volume)") != NULL);
    failures += expect_true(
        "audio_ascii_usage",
        strstr(command,
               "usage=audio status|probe|play <sd_path>|record <sd_path>|stop|volume <0-100>") != NULL);
    failures += expect_true(
        "audio_registered_once",
        count_text(command, "{\"audio\",") == 1U);

    failures += expect_true(
        "volume_bridge_declared",
        strstr(custom_header,
               "void audio_volume_slider_changed(int32_t value);") != NULL);
    failures += expect_true(
        "volume_bridge_defined",
        strstr(custom_source,
               "void audio_volume_slider_changed(int32_t value)") != NULL);
    failures += expect_true(
        "volume_bridge_queues_music_service",
        strstr(custom_source,
               "music_player_request_volume((uint8_t)value, HAL_GetTick())") != NULL);
    failures += expect_true(
        "slider_calls_volume_bridge_once",
        count_text(events, "audio_volume_slider_changed(val);") == 1U);

    free(command);
    free(custom_header);
    free(custom_source);
    free(events);

    if (failures != 0)
    {
        printf("FAIL audio_command_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS audio_command_contract\n");
    return 0;
}
