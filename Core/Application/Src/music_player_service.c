#include "music_player_service.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define MUSIC_PLAYER_SD_OWNER 0x0505U
#define MUSIC_PLAYER_VOLUME_INTERVAL_MS 50U
#define MUSIC_PLAYER_CAMERA_RESUME_RETRY_MS 100U

#if defined(MUSIC_PLAYER_SERVICE_HOST_TEST)
#include <stdlib.h>
#define music_player_alloc_zeroed(size) calloc(1U, (size))
extern int music_test_session_acquire(void);
extern void music_test_session_release(void);
extern int music_test_dir_open(const char *path);
extern int music_test_dir_read(char *name, uint32_t name_size,
                               uint8_t *is_directory, uint8_t *end);
extern void music_test_dir_close(void);
#else
#include "ff.h"
#include "lvgl.h"
#include "sdcard_manager.h"
#define music_player_alloc_zeroed(size) lv_malloc_zeroed(size)

static DIR g_music_dir;
static FILINFO g_music_file_info;

static int music_test_session_acquire(void)
{
    return (sdcard_manager_session_acquire(MUSIC_PLAYER_SD_OWNER) ==
            SDCARD_MANAGER_OK) ? 0 : 1;
}

static void music_test_session_release(void)
{
    (void)sdcard_manager_session_release(MUSIC_PLAYER_SD_OWNER);
}

static int music_test_dir_open(const char *path)
{
    return (f_opendir(&g_music_dir, path) == FR_OK) ? 0 : 1;
}

static int music_test_dir_read(char *name, uint32_t name_size,
                               uint8_t *is_directory, uint8_t *end)
{
    size_t length;

    if ((name == NULL) || (name_size == 0U) ||
        (is_directory == NULL) || (end == NULL))
    {
        return 1;
    }
    if (f_readdir(&g_music_dir, &g_music_file_info) != FR_OK)
    {
        return 1;
    }
    if (g_music_file_info.fname[0] == '\0')
    {
        name[0] = '\0';
        *is_directory = 0U;
        *end = 1U;
        return 0;
    }
    length = strlen(g_music_file_info.fname);
    if (length >= name_size)
    {
        return 1;
    }
    memcpy(name, g_music_file_info.fname, length + 1U);
    *is_directory = ((g_music_file_info.fattrib & AM_DIR) != 0U) ? 1U : 0U;
    *end = 0U;
    return 0;
}

static void music_test_dir_close(void)
{
    (void)f_closedir(&g_music_dir);
}
#endif

typedef struct
{
    uint8_t root_index;
    char file_name[MUSIC_PLAYER_NAME_SIZE];
} music_track_t;

static const char *const g_music_roots[] =
{
    "0:/MUSIC",
    "0:/SYSTEM/MUSIC"
};

static music_player_status_t g_music_status;
static music_track_t *g_music_tracks;
static char g_music_path_buffer[MUSIC_PLAYER_PATH_SIZE];
static char g_music_name_buffer[MUSIC_PLAYER_NAME_SIZE];
static uint8_t g_auto_advance_armed;
static uint8_t g_volume_dirty;
static uint8_t g_pending_volume;
static uint32_t g_volume_deadline_ms;
static uint8_t g_camera_audio_suspended;
static uint8_t g_camera_restore_pending;
static uint32_t g_camera_restore_deadline_ms;

static int music_player_time_reached(uint32_t now_ms, uint32_t deadline_ms)
{
    return ((int32_t)(now_ms - deadline_ms) >= 0) ? 1 : 0;
}

static int music_player_is_mp3(const char *name)
{
    size_t length;
    const char *extension;

    if (name == NULL)
    {
        return 0;
    }
    length = strlen(name);
    if (length < 4U)
    {
        return 0;
    }
    extension = &name[length - 4U];
    return ((extension[0] == '.') &&
            ((extension[1] == 'm') || (extension[1] == 'M')) &&
            ((extension[2] == 'p') || (extension[2] == 'P')) &&
            (extension[3] == '3')) ? 1 : 0;
}

