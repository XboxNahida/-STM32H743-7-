#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "avi.h"

static void put_u16(uint8_t *buffer, uint32_t offset, uint16_t value)
{
    buffer[offset] = (uint8_t)value;
    buffer[offset + 1U] = (uint8_t)(value >> 8);
}

static void put_u32(uint8_t *buffer, uint32_t offset, uint32_t value)
{
    buffer[offset] = (uint8_t)value;
    buffer[offset + 1U] = (uint8_t)(value >> 8);
    buffer[offset + 2U] = (uint8_t)(value >> 16);
    buffer[offset + 3U] = (uint8_t)(value >> 24);
}

static int expect_u32(const char *name, uint32_t actual, uint32_t expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=%lu expected=%lu\n", name,
               (unsigned long)actual, (unsigned long)expected);
        return 1;
    }
    return 0;
}

static void build_strf(uint8_t *buffer, uint32_t payload_size,
                       uint16_t format, uint16_t channels,
                       uint32_t sample_rate, uint32_t byte_rate,
                       uint16_t block_align, uint16_t bits_per_sample)
{
    memset(buffer, 0, 32U);
    memcpy(buffer, "strf", 4U);
    put_u32(buffer, 4U, payload_size);
    put_u16(buffer, 8U, format);
    put_u16(buffer, 10U, channels);
    put_u32(buffer, 12U, sample_rate);
    put_u32(buffer, 16U, byte_rate);
    put_u16(buffer, 20U, block_align);
    put_u16(buffer, 22U, bits_per_sample);
}

static int test_pcm_format(void)
{
    uint8_t chunk[32];
    AVI_INFO info;
    int failures = 0;

    memset(&info, 0, sizeof(info));
    build_strf(chunk, 18U, AVI_AUDIO_FORMAT_PCM, 2U, 11025U,
               44100U, 4U, 16U);
    failures += expect_u32("pcm_result",
                           avi_parse_audio_format(chunk, 26U, &info),
                           AVI_OK);
    failures += expect_u32("pcm_type", info.AudioType,
                           AVI_AUDIO_FORMAT_PCM);
    failures += expect_u32("pcm_channels", info.Channels, 2U);
    failures += expect_u32("pcm_sample_rate", info.SampleRate, 11025U);
    failures += expect_u32("pcm_byte_rate", info.AudioByteRate, 44100U);
    failures += expect_u32("pcm_block_align", info.AudioBlockAlign, 4U);
    failures += expect_u32("pcm_bits", info.BitsPerSample, 16U);
    return failures;
}

static int test_mp3_format(void)
{
    uint8_t chunk[40];
    AVI_INFO info;
    int failures = 0;

    memset(&info, 0, sizeof(info));
    build_strf(chunk, 30U, AVI_AUDIO_FORMAT_MP3, 2U, 44100U,
               16000U, 1152U, 0U);
    failures += expect_u32("mp3_result",
                           avi_parse_audio_format(chunk, 38U, &info),
                           AVI_OK);
    failures += expect_u32("mp3_type", info.AudioType,
                           AVI_AUDIO_FORMAT_MP3);
    failures += expect_u32("mp3_channels", info.Channels, 2U);
    failures += expect_u32("mp3_sample_rate", info.SampleRate, 44100U);
    failures += expect_u32("mp3_block_align", info.AudioBlockAlign, 1152U);
    return failures;
}

static int test_truncated_format(void)
{
    uint8_t chunk[32];
    AVI_INFO info;

    memset(&info, 0, sizeof(info));
    build_strf(chunk, 18U, AVI_AUDIO_FORMAT_PCM, 2U, 11025U,
               44100U, 4U, 16U);
    return expect_u32("truncated_result",
                      avi_parse_audio_format(chunk, 20U, &info),
                      AVI_STRF_ERR);
}

int main(void)
{
    int failures = 0;

    failures += test_pcm_format();
    failures += test_mp3_format();
    failures += test_truncated_format();
    if (failures != 0)
    {
        printf("FAIL avi_audio_format failures=%d\n", failures);
        return 1;
    }
    printf("PASS avi_audio_format\n");
    return 0;
}
