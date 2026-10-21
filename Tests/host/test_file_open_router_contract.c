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

int main(void)
{
    char *router = read_source("lvgl_GUI/custom/file_open_router.c");
    char *events = read_source("lvgl_GUI/generated/events_init.c");
    int failures = 0;

    if ((router == NULL) || (events == NULL))
    {
        printf("FAIL read_router_sources\n");
        free(router);
        free(events);
        return 1;
    }

    failures += expect_source("unknown_type_rejected", router,
        "if (kind == FILE_MANAGER_KIND_OTHER) return 1;");
    failures += expect_source("text_opens_notes", router,
        "events_open_notes_file(g_router.path)");
    failures += expect_source("unsupported_image_viewer", router,
        "image_viewer_ui_open_unsupported(g_router.path)");
    failures += expect_source("nes_uses_requested_path", router,
        "nes_player_ui_request_open_file(g_router.path)");
    failures += expect_source("wav_temporary_playback", router,
        "wav_playback_start(g_router.path)");
    failures += expect_source("route_waits_for_screen_transition", router,
        "lv_display_get_screen_loading(display) != NULL");
    failures += expect_source("settings_sd_readonly_browser", events,
        "file_manager_ui_request_readonly(guider_ui.DeskTop_cont_folder)");

    free(router);
    free(events);
    if (failures != 0) return 1;
    printf("PASS file_open_router_contract\n");
    return 0;
}
