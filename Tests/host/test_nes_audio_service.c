#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "audio_playback_service.h"
#include "nes_audio_service.h"

#define EXPECTED_BLOCKS_PER_HALF  1U
#define EXPECTED_FRAMES_PER_HALF  \
    (NES_AUDIO_FRAMES_PER_BLOCK * EXPECTED_BLOCKS_PER_HALF)

uint32_t clocks;

static audio_realtime_fill_t g_fill;
static void *g_fill_context;
static uint32_t g_sample_rate;
static uint16_t g_frames_per_half;
static int g_start_result;
static int g_stop_result;
static uint32_t g_process_count;
static uint32_t g_silent_process_count;
static uint32_t g_stop_count;

void apu_process(uint16_t *buffer, int samples)
{
    uint32_t index;

    ++g_process_count;
    if (buffer == NULL)
    {
        ++g_silent_process_count;
        return;
    }
    for (index = 0U; index < (uint32_t)samples; ++index)
    {
        buffer[index] = (uint16_t)(g_process_count * 1000U + index);
    }
}

int audio_realtime_start(audio_realtime_fill_t fill, void *context,
                         uint32_t sample_rate, uint16_t frames_per_half)
{
    int16_t initial[EXPECTED_FRAMES_PER_HALF * 2U];

    g_fill = fill;
    g_fill_context = context;
    g_sample_rate = sample_rate;
    g_frames_per_half = frames_per_half;
    memset(initial, 0x5A, sizeof(initial));
    fill(context, initial, frames_per_half);
    if ((initial[0] != 0) ||
        (initial[(uint32_t)frames_per_half * 2U - 1U] != 0))
    {
        return AUDIO_ERROR;
    }
    return g_start_result;
}

int audio_realtime_stop(void)
{
    ++g_stop_count;
    return g_stop_result;
}

static void reset_fixture(void)
{
    g_fill = NULL;
    g_fill_context = NULL;
    g_sample_rate = 0U;
    g_frames_per_half = 0U;
    g_start_result = AUDIO_OK;
    g_stop_result = AUDIO_OK;
    g_process_count = 0U;
    g_silent_process_count = 0U;
    g_stop_count = 0U;
    clocks = 12345U;
    nes_audio_init();
}

static int expect(const char *name, int condition)
{
    if (!condition)
    {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

int main(void)
{
    int16_t stereo[EXPECTED_FRAMES_PER_HALF * 2U];
    nes_audio_status_t status;
    uint32_t index;
    int failures = 0;

    reset_fixture();
    failures += expect("inactive ready", nes_audio_ready_for_frame() != 0U);
    failures += expect("inactive drains apu",
        nes_audio_submit_frame() == NES_AUDIO_OK &&
        g_silent_process_count == 1U && clocks == 0U);

    failures += expect("start", nes_audio_start() == NES_AUDIO_OK);
    failures += expect("realtime format",
        g_fill != NULL && g_sample_rate == NES_AUDIO_SAMPLE_RATE &&
        g_frames_per_half == EXPECTED_FRAMES_PER_HALF &&
        NES_AUDIO_QUEUE_BLOCKS == 4U);
    failures += expect("start idempotent", nes_audio_start() == NES_AUDIO_OK);

    for (index = 0U; index < NES_AUDIO_QUEUE_BLOCKS; ++index)
    {
        clocks = 20000U + index;
        failures += expect("queue frame",
            nes_audio_ready_for_frame() != 0U &&
            nes_audio_submit_frame() == NES_AUDIO_OK && clocks == 0U);
    }
    failures += expect("queue backpressure",
        nes_audio_ready_for_frame() == 0U &&
        nes_audio_submit_frame() == NES_AUDIO_BUSY);

    memset(stereo, 0, sizeof(stereo));
    g_fill(g_fill_context, stereo, EXPECTED_FRAMES_PER_HALF);
    failures += expect("mono expanded left", stereo[0] == 2000);
    failures += expect("mono expanded right", stereo[1] == 2000);
    failures += expect("one block complete",
        stereo[EXPECTED_FRAMES_PER_HALF * 2U - 1U] ==
            (int16_t)(2000U + NES_AUDIO_FRAMES_PER_BLOCK - 1U));
    failures += expect("space after consume", nes_audio_ready_for_frame() != 0U);

    nes_audio_set_paused(1U);
    memset(stereo, 0x5A, sizeof(stereo));
    g_fill(g_fill_context, stereo, EXPECTED_FRAMES_PER_HALF);
    failures += expect("pause silence",
        stereo[0] == 0 &&
        stereo[EXPECTED_FRAMES_PER_HALF * 2U - 1U] == 0);
    nes_audio_set_paused(0U);
    failures += expect("resume submit",
        nes_audio_submit_frame() == NES_AUDIO_OK);

    failures += expect("stop", nes_audio_stop() == NES_AUDIO_OK &&
                       g_stop_count == 1U);
    failures += expect("stop idempotent",
                       nes_audio_stop() == NES_AUDIO_OK && g_stop_count == 1U);
    nes_audio_get_status(&status);
    failures += expect("status",
        status.initialized != 0U && status.active == 0U &&
        status.generated_blocks == NES_AUDIO_QUEUE_BLOCKS + 1U &&
        status.consumed_blocks == EXPECTED_BLOCKS_PER_HALF);

    reset_fixture();
    g_start_result = AUDIO_BUSY;
    failures += expect("busy mapped",
        nes_audio_start() == NES_AUDIO_BUSY &&
        nes_audio_ready_for_frame() != 0U);

    if (failures == 0)
    {
        printf("PASS nes_audio_service\n");
    }
    return failures;
}
