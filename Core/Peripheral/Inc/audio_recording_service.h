#ifndef AUDIO_RECORDING_SERVICE_H
#define AUDIO_RECORDING_SERVICE_H

#include <stdint.h>

typedef enum
{
    AUDIO_RECORD_OK = 0,
    AUDIO_RECORD_INVALID_ARG,
    AUDIO_RECORD_BUSY,
    AUDIO_RECORD_NOT_READY,
    AUDIO_RECORD_IO_ERROR,
    AUDIO_RECORD_CODEC_ERROR,
    AUDIO_RECORD_DMA_ERROR,
    AUDIO_RECORD_LIMIT,
    AUDIO_RECORD_ERROR
} audio_record_result_t;

typedef enum
{
    AUDIO_RECORD_STATE_IDLE = 0,
    AUDIO_RECORD_STATE_RECORDING,
    AUDIO_RECORD_STATE_ERROR
} audio_record_state_t;

typedef struct
{
    uint8_t initialized;
    audio_record_state_t state;
    uint32_t sample_rate;
    uint8_t channels;
    uint8_t bits_per_sample;
    uint32_t bytes_written;
    uint32_t sample_count;
    uint32_t dma_error_count;
    uint32_t write_error_count;
    uint32_t overrun_count;
    uint8_t pending_events;
    int32_t last_result;
} audio_recording_status_t;

void audio_recording_service_init(void);
int audio_record_start(const char *path);
int audio_record_stop(void);
void audio_record_poll(void);
void audio_record_get_status(audio_recording_status_t *status);
void audio_record_print_status(void);
void audio_record_on_rx_half_complete(void);
void audio_record_on_rx_complete(void);
void audio_record_on_rx_error(void);

#endif
