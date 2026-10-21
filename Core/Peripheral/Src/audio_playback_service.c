#include "audio_playback_service.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "avi_audio_decoder.h"
#include "es8388_service.h"
#include "mp3_decoder_service.h"

#define AUDIO_PCM_HALF_SAMPLES 4608U
#define AUDIO_STREAM_PCM_HALF_SAMPLES 2048U
#define AUDIO_STREAM_HALF_SAMPLES 2304U
#define AUDIO_STREAM_PCM_PREBUFFER_HALVES 4U
#define AUDIO_STREAM_MP3_PREBUFFER_BYTES 4096U
#define AUDIO_OWNER 0x0401U

typedef enum
{
    AUDIO_SOURCE_NONE = 0,
    AUDIO_SOURCE_MP3_FILE,
    AUDIO_SOURCE_AVI_STREAM,
    AUDIO_SOURCE_REALTIME
} audio_source_t;

#if defined(AUDIO_PLAYBACK_SERVICE_HOST_TEST)
extern int audio_test_acquire(void);
extern void audio_test_release(void);
extern int audio_test_start_dma(int16_t *buffer, uint32_t samples);
extern int audio_test_stop_dma(void);
extern int audio_test_pause_dma(void);
extern int audio_test_resume_dma(void);
extern int audio_test_configure_sample_rate(uint32_t sample_rate);
#else
#include "main.h"
#include "audio_recording_service.h"
#include "resource_manager.h"
#include "sai.h"
#endif

static audio_status_t g_audio_status;
static int16_t g_audio_pcm[2][AUDIO_PCM_HALF_SAMPLES]
    __attribute__((section(".audio_dma"), aligned(32)));
static uint8_t g_audio_eof;
static uint8_t g_audio_eof_drain_halves;
static uint64_t g_audio_played_sample_frames;
static uint8_t g_audio_resource_held;
static uint8_t g_audio_dma_started;
static uint32_t g_audio_dma_half_samples;
static uint32_t g_audio_stream_sample_rate;
static audio_stream_format_t g_audio_stream_format;
static uint8_t g_audio_stream_channels;
static uint32_t g_audio_stream_prebuffer_bytes;
static audio_source_t g_audio_source;
static audio_state_t g_audio_camera_previous_state;
static volatile uint8_t g_audio_dma_half_event;
static volatile uint8_t g_audio_dma_full_event;
static audio_realtime_fill_t g_audio_realtime_fill;
static void *g_audio_realtime_context;

static void audio_clean_pcm_cache(const int16_t *buffer, uint32_t samples)
{
#if !defined(AUDIO_PLAYBACK_SERVICE_HOST_TEST)
    uintptr_t start = (uintptr_t)buffer;
    uintptr_t end = start + ((uintptr_t)samples * sizeof(int16_t));
    uint32_t aligned_start = (uint32_t)(start & ~(uintptr_t)31U);
    uint32_t aligned_end = (uint32_t)((end + 31U) & ~(uintptr_t)31U);

    SCB_CleanDCache_by_Addr((uint32_t *)aligned_start,
                            (int32_t)(aligned_end - aligned_start));
#else
    (void)buffer;
    (void)samples;
#endif
}

static void audio_increment_saturated(uint32_t *value)
{
    if (*value != UINT32_MAX)
    {
        (*value)++;
    }
}

static int audio_acquire(void)
{
    int result;

    if (g_audio_resource_held != 0U)
    {
        return 0;
    }
#if defined(AUDIO_PLAYBACK_SERVICE_HOST_TEST)
    result = audio_test_acquire();
#else
    result = resource_acquire(RESOURCE_SAI1, AUDIO_OWNER);
#endif
    if (result == 0)
    {
        g_audio_resource_held = 1U;
    }
    return result;
}

static void audio_release(void)
{
    if (g_audio_resource_held == 0U)
    {
        return;
    }
#if defined(AUDIO_PLAYBACK_SERVICE_HOST_TEST)
    audio_test_release();
#else
    (void)resource_release(RESOURCE_SAI1, AUDIO_OWNER);
#endif
    g_audio_resource_held = 0U;
}

