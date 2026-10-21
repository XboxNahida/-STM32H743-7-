#include <stdint.h>
#include <stdio.h>

#include "piano_synth.h"

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

static void render_frames(piano_synth_t *synth, uint32_t frame_count)
{
    int16_t samples[256];

    while (frame_count >= 256U)
    {
        piano_synth_render(synth, samples, 256U);
        frame_count -= 256U;
    }
    if (frame_count != 0U)
    {
        piano_synth_render(synth, samples, frame_count);
    }
}

static int test_note_frequency_table(void)
{
    int failures = 0;

    failures += expect_i32("c3_frequency_millihz",
        (int32_t)piano_synth_note_frequency_millihz(0U), 130813);
    failures += expect_i32("a4_frequency_millihz",
        (int32_t)piano_synth_note_frequency_millihz(21U), 440000);
    failures += expect_i32("b6_frequency_millihz",
        (int32_t)piano_synth_note_frequency_millihz(47U), 1975533);
    failures += expect_i32("invalid_frequency",
        (int32_t)piano_synth_note_frequency_millihz(48U), 0);
    return failures;
}

static int test_silence_and_single_note(void)
{
    piano_synth_t synth;
    int16_t samples[512];
    uint32_t index;
    int nonzero = 0;
    int failures = 0;

    piano_synth_init(&synth);
    piano_synth_render(&synth, samples, 512U);
    for (index = 0U; index < 512U; index++)
    {
        failures += expect_i32("idle_silence", samples[index], 0);
    }

    failures += expect_i32("note_on_a4",
        piano_synth_note_on(&synth, 21U, 100U), PIANO_SYNTH_OK);
    piano_synth_render(&synth, samples, 512U);
    for (index = 0U; index < 512U; index++)
    {
        if (samples[index] != 0)
        {
            nonzero = 1;
        }
    }
    failures += expect_true("single_note_nonzero", nonzero);
    failures += expect_i32("single_voice_active",
        (int32_t)piano_synth_active_voice_count(&synth), 1);
    return failures;
}

static int test_five_voice_limit(void)
{
    piano_synth_t synth;
    int16_t samples[1024];
    uint8_t note;
    int failures = 0;

    piano_synth_init(&synth);
    for (note = 0U; note < PIANO_SYNTH_VOICE_COUNT; note++)
    {
        failures += expect_i32("five_note_on",
            piano_synth_note_on(&synth, (uint8_t)(12U + note), 127U),
            PIANO_SYNTH_OK);
    }
    failures += expect_i32("sixth_note_rejected",
        piano_synth_note_on(&synth, 30U, 127U), PIANO_SYNTH_FULL);
    failures += expect_i32("five_voices_active",
        (int32_t)piano_synth_active_voice_count(&synth),
        PIANO_SYNTH_VOICE_COUNT);
    piano_synth_render(&synth, samples, 1024U);
    failures += expect_i32("five_voices_still_active",
        (int32_t)piano_synth_active_voice_count(&synth),
        PIANO_SYNTH_VOICE_COUNT);
    return failures;
}

static int test_released_voice_can_be_reused(void)
{
    piano_synth_t synth;
    uint8_t note;
    int failures = 0;

    piano_synth_init(&synth);
    for (note = 0U; note < PIANO_SYNTH_VOICE_COUNT; note++)
    {
        failures += expect_i32("reuse_fill_voice",
            piano_synth_note_on(&synth, (uint8_t)(12U + note), 100U),
            PIANO_SYNTH_OK);
    }
    failures += expect_i32("reuse_release",
        piano_synth_note_off(&synth, 12U), PIANO_SYNTH_OK);
    failures += expect_i32("reuse_released_voice",
        piano_synth_note_on(&synth, 30U, 100U), PIANO_SYNTH_OK);
    failures += expect_i32("reuse_keeps_five",
        piano_synth_active_voice_count(&synth), PIANO_SYNTH_VOICE_COUNT);
    return failures;
}

