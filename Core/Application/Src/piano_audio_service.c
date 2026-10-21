#include "piano_audio_service.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>

#include "audio_playback_service.h"
#include "piano_synth.h"

#define PIANO_AUDIO_FRAMES_PER_HALF  1024U

static piano_synth_t g_piano_synth;
static piano_audio_status_t g_piano_status;
static uint64_t g_piano_held_mask;

static uint8_t piano_audio_popcount(uint64_t value)
{
    uint8_t count = 0U;

    while (value != 0U)
    {
        count = (uint8_t)(count + (uint8_t)(value & 1U));
        value >>= 1U;
    }
    return count;
}

static int piano_audio_map_audio_result(int result)
{
    if (result == AUDIO_OK)
    {
        return PIANO_AUDIO_OK;
    }
    if (result == AUDIO_BUSY)
    {
        return PIANO_AUDIO_BUSY;
    }
    if (result == AUDIO_INVALID_ARG)
    {
        return PIANO_AUDIO_INVALID_ARG;
    }
    return PIANO_AUDIO_ERROR;
}

static void piano_audio_fill(void *context, int16_t *stereo,
                             uint32_t frame_count)
{
    piano_synth_t *synth = (piano_synth_t *)context;

    piano_synth_render_stereo(synth, stereo, frame_count);
    if (g_piano_status.render_count != UINT32_MAX)
    {
        g_piano_status.render_count++;
    }
}

void piano_audio_init(void)
{
    piano_synth_init(&g_piano_synth);
    memset(&g_piano_status, 0, sizeof(g_piano_status));
    g_piano_status.initialized = 1U;
    g_piano_status.last_result = PIANO_AUDIO_NOT_READY;
    g_piano_held_mask = 0U;
}

int piano_audio_start(void)
{
    int result;

    if (g_piano_status.active != 0U)
    {
        return PIANO_AUDIO_OK;
    }
    piano_synth_init(&g_piano_synth);
    g_piano_held_mask = 0U;
    g_piano_status.render_count = 0U;
    result = audio_realtime_start(piano_audio_fill, &g_piano_synth,
                                  PIANO_SYNTH_SAMPLE_RATE,
                                  PIANO_AUDIO_FRAMES_PER_HALF);
    result = piano_audio_map_audio_result(result);
    if (result != PIANO_AUDIO_OK)
    {
        g_piano_status.active = 0U;
        g_piano_status.active_voices = 0U;
        g_piano_status.last_result = result;
        return result;
    }
    g_piano_status.active = 1U;
    g_piano_status.active_voices = 0U;
    g_piano_status.held_notes = 0U;
    g_piano_status.sustain_enabled = 0U;
    g_piano_status.last_result = PIANO_AUDIO_OK;
    return PIANO_AUDIO_OK;
}

int piano_audio_stop(void)
{
    int result;

    if (g_piano_status.active == 0U)
    {
        return PIANO_AUDIO_OK;
    }
    result = piano_audio_map_audio_result(audio_realtime_stop());
    piano_synth_init(&g_piano_synth);
    g_piano_held_mask = 0U;
    g_piano_status.active = 0U;
    g_piano_status.active_voices = 0U;
    g_piano_status.held_notes = 0U;
    g_piano_status.sustain_enabled = 0U;
    g_piano_status.last_result = result;
    return result;
}

int piano_audio_note_on(uint8_t note, uint8_t velocity)
{
    int result;

    if (g_piano_status.active == 0U)
    {
        return PIANO_AUDIO_NOT_READY;
    }
    result = piano_synth_note_on(&g_piano_synth, note, velocity);
    if (result == PIANO_SYNTH_OK)
    {
        result = PIANO_AUDIO_OK;
        g_piano_held_mask |= 1ULL << note;
    }
    else if (result == PIANO_SYNTH_FULL)
    {
        result = PIANO_AUDIO_BUSY;
    }
    else
    {
        result = PIANO_AUDIO_INVALID_ARG;
    }
    g_piano_status.active_voices =
        piano_synth_active_voice_count(&g_piano_synth);
    g_piano_status.last_result = result;
    return result;
}

int piano_audio_note_off(uint8_t note)
{
    int result;

    if (g_piano_status.active == 0U)
    {
        return PIANO_AUDIO_NOT_READY;
    }
    result = piano_synth_note_off(&g_piano_synth, note);
    if (result == PIANO_SYNTH_OK)
    {
        result = PIANO_AUDIO_OK;
        g_piano_held_mask &= ~(1ULL << note);
    }
    else if (result == PIANO_SYNTH_NOT_FOUND)
    {
        result = PIANO_AUDIO_NOT_FOUND;
    }
    else
    {
        result = PIANO_AUDIO_INVALID_ARG;
    }
    g_piano_status.last_result = result;
    return result;
}

int piano_audio_set_sustain(uint8_t enabled)
{
    if (enabled > 1U)
    {
        return PIANO_AUDIO_INVALID_ARG;
    }
    if (g_piano_status.active == 0U)
    {
        return PIANO_AUDIO_NOT_READY;
    }
    piano_synth_set_sustain(&g_piano_synth, enabled);
    g_piano_status.sustain_enabled = enabled;
    g_piano_status.last_result = PIANO_AUDIO_OK;
    return PIANO_AUDIO_OK;
}

int piano_audio_apply_note_mask(uint64_t note_mask)
{
    uint64_t released;
    uint64_t pressed;
    uint8_t note;

    if (g_piano_status.active == 0U)
    {
        return PIANO_AUDIO_NOT_READY;
    }
    if (((note_mask >> 48U) != 0U) ||
        (piano_audio_popcount(note_mask) > PIANO_SYNTH_VOICE_COUNT))
    {
        return PIANO_AUDIO_INVALID_ARG;
    }

    released = g_piano_held_mask & ~note_mask;
    pressed = note_mask & ~g_piano_held_mask;
    for (note = 0U; note < 48U; note++)
    {
        if ((released & (1ULL << note)) != 0U)
        {
            (void)piano_synth_note_off(&g_piano_synth, note);
        }
    }
    for (note = 0U; note < 48U; note++)
    {
        if ((pressed & (1ULL << note)) != 0U)
        {
            if (piano_synth_note_on(&g_piano_synth, note, 112U) !=
                PIANO_SYNTH_OK)
            {
                g_piano_status.last_result = PIANO_AUDIO_BUSY;
                return PIANO_AUDIO_BUSY;
            }
        }
    }
    g_piano_held_mask = note_mask;
    g_piano_status.held_notes = piano_audio_popcount(note_mask);
    g_piano_status.active_voices =
        piano_synth_active_voice_count(&g_piano_synth);
    g_piano_status.last_result = PIANO_AUDIO_OK;
    return PIANO_AUDIO_OK;
}

void piano_audio_all_notes_off(void)
{
    if (g_piano_status.active != 0U)
    {
        piano_synth_all_notes_off(&g_piano_synth);
        g_piano_held_mask = 0U;
        g_piano_status.held_notes = 0U;
    }
}

void piano_audio_get_status(piano_audio_status_t *status)
{
    if (status != NULL)
    {
        g_piano_status.active_voices =
            piano_synth_active_voice_count(&g_piano_synth);
        *status = g_piano_status;
    }
}
