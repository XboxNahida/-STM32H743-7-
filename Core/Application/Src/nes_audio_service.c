#include "nes_audio_service.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>

#include "audio_playback_service.h"
#include "nes_apu.h"

extern uint32_t clocks;

static int16_t g_nes_audio_queue[NES_AUDIO_QUEUE_BLOCKS]
                                [NES_AUDIO_FRAMES_PER_BLOCK];
static nes_audio_status_t g_nes_audio_status;
static uint8_t g_nes_audio_read_index;
static uint8_t g_nes_audio_write_index;
static uint8_t g_nes_audio_count;

static void nes_audio_increment(uint32_t *value)
{
    if (*value != UINT32_MAX)
    {
        ++(*value);
    }
}

static void nes_audio_reset_queue(void)
{
    g_nes_audio_read_index = 0U;
    g_nes_audio_write_index = 0U;
    g_nes_audio_count = 0U;
    g_nes_audio_status.queue_depth = 0U;
}

static int nes_audio_map_result(int result)
{
    if (result == AUDIO_OK)
    {
        return NES_AUDIO_OK;
    }
    if (result == AUDIO_BUSY)
    {
        return NES_AUDIO_BUSY;
    }
    if (result == AUDIO_INVALID_ARG)
    {
        return NES_AUDIO_INVALID_ARG;
    }
    return NES_AUDIO_ERROR;
}

static void nes_audio_fill(void *context, int16_t *stereo,
                           uint32_t frame_count)
{
    const int16_t *mono;
    uint32_t block;
    uint32_t index;

    (void)context;
    if (stereo == NULL)
    {
        return;
    }
    if ((frame_count != NES_AUDIO_FRAMES_PER_HALF) ||
        (g_nes_audio_status.active == 0U) ||
        (g_nes_audio_status.paused != 0U))
    {
        memset(stereo, 0, frame_count * 2U * sizeof(*stereo));
        return;
    }

    for (block = 0U; block < NES_AUDIO_BLOCKS_PER_HALF; ++block)
    {
        uint32_t stereo_offset = block * NES_AUDIO_FRAMES_PER_BLOCK * 2U;

        if (g_nes_audio_count == 0U)
        {
            memset(&stereo[stereo_offset], 0,
                   (frame_count * 2U - stereo_offset) * sizeof(*stereo));
            nes_audio_increment(&g_nes_audio_status.underrun_count);
            return;
        }
        mono = g_nes_audio_queue[g_nes_audio_read_index];
        for (index = 0U; index < NES_AUDIO_FRAMES_PER_BLOCK; ++index)
        {
            stereo[stereo_offset + index * 2U] = mono[index];
            stereo[stereo_offset + index * 2U + 1U] = mono[index];
        }
        g_nes_audio_read_index = (uint8_t)(
            (g_nes_audio_read_index + 1U) % NES_AUDIO_QUEUE_BLOCKS);
        --g_nes_audio_count;
        g_nes_audio_status.queue_depth = g_nes_audio_count;
        nes_audio_increment(&g_nes_audio_status.consumed_blocks);
    }
}

void nes_audio_init(void)
{
    memset(&g_nes_audio_status, 0, sizeof(g_nes_audio_status));
    nes_audio_reset_queue();
    g_nes_audio_status.initialized = 1U;
    g_nes_audio_status.last_result = NES_AUDIO_NOT_READY;
}

int nes_audio_start(void)
{
    int result;

    if (g_nes_audio_status.active != 0U)
    {
        return NES_AUDIO_OK;
    }
    nes_audio_reset_queue();
    g_nes_audio_status.paused = 0U;
    g_nes_audio_status.generated_blocks = 0U;
    g_nes_audio_status.consumed_blocks = 0U;
    g_nes_audio_status.underrun_count = 0U;
    result = nes_audio_map_result(audio_realtime_start(
        nes_audio_fill, NULL, NES_AUDIO_SAMPLE_RATE,
        NES_AUDIO_FRAMES_PER_HALF));
    if (result != NES_AUDIO_OK)
    {
        g_nes_audio_status.active = 0U;
        g_nes_audio_status.last_result = result;
        return result;
    }
    g_nes_audio_status.active = 1U;
    g_nes_audio_status.last_result = NES_AUDIO_OK;
    return NES_AUDIO_OK;
}

uint8_t nes_audio_ready_for_frame(void)
{
    if ((g_nes_audio_status.active == 0U) ||
        (g_nes_audio_status.paused != 0U))
    {
        return 1U;
    }
    return (g_nes_audio_count < NES_AUDIO_QUEUE_BLOCKS) ? 1U : 0U;
}

int nes_audio_submit_frame(void)
{
    if ((g_nes_audio_status.active != 0U) &&
        (g_nes_audio_status.paused == 0U))
    {
        if (g_nes_audio_count >= NES_AUDIO_QUEUE_BLOCKS)
        {
            g_nes_audio_status.last_result = NES_AUDIO_BUSY;
            return NES_AUDIO_BUSY;
        }
        apu_process((uint16_t *)g_nes_audio_queue[g_nes_audio_write_index],
                    (int)NES_AUDIO_FRAMES_PER_BLOCK);
        g_nes_audio_write_index = (uint8_t)(
            (g_nes_audio_write_index + 1U) % NES_AUDIO_QUEUE_BLOCKS);
        ++g_nes_audio_count;
        g_nes_audio_status.queue_depth = g_nes_audio_count;
        nes_audio_increment(&g_nes_audio_status.generated_blocks);
    }
    else
    {
        apu_process(NULL, (int)NES_AUDIO_FRAMES_PER_BLOCK);
    }
    clocks = 0U;
    g_nes_audio_status.last_result = NES_AUDIO_OK;
    return NES_AUDIO_OK;
}

void nes_audio_set_paused(uint8_t paused)
{
    g_nes_audio_status.paused = (paused != 0U) ? 1U : 0U;
    if (g_nes_audio_status.paused != 0U)
    {
        nes_audio_reset_queue();
    }
}

int nes_audio_stop(void)
{
    int result;

    if (g_nes_audio_status.active == 0U)
    {
        return NES_AUDIO_OK;
    }
    g_nes_audio_status.active = 0U;
    g_nes_audio_status.paused = 0U;
    nes_audio_reset_queue();
    result = nes_audio_map_result(audio_realtime_stop());
    g_nes_audio_status.last_result = result;
    return result;
}

void nes_audio_get_status(nes_audio_status_t *status)
{
    if (status != NULL)
    {
        *status = g_nes_audio_status;
    }
}
