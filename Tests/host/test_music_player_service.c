#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "music_player_service.h"

static const char *g_root_entries[] = {
    "Root.mp3", "Shared.mp3"
};
static const char *g_system_entries[] = {
    "Test.mp3", "Album", "readme.txt", "YS.MP3", "Shared.mp3"
};
static const uint8_t g_system_entry_is_dir[] = {0U, 1U, 0U, 0U, 0U};
static uint32_t g_entry_index;
static uint8_t g_active_root;
static uint8_t g_capacity_mode;
static uint32_t g_session_acquire_count;
static uint32_t g_session_release_count;
static uint32_t g_volume_write_count;
static uint8_t g_last_volume;
static uint32_t g_play_count;
static uint32_t g_pause_count;
static uint32_t g_resume_count;
static int g_resume_result;
static uint32_t g_camera_suspend_count;
static uint32_t g_camera_restore_count;
static int g_camera_restore_result;
static audio_state_t g_camera_saved_state;
static uint32_t g_stop_count;
static char g_last_play_path[MUSIC_PLAYER_PATH_SIZE];
static audio_status_t g_audio_status;

int music_test_session_acquire(void)
{
    g_session_acquire_count++;
    return 0;
}

void music_test_session_release(void)
{
    g_session_release_count++;
}

int music_test_dir_open(const char *path)
{
    if (strcmp(path, "0:/MUSIC") == 0)
    {
        g_active_root = 1U;
        g_entry_index = 0U;
        return 0;
    }
    if (strcmp(path, "0:/SYSTEM/MUSIC") == 0)
    {
        g_active_root = 2U;
        g_entry_index = 0U;
        return 0;
    }
    return 1;
}

int music_test_dir_read(char *name, uint32_t name_size,
                        uint8_t *is_directory, uint8_t *end)
{
    size_t length;

    const char *entry;
    uint8_t entry_is_dir;
    uint32_t entry_count;

    if ((g_capacity_mode != 0U) && (g_active_root == 1U))
    {
        int written;

        if (g_entry_index >= MUSIC_PLAYER_MAX_TRACKS)
        {
            name[0] = '\0';
            *is_directory = 0U;
            *end = 1U;
            return 0;
        }
        written = snprintf(name, name_size, "Track%03lu.mp3",
                           (unsigned long)g_entry_index);
        if ((written < 0) || ((uint32_t)written >= name_size))
        {
            return 1;
        }
        g_entry_index++;
        *is_directory = 0U;
        *end = 0U;
        return 0;
    }
    if ((g_capacity_mode != 0U) && (g_active_root == 2U))
    {
        name[0] = '\0';
        *is_directory = 0U;
        *end = 1U;
        return 0;
    }

    if (g_active_root == 1U)
    {
        entry_count = (uint32_t)(sizeof(g_root_entries) /
                                 sizeof(g_root_entries[0]));
        entry = (g_entry_index < entry_count) ?
                g_root_entries[g_entry_index] : NULL;
        entry_is_dir = 0U;
    }
    else
    {
        entry_count = (uint32_t)(sizeof(g_system_entries) /
                                 sizeof(g_system_entries[0]));
        entry = (g_entry_index < entry_count) ?
                g_system_entries[g_entry_index] : NULL;
        entry_is_dir = (g_entry_index < entry_count) ?
                       g_system_entry_is_dir[g_entry_index] : 0U;
    }
    if (entry == NULL)
    {
        name[0] = '\0';
        *is_directory = 0U;
        *end = 1U;
        return 0;
    }
    length = strlen(entry);
    if (length >= name_size)
    {
        return 1;
    }
    memcpy(name, entry, length + 1U);
    *is_directory = entry_is_dir;
    *end = 0U;
    g_entry_index++;
    return 0;
}

void music_test_dir_close(void) {}

int audio_play(const char *path)
{
    size_t length = strlen(path);

    if (length >= sizeof(g_last_play_path))
    {
        return AUDIO_INVALID_ARG;
    }
    memcpy(g_last_play_path, path, length + 1U);
    g_play_count++;
    g_audio_status.state = AUDIO_STATE_PLAYING;
    g_audio_status.duration_ms = 180000U;
    g_audio_status.last_result = AUDIO_OK;
    return AUDIO_OK;
}

int audio_pause(void)
{
    g_pause_count++;
    g_audio_status.state = AUDIO_STATE_PAUSED;
    return AUDIO_OK;
}

int audio_resume(void)
{
    g_resume_count++;
    if (g_resume_result == AUDIO_OK)
    {
        g_audio_status.state = AUDIO_STATE_PLAYING;
    }
    return g_resume_result;
}

int audio_stop(void)
{
    g_stop_count++;
    g_audio_status.state = AUDIO_STATE_IDLE;
    return AUDIO_OK;
}