static int audio_start_dma(void)
{
#if defined(AUDIO_PLAYBACK_SERVICE_HOST_TEST)
    return audio_test_start_dma(&g_audio_pcm[0][0],
                                g_audio_dma_half_samples * 2U);
#else
    /* The current PLL3 clock uses the non-OSR path for the 48 kHz SAI frame. */
    CLEAR_BIT(hsai_BlockA1.Instance->CR1, SAI_xCR1_OSR);
    return (HAL_SAI_Transmit_DMA(&hsai_BlockA1,
                                 (uint8_t *)&g_audio_pcm[0][0],
                                 g_audio_dma_half_samples * 2U) == HAL_OK) ?
           0 : 1;
#endif
}

static int audio_configure_sample_rate(uint32_t sample_rate)
{
#if defined(AUDIO_PLAYBACK_SERVICE_HOST_TEST)
    return audio_test_configure_sample_rate(sample_rate);
#else
    if (sample_rate == 0U)
    {
        return 1;
    }
    __HAL_SAI_DISABLE(&hsai_BlockA1);
    hsai_BlockA1.Init.AudioFrequency = sample_rate;
    CLEAR_BIT(hsai_BlockA1.Instance->CR1, SAI_xCR1_OSR);
    return (HAL_SAI_Init(&hsai_BlockA1) == HAL_OK) ? 0 : 1;
#endif
}

static int audio_stop_dma(void)
{
#if defined(AUDIO_PLAYBACK_SERVICE_HOST_TEST)
    return audio_test_stop_dma();
#else
    return (HAL_SAI_DMAStop(&hsai_BlockA1) == HAL_OK) ? 0 : 1;
#endif
}

static int audio_pause_dma(void)
{
#if defined(AUDIO_PLAYBACK_SERVICE_HOST_TEST)
    return audio_test_pause_dma();
#else
    return (HAL_SAI_DMAPause(&hsai_BlockA1) == HAL_OK) ? 0 : 1;
#endif
}

static int audio_resume_dma(void)
{
#if defined(AUDIO_PLAYBACK_SERVICE_HOST_TEST)
    return audio_test_resume_dma();
#else
    return (HAL_SAI_DMAResume(&hsai_BlockA1) == HAL_OK) ? 0 : 1;
#endif
}

static void audio_account_completed_half(void)
{
    if (g_audio_status.sample_rate == 0U)
    {
        return;
    }
    g_audio_played_sample_frames += g_audio_dma_half_samples / 2U;
    g_audio_status.elapsed_ms = (uint32_t)(
        (g_audio_played_sample_frames * 1000ULL) /
        g_audio_status.sample_rate);
}

static int audio_fill_half(uint8_t half)
{
    int16_t *half_buffer = &g_audio_pcm[0][0] +
                           (uint32_t)half * g_audio_dma_half_samples;
    uint32_t filled = 0U;
    uint8_t channels = 0U;
    int result = MP3_DECODER_OK;

    if (g_audio_eof != 0U)
    {
        memset(half_buffer, 0,
               g_audio_dma_half_samples * sizeof(int16_t));
        audio_clean_pcm_cache(half_buffer, g_audio_dma_half_samples);
        return AUDIO_EOF;
    }

    while ((filled < g_audio_dma_half_samples) && (g_audio_eof == 0U))
    {
        uint32_t samples = 0U;

        result = mp3_decoder_decode_next(&half_buffer[filled],
                                         g_audio_dma_half_samples - filled,
                                         &samples, &channels);
        if ((result != MP3_DECODER_OK) && (result != MP3_DECODER_EOF))
        {
            return AUDIO_IO_ERROR;
        }
        if (samples > (g_audio_dma_half_samples - filled))
        {
            return AUDIO_IO_ERROR;
        }
        filled += samples;
        if (samples != 0U)
        {
            audio_increment_saturated(&g_audio_status.frame_count);
        }
        if (result == MP3_DECODER_EOF)
        {
            g_audio_eof = 1U;
            g_audio_eof_drain_halves = 2U;
        }
        if ((samples == 0U) && (result == MP3_DECODER_OK))
        {
            return AUDIO_IO_ERROR;
        }
    }

    if (filled < g_audio_dma_half_samples)
    {
        memset(&half_buffer[filled], 0,
               (g_audio_dma_half_samples - filled) * sizeof(int16_t));
    }
    audio_clean_pcm_cache(half_buffer, g_audio_dma_half_samples);
    (void)channels;
    return (result == MP3_DECODER_EOF) ? AUDIO_EOF : AUDIO_OK;
}

