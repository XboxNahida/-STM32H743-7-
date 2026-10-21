#include "piano_synth.h"

#include <stddef.h>
#include <string.h>

#define PIANO_WAVE_BITS             6U
#define PIANO_WAVE_SIZE             (1U << PIANO_WAVE_BITS)
#define PIANO_CONTROL_INTERVAL      32U
#define PIANO_ATTACK_STEP           4096U
#define PIANO_DECAY_STEP            8U
#define PIANO_RELEASE_STEP          64U
#define PIANO_INITIAL_FM_DEPTH      32768U
#define PIANO_FINAL_FM_DEPTH        16384U
#define PIANO_FM_DECAY              64U

static const int16_t g_sine_wave[PIANO_WAVE_SIZE] = {
     1608,  4808,  7962, 11039, 14010, 16846, 19519, 22005,
    24279, 26319, 28105, 29621, 30852, 31785, 32412, 32728,
    32728, 32412, 31785, 30852, 29621, 28105, 26319, 24279,
    22005, 19519, 16846, 14010, 11039,  7962,  4808,  1608,
    -1608, -4808, -7962,-11039,-14010,-16846,-19519,-22005,
   -24279,-26319,-28105,-29621,-30852,-31785,-32412,-32728,
   -32728,-32412,-31785,-30852,-29621,-28105,-26319,-24279,
   -22005,-19519,-16846,-14010,-11039, -7962, -4808, -1608
};

static const uint32_t g_note_frequency_millihz[PIANO_SYNTH_NOTE_COUNT] = {
     130813U,  138591U,  146832U,  155563U,  164814U,  174614U,
     184997U,  195998U,  207652U,  220000U,  233082U,  246942U,
     261626U,  277183U,  293665U,  311127U,  329628U,  349228U,
     369994U,  391995U,  415305U,  440000U,  466164U,  493883U,
     523251U,  554365U,  587330U,  622254U,  659255U,  698456U,
     739989U,  783991U,  830609U,  880000U,  932328U,  987767U,
    1046502U, 1108731U, 1174659U, 1244508U, 1318510U, 1396913U,
    1479978U, 1567982U, 1661219U, 1760000U, 1864655U, 1975533U
};

static const uint32_t g_phase_increment[PIANO_SYNTH_NOTE_COUNT] = {
     11704930U,  12400941U,  13138339U,  13919586U,  14747287U,
     15624207U,  16553270U,  17537579U,  18580418U,  19685267U,
     20855814U,  22095965U,  23409859U,  24801882U,  26276679U,
     27839171U,  29494575U,  31248413U,  33106541U,  35075158U,
     37160835U,  39370534U,  41711627U,  44191930U,  46819719U,
     49603764U,  52553357U,  55678342U,  58989149U,  62496826U,
     66213081U,  70150316U,  74321671U,  78741067U,  83423255U,
     88383859U,  93639437U,  99207528U, 105106715U, 111356685U,
    117978298U, 124993653U, 132426162U, 140300631U, 148643341U,
    157482134U, 166846509U, 176767719U
};

static void piano_voice_control_step(piano_synth_voice_t *voice)
{
    uint32_t reduction;

    if (voice->envelope_state == PIANO_SYNTH_ENV_ATTACK)
    {
        if (voice->envelope >= (65535U - PIANO_ATTACK_STEP))
        {
            voice->envelope = 65535U;
            voice->envelope_state = PIANO_SYNTH_ENV_DECAY;
        }
        else
        {
            voice->envelope += PIANO_ATTACK_STEP;
        }
    }
    else if ((voice->envelope_state == PIANO_SYNTH_ENV_DECAY) ||
             (voice->envelope_state == PIANO_SYNTH_ENV_SUSTAIN_RELEASE))
    {
        if (voice->envelope <= PIANO_DECAY_STEP)
        {
            memset(voice, 0, sizeof(*voice));
            return;
        }
        voice->envelope -= PIANO_DECAY_STEP;
    }
    else if (voice->envelope_state == PIANO_SYNTH_ENV_RELEASE)
    {
        if (voice->envelope <= PIANO_RELEASE_STEP)
        {
            memset(voice, 0, sizeof(*voice));
            return;
        }
        voice->envelope -= PIANO_RELEASE_STEP;
    }

    if (voice->fm_exponent != 0U)
    {
        reduction = ((uint32_t)voice->fm_exponent * PIANO_FM_DECAY) >> 16;
        if (reduction == 0U)
        {
            reduction = 1U;
        }
        voice->fm_exponent = (voice->fm_exponent <= reduction) ? 0U :
            (uint16_t)(voice->fm_exponent - reduction);
        voice->fm_depth = (uint16_t)(PIANO_FINAL_FM_DEPTH +
            ((((uint32_t)PIANO_INITIAL_FM_DEPTH - PIANO_FINAL_FM_DEPTH) *
              voice->fm_exponent) >> 16));
    }
}

