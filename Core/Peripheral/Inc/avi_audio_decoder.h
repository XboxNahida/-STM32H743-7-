#ifndef AVI_AUDIO_DECODER_H
#define AVI_AUDIO_DECODER_H

#include <stdint.h>

#define AVI_AUDIO_DECODER_INPUT_CAPACITY 16384U

typedef enum
{
    AVI_AUDIO_DECODER_OK = 0,
    AVI_AUDIO_DECODER_NEED_DATA,
    AVI_AUDIO_DECODER_EOF,
    AVI_AUDIO_DECODER_INVALID_ARG,
    AVI_AUDIO_DECODER_UNSUPPORTED,
    AVI_AUDIO_DECODER_OVERFLOW,
    AVI_AUDIO_DECODER_FORMAT_ERROR,
    AVI_AUDIO_DECODER_ERROR
} avi_audio_decoder_result_t;

typedef enum
{
    AVI_AUDIO_DECODER_NONE = 0,
    AVI_AUDIO_DECODER_PCM = 0x0001,
    AVI_AUDIO_DECODER_MP3 = 0x0055
} avi_audio_decoder_format_t;

void avi_audio_decoder_init(void);
int avi_audio_decoder_open(avi_audio_decoder_format_t format,
                           uint32_t sample_rate,
                           uint8_t channels,
                           uint8_t bits_per_sample);
int avi_audio_decoder_push(const uint8_t *data, uint32_t size);
int avi_audio_decoder_decode_next(int16_t *pcm, uint32_t capacity,
                                  uint32_t *samples,
                                  uint32_t *sample_rate);
void avi_audio_decoder_finish(void);
void avi_audio_decoder_close(void);

#endif
