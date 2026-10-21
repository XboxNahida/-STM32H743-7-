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

static unsigned int count_text(const char *text, const char *needle)
{
    unsigned int count = 0U;
    size_t length = strlen(needle);

    while ((text = strstr(text, needle)) != NULL)
    {
        count++;
        text += length;
    }
    return count;
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
    char *events = read_source("lvgl_GUI/generated/events_init.c");
    int failures = 0;

    failures += expect_true("read_events", events != NULL);
    if (events == NULL) return 1;
    failures += expect_true("audio_service_header",
        strstr(events, "#include \"audio_playback_service.h\"") != NULL);
    failures += expect_true("music_service_header",
        strstr(events, "#include \"music_player_service.h\"") != NULL);
    failures += expect_true("generated_patch_marker",
        strstr(events, "AVI AUDIO USER PATCH") != NULL);
    failures += expect_true("stop_background_music",
        strstr(events, "music_player_stop();") != NULL);
    failures += expect_true("start_stream_from_avi_metadata",
        strstr(events, "audio_stream_start") != NULL &&
        strstr(events, "g_avix.SampleRate") != NULL &&
        strstr(events, "g_avix.Channels") != NULL &&
        strstr(events, "g_avix.BitsPerSample") != NULL);
    failures += expect_true("audio_chunks_read_and_pushed",
        strstr(events, "audio_stream_push") != NULL &&
        count_text(events, "avi_player_audio_read(") >= 5U);
    failures += expect_true("audio_polled_in_blocking_path",
        count_text(events, "audio_poll();") >= 2U);
    failures += expect_true("audio_stopped_on_all_player_exits",
        count_text(events, "avi_player_audio_stop();") >= 3U);
    failures += expect_true("old_audio_skip_removed",
        strstr(events, "Skip audio chunk") == NULL &&
        strstr(events, "audio_skip=") == NULL);
    free(events);
    if (failures != 0)
    {
        printf("FAIL avi_audio_integration_contract failures=%d\n",
               failures);
        return 1;
    }
    printf("PASS avi_audio_integration_contract\n");
    return 0;
}
