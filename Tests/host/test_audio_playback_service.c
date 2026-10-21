#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "audio_playback_service.h"
#include "avi_audio_decoder.h"
#include "es8388_service.h"
#include "mp3_decoder_service.h"

static int g_acquire_result;
static uint32_t g_acquire_count;
static uint32_t g_release_count;
static uint32_t g_start_count;
static uint32_t g_stop_count;
static uint32_t g_pause_count;
static uint32_t g_resume_count;
static uint32_t g_configure_rate_count;
static uint32_t g_configured_rate;
static uint32_t g_dma_samples;
static uint32_t g_decode_calls;
static uint32_t g_decoder_suspend_count;
static uint32_t g_decoder_resume_count;
static uint32_t g_decoder_close_count;
static uint32_t g_frames_remaining;
static int16_t g_second_frame_sample;
static uint8_t g_codec_volume;
static uint8_t g_codec_ready;
static uint32_t g_avi_decoder_open_count;
static uint32_t g_avi_decoder_push_count;
static uint32_t g_avi_decoder_close_count;
static uint32_t g_avi_decoder_decode_count;
static uint32_t g_realtime_fill_count;
static int16_t g_dma_first_sample;
static int16_t g_dma_second_half_first_sample;

int audio_test_acquire(void)
{
    g_acquire_count++;
    return g_acquire_result;
}

void audio_test_release(void)
{
    g_release_count++;
}

int audio_test_start_dma(int16_t *buffer, uint32_t samples)
{
    if ((buffer != NULL) && (samples >= 2U))
    {
        g_dma_first_sample = buffer[0];
        g_dma_second_half_first_sample = buffer[samples / 2U];
    }
    if ((buffer != NULL) && (samples > 2304U))
    {
        g_second_frame_sample = buffer[2304];
    }
    g_dma_samples = samples;
    g_start_count++;
    return 0;
}

static void realtime_fill(void *context, int16_t *stereo,
                          uint32_t frame_count)
{
    uint32_t frame;
    int16_t value;

    (void)context;
    g_realtime_fill_count++;
    value = (int16_t)g_realtime_fill_count;
    for (frame = 0U; frame < frame_count; frame++)
    {
        stereo[frame * 2U] = value;
        stereo[frame * 2U + 1U] = value;
    }
}

int audio_test_configure_sample_rate(uint32_t sample_rate)
{
    g_configure_rate_count++;
    g_configured_rate = sample_rate;
    return 0;
}

int audio_test_stop_dma(void)
{
    g_stop_count++;
    return 0;
}

int audio_test_pause_dma(void)
{
    g_pause_count++;
    return 0;
}

int audio_test_resume_dma(void)
{
    g_resume_count++;
    return 0;
}

int es8388_probe_readonly(uint32_t timeout_ms)
{
    (void)timeout_ms;
    g_codec_ready = 1U;
    return ES8388_OK;
}

int es8388_set_volume_percent(uint8_t percent)
{
    if (g_codec_ready == 0U)
    {
        return ES8388_NOT_READY;
    }
    g_codec_volume = percent;
    return ES8388_OK;
}

int es8388_prepare_playback(void)
{
    return (g_codec_ready != 0U) ? ES8388_OK : ES8388_NOT_READY;
}

void es8388_service_init(void) {}
void es8388_get_status(es8388_status_t *status) { (void)status; }
void es8388_print_status(void) {}

void mp3_decoder_service_init(void) {}

int mp3_decoder_open(const char *path)
{
    return (path != NULL) ? MP3_DECODER_OK : MP3_DECODER_INVALID_ARG;
}

int mp3_decoder_decode_next(int16_t *pcm, uint32_t capacity,
                            uint32_t *samples, uint8_t *channels)
{
    if ((pcm == NULL) || (samples == NULL) || (channels == NULL) ||
        (capacity < 2304U))
    {
        return MP3_DECODER_INVALID_ARG;
    }
    g_decode_calls++;
    if (g_frames_remaining == 0U)
    {
        *samples = 0U;
        *channels = 2U;
        return MP3_DECODER_EOF;
    }
    g_frames_remaining--;
    {
        uint32_t index;
        for (index = 0U; index < 2304U; index++)
        {
            pcm[index] = (int16_t)g_decode_calls;
        }
    }
    *samples = 2304U;
    *channels = 2U;
    return MP3_DECODER_OK;
}