static int audio_fill_stream_half(uint8_t half, uint8_t count_underrun)
{
    int16_t *half_buffer = &g_audio_pcm[0][0] +
                           (uint32_t)half * g_audio_dma_half_samples;
    uint32_t samples = 0U;
    uint32_t sample_rate = 0U;
    int result;

    result = avi_audio_decoder_decode_next(half_buffer,
                                           g_audio_dma_half_samples,
                                           &samples, &sample_rate);
    if (result == AVI_AUDIO_DECODER_OK)
    {
        if ((sample_rate != g_audio_stream_sample_rate) ||
            (samples > g_audio_dma_half_samples))
        {
            return AUDIO_IO_ERROR;
        }
    }
    else if (result == AVI_AUDIO_DECODER_EOF)
    {
        g_audio_eof = 1U;
        g_audio_eof_drain_halves = 2U;
        samples = 0U;
    }
    else if (result == AVI_AUDIO_DECODER_NEED_DATA)
    {
        samples = 0U;
        if (count_underrun != 0U)
        {
            audio_increment_saturated(&g_audio_status.underrun_count);
        }
    }
    else
    {
        return AUDIO_IO_ERROR;
    }
    if (samples < g_audio_dma_half_samples)
    {
        memset(&half_buffer[samples], 0,
               (g_audio_dma_half_samples - samples) * sizeof(int16_t));
    }
    audio_clean_pcm_cache(half_buffer, g_audio_dma_half_samples);
    return (result == AVI_AUDIO_DECODER_EOF) ? AUDIO_EOF : AUDIO_OK;
}

static void audio_fill_realtime_half(uint8_t half)
{
    int16_t *half_buffer = &g_audio_pcm[0][0] +
                           (uint32_t)half * g_audio_dma_half_samples;

    g_audio_realtime_fill(g_audio_realtime_context, half_buffer,
                          g_audio_dma_half_samples / 2U);
    audio_clean_pcm_cache(half_buffer, g_audio_dma_half_samples);
}

void audio_service_init(void)
{
    memset(&g_audio_status, 0, sizeof(g_audio_status));
    g_audio_status.initialized = 1U;
    g_audio_status.state = AUDIO_STATE_IDLE;
    g_audio_status.volume_percent = 50U;
    g_audio_status.last_result = AUDIO_NOT_READY;
    g_audio_eof = 0U;
    g_audio_eof_drain_halves = 0U;
    g_audio_played_sample_frames = 0U;
    g_audio_resource_held = 0U;
    g_audio_dma_started = 0U;
    g_audio_dma_half_samples = AUDIO_PCM_HALF_SAMPLES;
    g_audio_stream_sample_rate = 0U;
    g_audio_stream_format = AUDIO_STREAM_FORMAT_PCM;
    g_audio_stream_channels = 0U;
    g_audio_stream_prebuffer_bytes = 0U;
    g_audio_source = AUDIO_SOURCE_NONE;
    g_audio_camera_previous_state = AUDIO_STATE_IDLE;
    g_audio_dma_half_event = 0U;
    g_audio_dma_full_event = 0U;
    g_audio_realtime_fill = NULL;
    g_audio_realtime_context = NULL;
    mp3_decoder_service_init();
    avi_audio_decoder_init();
    es8388_service_init();
}

int audio_set_volume_percent(uint8_t percent)
{
    int result;

    if (percent > 100U)
    {
        return AUDIO_INVALID_ARG;
    }
    result = es8388_set_volume_percent(percent);
    g_audio_status.volume_percent = percent;
    if (result == ES8388_NOT_READY)
    {
        g_audio_status.last_result = AUDIO_OK;
        return AUDIO_OK;
    }
    if (result != ES8388_OK)
    {
        g_audio_status.last_result = AUDIO_CODEC_ERROR;
        return AUDIO_CODEC_ERROR;
    }
    g_audio_status.last_result = AUDIO_OK;
    return AUDIO_OK;
}