int audio_suspend_for_camera(void)
{
    g_camera_suspend_count++;
    g_camera_saved_state = g_audio_status.state;
    g_audio_status.state = AUDIO_STATE_SUSPENDED;
    return AUDIO_OK;
}

int audio_restore_after_camera(void)
{
    g_camera_restore_count++;
    if (g_camera_restore_result == AUDIO_OK)
    {
        g_audio_status.state = g_camera_saved_state;
    }
    return g_camera_restore_result;
}

int audio_set_volume_percent(uint8_t percent)
{
    g_volume_write_count++;
    g_last_volume = percent;
    g_audio_status.volume_percent = percent;
    return AUDIO_OK;
}

void audio_get_status(audio_status_t *status)
{
    if (status != NULL)
    {
        *status = g_audio_status;
    }
}

static int expect_u32(const char *name, uint32_t actual, uint32_t expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=%lu expected=%lu\n", name,
               (unsigned long)actual, (unsigned long)expected);
        return 1;
    }
    return 0;
}

static int expect_text(const char *name, const char *actual,
                       const char *expected)
{
    if ((actual == NULL) || (strcmp(actual, expected) != 0))
    {
        printf("FAIL %s actual=%s expected=%s\n", name,
               (actual != NULL) ? actual : "(null)", expected);
        return 1;
    }
    return 0;
}

static void reset_fixture(void)
{
    g_entry_index = 0U;
    g_active_root = 0U;
    g_capacity_mode = 0U;
    g_session_acquire_count = 0U;
    g_session_release_count = 0U;
    g_volume_write_count = 0U;
    g_last_volume = 0U;
    g_play_count = 0U;
    g_pause_count = 0U;
    g_resume_count = 0U;
    g_resume_result = AUDIO_OK;
    g_camera_suspend_count = 0U;
    g_camera_restore_count = 0U;
    g_camera_restore_result = AUDIO_OK;
    g_camera_saved_state = AUDIO_STATE_IDLE;
    g_stop_count = 0U;
    g_last_play_path[0] = '\0';
    memset(&g_audio_status, 0, sizeof(g_audio_status));
    g_audio_status.state = AUDIO_STATE_IDLE;
    g_audio_status.volume_percent = 50U;
    music_player_service_init();
}

static int test_scan_and_controls(void)
{
    music_player_status_t status;
    int failures = 0;

    reset_fixture();
    failures += expect_u32("scan", music_player_scan(), MUSIC_PLAYER_OK);
    music_player_get_status(&status);
    failures += expect_u32("capacity_at_least_512",
                           MUSIC_PLAYER_MAX_TRACKS >= 512U, 1U);
    failures += expect_u32("track_count", status.track_count, 4U);
    failures += expect_u32("roots_scanned", status.root_count, 2U);
    failures += expect_u32("duplicate_skipped", status.skipped_count, 1U);
    failures += expect_text("name_0", music_player_get_track_name(0U),
                            "Root");
    failures += expect_text("path_3", music_player_get_track_path(3U),
                            "0:/SYSTEM/MUSIC/YS.MP3");
    failures += expect_u32("session_acquire", g_session_acquire_count, 1U);
    failures += expect_u32("session_release", g_session_release_count, 1U);

    failures += expect_u32("play_0", music_player_play_index(0U),
                           MUSIC_PLAYER_OK);
    music_player_get_status(&status);
    failures += expect_u32("duration_propagated", status.duration_ms,
                           180000U);
    failures += expect_text("play_path_0", g_last_play_path,
                            "0:/MUSIC/Root.mp3");
    failures += expect_u32("next", music_player_next(), MUSIC_PLAYER_OK);
    failures += expect_text("play_path_1", g_last_play_path,
                            "0:/MUSIC/Shared.mp3");
    failures += expect_u32("previous", music_player_previous(),
                           MUSIC_PLAYER_OK);
    failures += expect_text("previous_wrap", g_last_play_path,
                            "0:/MUSIC/Root.mp3");
    failures += expect_u32("toggle_pause", music_player_toggle(),
                           MUSIC_PLAYER_OK);
    failures += expect_u32("pause_called", g_pause_count, 1U);
    failures += expect_u32("toggle_resume", music_player_toggle(),
                           MUSIC_PLAYER_OK);
    failures += expect_u32("resume_called", g_resume_count, 1U);
    return failures;
}