static int music_player_store_track(uint32_t root_index,
                                    const char *file_name)
{
    music_track_t *track;
    size_t file_length;

    if ((file_name == NULL) || (g_music_tracks == NULL) ||
        (root_index >= (sizeof(g_music_roots) / sizeof(g_music_roots[0]))) ||
        (g_music_status.track_count >= MUSIC_PLAYER_MAX_TRACKS))
    {
        return MUSIC_PLAYER_INVALID_ARG;
    }
    file_length = strlen(file_name);
    if ((file_length >= MUSIC_PLAYER_NAME_SIZE) ||
        ((strlen(g_music_roots[root_index]) + 1U + file_length) >=
         MUSIC_PLAYER_PATH_SIZE))
    {
        return MUSIC_PLAYER_INVALID_ARG;
    }
    track = &g_music_tracks[g_music_status.track_count];
    track->root_index = (uint8_t)root_index;
    memcpy(track->file_name, file_name, file_length + 1U);
    g_music_status.track_count++;
    return MUSIC_PLAYER_OK;
}

static int music_player_track_exists(const char *file_name)
{
    size_t title_length;
    uint32_t index;

    if (file_name == NULL)
    {
        return 0;
    }
    title_length = strlen(file_name);
    if (title_length < 4U)
    {
        return 0;
    }
    title_length -= 4U;
    for (index = 0U; index < g_music_status.track_count; index++)
    {
        if ((strlen(g_music_tracks[index].file_name) == (title_length + 4U)) &&
            (memcmp(g_music_tracks[index].file_name, file_name,
                    title_length) == 0))
        {
            return 1;
        }
    }
    return 0;
}

void music_player_service_init(void)
{
    audio_status_t audio_status;

    memset(&g_music_status, 0, sizeof(g_music_status));
    if (g_music_tracks == NULL)
    {
        g_music_tracks = (music_track_t *)music_player_alloc_zeroed(
            sizeof(music_track_t) * MUSIC_PLAYER_MAX_TRACKS);
    }
    else
    {
        memset(g_music_tracks, 0,
               sizeof(music_track_t) * MUSIC_PLAYER_MAX_TRACKS);
    }
    g_music_path_buffer[0] = '\0';
    g_music_name_buffer[0] = '\0';
    memset(&audio_status, 0, sizeof(audio_status));
    audio_get_status(&audio_status);
    g_music_status.initialized = (g_music_tracks != NULL) ? 1U : 0U;
    g_music_status.playback_state = audio_status.state;
    g_music_status.volume_percent = audio_status.volume_percent;
    g_music_status.duration_ms = audio_status.duration_ms;
    g_music_status.last_result = MUSIC_PLAYER_NOT_READY;
    g_auto_advance_armed = 0U;
    g_volume_dirty = 0U;
    g_pending_volume = audio_status.volume_percent;
    g_volume_deadline_ms = 0U;
    g_camera_audio_suspended = 0U;
    g_camera_restore_pending = 0U;
    g_camera_restore_deadline_ms = 0U;
}

int music_player_scan(void)
{
    uint32_t root_index;
    char file_name[MUSIC_PLAYER_PATH_SIZE];

    if (g_music_status.initialized == 0U)
    {
        return MUSIC_PLAYER_NOT_READY;
    }
    if (music_test_session_acquire() != 0)
    {
        g_music_status.last_result = MUSIC_PLAYER_BUSY;
        return MUSIC_PLAYER_BUSY;
    }
    g_music_status.track_count = 0U;
    g_music_status.root_count = 0U;
    g_music_status.skipped_count = 0U;
    for (root_index = 0U;
         root_index < (sizeof(g_music_roots) / sizeof(g_music_roots[0]));
         root_index++)
    {
        if (music_test_dir_open(g_music_roots[root_index]) != 0)
        {
            continue;
        }
        g_music_status.root_count++;
        for (;;)
        {
            uint8_t is_directory = 0U;
            uint8_t end = 0U;
            int read_result = music_test_dir_read(file_name,
                                                   sizeof(file_name),
                                                   &is_directory, &end);

            if (read_result != 0)
            {
                g_music_status.skipped_count++;
                break;
            }
            if (end != 0U)
            {
                break;
            }
            if ((is_directory == 0U) &&
                (music_player_is_mp3(file_name) != 0))
            {
                if (music_player_track_exists(file_name) != 0)
                {
                    g_music_status.skipped_count++;
                }
                else if (music_player_store_track(root_index,
                                                   file_name) !=
                         MUSIC_PLAYER_OK)
                {
                    g_music_status.skipped_count++;
                }
            }
        }
        music_test_dir_close();
    }
    music_test_session_release();
    g_music_status.scanned = 1U;
    g_music_status.selected_index = 0U;
    if (g_music_status.root_count == 0U)
    {
        g_music_status.last_result = MUSIC_PLAYER_IO_ERROR;
    }
    else
    {
        g_music_status.last_result = (g_music_status.track_count > 0U) ?
                                     MUSIC_PLAYER_OK : MUSIC_PLAYER_EMPTY;
    }
    return g_music_status.last_result;
}

