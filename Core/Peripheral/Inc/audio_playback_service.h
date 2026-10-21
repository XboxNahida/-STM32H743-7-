#ifndef AUDIO_PLAYBACK_SERVICE_H
#define AUDIO_PLAYBACK_SERVICE_H

#include <stdint.h>

typedef enum
{
    AUDIO_OK = 0,
    AUDIO_INVALID_ARG,
    AUDIO_BUSY,
    AUDIO_NOT_READY,
    AUDIO_IO_ERROR,
    AUDIO_CODEC_ERROR,
    AUDIO_DMA_ERROR,
    AUDIO_EOF,
    AUDIO_ERROR
} audio_result_t;

typedef enum
{
    AUDIO_STATE_IDLE = 0,
    AUDIO_STATE_PLAYING,
    AUDIO_STATE_PAUSED,
    AUDIO_STATE_ERROR,
    AUDIO_STATE_SUSPENDED,
    AUDIO_STATE_STREAMING,
    AUDIO_STATE_REALTIME
} audio_state_t;

typedef void (*audio_realtime_fill_t)(void *context, int16_t *stereo,
                                      uint32_t frame_count);

typedef enum
{
    AUDIO_STREAM_FORMAT_PCM = 0x0001,
    AUDIO_STREAM_FORMAT_MP3 = 0x0055
} audio_stream_format_t;

typedef struct
{
    uint8_t initialized;
    audio_state_t state;
    uint8_t volume_percent;
    uint32_t frame_count;
    uint32_t sample_rate;
    uint32_t elapsed_ms;
    uint32_t duration_ms;
    uint32_t underrun_count;
    int32_t last_result;
} audio_status_t;

void audio_service_init(void);
int audio_play(const char *path);
int audio_pause(void);
int audio_resume(void);
int audio_suspend_for_camera(void);
int audio_restore_after_camera(void);
int audio_stop(void);
int audio_stream_start(audio_stream_format_t format,
                       uint32_t sample_rate,
                       uint8_t channels,
                       uint8_t bits_per_sample);
int audio_stream_push(const uint8_t *data, uint32_t size);
void audio_stream_finish(void);
int audio_stream_stop(void);
int audio_realtime_start(audio_realtime_fill_t fill, void *context,
                         uint32_t sample_rate, uint16_t frames_per_half);
int audio_realtime_stop(void);
void audio_poll(void);
int audio_set_volume_percent(uint8_t percent);
void audio_get_status(audio_status_t *status);
void audio_print_status(void);
void audio_on_tx_half_complete(void);
void audio_on_tx_complete(void);

#endif
