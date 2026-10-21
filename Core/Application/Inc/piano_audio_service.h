#ifndef PIANO_AUDIO_SERVICE_H
#define PIANO_AUDIO_SERVICE_H

#include <stdint.h>

typedef enum
{
    PIANO_AUDIO_OK = 0,
    PIANO_AUDIO_INVALID_ARG,
    PIANO_AUDIO_BUSY,
    PIANO_AUDIO_NOT_READY,
    PIANO_AUDIO_NOT_FOUND,
    PIANO_AUDIO_ERROR
} piano_audio_result_t;

typedef struct
{
    uint8_t initialized;
    uint8_t active;
    uint8_t active_voices;
    uint8_t held_notes;
    uint8_t sustain_enabled;
    uint32_t render_count;
    int32_t last_result;
} piano_audio_status_t;

void piano_audio_init(void);
int piano_audio_start(void);
int piano_audio_stop(void);
int piano_audio_note_on(uint8_t note, uint8_t velocity);
int piano_audio_note_off(uint8_t note);
int piano_audio_set_sustain(uint8_t enabled);
int piano_audio_apply_note_mask(uint64_t note_mask);
void piano_audio_all_notes_off(void);
void piano_audio_get_status(piano_audio_status_t *status);

#endif
