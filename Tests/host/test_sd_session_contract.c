#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_source(const char *path)
{
    FILE *file;
    char *buffer;
    long length;

    file = fopen(path, "rb");
    if (file == NULL) return NULL;
    if ((fseek(file, 0L, SEEK_END) != 0) ||
        ((length = ftell(file)) < 0L) ||
        (fseek(file, 0L, SEEK_SET) != 0))
    {
        (void)fclose(file);
        return NULL;
    }
    buffer = (char *)malloc((size_t)length + 1U);
    if (buffer == NULL)
    {
        (void)fclose(file);
        return NULL;
    }
    if (fread(buffer, 1U, (size_t)length, file) != (size_t)length)
    {
        free(buffer);
        (void)fclose(file);
        return NULL;
    }
    buffer[length] = '\0';
    (void)fclose(file);
    return buffer;
}

static int contains_all(const char *source,
                        const char *first,
                        const char *second,
                        const char *third)
{
    return ((source != NULL) &&
            (strstr(source, first) != NULL) &&
            (strstr(source, second) != NULL) &&
            (strstr(source, third) != NULL)) ? 1 : 0;
}

static int expect_true(const char *name, int condition)
{
    if (condition == 0)
    {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

int main(void)
{
    char *mp3 = read_source("Core/Peripheral/Src/mp3_decoder_service.c");
    char *record = read_source("Core/Peripheral/Src/audio_recording_service.c");
    char *image = read_source("Core/Application/Src/image_viewer_app.c");
    char *events = read_source("lvgl_GUI/generated/events_init.c");
    int failures = 0;

    failures += expect_true(
        "mp3_holds_sd_session",
        contains_all(mp3,
                     "#define MP3_SD_SESSION_OWNER 0x0501U",
                     "sdcard_manager_session_acquire(MP3_SD_SESSION_OWNER)",
                     "mp3_decoder_release_session();"));
    failures += expect_true(
        "record_holds_sd_session",
        contains_all(record,
                     "#define AUDIO_RECORD_SD_SESSION_OWNER 0x0502U",
                     "sdcard_manager_session_acquire(AUDIO_RECORD_SD_SESSION_OWNER)",
                     "audio_record_release_sd_session();"));
    failures += expect_true(
        "image_holds_sd_session",
        contains_all(image,
                     "#define IMAGE_VIEWER_SD_SESSION_OWNER 0x0503U",
                     "sdcard_manager_session_acquire(IMAGE_VIEWER_SD_SESSION_OWNER)",
                     "sdcard_manager_session_release(IMAGE_VIEWER_SD_SESSION_OWNER)"));
    failures += expect_true(
        "avi_holds_sd_session",
        contains_all(events,
                     "#define AVI_SD_SESSION_OWNER 0x0504U",
                     "sdcard_manager_session_acquire(AVI_SD_SESSION_OWNER)",
                     "sdcard_manager_session_release(AVI_SD_SESSION_OWNER)"));

    free(events);
    free(image);
    free(record);
    free(mp3);

    if (failures == 0)
    {
        printf("PASS sd_session_contract\n");
        return 0;
    }
    printf("FAIL sd_session_contract failures=%d\n", failures);
    return 1;
}
