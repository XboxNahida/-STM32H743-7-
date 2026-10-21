#ifndef NES_AUDIO_SERVICE_H
#define NES_AUDIO_SERVICE_H

#include <stdint.h>

#define NES_AUDIO_SAMPLE_RATE       22050U
#define NES_AUDIO_FRAMES_PER_BLOCK  367U
#define NES_AUDIO_BLOCKS_PER_HALF   1U
#define NES_AUDIO_FRAMES_PER_HALF   \
    (NES_AUDIO_FRAMES_PER_BLOCK * NES_AUDIO_BLOCKS_PER_HALF)
#define NES_AUDIO_QUEUE_BLOCKS      4U

typedef enum
{
    NES_AUDIO_OK = 0,
    NES_AUDIO_INVALID_ARG,
    NES_AUDIO_BUSY,
    NES_AUDIO_NOT_READY,
    NES_AUDIO_ERROR
} nes_audio_result_t;

typedef struct
{
    uint8_t initialized;
    uint8_t active;
    uint8_t paused;
    uint8_t queue_depth;
    uint32_t generated_blocks;
    uint32_t consumed_blocks;
    uint32_t underrun_count;
    int32_t last_result;
} nes_audio_status_t;

void nes_audio_init(void);
int nes_audio_start(void);
uint8_t nes_audio_ready_for_frame(void);
int nes_audio_submit_frame(void);
void nes_audio_set_paused(uint8_t paused);
int nes_audio_stop(void);
void nes_audio_get_status(nes_audio_status_t *status);

#endif
