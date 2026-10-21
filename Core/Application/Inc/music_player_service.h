#ifndef MUSIC_PLAYER_SERVICE_H
#define MUSIC_PLAYER_SERVICE_H

#include <stdint.h>

#include "audio_playback_service.h"

#define MUSIC_PLAYER_MAX_TRACKS 512U
#define MUSIC_PLAYER_PATH_SIZE 288U
#define MUSIC_PLAYER_NAME_SIZE 256U

typedef enum
{
    MUSIC_PLAYER_OK = 0,
    MUSIC_PLAYER_INVALID_ARG,
    MUSIC_PLAYER_NOT_READY,
    MUSIC_PLAYER_EMPTY,
    MUSIC_PLAYER_BUSY,
    MUSIC_PLAYER_IO_ERROR,
    MUSIC_PLAYER_AUDIO_ERROR
} music_player_result_t;

typedef struct
{
    uint8_t initialized;
    uint8_t scanned;
    uint16_t track_count;
    uint16_t selected_index;
    uint8_t root_count;
    uint16_t skipped_count;
    audio_state_t playback_state;
    uint8_t volume_percent;
    uint32_t elapsed_ms;
    uint32_t duration_ms;
    int32_t last_result;
} music_player_status_t;

void music_player_service_init(void);
int music_player_scan(void);
int music_player_play_index(uint32_t index);
int music_player_toggle(void);
int music_player_stop(void);
int music_player_suspend_for_camera(void);
void music_player_request_restore_after_camera(void);
int music_player_next(void);
int music_player_previous(void);
void music_player_request_volume(uint8_t percent, uint32_t now_ms);
void music_player_poll(uint32_t now_ms);
void music_player_get_status(music_player_status_t *status);
const char *music_player_get_track_name(uint32_t index);
const char *music_player_get_track_path(uint32_t index);

#endif