int music_player_play_index(uint32_t index)
{
    audio_status_t audio_status;
    int result;

    if (index >= g_music_status.track_count)
    {
        return MUSIC_PLAYER_INVALID_ARG;
    }
    memset(&audio_status, 0, sizeof(audio_status));
    audio_get_status(&audio_status);
    if ((audio_status.state == AUDIO_STATE_PLAYING) ||
        (audio_status.state == AUDIO_STATE_PAUSED))
    {
        (void)audio_stop();
    }
    result = audio_play(music_player_get_track_path(index));
    if (result != AUDIO_OK)
    {
        g_music_status.last_result = MUSIC_PLAYER_AUDIO_ERROR;
        return MUSIC_PLAYER_AUDIO_ERROR;
    }
    g_music_status.selected_index = (uint16_t)index;
    g_music_status.playback_state = AUDIO_STATE_PLAYING;
    g_music_status.elapsed_ms = 0U;
    audio_get_status(&audio_status);
    g_music_status.duration_ms = audio_status.duration_ms;
    g_music_status.last_result = MUSIC_PLAYER_OK;
    g_auto_advance_armed = 1U;
    return MUSIC_PLAYER_OK;
}

int music_player_toggle(void)
{
    audio_status_t audio_status;
    int result;

    audio_get_status(&audio_status);
    if (audio_status.state == AUDIO_STATE_PLAYING)
    {
        result = audio_pause();
    }
    else if (audio_status.state == AUDIO_STATE_PAUSED)
    {
        result = audio_resume();
    }
    else
    {
        return (g_music_status.track_count > 0U) ?
               music_player_play_index(g_music_status.selected_index) :
               MUSIC_PLAYER_EMPTY;
    }
    if (result != AUDIO_OK)
    {
        g_music_status.last_result = MUSIC_PLAYER_AUDIO_ERROR;
        return MUSIC_PLAYER_AUDIO_ERROR;
    }
    audio_get_status(&audio_status);
    g_music_status.playback_state = audio_status.state;
    g_music_status.last_result = MUSIC_PLAYER_OK;
    return MUSIC_PLAYER_OK;
}

int music_player_stop(void)
{
    int result;

    g_auto_advance_armed = 0U;
    g_camera_audio_suspended = 0U;
    g_camera_restore_pending = 0U;
    result = audio_stop();
    if (result != AUDIO_OK)
    {
        g_music_status.last_result = MUSIC_PLAYER_AUDIO_ERROR;
        return MUSIC_PLAYER_AUDIO_ERROR;
    }
    g_music_status.playback_state = AUDIO_STATE_IDLE;
    g_music_status.last_result = MUSIC_PLAYER_OK;
    return MUSIC_PLAYER_OK;
}

int music_player_suspend_for_camera(void)
{
    audio_status_t audio_status;
    int result;

    if (g_camera_audio_suspended != 0U)
    {
        return MUSIC_PLAYER_OK;
    }
    memset(&audio_status, 0, sizeof(audio_status));
    audio_get_status(&audio_status);
    g_camera_restore_pending = 0U;
    if ((audio_status.state != AUDIO_STATE_PLAYING) &&
        (audio_status.state != AUDIO_STATE_PAUSED))
    {
        g_camera_audio_suspended = 0U;
        return MUSIC_PLAYER_OK;
    }
    result = audio_suspend_for_camera();
    if (result != AUDIO_OK)
    {
        g_music_status.last_result = MUSIC_PLAYER_AUDIO_ERROR;
        return MUSIC_PLAYER_AUDIO_ERROR;
    }
    g_camera_audio_suspended = 1U;
    g_music_status.playback_state = AUDIO_STATE_SUSPENDED;
    g_music_status.last_result = MUSIC_PLAYER_OK;
    return MUSIC_PLAYER_OK;
}

void music_player_request_restore_after_camera(void)
{
    if (g_camera_audio_suspended != 0U)
    {
        g_camera_restore_pending = 1U;
        g_camera_restore_deadline_ms = 0U;
    }
}

int music_player_next(void)
{
    uint32_t index;

    if (g_music_status.track_count == 0U)
    {
        return MUSIC_PLAYER_EMPTY;
    }
    index = ((uint32_t)g_music_status.selected_index + 1U) %
            g_music_status.track_count;
    return music_player_play_index(index);
}

