#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "audio_recording_service.h"
#include "es8388_service.h"

static uint8_t g_file[65536];
static uint32_t g_file_pos;
static uint32_t g_file_size;
static uint32_t g_write_count;

int audio_record_test_acquire(void) { return 0; }
void audio_record_test_release(void) {}
int audio_record_test_start_clock(int16_t *buffer, uint32_t samples)
{
    (void)buffer;
    (void)samples;
    return 0;
}
int audio_record_test_start_rx(int16_t *buffer, uint32_t samples)
{
    (void)buffer;
    (void)samples;
    return 0;
}
int audio_record_test_stop_rx(void) { return 0; }
int audio_record_test_stop_clock(void) { return 0; }
int es8388_probe_readonly(uint32_t timeout_ms)
{
    return (timeout_ms != 0U) ? ES8388_OK : ES8388_INVALID_ARG;
}
int es8388_prepare_recording(void) { return ES8388_OK; }

int audio_record_test_open_file(const char *path)
{
    if ((path == NULL) || (path[0] == '\0'))
    {
        return 1;
    }
    memset(g_file, 0, sizeof(g_file));
    g_file_pos = 0U;
    g_file_size = 0U;
    g_write_count = 0U;
    return 0;
}

int audio_record_test_write_file(const void *data, uint32_t size)
{
    if ((data == NULL) || (size > (sizeof(g_file) - g_file_pos)))
    {
        return 1;
    }
    memcpy(&g_file[g_file_pos], data, size);
    g_file_pos += size;
    if (g_file_pos > g_file_size)
    {
        g_file_size = g_file_pos;
    }
    g_write_count++;
    return 0;
}

int audio_record_test_seek_file(uint32_t position)
{
    if (position > sizeof(g_file))
    {
        return 1;
    }
    g_file_pos = position;
    return 0;
}

int audio_record_test_sync_file(void) { return 0; }
int audio_record_test_close_file(void) { return 0; }
int audio_record_test_remove_file(void) { return 0; }

static uint32_t read_u32(const uint8_t *data)
{
    return ((uint32_t)data[0]) |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

static uint16_t read_u16(const uint8_t *data)
{
    return (uint16_t)(((uint16_t)data[0]) | ((uint16_t)data[1] << 8));
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

static int expect_bytes(const char *name, const uint8_t *actual,
                        const char *expected, uint32_t length)
{
    if (memcmp(actual, expected, length) != 0)
    {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

int main(void)
{
    int failures = 0;

    audio_recording_service_init();
    failures += expect_u32("start", audio_record_start("0:/REC001.WAV"),
                           AUDIO_RECORD_OK);
    failures += expect_u32("header_write", g_write_count > 0U, 1U);
    failures += expect_bytes("riff", g_file, "RIFF", 4U);
    failures += expect_bytes("wave", &g_file[8], "WAVE", 4U);
    failures += expect_bytes("fmt", &g_file[12], "fmt ", 4U);
    failures += expect_u32("audio_format", read_u16(&g_file[20]), 1U);
    failures += expect_u32("channels", read_u16(&g_file[22]), 2U);
    failures += expect_u32("sample_rate", read_u32(&g_file[24]), 48000U);
    failures += expect_u32("byte_rate", read_u32(&g_file[28]), 192000U);
    failures += expect_u32("block_align", read_u16(&g_file[32]), 4U);
    failures += expect_u32("bits", read_u16(&g_file[34]), 16U);
    failures += expect_bytes("data", &g_file[36], "data", 4U);

    audio_record_on_rx_half_complete();
    audio_record_poll();
    failures += expect_u32("pcm_written", g_file_size > 44U, 1U);
    failures += expect_u32("stop", audio_record_stop(), AUDIO_RECORD_OK);
    failures += expect_u32("data_size", read_u32(&g_file[40]), 8192U);
    failures += expect_u32("riff_size", read_u32(&g_file[4]), 8228U);
    if (failures != 0)
    {
        printf("FAIL wav_recording_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS wav_recording_contract\n");
    return 0;
}