int mp3_decoder_close(void)
{
    g_decoder_close_count++;
    return MP3_DECODER_OK;
}

int mp3_decoder_suspend(void)
{
    g_decoder_suspend_count++;
    return MP3_DECODER_OK;
}

int mp3_decoder_resume(void)
{
    g_decoder_resume_count++;
    return MP3_DECODER_OK;
}

void mp3_decoder_get_status(mp3_decoder_status_t *status)
{
    if (status != NULL)
    {
        memset(status, 0, sizeof(*status));
        status->sample_rate = 48000U;
        status->bitrate = 128000U;
        status->duration_ms = 120000U;
    }
}
void mp3_decoder_print_status(void) {}

void avi_audio_decoder_init(void) {}

int avi_audio_decoder_open(avi_audio_decoder_format_t format,
                           uint32_t sample_rate,
                           uint8_t channels,
                           uint8_t bits_per_sample)
{
    if ((format == AVI_AUDIO_DECODER_NONE) || (sample_rate == 0U) ||
        (channels == 0U) ||
        ((format == AVI_AUDIO_DECODER_PCM) && (bits_per_sample != 16U)))
    {
        return AVI_AUDIO_DECODER_UNSUPPORTED;
    }
    g_avi_decoder_open_count++;
    return AVI_AUDIO_DECODER_OK;
}

int avi_audio_decoder_push(const uint8_t *data, uint32_t size)
{
    if ((data == NULL) || (size == 0U))
    {
        return AVI_AUDIO_DECODER_INVALID_ARG;
    }
    g_avi_decoder_push_count++;
    return AVI_AUDIO_DECODER_OK;
}

int avi_audio_decoder_decode_next(int16_t *pcm, uint32_t capacity,
                                  uint32_t *samples,
                                  uint32_t *sample_rate)
{
    if ((pcm == NULL) || (capacity < 4U) ||
        (samples == NULL) || (sample_rate == NULL))
    {
        return AVI_AUDIO_DECODER_INVALID_ARG;
    }
    g_avi_decoder_decode_count++;
    if (g_avi_decoder_decode_count == 1U)
    {
        pcm[0] = 11;
        pcm[1] = 12;
        pcm[2] = 21;
        pcm[3] = 22;
        *samples = 4U;
        *sample_rate = 11025U;
        return AVI_AUDIO_DECODER_OK;
    }
    *samples = 0U;
    *sample_rate = 11025U;
    return AVI_AUDIO_DECODER_NEED_DATA;
}

void avi_audio_decoder_finish(void) {}

void avi_audio_decoder_close(void)
{
    g_avi_decoder_close_count++;
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
    g_start_count = 0U;
    g_stop_count = 0U;
    g_pause_count = 0U;
    g_resume_count = 0U;
    g_configure_rate_count = 0U;
    g_configured_rate = 0U;
    g_dma_samples = 0U;
    g_decode_calls = 0U;
    g_decoder_suspend_count = 0U;
    g_decoder_resume_count = 0U;
    g_decoder_close_count = 0U;
    g_frames_remaining = 8U;
    g_second_frame_sample = 0;
    g_codec_volume = 0U;
    g_codec_ready = 0U;
    g_avi_decoder_open_count = 0U;
    g_avi_decoder_push_count = 0U;
    g_avi_decoder_close_count = 0U;
    g_avi_decoder_decode_count = 0U;
    g_realtime_fill_count = 0U;
    g_dma_first_sample = 0;
    g_dma_second_half_first_sample = 0;
    audio_service_init();
}