int audio_play(const char *path)
{
    mp3_decoder_status_t decoder_status;
    int result;

    if ((path == NULL) || (path[0] == '\0'))
    {
        return AUDIO_INVALID_ARG;
    }
    if ((g_audio_status.state != AUDIO_STATE_IDLE) &&
        (g_audio_status.state != AUDIO_STATE_ERROR))
    {
        return AUDIO_BUSY;
    }
    if (audio_acquire() != 0)
    {
        g_audio_status.last_result = AUDIO_BUSY;
        return AUDIO_BUSY;
    }
    result = es8388_probe_readonly(10U);
    if (result != ES8388_OK)
    {
        audio_release();
        g_audio_status.last_result = AUDIO_CODEC_ERROR;
        return AUDIO_CODEC_ERROR;
    }
    result = es8388_prepare_playback();
    if (result != ES8388_OK)
    {
        audio_release();
        g_audio_status.last_result = AUDIO_CODEC_ERROR;
        return AUDIO_CODEC_ERROR;
    }
    result = audio_set_volume_percent(g_audio_status.volume_percent);
    if (result != AUDIO_OK)
    {
        audio_release();
        return result;
    }
    result = mp3_decoder_open(path);
    if (result != MP3_DECODER_OK)
    {
        audio_release();
        g_audio_status.last_result = AUDIO_IO_ERROR;
        return AUDIO_IO_ERROR;
    }
    g_audio_eof = 0U;
    g_audio_eof_drain_halves = 0U;
    g_audio_played_sample_frames = 0U;
    g_audio_dma_half_event = 0U;
    g_audio_dma_full_event = 0U;
    g_audio_dma_half_samples = AUDIO_PCM_HALF_SAMPLES;
    g_audio_source = AUDIO_SOURCE_MP3_FILE;
    g_audio_status.frame_count = 0U;
    g_audio_status.sample_rate = 0U;
    g_audio_status.elapsed_ms = 0U;
    g_audio_status.duration_ms = 0U;
    if ((audio_fill_half(0U) == AUDIO_IO_ERROR) ||
        (audio_fill_half(1U) == AUDIO_IO_ERROR))
    {
        (void)mp3_decoder_close();
        audio_release();
        g_audio_source = AUDIO_SOURCE_NONE;
        g_audio_status.last_result = AUDIO_DMA_ERROR;
        return AUDIO_DMA_ERROR;
    }
    memset(&decoder_status, 0, sizeof(decoder_status));
    mp3_decoder_get_status(&decoder_status);
    if ((audio_configure_sample_rate(decoder_status.sample_rate) != 0) ||
        (audio_start_dma() != 0))
    {
        (void)mp3_decoder_close();
        audio_release();
        g_audio_source = AUDIO_SOURCE_NONE;
        g_audio_status.last_result = AUDIO_DMA_ERROR;
        return AUDIO_DMA_ERROR;
    }
    g_audio_dma_started = 1U;
    g_audio_status.sample_rate = decoder_status.sample_rate;
    g_audio_status.duration_ms = decoder_status.duration_ms;
    g_audio_status.state = AUDIO_STATE_PLAYING;
    g_audio_status.last_result = AUDIO_OK;
    return AUDIO_OK;
}

int audio_pause(void)
{
    if (g_audio_status.state == AUDIO_STATE_PAUSED)
    {
        return AUDIO_OK;
    }
    if (g_audio_status.state != AUDIO_STATE_PLAYING)
    {
        return AUDIO_NOT_READY;
    }
    if (audio_pause_dma() != 0)
    {
        g_audio_status.last_result = AUDIO_DMA_ERROR;
        return AUDIO_DMA_ERROR;
    }
    g_audio_status.state = AUDIO_STATE_PAUSED;
    g_audio_status.last_result = AUDIO_OK;
    return AUDIO_OK;
}

int audio_resume(void)
{
    if (g_audio_status.state == AUDIO_STATE_PLAYING)
    {
        return AUDIO_OK;
    }
    if (g_audio_status.state != AUDIO_STATE_PAUSED)
    {
        return AUDIO_NOT_READY;
    }
    if (audio_resume_dma() != 0)
    {
        g_audio_status.last_result = AUDIO_DMA_ERROR;
        return AUDIO_DMA_ERROR;
    }
    g_audio_status.state = AUDIO_STATE_PLAYING;
    g_audio_status.last_result = AUDIO_OK;
    return AUDIO_OK;
}

int audio_suspend_for_camera(void)
{
    audio_state_t previous_state = g_audio_status.state;

    if (previous_state == AUDIO_STATE_SUSPENDED)
    {
        return AUDIO_OK;
    }
    if ((previous_state != AUDIO_STATE_PLAYING) &&
        (previous_state != AUDIO_STATE_PAUSED))
    {
        return AUDIO_OK;
    }
    if (audio_stop_dma() != 0)
    {
        g_audio_status.last_result = AUDIO_DMA_ERROR;
        return AUDIO_DMA_ERROR;
    }
    if (mp3_decoder_suspend() != MP3_DECODER_OK)
    {
        g_audio_status.state = AUDIO_STATE_ERROR;
        g_audio_status.last_result = AUDIO_IO_ERROR;
        return AUDIO_IO_ERROR;
    }
    audio_release();
    g_audio_camera_previous_state = previous_state;
    g_audio_dma_half_event = 0U;
    g_audio_dma_full_event = 0U;
    g_audio_status.state = AUDIO_STATE_SUSPENDED;
    g_audio_status.last_result = AUDIO_OK;
    return AUDIO_OK;
}