void piano_synth_init(piano_synth_t *synth)
{
    if (synth != NULL)
    {
        memset(synth, 0, sizeof(*synth));
    }
}

int piano_synth_note_on(piano_synth_t *synth, uint8_t note,
                        uint8_t velocity)
{
    piano_synth_voice_t *voice = NULL;
    piano_synth_voice_t *released_voice = NULL;
    uint8_t index;

    if ((synth == NULL) || (note >= PIANO_SYNTH_NOTE_COUNT) ||
        (velocity == 0U) || (velocity > 127U))
    {
        return PIANO_SYNTH_INVALID_ARG;
    }
    for (index = 0U; index < PIANO_SYNTH_VOICE_COUNT; index++)
    {
        if ((synth->voices[index].envelope_state != PIANO_SYNTH_ENV_IDLE) &&
            (synth->voices[index].note == note))
        {
            voice = &synth->voices[index];
            break;
        }
        if ((voice == NULL) &&
            (synth->voices[index].envelope_state == PIANO_SYNTH_ENV_IDLE))
        {
            voice = &synth->voices[index];
        }
        else if ((released_voice == NULL) &&
                 ((synth->voices[index].envelope_state ==
                   PIANO_SYNTH_ENV_RELEASE) ||
                  (synth->voices[index].envelope_state ==
                   PIANO_SYNTH_ENV_SUSTAIN_RELEASE)))
        {
            released_voice = &synth->voices[index];
        }
    }
    if (voice == NULL)
    {
        voice = released_voice;
    }
    if (voice == NULL)
    {
        return PIANO_SYNTH_FULL;
    }

    memset(voice, 0, sizeof(*voice));
    voice->phase_increment = g_phase_increment[note];
    voice->fm_increment = g_phase_increment[note];
    voice->gain = (uint16_t)((uint16_t)velocity * 258U);
    voice->fm_depth = PIANO_INITIAL_FM_DEPTH;
    voice->fm_exponent = 65535U;
    voice->note = note;
    voice->envelope_state = PIANO_SYNTH_ENV_ATTACK;
    return PIANO_SYNTH_OK;
}

int piano_synth_note_off(piano_synth_t *synth, uint8_t note)
{
    uint8_t index;

    if ((synth == NULL) || (note >= PIANO_SYNTH_NOTE_COUNT))
    {
        return PIANO_SYNTH_INVALID_ARG;
    }
    for (index = 0U; index < PIANO_SYNTH_VOICE_COUNT; index++)
    {
        if ((synth->voices[index].envelope_state != PIANO_SYNTH_ENV_IDLE) &&
            (synth->voices[index].note == note))
        {
            synth->voices[index].envelope_state =
                (synth->sustain_enabled != 0U) ?
                PIANO_SYNTH_ENV_SUSTAIN_RELEASE : PIANO_SYNTH_ENV_RELEASE;
            return PIANO_SYNTH_OK;
        }
    }
    return PIANO_SYNTH_NOT_FOUND;
}

void piano_synth_set_sustain(piano_synth_t *synth, uint8_t enabled)
{
    uint8_t index;

    if (synth == NULL)
    {
        return;
    }
    synth->sustain_enabled = (enabled != 0U) ? 1U : 0U;
    if (synth->sustain_enabled == 0U)
    {
        for (index = 0U; index < PIANO_SYNTH_VOICE_COUNT; index++)
        {
            if (synth->voices[index].envelope_state ==
                PIANO_SYNTH_ENV_SUSTAIN_RELEASE)
            {
                synth->voices[index].envelope_state =
                    PIANO_SYNTH_ENV_RELEASE;
            }
        }
    }
}

