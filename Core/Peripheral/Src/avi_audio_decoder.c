#include "avi_audio_decoder.h"

#include <stddef.h>
#include <string.h>

#define AVI_AUDIO_MP3_DECODE_THRESHOLD 2048U
#define AVI_AUDIO_MP3_MAIN_DATA_UNDERFLOW (-2)
#define AVI_AUDIO_MP3_WARMUP_LIMIT 16U

#if defined(AVI_AUDIO_DECODER_HOST_TEST)
extern int avi_audio_test_mp3_open(void);
extern void avi_audio_test_mp3_close(void);
extern int avi_audio_test_mp3_decode(const uint8_t *input,
                                     uint32_t input_size,
                                     int16_t *pcm,
                                     uint32_t capacity,
                                     uint32_t *consumed,
                                     uint32_t *samples,
                                     uint32_t *sample_rate,
                                     uint8_t *channels);
#else
#include "mp3dec.h"
static HMP3Decoder g_avi_mp3_decoder;
#endif

static uint8_t g_avi_audio_input[AVI_AUDIO_DECODER_INPUT_CAPACITY]
    __attribute__((section(".audio_dma"), aligned(32)));
static uint32_t g_avi_audio_input_size;
static uint32_t g_avi_audio_sample_rate;
static avi_audio_decoder_format_t g_avi_audio_format;
static uint8_t g_avi_audio_channels;
static uint8_t g_avi_audio_open;
static uint8_t g_avi_audio_finished;

static int16_t avi_audio_read_s16_le(const uint8_t *data)
{
    return (int16_t)((uint16_t)data[0] |
                     ((uint16_t)data[1] << 8));
}

static void avi_audio_consume(uint32_t size)
{
    if (size >= g_avi_audio_input_size)
    {
        g_avi_audio_input_size = 0U;
        return;
    }
    memmove(g_avi_audio_input, g_avi_audio_input + size,
            g_avi_audio_input_size - size);
    g_avi_audio_input_size -= size;
}

static int avi_audio_decode_pcm(int16_t *pcm, uint32_t capacity,
                                uint32_t *samples,
                                uint32_t *sample_rate)
{
    uint32_t frame_bytes = (uint32_t)g_avi_audio_channels * 2U;
    uint32_t frame_count = g_avi_audio_input_size / frame_bytes;
    uint32_t capacity_frames = capacity / 2U;
    uint32_t index;

    if (frame_count > capacity_frames)
    {
        frame_count = capacity_frames;
    }
    if (frame_count == 0U)
    {
        if (g_avi_audio_finished == 0U)
        {
            return AVI_AUDIO_DECODER_NEED_DATA;
        }
        return (g_avi_audio_input_size == 0U) ?
               AVI_AUDIO_DECODER_EOF :
               AVI_AUDIO_DECODER_FORMAT_ERROR;
    }
    for (index = 0U; index < frame_count; index++)
    {
        const uint8_t *frame = g_avi_audio_input + index * frame_bytes;
        int16_t left = avi_audio_read_s16_le(frame);
        int16_t right = (g_avi_audio_channels == 2U) ?
                        avi_audio_read_s16_le(frame + 2U) : left;

        pcm[index * 2U] = left;
        pcm[index * 2U + 1U] = right;
    }
    avi_audio_consume(frame_count * frame_bytes);
    *samples = frame_count * 2U;
    *sample_rate = g_avi_audio_sample_rate;
    return AVI_AUDIO_DECODER_OK;
}