int audio_restore_after_camera(void)
{
    audio_state_t restore_state;

    if (g_audio_status.state != AUDIO_STATE_SUSPENDED)
    {
        return AUDIO_OK;
    }
    restore_state = g_audio_camera_previous_state;
    if (audio_acquire() != 0)
    {
        g_audio_status.last_result = AUDIO_BUSY;
        return AUDIO_BUSY;
    }
    if (mp3_decoder_resume() != MP3_DECODER_OK)
    {
        audio_release();
        g_audio_status.last_result = AUDIO_IO_ERROR;
        return AUDIO_IO_ERROR;
    }
    g_audio_eof = 0U;
    g_audio_eof_drain_halves = 0U;
    g_audio_dma_half_event = 0U;
    g_audio_dma_full_event = 0U;
    if ((audio_fill_half(0U) == AUDIO_IO_ERROR) ||
        (audio_fill_half(1U) == AUDIO_IO_ERROR) ||
        (audio_configure_sample_rate(g_audio_status.sample_rate) != 0) ||
        (audio_start_dma() != 0))
    {
        (void)mp3_decoder_suspend();
        audio_release();
        g_audio_status.last_result = AUDIO_DMA_ERROR;
        return AUDIO_DMA_ERROR;
    }
    g_audio_dma_started = 1U;
    if ((restore_state == AUDIO_STATE_PAUSED) && (audio_pause_dma() != 0))
    {
        (void)audio_stop_dma();
        (void)mp3_decoder_suspend();
        audio_release();
        g_audio_status.last_result = AUDIO_DMA_ERROR;
        return AUDIO_DMA_ERROR;
    }
    g_audio_camera_previous_state = AUDIO_STATE_IDLE;
    g_audio_status.state = restore_state;
    g_audio_status.last_result = AUDIO_OK;
    return AUDIO_OK;
}

int audio_stop(void)
{
    if (g_audio_status.state == AUDIO_STATE_REALTIME)
    {
        return audio_realtime_stop();
    }
    if (g_audio_status.state == AUDIO_STATE_STREAMING)
    {
        return audio_stream_stop();
    }
    if ((g_audio_status.state != AUDIO_STATE_PLAYING) &&
        (g_audio_status.state != AUDIO_STATE_PAUSED) &&
        (g_audio_status.state != AUDIO_STATE_SUSPENDED))
    {
        return AUDIO_OK;
    }
    if (g_audio_status.state != AUDIO_STATE_SUSPENDED)
    {
        if (g_audio_dma_started != 0U)
        {
            (void)audio_stop_dma();
        }
    }
    (void)mp3_decoder_close();
    audio_release();
    g_audio_status.state = AUDIO_STATE_IDLE;
    g_audio_status.last_result = AUDIO_OK;
    g_audio_eof = 0U;
    g_audio_eof_drain_halves = 0U;
    g_audio_dma_half_event = 0U;
    g_audio_dma_full_event = 0U;
    g_audio_camera_previous_state = AUDIO_STATE_IDLE;
    g_audio_dma_started = 0U;
    g_audio_source = AUDIO_SOURCE_NONE;
    return AUDIO_OK;
}

