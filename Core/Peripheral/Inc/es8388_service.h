#ifndef ES8388_SERVICE_H
#define ES8388_SERVICE_H

#include <stdint.h>

typedef enum
{
    ES8388_OK = 0,
    ES8388_INVALID_ARG,
    ES8388_NO_ACK,
    ES8388_NOT_READY,
    ES8388_TIMEOUT,
    ES8388_ERROR
} es8388_result_t;

typedef struct
{
    uint8_t initialized;
    uint8_t present;
    uint8_t ready;
    uint8_t playback_configured;
    uint8_t recording_configured;
    uint8_t volume_percent;
    int32_t last_result;
    uint32_t probe_count;
    uint32_t write_count;
    uint32_t configure_count;
    uint32_t recording_configure_count;
} es8388_status_t;

void es8388_service_init(void);
int es8388_probe_readonly(uint32_t timeout_ms);
int es8388_prepare_playback(void);
int es8388_prepare_recording(void);
int es8388_set_volume_percent(uint8_t percent);
void es8388_get_status(es8388_status_t *status);
void es8388_print_status(void);

#endif