static int test_piano_envelope_and_fm_profile(void)
{
    piano_synth_t synth;
    int16_t samples[256];
    uint32_t block;
    int failures = 0;

    piano_synth_init(&synth);
    failures += expect_i32("profile_note_on",
        piano_synth_note_on(&synth, 21U, 100U), PIANO_SYNTH_OK);
    failures += expect_i32("profile_fm_ratio",
        (int32_t)synth.voices[0].fm_increment,
        (int32_t)synth.voices[0].phase_increment);
    failures += expect_i32("profile_fm_depth",
        synth.voices[0].fm_depth, 32768);

    piano_synth_render(&synth, samples, 32U);
    failures += expect_true("profile_attack_not_instant",
        (synth.voices[0].envelope > 0U) &&
        (synth.voices[0].envelope <= 4096U));
    for (block = 0U; block < 562U; block++)
    {
        piano_synth_render(&synth, samples, 256U);
    }
    failures += expect_true("profile_held_note_decays",
        (synth.voices[0].envelope > 0U) &&
        (synth.voices[0].envelope < 50000U));

    failures += expect_i32("profile_note_off",
        piano_synth_note_off(&synth, 21U), PIANO_SYNTH_OK);
    for (block = 0U; block < 141U; block++)
    {
        piano_synth_render(&synth, samples, 256U);
    }
    failures += expect_i32("profile_release_finishes",
        piano_synth_active_voice_count(&synth), 0);
    return failures;
}

static int test_sustain_switch(void)
{
    piano_synth_t synth;
    int16_t samples[256];
    uint32_t block;
    int failures = 0;

    piano_synth_init(&synth);
    failures += expect_i32("sustain_default_off",
        synth.sustain_enabled, 0);
    failures += expect_i32("sustain_default_note_on",
        piano_synth_note_on(&synth, 21U, 100U), PIANO_SYNTH_OK);
    render_frames(&synth, 1600U);
    failures += expect_i32("sustain_default_note_off",
        piano_synth_note_off(&synth, 21U), PIANO_SYNTH_OK);
    failures += expect_i32("sustain_default_release_state",
        synth.voices[0].envelope_state, PIANO_SYNTH_ENV_RELEASE);

    piano_synth_init(&synth);
    piano_synth_set_sustain(&synth, 1U);
    failures += expect_i32("sustain_enabled",
        synth.sustain_enabled, 1);
    failures += expect_i32("sustain_note_on",
        piano_synth_note_on(&synth, 21U, 100U), PIANO_SYNTH_OK);
    render_frames(&synth, 1600U);
    failures += expect_i32("sustain_note_off",
        piano_synth_note_off(&synth, 21U), PIANO_SYNTH_OK);
    failures += expect_i32("sustain_release_state",
        synth.voices[0].envelope_state,
        PIANO_SYNTH_ENV_SUSTAIN_RELEASE);
    for (block = 0U; block < 141U; block++)
    {
        piano_synth_render(&synth, samples, 256U);
    }
    failures += expect_i32("sustain_tail_still_active",
        piano_synth_active_voice_count(&synth), 1);
    piano_synth_set_sustain(&synth, 0U);
    failures += expect_i32("sustain_tail_converted",
        synth.voices[0].envelope_state, PIANO_SYNTH_ENV_RELEASE);
    for (block = 0U; block < 141U; block++)
    {
        piano_synth_render(&synth, samples, 256U);
    }
    failures += expect_i32("sustain_tail_finishes",
        piano_synth_active_voice_count(&synth), 0);
    return failures;
}

static int test_sustain_voice_reuse(void)
{
    piano_synth_t synth;
    uint8_t note;
    int failures = 0;

    piano_synth_init(&synth);
    piano_synth_set_sustain(&synth, 1U);
    for (note = 0U; note < PIANO_SYNTH_VOICE_COUNT; note++)
    {
        failures += expect_i32("sustain_fill_voice",
            piano_synth_note_on(&synth, (uint8_t)(12U + note), 100U),
            PIANO_SYNTH_OK);
    }
    failures += expect_i32("sustain_held_not_stolen",
        piano_synth_note_on(&synth, 30U, 100U), PIANO_SYNTH_FULL);
    failures += expect_i32("sustain_release_for_reuse",
        piano_synth_note_off(&synth, 12U), PIANO_SYNTH_OK);
    failures += expect_i32("sustain_tail_reused",
        piano_synth_note_on(&synth, 30U, 100U), PIANO_SYNTH_OK);
    failures += expect_i32("sustain_reuse_keeps_five",
        piano_synth_active_voice_count(&synth), PIANO_SYNTH_VOICE_COUNT);
    return failures;
}