#if !defined(AVI_AUDIO_DECODER_HOST_TEST)
static int avi_audio_decode_mp3_frame(const uint8_t *input,
                                      uint32_t input_size,
                                      int16_t *pcm,
                                      uint32_t capacity,
                                      uint32_t *consumed,
                                      uint32_t *samples,
                                      uint32_t *sample_rate,
                                      uint8_t *channels)
{
    MP3FrameInfo frame_info;
    uint8_t *read_ptr;
    int bytes_left;
    int sync_offset;
    int error;
    uint32_t warmup_count = 0U;

    if (capacity < 2304U)
    {
        return AVI_AUDIO_DECODER_INVALID_ARG;
    }
    read_ptr = (uint8_t *)input;
    bytes_left = (int)input_size;
    for (;;)
    {
        sync_offset = MP3FindSyncWord(read_ptr, bytes_left);
        if (sync_offset < 0)
        {
            return AVI_AUDIO_DECODER_NEED_DATA;
        }
        read_ptr += sync_offset;
        bytes_left -= sync_offset;
        error = MP3Decode(g_avi_mp3_decoder, &read_ptr, &bytes_left,
                          pcm, 0);
        if (error == 0)
        {
            break;
        }
        if ((error == AVI_AUDIO_MP3_MAIN_DATA_UNDERFLOW) &&
            (warmup_count < AVI_AUDIO_MP3_WARMUP_LIMIT))
        {
            warmup_count++;
            continue;
        }
        if (error == ERR_MP3_INDATA_UNDERFLOW)
        {
            return AVI_AUDIO_DECODER_NEED_DATA;
        }
        return AVI_AUDIO_DECODER_FORMAT_ERROR;
    }
    MP3GetLastFrameInfo(g_avi_mp3_decoder, &frame_info);
    if ((frame_info.samprate <= 0) ||
        ((frame_info.nChans != 1) && (frame_info.nChans != 2)) ||
        (frame_info.outputSamps <= 0))
    {
        return AVI_AUDIO_DECODER_FORMAT_ERROR;
    }
    if (frame_info.nChans == 1)
    {
        uint32_t mono_samples = (uint32_t)frame_info.outputSamps;
        uint32_t index;

        if ((mono_samples * 2U) > capacity)
        {
            return AVI_AUDIO_DECODER_INVALID_ARG;
        }
        for (index = mono_samples; index > 0U; index--)
        {
            int16_t value = pcm[index - 1U];
            pcm[(index - 1U) * 2U] = value;
            pcm[(index - 1U) * 2U + 1U] = value;
        }
        *samples = mono_samples * 2U;
        *channels = 2U;
    }
    else
    {
        *samples = (uint32_t)frame_info.outputSamps;
        *channels = 2U;
    }
    *sample_rate = (uint32_t)frame_info.samprate;
    *consumed = input_size - (uint32_t)bytes_left;
    return AVI_AUDIO_DECODER_OK;
}
#endif

static int avi_audio_decode_mp3(int16_t *pcm, uint32_t capacity,
                                uint32_t *samples,
                                uint32_t *sample_rate)
{
    uint32_t consumed = 0U;
    uint8_t channels = 0U;
    int result;

    if ((g_avi_audio_input_size < AVI_AUDIO_MP3_DECODE_THRESHOLD) &&
        (g_avi_audio_finished == 0U))
    {
        return AVI_AUDIO_DECODER_NEED_DATA;
    }
    if (g_avi_audio_input_size == 0U)
    {
        return (g_avi_audio_finished != 0U) ?
               AVI_AUDIO_DECODER_EOF :
               AVI_AUDIO_DECODER_NEED_DATA;
    }
#if defined(AVI_AUDIO_DECODER_HOST_TEST)
    result = avi_audio_test_mp3_decode(g_avi_audio_input,
                                       g_avi_audio_input_size,
                                       pcm, capacity, &consumed,
                                       samples, sample_rate, &channels);
#else
    result = avi_audio_decode_mp3_frame(g_avi_audio_input,
                                        g_avi_audio_input_size,
                                        pcm, capacity, &consumed,
                                        samples, sample_rate, &channels);
#endif
    if (result == AVI_AUDIO_DECODER_NEED_DATA)
    {
        return (g_avi_audio_finished != 0U) ?
               AVI_AUDIO_DECODER_EOF : result;
    }
    if ((result != AVI_AUDIO_DECODER_OK) ||
        (consumed == 0U) || (consumed > g_avi_audio_input_size) ||
        (channels != 2U))
    {
        return (result == AVI_AUDIO_DECODER_OK) ?
               AVI_AUDIO_DECODER_FORMAT_ERROR : result;
    }
    avi_audio_consume(consumed);
    return AVI_AUDIO_DECODER_OK;
}