int music_player_previous(void)
{
    uint32_t index;

    if (g_music_status.track_count == 0U)
    {
        return MUSIC_PLAYER_EMPTY;
    }
    index = (g_music_status.selected_index == 0U) ?
            ((uint32_t)g_music_status.track_count - 1U) :
            ((uint32_t)g_music_status.selected_index - 1U);
    return music_player_play_index(index);
}

void music_player_request_volume(uint8_t percent, uint32_t now_ms)
{
    if (percent > 100U)
    {
        percent = 100U;
    }
    g_pending_volume = percent;
    g_music_status.volume_percent = percent;
    g_volume_dirty = 1U;
    g_volume_deadline_ms = now_ms + MUSIC_PLAYER_VOLUME_INTERVAL_MS;
}

void music_player_poll(uint32_t now_ms)
{
    audio_status_t audio_status;

    if ((g_camera_restore_pending != 0U) &&
        (music_player_time_reached(now_ms,
                                   g_camera_restore_deadline_ms) != 0))
    {
        if (audio_restore_after_camera() == AUDIO_OK)
        {
            g_camera_restore_pending = 0U;
            g_camera_audio_suspended = 0U;
        }
        else
        {
            g_camera_restore_deadline_ms =
                now_ms + MUSIC_PLAYER_CAMERA_RESUME_RETRY_MS;
        }
    }

    if ((g_volume_dirty != 0U) &&
        (music_player_time_reached(now_ms, g_volume_deadline_ms) != 0))
    {
        if (audio_set_volume_percent(g_pending_volume) == AUDIO_OK)
        {
            g_music_status.volume_percent = g_pending_volume;
            g_music_status.last_result = MUSIC_PLAYER_OK;
        }
        else
        {
            g_music_status.last_result = MUSIC_PLAYER_AUDIO_ERROR;
        }
        g_volume_dirty = 0U;
    }

    memset(&audio_status, 0, sizeof(audio_status));
    audio_get_status(&audio_status);
    g_music_status.playback_state = audio_status.state;
    g_music_status.elapsed_ms = audio_status.elapsed_ms;
    g_music_status.duration_ms = audio_status.duration_ms;
    if (g_volume_dirty == 0U)
    {
        g_music_status.volume_percent = audio_status.volume_percent;
    }
    if ((audio_status.state == AUDIO_STATE_PLAYING) ||
        (audio_status.state == AUDIO_STATE_PAUSED))
    {
        g_auto_advance_armed = 1U;
    }
    else if ((g_auto_advance_armed != 0U) &&
             (audio_status.state == AUDIO_STATE_IDLE) &&
             (audio_status.last_result == AUDIO_OK))
    {
        g_auto_advance_armed = 0U;
        (void)music_player_next();
    }
    else if (audio_status.state == AUDIO_STATE_ERROR)
    {
        g_auto_advance_armed = 0U;
        g_music_status.last_result = MUSIC_PLAYER_AUDIO_ERROR;
    }
}

void music_player_get_status(music_player_status_t *status)
{
    if (status != NULL)
    {
        *status = g_music_status;
    }
}

const char *music_player_get_track_name(uint32_t index)
{
    size_t length;

    if ((g_music_tracks == NULL) || (index >= g_music_status.track_count))
    {
        return NULL;
    }
    length = strlen(g_music_tracks[index].file_name);
    if ((length < 4U) || (length >= sizeof(g_music_name_buffer)))
    {
        return NULL;
    }
    memcpy(g_music_name_buffer, g_music_tracks[index].file_name,
           length - 4U);
    g_music_name_buffer[length - 4U] = '\0';
    return g_music_name_buffer;
}

const char *music_player_get_track_path(uint32_t index)
{
    const music_track_t *track;
    int length;

    if ((g_music_tracks == NULL) || (index >= g_music_status.track_count))
    {
        return NULL;
    }
    track = &g_music_tracks[index];
    if (track->root_index >=
        (sizeof(g_music_roots) / sizeof(g_music_roots[0])))
    {
        return NULL;
    }
    length = snprintf(g_music_path_buffer, sizeof(g_music_path_buffer),
                      "%s/%s", g_music_roots[track->root_index],
                      track->file_name);
    return ((length >= 0) &&
            ((size_t)length < sizeof(g_music_path_buffer))) ?
           g_music_path_buffer : NULL;
}