static int test_play_stop_and_eof(void)
{
    audio_status_t status;
    int failures = 0;

    reset_fixture();
    g_frames_remaining = 2U;
    failures += expect_i32("play", audio_play("0:/MUSIC/Test.mp3"),
                           AUDIO_OK);
    audio_get_status(&status);
    failures += expect_u32("state_playing", status.state,
                          AUDIO_STATE_PLAYING);
    failures += expect_u32("duration_ms", status.duration_ms, 120000U);
    failures += expect_u32("dma_started", g_start_count, 1U);
    failures += expect_i32("half_filled_with_second_frame",
                           g_second_frame_sample, 2);
    audio_on_tx_half_complete();
    audio_poll();
    audio_get_status(&status);
    failures += expect_u32("eof_first_half_still_playing", status.state,
                           AUDIO_STATE_PLAYING);
    failures += expect_u32("elapsed_first_half_ms", status.elapsed_ms, 48U);
    audio_on_tx_complete();
    audio_poll();
    audio_get_status(&status);
    failures += expect_u32("eof_drained_idle", status.state,
                           AUDIO_STATE_IDLE);
    failures += expect_u32("elapsed_full_buffer_ms", status.elapsed_ms, 96U);
    failures += expect_i32("stop", audio_stop(), AUDIO_OK);
    failures += expect_u32("released", g_release_count, 1U);
    return failures;
}

static int test_pause_and_resume(void)
{
    audio_status_t status;
    int failures = 0;

    reset_fixture();
    failures += expect_i32("pause_play", audio_play("0:/MUSIC/Test.mp3"),
                           AUDIO_OK);
    failures += expect_i32("pause", audio_pause(), AUDIO_OK);
    audio_get_status(&status);
    failures += expect_u32("state_paused", status.state,
                           AUDIO_STATE_PAUSED);
    failures += expect_u32("pause_uses_dma_pause", g_pause_count, 1U);
    failures += expect_u32("pause_keeps_decoder",
                           g_decoder_suspend_count, 0U);
    failures += expect_u32("pause_keeps_resource", g_release_count, 0U);
    audio_on_tx_half_complete();
    audio_poll();
    audio_get_status(&status);
    failures += expect_u32("paused_elapsed_unchanged", status.elapsed_ms, 0U);
    failures += expect_i32("resume", audio_resume(), AUDIO_OK);
    audio_get_status(&status);
    failures += expect_u32("state_resumed", status.state,
                           AUDIO_STATE_PLAYING);
    failures += expect_u32("resume_uses_dma_resume", g_resume_count, 1U);
    failures += expect_u32("resume_keeps_resource", g_acquire_count, 1U);
    failures += expect_u32("resume_keeps_decoder",
                           g_decoder_resume_count, 0U);
    failures += expect_u32("resume_does_not_restart_dma", g_start_count, 1U);
    failures += expect_i32("stop_resumed_track", audio_stop(), AUDIO_OK);
    failures += expect_u32("stop_closes_decoder", g_decoder_close_count, 1U);
    return failures;
}

static int test_camera_suspend_and_restore(void)
{
    audio_status_t status;
    int failures = 0;

    reset_fixture();
    failures += expect_i32("camera_play",
                           audio_play("0:/MUSIC/Test.mp3"), AUDIO_OK);
    failures += expect_i32("camera_suspend", audio_suspend_for_camera(),
                           AUDIO_OK);
    audio_get_status(&status);
    failures += expect_u32("camera_suspended_state", status.state,
                           AUDIO_STATE_SUSPENDED);
    failures += expect_u32("camera_suspend_stops_dma", g_stop_count, 1U);
    failures += expect_u32("camera_suspend_closes_decoder",
                           g_decoder_suspend_count, 1U);
    failures += expect_u32("camera_suspend_releases_resource",
                           g_release_count, 1U);
    failures += expect_i32("camera_restore", audio_restore_after_camera(),
                           AUDIO_OK);
    audio_get_status(&status);
    failures += expect_u32("camera_restore_playing", status.state,
                           AUDIO_STATE_PLAYING);
    failures += expect_u32("camera_restore_reacquires", g_acquire_count, 2U);
    failures += expect_u32("camera_restore_reopens_decoder",
                           g_decoder_resume_count, 1U);
    failures += expect_u32("camera_restore_restarts_dma", g_start_count, 2U);
    (void)audio_stop();
    return failures;
}

