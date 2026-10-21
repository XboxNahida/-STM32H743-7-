#ifndef PIANO_SYNTH_H
#define PIANO_SYNTH_H

#include <stdint.h>

#define PIANO_SYNTH_SAMPLE_RATE  48000U
#define PIANO_SYNTH_NOTE_COUNT   48U
#define PIANO_SYNTH_VOICE_COUNT  5U

typedef enum
{
    PIANO_SYNTH_OK = 0,
    PIANO_SYNTH_INVALID_ARG,
    PIANO_SYNTH_FULL,
    PIANO_SYNTH_NOT_FOUND
} piano_synth_result_t;

typedef enum
{
    PIANO_SYNTH_ENV_IDLE = 0,
    PIANO_SYNTH_ENV_ATTACK,
    PIANO_SYNTH_ENV_DECAY,
    PIANO_SYNTH_ENV_SUSTAIN_RELEASE,
    PIANO_SYNTH_ENV_RELEASE
} piano_synth_envelope_t;

typedef struct
{
    uint32_t phase;
    uint32_t phase_increment;
    uint32_t fm_phase;
    uint32_t fm_increment;
    uint32_t envelope;
    uint16_t gain;
    uint16_t fm_depth;
    uint16_t fm_exponent;
    uint8_t note;
    uint8_t envelope_state;
} piano_synth_voice_t;

typedef struct
{
    piano_synth_voice_t voices[PIANO_SYNTH_VOICE_COUNT];
    uint8_t control_countdown;
    uint8_t sustain_enabled;
} piano_synth_t;

void piano_synth_init(piano_synth_t *synth);
int piano_synth_note_on(piano_synth_t *synth, uint8_t note,
                        uint8_t velocity);
int piano_synth_note_off(piano_synth_t *synth, uint8_t note);
void piano_synth_set_sustain(piano_synth_t *synth, uint8_t enabled);
void piano_synth_all_notes_off(piano_synth_t *synth);
void piano_synth_render(piano_synth_t *synth, int16_t *samples,
                        uint32_t sample_count);
void piano_synth_render_stereo(piano_synth_t *synth, int16_t *samples,
                               uint32_t frame_count);
uint8_t piano_synth_active_voice_count(const piano_synth_t *synth);
uint32_t piano_synth_note_frequency_millihz(uint8_t note);

#endif