int audio_stream_start(audio_stream_format_t format,
                       uint32_t sample_rate,
                       uint8_t channels,
                       uint8_t bits_per_sample)
{
    avi_audio_decoder_format_t decoder_format;
    int result;

    if ((g_audio_status.state != AUDIO_STATE_IDLE) &&
        (g_audio_status.state != AUDIO_STATE_ERROR))
    {
        return AUDIO_BUSY;
    }
    decoder_format = (format == AUDIO_STREAM_FORMAT_PCM) ?
                     AVI_AUDIO_DECODER_PCM :
                     (format == AUDIO_STREAM_FORMAT_MP3) ?
                     AVI_AUDIO_DECODER_MP3 :
                     AVI_AUDIO_DECODER_NONE;
    if (decoder_format == AVI_AUDIO_DECODER_NONE)
    {
        return AUDIO_INVALID_ARG;
    }
    if (audio_acquire() != 0)
    {
        return AUDIO_BUSY;
    }
    result = es8388_probe_readonly(10U);
    if ((result != ES8388_OK) ||
        (es8388_prepare_playback() != ES8388_OK) ||
        (audio_set_volume_percent(g_audio_status.volume_percent) != AUDIO_OK))
    {
        audio_release();
        return AUDIO_CODEC_ERROR;
    }
    result = avi_audio_decoder_open(decoder_format, sample_rate,
                                    channels, bits_per_sample);
    if (result != AVI_AUDIO_DECODER_OK)
    {
        audio_release();
        return (result == AVI_AUDIO_DECODER_UNSUPPORTED) ?
               AUDIO_INVALID_ARG : AUDIO_IO_ERROR;
    }
    if (audio_configure_sample_rate(sample_rate) != 0)
    {
        avi_audio_decoder_close();
        audio_release();
        return AUDIO_DMA_ERROR;
    }
    g_audio_source = AUDIO_SOURCE_AVI_STREAM;
    g_audio_dma_half_samples = (format == AUDIO_STREAM_FORMAT_PCM) ?
                               AUDIO_STREAM_PCM_HALF_SAMPLES :
                               AUDIO_STREAM_HALF_SAMPLES;
    g_audio_stream_sample_rate = sample_rate;
    g_audio_stream_format = format;
    g_audio_stream_channels = channels;
    g_audio_stream_prebuffer_bytes = 0U;
    g_audio_dma_started = 0U;
    g_audio_eof = 0U;
    g_audio_eof_drain_halves = 0U;
    g_audio_dma_half_event = 0U;
    g_audio_dma_full_event = 0U;
    g_audio_played_sample_frames = 0U;
    g_audio_status.frame_count = 0U;
    g_audio_status.sample_rate = sample_rate;
    g_audio_status.elapsed_ms = 0U;
    g_audio_status.duration_ms = 0U;
    g_audio_status.underrun_count = 0U;
    g_audio_status.state = AUDIO_STATE_STREAMING;
    g_audio_status.last_result = AUDIO_OK;
    return AUDIO_OK;
}

int audio_stream_push(const uint8_t *data, uint32_t size)
{
    int result;
    uint32_t prebuffer_target;

    if ((g_audio_status.state != AUDIO_STATE_STREAMING) ||
        (g_audio_source != AUDIO_SOURCE_AVI_STREAM))
    {
        return AUDIO_NOT_READY;
    }
    result = avi_audio_decoder_push(data, size);
    if (result != AVI_AUDIO_DECODER_OK)
    {
        g_audio_status.last_result = AUDIO_IO_ERROR;
        return (result == AVI_AUDIO_DECODER_OVERFLOW) ?
               AUDIO_BUSY : AUDIO_IO_ERROR;
    }
    if (g_audio_dma_started == 0U)
    {
        if (g_audio_stream_format == AUDIO_STREAM_FORMAT_PCM)
        {
            prebuffer_target = (g_audio_dma_half_samples / 2U) *
                               (uint32_t)g_audio_stream_channels *
                               sizeof(int16_t) *
                               AUDIO_STREAM_PCM_PREBUFFER_HALVES;
        }
        else
        {
            prebuffer_target = AUDIO_STREAM_MP3_PREBUFFER_BYTES;
        }
        if (g_audio_stream_prebuffer_bytes > (UINT32_MAX - size))
        {
            return AUDIO_IO_ERROR;
        }
        g_audio_stream_prebuffer_bytes += size;
        if (g_audio_stream_prebuffer_bytes < prebuffer_target)
        {
            return AUDIO_OK;
        }
    }
    if (g_audio_dma_started == 0U)
    {
        if ((audio_fill_stream_half(0U, 0U) == AUDIO_IO_ERROR) ||
            (audio_fill_stream_half(1U, 0U) == AUDIO_IO_ERROR) ||
            (audio_start_dma() != 0))
        {
            g_audio_status.last_result = AUDIO_DMA_ERROR;
            return AUDIO_DMA_ERROR;
        }
        g_audio_dma_started = 1U;
        g_audio_stream_prebuffer_bytes = 0U;
    }
    return AUDIO_OK;
}

void audio_stream_finish(void)
{
    if (g_audio_status.state == AUDIO_STATE_STREAMING)
    {
        avi_audio_decoder_finish();
    }
}