void avi_audio_decoder_init(void)
{
    avi_audio_decoder_close();
    g_avi_audio_input_size = 0U;
    g_avi_audio_sample_rate = 0U;
    g_avi_audio_format = AVI_AUDIO_DECODER_NONE;
    g_avi_audio_channels = 0U;
    g_avi_audio_finished = 0U;
}

int avi_audio_decoder_open(avi_audio_decoder_format_t format,
                           uint32_t sample_rate,
                           uint8_t channels,
                           uint8_t bits_per_sample)
{
    if ((sample_rate == 0U) || ((channels != 1U) && (channels != 2U)))
    {
        return AVI_AUDIO_DECODER_INVALID_ARG;
    }
    if ((format != AVI_AUDIO_DECODER_PCM) &&
        (format != AVI_AUDIO_DECODER_MP3))
    {
        return AVI_AUDIO_DECODER_UNSUPPORTED;
    }
    if ((format == AVI_AUDIO_DECODER_PCM) && (bits_per_sample != 16U))
    {
        return AVI_AUDIO_DECODER_UNSUPPORTED;
    }
    avi_audio_decoder_close();
    if (format == AVI_AUDIO_DECODER_MP3)
    {
#if defined(AVI_AUDIO_DECODER_HOST_TEST)
        if (avi_audio_test_mp3_open() != 0)
#else
        g_avi_mp3_decoder = MP3InitDecoder();
        if (g_avi_mp3_decoder == NULL)
#endif
        {
            return AVI_AUDIO_DECODER_ERROR;
        }
    }
    g_avi_audio_input_size = 0U;
    g_avi_audio_sample_rate = sample_rate;
    g_avi_audio_format = format;
    g_avi_audio_channels = channels;
    g_avi_audio_finished = 0U;
    g_avi_audio_open = 1U;
    return AVI_AUDIO_DECODER_OK;
}

int avi_audio_decoder_push(const uint8_t *data, uint32_t size)
{
    if ((data == NULL) || (size == 0U))
    {
        return AVI_AUDIO_DECODER_INVALID_ARG;
    }
    if ((g_avi_audio_open == 0U) || (g_avi_audio_finished != 0U))
    {
        return AVI_AUDIO_DECODER_INVALID_ARG;
    }
    if (size > (AVI_AUDIO_DECODER_INPUT_CAPACITY -
                g_avi_audio_input_size))
    {
        return AVI_AUDIO_DECODER_OVERFLOW;
    }
    memcpy(g_avi_audio_input + g_avi_audio_input_size, data, size);
    g_avi_audio_input_size += size;
    return AVI_AUDIO_DECODER_OK;
}

int avi_audio_decoder_decode_next(int16_t *pcm, uint32_t capacity,
                                  uint32_t *samples,
                                  uint32_t *sample_rate)
{
    if ((pcm == NULL) || (capacity < 2U) ||
        (samples == NULL) || (sample_rate == NULL))
    {
        return AVI_AUDIO_DECODER_INVALID_ARG;
    }
    if (g_avi_audio_open == 0U)
    {
        return AVI_AUDIO_DECODER_INVALID_ARG;
    }
    *samples = 0U;
    *sample_rate = g_avi_audio_sample_rate;
    if (g_avi_audio_format == AVI_AUDIO_DECODER_PCM)
    {
        return avi_audio_decode_pcm(pcm, capacity, samples, sample_rate);
    }
    return avi_audio_decode_mp3(pcm, capacity, samples, sample_rate);
}

void avi_audio_decoder_finish(void)
{
    if (g_avi_audio_open != 0U)
    {
        g_avi_audio_finished = 1U;
    }
}

void avi_audio_decoder_close(void)
{
    if ((g_avi_audio_open != 0U) &&
        (g_avi_audio_format == AVI_AUDIO_DECODER_MP3))
    {
#if defined(AVI_AUDIO_DECODER_HOST_TEST)
        avi_audio_test_mp3_close();
#else
        if (g_avi_mp3_decoder != NULL)
        {
            MP3FreeDecoder(g_avi_mp3_decoder);
            g_avi_mp3_decoder = NULL;
        }
#endif
    }
    g_avi_audio_open = 0U;
    g_avi_audio_finished = 0U;
    g_avi_audio_input_size = 0U;
    g_avi_audio_format = AVI_AUDIO_DECODER_NONE;
}