static int test_busy_and_volume(void)
{
    audio_status_t status;
    int failures = 0;

    reset_fixture();
    g_acquire_result = 1;
    failures += expect_i32("busy", audio_play("0:/MUSIC/Test.mp3"),
                           AUDIO_BUSY);
    failures += expect_u32("busy_no_dma", g_start_count, 0U);

    reset_fixture();
    failures += expect_i32("volume", audio_set_volume_percent(73U),
                           AUDIO_OK);
    audio_get_status(&status);
    failures += expect_u32("volume_status", status.volume_percent, 73U);
    failures += expect_u32("volume_cached_before_probe", g_codec_volume, 0U);
    failures += expect_i32("play_applies_volume",
                           audio_play("0:/MUSIC/Test.mp3"), AUDIO_OK);
    failures += expect_u32("volume_codec", g_codec_volume, 73U);
    (void)audio_stop();
    failures += expect_i32("volume_invalid",
                           audio_set_volume_percent(101U),
                           AUDIO_INVALID_ARG);
    return failures;
}

static int test_avi_stream_playback(void)
{
    static const uint8_t pcm_chunk_large[4096] = {0U};
    audio_status_t status;
    int failures = 0;

    reset_fixture();
    failures += expect_i32("stream_start",
        audio_stream_start(AUDIO_STREAM_FORMAT_PCM, 11025U, 2U, 16U),
        AUDIO_OK);
    audio_get_status(&status);
    failures += expect_u32("stream_state", status.state,
                           AUDIO_STATE_STREAMING);
    failures += expect_u32("stream_decoder_open",
                           g_avi_decoder_open_count, 1U);
    failures += expect_u32("stream_dma_waits_for_data", g_start_count, 0U);
    failures += expect_u32("stream_rate_configured",
                           g_configured_rate, 11025U);
    failures += expect_i32("stream_push_4096_a",
        audio_stream_push(pcm_chunk_large, sizeof(pcm_chunk_large)), AUDIO_OK);
    failures += expect_i32("stream_push_4096_b",
        audio_stream_push(pcm_chunk_large, sizeof(pcm_chunk_large)), AUDIO_OK);
    failures += expect_u32("stream_dma_waits_for_full_prebuffer",
                           g_start_count, 0U);
    failures += expect_i32("stream_push_4096_c",
        audio_stream_push(pcm_chunk_large, sizeof(pcm_chunk_large)), AUDIO_OK);
    failures += expect_u32("stream_dma_reserves_two_halves",
                           g_start_count, 0U);
    failures += expect_i32("stream_push_4096_d",
        audio_stream_push(pcm_chunk_large, sizeof(pcm_chunk_large)), AUDIO_OK);
    failures += expect_u32("stream_push_count",
                           g_avi_decoder_push_count, 4U);
    failures += expect_u32("stream_dma_started", g_start_count, 1U);
    failures += expect_u32("stream_dma_half_size", g_dma_samples, 4096U);
    audio_on_tx_half_complete();
    audio_poll();
    audio_get_status(&status);
    failures += expect_u32("stream_underflow_count",
                           status.underrun_count, 1U);
    failures += expect_i32("stream_stop", audio_stream_stop(), AUDIO_OK);
    failures += expect_u32("stream_dma_stopped", g_stop_count, 1U);
    failures += expect_u32("stream_decoder_closed",
                           g_avi_decoder_close_count, 1U);
    failures += expect_u32("stream_resource_released", g_release_count, 1U);
    failures += expect_u32("stream_rate_configured_once",
                           g_configure_rate_count, 1U);
    return failures;
}