int audio_stream_stop(void)
{
    if (g_audio_status.state != AUDIO_STATE_STREAMING)
    {
        return AUDIO_OK;
    }
    if (g_audio_dma_started != 0U)
    {
        (void)audio_stop_dma();
    }
    avi_audio_decoder_close();
    audio_release();
    g_audio_dma_started = 0U;
    g_audio_stream_prebuffer_bytes = 0U;
    g_audio_source = AUDIO_SOURCE_NONE;
    g_audio_status.state = AUDIO_STATE_IDLE;
    g_audio_status.last_result = AUDIO_OK;
    g_audio_dma_half_event = 0U;
    g_audio_dma_full_event = 0U;
    return AUDIO_OK;
}

int audio_realtime_start(audio_realtime_fill_t fill, void *context,
                         uint32_t sample_rate, uint16_t frames_per_half)
{
    int result;

    if ((fill == NULL) || (sample_rate == 0U) ||
        (frames_per_half == 0U) ||
        (frames_per_half > (AUDIO_PCM_HALF_SAMPLES / 2U)))
    {
        return AUDIO_INVALID_ARG;
    }
    if ((g_audio_status.state != AUDIO_STATE_IDLE) &&
        (g_audio_status.state != AUDIO_STATE_ERROR))
    {
        return AUDIO_BUSY;
    }
    if (audio_acquire() != 0)
    {
        return AUDIO_BUSY;
    }
    result = es8388_probe_readonly(10U);
    if ((result != ES8388_OK) ||
        (es8388_prepare_playback() != ES8388_OK) ||
        (audio_set_volume_percent(g_audio_status.volume_percent) != AUDIO_OK))
    {
        audio_release();
        return AUDIO_CODEC_ERROR;
    }
    if (audio_configure_sample_rate(sample_rate) != 0)
    {
        audio_release();
        return AUDIO_DMA_ERROR;
    }

    g_audio_realtime_fill = fill;
    g_audio_realtime_context = context;
    g_audio_dma_half_samples = (uint32_t)frames_per_half * 2U;
    g_audio_dma_half_event = 0U;
    g_audio_dma_full_event = 0U;
    g_audio_played_sample_frames = 0U;
    g_audio_status.frame_count = 0U;
    g_audio_status.sample_rate = sample_rate;
    g_audio_status.elapsed_ms = 0U;
    g_audio_status.duration_ms = 0U;
    g_audio_status.underrun_count = 0U;
    audio_fill_realtime_half(0U);
    audio_fill_realtime_half(1U);
    if (audio_start_dma() != 0)
    {
        g_audio_realtime_fill = NULL;
        g_audio_realtime_context = NULL;
        audio_release();
        return AUDIO_DMA_ERROR;
    }
    g_audio_dma_started = 1U;
    g_audio_source = AUDIO_SOURCE_REALTIME;
    g_audio_status.state = AUDIO_STATE_REALTIME;
    g_audio_status.last_result = AUDIO_OK;
    return AUDIO_OK;
}

int audio_realtime_stop(void)
{
    if (g_audio_status.state != AUDIO_STATE_REALTIME)
    {
        return AUDIO_OK;
    }
    if (g_audio_dma_started != 0U)
    {
        (void)audio_stop_dma();
    }
    audio_release();
    g_audio_dma_started = 0U;
    g_audio_source = AUDIO_SOURCE_NONE;
    g_audio_realtime_fill = NULL;
    g_audio_realtime_context = NULL;
    g_audio_status.state = AUDIO_STATE_IDLE;
    g_audio_status.last_result = AUDIO_OK;
    g_audio_dma_half_event = 0U;
    g_audio_dma_full_event = 0U;
    return AUDIO_OK;
}