static int test_release_and_all_notes_off(void)
{
    piano_synth_t synth;
    int16_t samples[256];
    uint32_t block;
    int failures = 0;

    piano_synth_init(&synth);
    failures += expect_i32("release_note_on",
        piano_synth_note_on(&synth, 21U, 100U), PIANO_SYNTH_OK);
    piano_synth_render(&synth, samples, 256U);
    failures += expect_i32("note_off",
        piano_synth_note_off(&synth, 21U), PIANO_SYNTH_OK);
    for (block = 0U; block < 188U; block++)
    {
        piano_synth_render(&synth, samples, 256U);
    }
    failures += expect_i32("released_voice_inactive",
        (int32_t)piano_synth_active_voice_count(&synth), 0);

    failures += expect_i32("all_off_first_note",
        piano_synth_note_on(&synth, 12U, 100U), PIANO_SYNTH_OK);
    failures += expect_i32("all_off_second_note",
        piano_synth_note_on(&synth, 16U, 100U), PIANO_SYNTH_OK);
    piano_synth_all_notes_off(&synth);
    for (block = 0U; block < 188U; block++)
    {
        piano_synth_render(&synth, samples, 256U);
    }
    failures += expect_i32("all_notes_released",
        (int32_t)piano_synth_active_voice_count(&synth), 0);
    return failures;
}

static int test_stereo_render_duplicates_channels(void)
{
    piano_synth_t synth;
    int16_t samples[512];
    uint32_t frame;
    int nonzero = 0;
    int failures = 0;

    piano_synth_init(&synth);
    failures += expect_i32("stereo_note_on",
        piano_synth_note_on(&synth, 21U, 100U), PIANO_SYNTH_OK);
    piano_synth_render_stereo(&synth, samples, 256U);
    for (frame = 0U; frame < 256U; frame++)
    {
        failures += expect_i32("stereo_channels_equal",
            samples[frame * 2U], samples[frame * 2U + 1U]);
        if (samples[frame * 2U] != 0)
        {
            nonzero = 1;
        }
    }
    failures += expect_true("stereo_render_nonzero", nonzero);
    return failures;
}

static int test_invalid_arguments(void)
{
    piano_synth_t synth;
    int failures = 0;

    piano_synth_init(&synth);
    failures += expect_i32("invalid_note",
        piano_synth_note_on(&synth, 48U, 100U), PIANO_SYNTH_INVALID_ARG);
    failures += expect_i32("zero_velocity",
        piano_synth_note_on(&synth, 21U, 0U), PIANO_SYNTH_INVALID_ARG);
    failures += expect_i32("invalid_velocity",
        piano_synth_note_on(&synth, 21U, 128U), PIANO_SYNTH_INVALID_ARG);
    failures += expect_i32("missing_note_off",
        piano_synth_note_off(&synth, 21U), PIANO_SYNTH_NOT_FOUND);
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_note_frequency_table();
    failures += test_silence_and_single_note();
    failures += test_five_voice_limit();
    failures += test_released_voice_can_be_reused();
    failures += test_piano_envelope_and_fm_profile();
    failures += test_sustain_switch();
    failures += test_sustain_voice_reuse();
    failures += test_release_and_all_notes_off();
    failures += test_stereo_render_duplicates_channels();
    failures += test_invalid_arguments();
    if (failures != 0)
    {
        printf("FAIL piano_synth failures=%d\n", failures);
        return 1;
    }
    printf("PASS piano_synth\n");
    return 0;
}