static int test_realtime_pcm_producer(void)
{
    audio_status_t status;
    int failures = 0;

    reset_fixture();
    failures += expect_i32("realtime_start",
        audio_realtime_start(realtime_fill, NULL, 48000U, 256U), AUDIO_OK);
    audio_get_status(&status);
    failures += expect_u32("realtime_state", status.state,
                           AUDIO_STATE_REALTIME);
    failures += expect_u32("realtime_rate", status.sample_rate, 48000U);
    failures += expect_u32("realtime_prefills_both_halves",
                           g_realtime_fill_count, 2U);
    failures += expect_u32("realtime_dma_samples", g_dma_samples, 1024U);
    failures += expect_i32("realtime_first_half_data",
                           g_dma_first_sample, 1);
    failures += expect_i32("realtime_second_half_data",
                           g_dma_second_half_first_sample, 2);

    audio_on_tx_half_complete();
    audio_poll();
    failures += expect_u32("realtime_half_refill", g_realtime_fill_count, 3U);
    audio_on_tx_complete();
    audio_poll();
    failures += expect_u32("realtime_full_refill", g_realtime_fill_count, 4U);
    failures += expect_i32("realtime_stop", audio_realtime_stop(), AUDIO_OK);
    failures += expect_u32("realtime_dma_stopped", g_stop_count, 1U);
    failures += expect_u32("realtime_resource_released", g_release_count, 1U);

    reset_fixture();
    failures += expect_i32("realtime_null_fill",
        audio_realtime_start(NULL, NULL, 48000U, 256U), AUDIO_INVALID_ARG);
    failures += expect_i32("realtime_zero_frames",
        audio_realtime_start(realtime_fill, NULL, 48000U, 0U),
        AUDIO_INVALID_ARG);
    failures += expect_i32("realtime_oversize_frames",
        audio_realtime_start(realtime_fill, NULL, 48000U, 2305U),
        AUDIO_INVALID_ARG);
    return failures;
}

static int test_dma_buffer_placement_contract(void)
{
    FILE *source_file;
    FILE *scatter_file;
    FILE *project_file;
    char source[20000];
    char scatter[4096];
    char project[400000];
    size_t source_length;
    size_t scatter_length;
    size_t project_length;
    char *axi_region;

    source_file = fopen("Core/Peripheral/Src/audio_playback_service.c", "rb");
    scatter_file = fopen("MDK-ARM/H743_LCD_Keil5/H743_LCD_Keil5_custom.sct", "rb");
    project_file = fopen("MDK-ARM/H743_LCD_Keil5.uvprojx", "rb");
    if ((source_file == NULL) || (scatter_file == NULL) ||
        (project_file == NULL))
    {
        if (source_file != NULL) fclose(source_file);
        if (scatter_file != NULL) fclose(scatter_file);
        if (project_file != NULL) fclose(project_file);
        printf("FAIL audio_dma_placement_files\n");
        return 1;
    }
    source_length = fread(source, 1U, sizeof(source) - 1U, source_file);
    scatter_length = fread(scatter, 1U, sizeof(scatter) - 1U, scatter_file);
    project_length = fread(project, 1U, sizeof(project) - 1U, project_file);
    fclose(source_file);
    fclose(scatter_file);
    fclose(project_file);
    source[source_length] = '\0';
    scatter[scatter_length] = '\0';
    project[project_length] = '\0';
    axi_region = strstr(scatter, "RW_IRAM2 0x24000000");
    if ((strstr(source, "section(\".audio_dma\")") == NULL) ||
        (axi_region == NULL) ||
        (strstr(axi_region, "*(.audio_dma)") == NULL) ||
        (strstr(project, "<umfTarg>0</umfTarg>") == NULL) ||
        (strstr(project, "<useFile>1</useFile>") == NULL) ||
        (strstr(project, "H743_LCD_Keil5_custom.sct") == NULL))
    {
        printf("FAIL audio_dma_not_fixed_in_axi\n");
        return 1;
    }
    return 0;
}

int main(void)
{
    int failures = 0;

    failures += test_play_stop_and_eof();
    failures += test_pause_and_resume();
    failures += test_camera_suspend_and_restore();
    failures += test_busy_and_volume();
    failures += test_avi_stream_playback();
    failures += test_realtime_pcm_producer();
    failures += test_dma_buffer_placement_contract();
    if (failures != 0)
    {
        printf("FAIL audio_playback_service failures=%d\n", failures);
        return 1;
    }
    printf("PASS audio_playback_service\n");
    return 0;
}