void audio_poll(void)
{
    if (g_audio_status.state == AUDIO_STATE_REALTIME)
    {
        if (g_audio_dma_half_event != 0U)
        {
            g_audio_dma_half_event = 0U;
            audio_account_completed_half();
            audio_fill_realtime_half(0U);
            audio_increment_saturated(&g_audio_status.frame_count);
        }
        if (g_audio_dma_full_event != 0U)
        {
            g_audio_dma_full_event = 0U;
            audio_account_completed_half();
            audio_fill_realtime_half(1U);
            audio_increment_saturated(&g_audio_status.frame_count);
        }
        return;
    }
    if (g_audio_status.state == AUDIO_STATE_STREAMING)
    {
        if (g_audio_dma_half_event != 0U)
        {
            g_audio_dma_half_event = 0U;
            audio_account_completed_half();
            if (g_audio_eof != 0U)
            {
                if ((g_audio_eof_drain_halves > 0U) &&
                    (--g_audio_eof_drain_halves == 0U))
                {
                    (void)audio_stream_stop();
                    return;
                }
            }
            else if (audio_fill_stream_half(0U, 1U) == AUDIO_IO_ERROR)
            {
                (void)audio_stream_stop();
                g_audio_status.state = AUDIO_STATE_ERROR;
                g_audio_status.last_result = AUDIO_IO_ERROR;
                return;
            }
        }
        if (g_audio_dma_full_event != 0U)
        {
            g_audio_dma_full_event = 0U;
            audio_account_completed_half();
            if (g_audio_eof != 0U)
            {
                if ((g_audio_eof_drain_halves > 0U) &&
                    (--g_audio_eof_drain_halves == 0U))
                {
                    (void)audio_stream_stop();
                }
            }
            else if (audio_fill_stream_half(1U, 1U) == AUDIO_IO_ERROR)
            {
                (void)audio_stream_stop();
                g_audio_status.state = AUDIO_STATE_ERROR;
                g_audio_status.last_result = AUDIO_IO_ERROR;
            }
        }
        return;
    }
    if (g_audio_status.state != AUDIO_STATE_PLAYING)
    {
        return;
    }
    if (g_audio_dma_half_event != 0U)
    {
        g_audio_dma_half_event = 0U;
        audio_account_completed_half();
        if (g_audio_eof != 0U)
        {
            if (g_audio_eof_drain_halves > 0U)
            {
                g_audio_eof_drain_halves--;
            }
            if (g_audio_eof_drain_halves == 0U)
            {
                (void)audio_stop();
                return;
            }
        }
        else if (audio_fill_half(0U) == AUDIO_IO_ERROR)
        {
            (void)audio_stop();
            g_audio_status.state = AUDIO_STATE_ERROR;
            return;
        }
    }
    if (g_audio_dma_full_event != 0U)
    {
        g_audio_dma_full_event = 0U;
        audio_account_completed_half();
        if (g_audio_eof != 0U)
        {
            if (g_audio_eof_drain_halves > 0U)
            {
                g_audio_eof_drain_halves--;
            }
            if (g_audio_eof_drain_halves == 0U)
            {
                (void)audio_stop();
                return;
            }
        }
        else if (audio_fill_half(1U) == AUDIO_IO_ERROR)
        {
            (void)audio_stop();
            g_audio_status.state = AUDIO_STATE_ERROR;
            return;
        }
    }
}

void audio_get_status(audio_status_t *status)
{
    if (status != NULL)
    {
        *status = g_audio_status;
    }
}

void audio_print_status(void)
{
    audio_status_t status;

    audio_get_status(&status);
    printf("audio status initialized=%u state=%u volume_percent=%u frame_count=%lu sample_rate=%lu elapsed_ms=%lu duration_ms=%lu underrun_count=%lu result=%ld\r\n",
           (unsigned int)status.initialized,
           (unsigned int)status.state,
           (unsigned int)status.volume_percent,
           (unsigned long)status.frame_count,
           (unsigned long)status.sample_rate,
           (unsigned long)status.elapsed_ms,
           (unsigned long)status.duration_ms,
           (unsigned long)status.underrun_count,
           (long)status.last_result);
}

void audio_on_tx_half_complete(void)
{
    g_audio_dma_half_event = 1U;
}

void audio_on_tx_complete(void)
{
    g_audio_dma_full_event = 1U;
}

#if !defined(AUDIO_PLAYBACK_SERVICE_HOST_TEST)
void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
    if ((hsai != NULL) && (hsai->Instance == SAI1_Block_A))
    {
        audio_on_tx_half_complete();
    }
}

void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai)
{
    if ((hsai != NULL) && (hsai->Instance == SAI1_Block_A))
    {
        audio_on_tx_complete();
    }
}

void HAL_SAI_ErrorCallback(SAI_HandleTypeDef *hsai)
{
    if ((hsai != NULL) && (hsai->Instance == SAI1_Block_A))
    {
        audio_increment_saturated(&g_audio_status.underrun_count);
        g_audio_status.last_result = AUDIO_DMA_ERROR;
        audio_record_on_rx_error();
    }
    else if ((hsai != NULL) && (hsai->Instance == SAI1_Block_B))
    {
        audio_record_on_rx_error();
    }
}
#endif
