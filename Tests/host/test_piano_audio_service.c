#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "audio_playback_service.h"
#include "piano_audio_service.h"

static audio_realtime_fill_t g_fill;
static void *g_fill_context;
static uint32_t g_sample_rate;
static uint16_t g_frames_per_half;
static int g_start_result;
static uint32_t g_start_count;
static uint32_t g_stop_count;

int audio_realtime_start(audio_realtime_fill_t fill, void *context,
                         uint32_t sample_rate, uint16_t frames_per_half)
{
    g_start_count++;
    g_fill = fill;
    g_fill_context = context;
    g_sample_rate = sample_rate;
    g_frames_per_half = frames_per_half;
    return g_start_result;
}

int audio_realtime_stop(void)
{
    g_stop_count++;
    return AUDIO_OK;
}

static int expect_i32(const char *name, int32_t actual, int32_t expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=%ld expected=%ld\n", name,
               (long)actual, (long)expected);
        return 1;
    }
    return 0;
}

static int expect_true(const char *name, int condition)
{
    if (!condition)
    {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

static void reset_fixture(void)
{
    g_fill = NULL;
    g_fill_context = NULL;
    g_sample_rate = 0U;
    g_frames_per_half = 0U;
    g_start_result = AUDIO_OK;
    g_start_count = 0U;
    g_stop_count = 0U;
    piano_audio_init();
}

static int test_start_and_render_five_notes(void)
{
    piano_audio_status_t status;
    int16_t stereo[512];
    uint32_t frame;
    uint8_t note;
    int nonzero = 0;
    int failures = 0;

    reset_fixture();
    failures += expect_i32("piano_start", piano_audio_start(),
                           PIANO_AUDIO_OK);
    failures += expect_i32("audio_start_once", (int32_t)g_start_count, 1);
    failures += expect_i32("sample_rate", (int32_t)g_sample_rate, 48000);
    failures += expect_i32("frames_per_half", g_frames_per_half, 1024);
    failures += expect_true("fill_registered", g_fill != NULL);
    failures += expect_true("fill_context", g_fill_context != NULL);

    for (note = 0U; note < 5U; note++)
    {
        failures += expect_i32("five_note_on",
            piano_audio_note_on((uint8_t)(12U + note), 127U),
            PIANO_AUDIO_OK);
    }
    failures += expect_i32("sixth_note_busy",
        piano_audio_note_on(30U, 127U), PIANO_AUDIO_BUSY);
    memset(stereo, 0, sizeof(stereo));
    g_fill(g_fill_context, stereo, 256U);
    for (frame = 0U; frame < 256U; frame++)
    {
        failures += expect_i32("left_right_equal",
            stereo[frame * 2U], stereo[frame * 2U + 1U]);
        if (stereo[frame * 2U] != 0)
        {
            nonzero = 1;
        }
    }
    failures += expect_true("render_nonzero", nonzero);
    piano_audio_get_status(&status);
    failures += expect_i32("active", status.active, 1);
    failures += expect_i32("five_active_voices", status.active_voices, 5);
    failures += expect_i32("render_count", (int32_t)status.render_count, 1);
    return failures;
}

static int test_note_release_and_stop(void)
{
    piano_audio_status_t status;
    int failures = 0;

    reset_fixture();
    failures += expect_i32("release_start", piano_audio_start(),
                           PIANO_AUDIO_OK);
    failures += expect_i32("release_note_on",
        piano_audio_note_on(21U, 100U), PIANO_AUDIO_OK);
    failures += expect_i32("release_note_off",
        piano_audio_note_off(21U), PIANO_AUDIO_OK);
    piano_audio_all_notes_off();
    failures += expect_i32("piano_stop", piano_audio_stop(),
                           PIANO_AUDIO_OK);
    failures += expect_i32("audio_stop_once", (int32_t)g_stop_count, 1);
    piano_audio_get_status(&status);
    failures += expect_i32("inactive", status.active, 0);
    failures += expect_i32("voices_cleared", status.active_voices, 0);
    failures += expect_i32("note_when_stopped",
        piano_audio_note_on(21U, 100U), PIANO_AUDIO_NOT_READY);
    return failures;
}

static int test_note_mask_diff(void)
{
    piano_audio_status_t status;
    uint64_t first = (1ULL << 12) | (1ULL << 14) | (1ULL << 16) |
                     (1ULL << 17) | (1ULL << 19);
    uint64_t second = (1ULL << 14) | (1ULL << 17) | (1ULL << 21);
    int failures = 0;

    reset_fixture();
    failures += expect_i32("mask_start", piano_audio_start(),
                           PIANO_AUDIO_OK);
    failures += expect_i32("mask_five", piano_audio_apply_note_mask(first),
                           PIANO_AUDIO_OK);
    piano_audio_get_status(&status);
    failures += expect_i32("mask_five_held", status.held_notes, 5);
    failures += expect_i32("mask_replace",
        piano_audio_apply_note_mask(second), PIANO_AUDIO_OK);
    piano_audio_get_status(&status);
    failures += expect_i32("mask_three_held", status.held_notes, 3);
    failures += expect_i32("mask_too_many",
        piano_audio_apply_note_mask(first | (1ULL << 21)),
        PIANO_AUDIO_INVALID_ARG);
    failures += expect_i32("mask_out_of_range",
        piano_audio_apply_note_mask(1ULL << 48), PIANO_AUDIO_INVALID_ARG);
    piano_audio_get_status(&status);
    failures += expect_i32("invalid_mask_keeps_old", status.held_notes, 3);
    failures += expect_i32("mask_release_all",
        piano_audio_apply_note_mask(0U), PIANO_AUDIO_OK);
    piano_audio_get_status(&status);
    failures += expect_i32("mask_zero_held", status.held_notes, 0);
    (void)piano_audio_stop();
    return failures;
}

static int test_audio_start_failure(void)
{
    piano_audio_status_t status;
    int failures = 0;

    reset_fixture();
    g_start_result = AUDIO_BUSY;
    failures += expect_i32("busy_start", piano_audio_start(),
                           PIANO_AUDIO_BUSY);
    piano_audio_get_status(&status);
    failures += expect_i32("busy_not_active", status.active, 0);
    failures += expect_i32("busy_result", status.last_result,
                           PIANO_AUDIO_BUSY);
    return failures;
}

static int test_sustain_control(void)
{
    piano_audio_status_t status;
    int failures = 0;

    reset_fixture();
    piano_audio_get_status(&status);
    failures += expect_i32("sustain_status_default",
        status.sustain_enabled, 0);
    failures += expect_i32("sustain_before_start",
        piano_audio_set_sustain(1U), PIANO_AUDIO_NOT_READY);
    failures += expect_i32("sustain_start", piano_audio_start(),
                           PIANO_AUDIO_OK);
    failures += expect_i32("sustain_invalid",
        piano_audio_set_sustain(2U), PIANO_AUDIO_INVALID_ARG);
    failures += expect_i32("sustain_enable",
        piano_audio_set_sustain(1U), PIANO_AUDIO_OK);
    piano_audio_get_status(&status);
    failures += expect_i32("sustain_status_enabled",
        status.sustain_enabled, 1);
    failures += expect_i32("sustain_disable",
        piano_audio_set_sustain(0U), PIANO_AUDIO_OK);
    piano_audio_get_status(&status);
    failures += expect_i32("sustain_status_disabled",
        status.sustain_enabled, 0);
    (void)piano_audio_stop();
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_start_and_render_five_notes();
    failures += test_note_release_and_stop();
    failures += test_note_mask_diff();
    failures += test_audio_start_failure();
    failures += test_sustain_control();
    if (failures != 0)
    {
        printf("FAIL piano_audio_service failures=%d\n", failures);
        return 1;
    }
    printf("PASS piano_audio_service\n");
    return 0;
}