void piano_synth_all_notes_off(piano_synth_t *synth)
{
    uint8_t index;

    if (synth == NULL)
    {
        return;
    }
    for (index = 0U; index < PIANO_SYNTH_VOICE_COUNT; index++)
    {
        if (synth->voices[index].envelope_state != PIANO_SYNTH_ENV_IDLE)
        {
            synth->voices[index].envelope_state = PIANO_SYNTH_ENV_RELEASE;
        }
    }
}

static int16_t piano_synth_render_sample(piano_synth_t *synth)
{
    int32_t mix = 0;
    uint8_t voice_index;

    if (synth->control_countdown == 0U)
    {
        for (voice_index = 0U; voice_index < PIANO_SYNTH_VOICE_COUNT;
             voice_index++)
        {
            if (synth->voices[voice_index].envelope_state !=
                PIANO_SYNTH_ENV_IDLE)
            {
                piano_voice_control_step(&synth->voices[voice_index]);
            }
        }
        synth->control_countdown = PIANO_CONTROL_INTERVAL - 1U;
    }
    else
    {
        synth->control_countdown--;
    }

    for (voice_index = 0U; voice_index < PIANO_SYNTH_VOICE_COUNT;
         voice_index++)
    {
        piano_synth_voice_t *voice = &synth->voices[voice_index];
        int32_t mod_sample;
        int32_t phase_offset;
        int32_t voice_sample;
        uint32_t wave_index;

        if (voice->envelope_state == PIANO_SYNTH_ENV_IDLE)
        {
            continue;
        }
        voice->fm_phase += voice->fm_increment;
        voice->phase += voice->phase_increment;
        mod_sample = g_sine_wave[voice->fm_phase >>
            (32U - PIANO_WAVE_BITS)];
        phase_offset = mod_sample * (int32_t)voice->fm_depth;
        wave_index = (voice->phase + (uint32_t)phase_offset) >>
            (32U - PIANO_WAVE_BITS);
        voice_sample = g_sine_wave[wave_index];
        voice_sample = (voice_sample * (int32_t)voice->gain) >> 15;
        voice_sample = (voice_sample * (int32_t)voice->envelope) >> 16;
        mix += voice_sample;
    }
    return (int16_t)(mix / (int32_t)PIANO_SYNTH_VOICE_COUNT);
}

void piano_synth_render(piano_synth_t *synth, int16_t *samples,
                        uint32_t sample_count)
{
    uint32_t sample_index;

    if ((synth == NULL) || (samples == NULL))
    {
        return;
    }
    for (sample_index = 0U; sample_index < sample_count; sample_index++)
    {
        samples[sample_index] = piano_synth_render_sample(synth);
    }
}

void piano_synth_render_stereo(piano_synth_t *synth, int16_t *samples,
                               uint32_t frame_count)
{
    uint32_t frame;

    if ((synth == NULL) || (samples == NULL))
    {
        return;
    }
    for (frame = 0U; frame < frame_count; frame++)
    {
        int16_t sample = piano_synth_render_sample(synth);

        samples[frame * 2U] = sample;
        samples[frame * 2U + 1U] = sample;
    }
}

uint8_t piano_synth_active_voice_count(const piano_synth_t *synth)
{
    uint8_t index;
    uint8_t count = 0U;

    if (synth == NULL)
    {
        return 0U;
    }
    for (index = 0U; index < PIANO_SYNTH_VOICE_COUNT; index++)
    {
        if (synth->voices[index].envelope_state != PIANO_SYNTH_ENV_IDLE)
        {
            count++;
        }
    }
    return count;
}

uint32_t piano_synth_note_frequency_millihz(uint8_t note)
{
    return (note < PIANO_SYNTH_NOTE_COUNT) ?
        g_note_frequency_millihz[note] : 0U;
}
