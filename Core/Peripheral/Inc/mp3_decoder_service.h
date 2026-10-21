#ifndef MP3_DECODER_SERVICE_H
#define MP3_DECODER_SERVICE_H

#include <stdint.h>

typedef enum
{
    MP3_DECODER_OK = 0,
    MP3_DECODER_INVALID_ARG,
    MP3_DECODER_NOT_READY,
    MP3_DECODER_IO_ERROR,
    MP3_DECODER_FORMAT_ERROR,
    MP3_DECODER_EOF,
    MP3_DECODER_ERROR
} mp3_decoder_result_t;

typedef struct
{
    uint8_t initialized;
    uint8_t opened;
    uint8_t suspended;
    uint8_t channels;
    uint32_t sample_rate;
    uint32_t bitrate;
    uint32_t duration_ms;
    uint32_t frame_count;
    uint32_t resume_offset;
    int32_t last_result;
} mp3_decoder_status_t;

void mp3_decoder_service_init(void);
int mp3_decoder_open(const char *path);
int mp3_decoder_decode_next(int16_t *pcm,
                            uint32_t capacity,
                            uint32_t *samples,
                            uint8_t *channels);
int mp3_decoder_suspend(void);
int mp3_decoder_resume(void);
int mp3_decoder_close(void);
void mp3_decoder_get_status(mp3_decoder_status_t *status);
void mp3_decoder_print_status(void);

#endif
