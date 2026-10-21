#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "audio_recording_service.h"
#include "es8388_service.h"

static int g_acquire_result;
static uint32_t g_acquire_count;
static uint32_t g_release_count;
static uint32_t g_tx_start_count;
static uint32_t g_rx_start_count;
static uint32_t g_rx_stop_count;
static uint32_t g_clock_stop_count;
static uint32_t g_codec_probe_count;
static uint32_t g_codec_prepare_count;

int audio_record_test_acquire(void)
{
    g_acquire_count++;
    return g_acquire_result;
}

void audio_record_test_release(void)
{
    g_release_count++;
}

int audio_record_test_start_clock(int16_t *buffer, uint32_t samples)
{
    if ((buffer == NULL) || (samples == 0U))
    {
        return 1;
    }
    g_tx_start_count++;
    return 0;
}

int audio_record_test_start_rx(int16_t *buffer, uint32_t samples)
{
    if ((buffer == NULL) || (samples == 0U))
    {
        return 1;
    }
    g_rx_start_count++;
    return 0;
}

int audio_record_test_stop_rx(void)
{
    g_rx_stop_count++;
    return 0;
}

int audio_record_test_stop_clock(void)
{
    g_clock_stop_count++;
    return 0;
}

int es8388_probe_readonly(uint32_t timeout_ms)
{
    g_codec_probe_count++;
    return (timeout_ms != 0U) ? ES8388_OK : ES8388_INVALID_ARG;
}

int es8388_prepare_recording(void)
{
    g_codec_prepare_count++;
    return ES8388_OK;
}

int audio_record_test_open_file(const char *path)
{
    return ((path != NULL) && (path[0] != '\0')) ? 0 : 1;
}

int audio_record_test_close_file(void)
{
    return 0;
}

int audio_record_test_write_file(const void *data, uint32_t size)
{
    return ((data != NULL) && (size != 0U)) ? 0 : 1;
}

int audio_record_test_seek_file(uint32_t position)
{
    (void)position;
    return 0;
}

int audio_record_test_sync_file(void)
{
    return 0;
}

int audio_record_test_remove_file(void)
{
    return 0;
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

static void reset_fixture(void)
{
    g_acquire_result = 0;
    g_acquire_count = 0U;
    g_release_count = 0U;
    g_tx_start_count = 0U;
    g_rx_start_count = 0U;
    g_rx_stop_count = 0U;
    g_clock_stop_count = 0U;
    g_codec_probe_count = 0U;
    g_codec_prepare_count = 0U;
    audio_recording_service_init();
}

static int test_lifecycle(void)
{
    audio_recording_status_t status;
    int failures = 0;

    reset_fixture();
    audio_record_get_status(&status);
    failures += expect_u32("initialized", status.initialized, 1U);
    failures += expect_u32("idle", status.state, AUDIO_RECORD_STATE_IDLE);
    failures += expect_u32("channels", status.channels, 2U);
    failures += expect_u32("sample_rate", status.sample_rate, 48000U);
    failures += expect_i32("empty_path", audio_record_start(""),
                           AUDIO_RECORD_INVALID_ARG);
    failures += expect_i32("start", audio_record_start("0:/REC001.WAV"),
                           AUDIO_RECORD_OK);
    audio_record_get_status(&status);
    failures += expect_u32("recording", status.state,
                           AUDIO_RECORD_STATE_RECORDING);
    failures += expect_u32("clock_started", g_tx_start_count, 1U);
    failures += expect_u32("rx_started", g_rx_start_count, 1U);
    failures += expect_u32("codec_probed", g_codec_probe_count, 1U);
    failures += expect_u32("codec_prepared", g_codec_prepare_count, 1U);
    failures += expect_i32("busy_start", audio_record_start("0:/REC002.WAV"),
                           AUDIO_RECORD_BUSY);
    failures += expect_i32("stop", audio_record_stop(), AUDIO_RECORD_OK);
    failures += expect_u32("released", g_release_count, 1U);
    failures += expect_u32("rx_stopped", g_rx_stop_count, 1U);
    failures += expect_u32("clock_stopped", g_clock_stop_count, 1U);
    failures += expect_i32("stop_idempotent", audio_record_stop(),
                           AUDIO_RECORD_OK);
    return failures;
}

static int test_resource_busy(void)
{
    int failures = 0;

    reset_fixture();
    g_acquire_result = 1;
    failures += expect_i32("resource_busy",
                           audio_record_start("0:/REC001.WAV"),
                           AUDIO_RECORD_BUSY);
    failures += expect_u32("no_clock_on_busy", g_tx_start_count, 0U);
    failures += expect_u32("no_rx_on_busy", g_rx_start_count, 0U);
    return failures;
}

static int test_event_consumption(void)
{
    audio_recording_status_t status;
    int failures = 0;

    reset_fixture();
    failures += expect_i32("start_events", audio_record_start("0:/REC001.WAV"),
                           AUDIO_RECORD_OK);
    audio_record_on_rx_half_complete();
    audio_record_on_rx_half_complete();
    audio_record_on_rx_complete();
    audio_record_poll();
    audio_record_get_status(&status);
    failures += expect_u32("events_consumed", status.pending_events, 0U);
    failures += expect_u32("bytes_nonzero", status.bytes_written > 0U, 1U);
    failures += expect_u32("duplicate_event_overrun", status.overrun_count,
                           1U);
    (void)audio_record_stop();
    return failures;
}

static int test_sixty_minute_limit(void)
{
    audio_recording_status_t status;
    uint32_t index;
    int failures = 0;

    reset_fixture();
    failures += expect_i32("start_limit", audio_record_start("0:/REC001.WAV"),
                           AUDIO_RECORD_OK);
    for (index = 0U; index < 42188U; index++)
    {
        audio_record_on_rx_half_complete();
        audio_record_poll();
    }
    audio_record_get_status(&status);
    failures += expect_u32("halfway_recording", status.state,
                           AUDIO_RECORD_STATE_RECORDING);
    for (index = 0U; index < 42188U; index++)
    {
        audio_record_on_rx_half_complete();
        audio_record_poll();
    }
    audio_record_get_status(&status);
    failures += expect_u32("limit_idle", status.state,
                           AUDIO_RECORD_STATE_IDLE);
    failures += expect_i32("limit_result", status.last_result,
                           AUDIO_RECORD_LIMIT);
    failures += expect_u32("limit_bytes", status.bytes_written,
                           691200000UL);
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_lifecycle();
    failures += test_resource_busy();
    failures += test_event_consumption();
    failures += test_sixty_minute_limit();
    if (failures != 0)
    {
        printf("FAIL audio_recording_service failures=%d\n", failures);
        return 1;
    }
    printf("PASS audio_recording_service\n");
    return 0;
}
