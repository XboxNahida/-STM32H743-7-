#include "file_open_router.h"

#include <string.h>

#include "audio_playback_service.h"
#include "drawing_app_ui.h"
#include "events_init.h"
#include "file_manager_path.h"
#include "gui_guider.h"
#include "image_viewer_ui.h"
#include "music_player_service.h"
#include "music_player_ui.h"
#include "nes_player_ui.h"
#include "wav_playback_service.h"

typedef struct
{
    char path[FILE_MANAGER_PATH_CAPACITY];
    file_manager_kind_t kind;
    uint8_t pending;
} file_open_router_context_t;

static file_open_router_context_t g_router;

void file_open_router_init(void)
{
    memset(&g_router, 0, sizeof(g_router));
}

int file_open_router_request(const char *path)
{
    size_t length;
    file_manager_kind_t kind;

    if (path == NULL) return 1;
    length = strlen(path);
    if ((length == 0U) || (length >= sizeof(g_router.path))) return 1;
    kind = file_manager_classify(path);
    if (kind == FILE_MANAGER_KIND_OTHER) return 1;
    memcpy(g_router.path, path, length + 1U);
    g_router.kind = kind;
    g_router.pending = 1U;
    return 0;
}

void file_open_router_poll(void)
{
    lv_display_t *display;
    int result = 1;

    if (g_router.pending == 0U) return;
    if ((guider_ui.DeskTop == NULL) || !lv_obj_is_valid(guider_ui.DeskTop) ||
        (lv_screen_active() != guider_ui.DeskTop)) return;
    display = lv_display_get_default();
    if ((display == NULL) ||
        (lv_display_get_screen_loading(display) != NULL)) return;
    switch (g_router.kind)
    {
    case FILE_MANAGER_KIND_TEXT:
        result = events_open_notes_file(g_router.path);
        break;
    case FILE_MANAGER_KIND_JPEG:
        result = image_viewer_ui_open_file(g_router.path);
        break;
    case FILE_MANAGER_KIND_IMAGE_UNSUPPORTED:
        result = image_viewer_ui_open_unsupported(g_router.path);
        break;
    case FILE_MANAGER_KIND_AVI:
        result = events_open_avi_file(g_router.path);
        break;
    case FILE_MANAGER_KIND_MP3:
        (void)music_player_stop();
        result = audio_play(g_router.path);
        if (result == AUDIO_OK) music_player_ui_request_open();
        break;
    case FILE_MANAGER_KIND_NES:
        result = nes_player_ui_request_open_file(g_router.path);
        break;
    case FILE_MANAGER_KIND_WAV:
        (void)music_player_stop();
        result = wav_playback_start(g_router.path);
        if (result == 0) music_player_ui_request_open();
        break;
    case FILE_MANAGER_KIND_DRAWING_BMP:
        result = drawing_app_ui_request_open_file(g_router.path);
        break;
    default:
        result = 1;
        break;
    }
    g_router.pending = 0U;
}