static int test_auto_next_and_volume_throttle(void)
{
    music_player_status_t status;
    int failures = 0;

    reset_fixture();
    (void)music_player_scan();
    (void)music_player_play_index(0U);
    g_audio_status.state = AUDIO_STATE_IDLE;
    g_audio_status.last_result = AUDIO_OK;
    music_player_poll(10U);
    music_player_get_status(&status);
    failures += expect_u32("auto_next_index", status.selected_index, 1U);
    failures += expect_u32("auto_next_play", g_play_count, 2U);

    music_player_request_volume(20U, 100U);
    music_player_request_volume(40U, 110U);
    music_player_request_volume(70U, 120U);
    music_player_poll(169U);
    failures += expect_u32("volume_not_early", g_volume_write_count, 0U);
    music_player_poll(170U);
    failures += expect_u32("volume_one_write", g_volume_write_count, 1U);
    failures += expect_u32("volume_latest", g_last_volume, 70U);
    music_player_request_volume(30U, 180U);
    music_player_request_volume(60U, 190U);
    music_player_poll(239U);
    failures += expect_u32("volume_second_not_early",
                           g_volume_write_count, 1U);
    music_player_poll(240U);
    failures += expect_u32("volume_second_one_write",
                           g_volume_write_count, 2U);
    failures += expect_u32("volume_second_latest", g_last_volume, 60U);
    music_player_get_status(&status);
    failures += expect_u32("volume_status", status.volume_percent, 60U);
    return failures;
}

static int test_manual_stop_does_not_auto_advance(void)
{
    music_player_status_t status;
    int failures = 0;

    reset_fixture();
    (void)music_player_scan();
    (void)music_player_play_index(0U);
    failures += expect_u32("manual_stop", music_player_stop(),
                           MUSIC_PLAYER_OK);
    music_player_poll(10U);
    music_player_get_status(&status);
    failures += expect_u32("manual_stop_idle", status.playback_state,
                           AUDIO_STATE_IDLE);
    failures += expect_u32("manual_stop_called_once", g_stop_count, 1U);
    failures += expect_u32("manual_stop_no_next", g_play_count, 1U);
    failures += expect_u32("manual_stop_keeps_index", status.selected_index,
                           0U);
    return failures;
}

static int test_camera_suspend_and_deferred_restore(void)
{
    music_player_status_t status;
    int failures = 0;

    reset_fixture();
    (void)music_player_scan();
    (void)music_player_play_index(0U);
    failures += expect_u32("camera_suspend_playing",
                           music_player_suspend_for_camera(),
                           MUSIC_PLAYER_OK);
    failures += expect_u32("camera_uses_dedicated_suspend",
                           g_camera_suspend_count, 1U);
    failures += expect_u32("camera_does_not_use_normal_pause",
                           g_pause_count, 0U);
    music_player_request_restore_after_camera();
    g_camera_restore_result = AUDIO_IO_ERROR;
    music_player_poll(100U);
    failures += expect_u32("camera_restore_waits_for_sd",
                           g_camera_restore_count, 1U);
    music_player_poll(150U);
    failures += expect_u32("camera_restore_is_throttled",
                           g_camera_restore_count, 1U);
    g_camera_restore_result = AUDIO_OK;
    music_player_poll(200U);
    music_player_get_status(&status);
    failures += expect_u32("camera_restore_retries",
                           g_camera_restore_count, 2U);
    failures += expect_u32("camera_restore_playing", status.playback_state,
                           AUDIO_STATE_PLAYING);

    reset_fixture();
    (void)music_player_scan();
    (void)music_player_play_index(0U);
    (void)music_player_toggle();
    failures += expect_u32("camera_suspend_already_paused",
                           music_player_suspend_for_camera(),
                           MUSIC_PLAYER_OK);
    music_player_request_restore_after_camera();
    music_player_poll(100U);
    failures += expect_u32("camera_keeps_manual_pause",
                           g_camera_restore_count, 1U);
    music_player_get_status(&status);
    failures += expect_u32("camera_restores_manual_pause_state",
                           status.playback_state, AUDIO_STATE_PAUSED);
    return failures;
}

static int test_full_512_track_capacity(void)
{
    music_player_status_t status;
    int failures = 0;

    reset_fixture();
    g_capacity_mode = 1U;
    failures += expect_u32("scan_512", music_player_scan(),
                           MUSIC_PLAYER_OK);
    music_player_get_status(&status);
    failures += expect_u32("track_count_512", status.track_count, 512U);
    failures += expect_u32("play_index_511",
                           music_player_play_index(511U),
                           MUSIC_PLAYER_OK);
    music_player_get_status(&status);
    failures += expect_u32("selected_index_511",
                           status.selected_index, 511U);
    failures += expect_text("play_path_511", g_last_play_path,
                            "0:/MUSIC/Track511.mp3");
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_scan_and_controls();
    failures += test_auto_next_and_volume_throttle();
    failures += test_manual_stop_does_not_auto_advance();
    failures += test_camera_suspend_and_deferred_restore();
    failures += test_full_512_track_capacity();
    if (failures != 0)
    {
        printf("FAIL music_player_service failures=%d\n", failures);
        return 1;
    }
    printf("PASS music_player_service\n");
    return 0;
}
