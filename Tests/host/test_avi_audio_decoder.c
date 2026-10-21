#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "avi_audio_decoder.h"

static uint32_t g_mp3_decode_calls;
static uint32_t g_mp3_last_input_size;

int avi_audio_test_mp3_open(void)
{
    return 0;
}

void avi_audio_test_mp3_close(void)
{
}

int avi_audio_test_mp3_decode(const uint8_t *input, uint32_t input_size,
                              int16_t *pcm, uint32_t capacity,
                              uint32_t *consumed, uint32_t *samples,
                              uint32_t *sample_rate, uint8_t *channels)
{
    g_mp3_decode_calls++;
    g_mp3_last_input_size = input_size;
    if ((input == NULL) || (input_size < 4U) || (capacity < 4U))
    {
        return AVI_AUDIO_DECODER_NEED_DATA;
    }
    pcm[0] = 101;
    pcm[1] = -101;
    pcm[2] = 202;
    pcm[3] = -202;
    *consumed = 4U;
    *samples = 4U;
    *sample_rate = 44100U;
    *channels = 2U;
    return AVI_AUDIO_DECODER_OK;
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

static int expect_u32(const char *name, uint32_t actual, uint32_t expected)
{
    return expect_i32(name, (int32_t)actual, (int32_t)expected);
}

static int test_pcm_stereo_cross_chunk(void)
{
    static const uint8_t first[] = {0x01U, 0x00U, 0x02U};
    static const uint8_t second[] = {0x00U, 0xFFU, 0xFFU, 0x00U, 0x80U};
    int16_t pcm[8];
    uint32_t samples = 0U;
    uint32_t rate = 0U;
    int failures = 0;

    avi_audio_decoder_init();
    failures += expect_i32("pcm_open",
        avi_audio_decoder_open(AVI_AUDIO_DECODER_PCM, 11025U, 2U, 16U),
        AVI_AUDIO_DECODER_OK);
    failures += expect_i32("pcm_push_first",
        avi_audio_decoder_push(first, sizeof(first)),
        AVI_AUDIO_DECODER_OK);
    failures += expect_i32("pcm_wait_partial",
        avi_audio_decoder_decode_next(pcm, 8U, &samples, &rate),
        AVI_AUDIO_DECODER_NEED_DATA);
    failures += expect_i32("pcm_push_second",
        avi_audio_decoder_push(second, sizeof(second)),
        AVI_AUDIO_DECODER_OK);
    failures += expect_i32("pcm_decode",
        avi_audio_decoder_decode_next(pcm, 8U, &samples, &rate),
        AVI_AUDIO_DECODER_OK);
    failures += expect_u32("pcm_samples", samples, 4U);
    failures += expect_u32("pcm_rate", rate, 11025U);
    failures += expect_i32("pcm_l0", pcm[0], 1);
    failures += expect_i32("pcm_r0", pcm[1], 2);
    failures += expect_i32("pcm_l1", pcm[2], -1);
    failures += expect_i32("pcm_r1", pcm[3], -32768);
    return failures;
}

static int test_pcm_mono_duplicates_channels(void)
{
    static const uint8_t mono[] = {0x34U, 0x12U, 0xCCU, 0xEDU};
    int16_t pcm[8];
    uint32_t samples = 0U;
    uint32_t rate = 0U;
    int failures = 0;

    avi_audio_decoder_init();
    failures += expect_i32("mono_open",
        avi_audio_decoder_open(AVI_AUDIO_DECODER_PCM, 22050U, 1U, 16U),
        AVI_AUDIO_DECODER_OK);
    failures += expect_i32("mono_push",
        avi_audio_decoder_push(mono, sizeof(mono)),
        AVI_AUDIO_DECODER_OK);
    failures += expect_i32("mono_decode",
        avi_audio_decoder_decode_next(pcm, 8U, &samples, &rate),
        AVI_AUDIO_DECODER_OK);
    failures += expect_u32("mono_samples", samples, 4U);
    failures += expect_i32("mono_left_0", pcm[0], 0x1234);
    failures += expect_i32("mono_right_0", pcm[1], 0x1234);
    failures += expect_i32("mono_left_1", pcm[2], -0x1234);
    failures += expect_i32("mono_right_1", pcm[3], -0x1234);
    return failures;
}

static int test_mp3_waits_for_buffer(void)
{
    uint8_t input[2048];
    int16_t pcm[8];
    uint32_t samples = 0U;
    uint32_t rate = 0U;
    int failures = 0;

    memset(input, 0x55, sizeof(input));
    g_mp3_decode_calls = 0U;
    g_mp3_last_input_size = 0U;
    avi_audio_decoder_init();
    failures += expect_i32("mp3_open",
        avi_audio_decoder_open(AVI_AUDIO_DECODER_MP3, 44100U, 2U, 0U),
        AVI_AUDIO_DECODER_OK);
    failures += expect_i32("mp3_push_half",
        avi_audio_decoder_push(input, 1024U), AVI_AUDIO_DECODER_OK);
    failures += expect_i32("mp3_need_more",
        avi_audio_decoder_decode_next(pcm, 8U, &samples, &rate),
        AVI_AUDIO_DECODER_NEED_DATA);
    failures += expect_u32("mp3_not_called_early", g_mp3_decode_calls, 0U);
    failures += expect_i32("mp3_push_full",
        avi_audio_decoder_push(input + 1024U, 1024U),
        AVI_AUDIO_DECODER_OK);
    failures += expect_i32("mp3_decode",
        avi_audio_decoder_decode_next(pcm, 8U, &samples, &rate),
        AVI_AUDIO_DECODER_OK);
    failures += expect_u32("mp3_called", g_mp3_decode_calls, 1U);
    failures += expect_u32("mp3_input_size", g_mp3_last_input_size, 2048U);
    failures += expect_u32("mp3_samples", samples, 4U);
    failures += expect_u32("mp3_rate", rate, 44100U);
    failures += expect_i32("mp3_pcm", pcm[2], 202);
    return failures;
}

static int test_finish_and_validation(void)
{
    uint8_t input[AVI_AUDIO_DECODER_INPUT_CAPACITY + 1U];
    int16_t pcm[4];
    uint32_t samples = 0U;
    uint32_t rate = 0U;
    int failures = 0;

    memset(input, 0, sizeof(input));
    avi_audio_decoder_init();
    failures += expect_i32("reject_pcm_8bit",
        avi_audio_decoder_open(AVI_AUDIO_DECODER_PCM, 11025U, 2U, 8U),
        AVI_AUDIO_DECODER_UNSUPPORTED);
    failures += expect_i32("finish_open",
        avi_audio_decoder_open(AVI_AUDIO_DECODER_PCM, 11025U, 2U, 16U),
        AVI_AUDIO_DECODER_OK);
    failures += expect_i32("overflow",
        avi_audio_decoder_push(input, sizeof(input)),
        AVI_AUDIO_DECODER_OVERFLOW);
    avi_audio_decoder_finish();
    failures += expect_i32("finished_eof",
        avi_audio_decoder_decode_next(pcm, 4U, &samples, &rate),
        AVI_AUDIO_DECODER_EOF);
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_pcm_stereo_cross_chunk();
    failures += test_pcm_mono_duplicates_channels();
    failures += test_mp3_waits_for_buffer();
    failures += test_finish_and_validation();
    if (failures != 0)
    {
        printf("FAIL avi_audio_decoder failures=%d\n", failures);
        return 1;
    }
    printf("PASS avi_audio_decoder\n");
    return 0;
}
