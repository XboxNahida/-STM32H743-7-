#include "audio_recording_service.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "es8388_service.h"

#define AUDIO_RECORD_PCM_HALF_SAMPLES 4096U
#define AUDIO_RECORD_BYTES_PER_SAMPLE 2U
#define AUDIO_RECORD_PATH_MAX 128U
#define AUDIO_RECORD_WAV_HEADER_SIZE 44U
#define AUDIO_RECORD_MAX_DATA_BYTES 691200000UL
#define AUDIO_RECORD_EVENT_HALF 0x01U
#define AUDIO_RECORD_EVENT_FULL 0x02U
#define AUDIO_RECORD_EVENT_ERROR 0x04U
#define AUDIO_RECORD_SD_SESSION_OWNER 0x0502U

#if defined(AUDIO_RECORDING_SERVICE_HOST_TEST)
extern int audio_record_test_acquire(void);
extern void audio_record_test_release(void);
extern int audio_record_test_start_clock(int16_t *buffer, uint32_t samples);
extern int audio_record_test_start_rx(int16_t *buffer, uint32_t samples);
extern int audio_record_test_stop_rx(void);
extern int audio_record_test_stop_clock(void);
extern int audio_record_test_open_file(const char *path);
extern int audio_record_test_close_file(void);
extern int audio_record_test_write_file(const void *data, uint32_t size);
extern int audio_record_test_seek_file(uint32_t position);
extern int audio_record_test_sync_file(void);
extern int audio_record_test_remove_file(void);
#else
#include "ff.h"
#include "main.h"
#include "resource_manager.h"
#include "sai.h"
#include "sdcard_manager.h"
#endif

static audio_recording_status_t g_audio_record_status;
static int16_t g_audio_record_pcm[2][AUDIO_RECORD_PCM_HALF_SAMPLES];
static int16_t g_audio_record_clock[AUDIO_RECORD_PCM_HALF_SAMPLES];
static volatile uint8_t g_audio_record_events;
#if !defined(AUDIO_RECORDING_SERVICE_HOST_TEST)
static FIL g_audio_record_file;
#endif
static uint8_t g_audio_record_file_open;
static uint8_t g_audio_record_sd_session_held;
static char g_audio_record_path[AUDIO_RECORD_PATH_MAX];

static void audio_record_increment(uint32_t *value)
{
    if (*value != UINT32_MAX)
    {
        (*value)++;
    }
}

static int audio_record_acquire(void)
{
#if defined(AUDIO_RECORDING_SERVICE_HOST_TEST)
    return audio_record_test_acquire();
#else
    return resource_acquire(RESOURCE_SAI1, 0x0402U);
#endif
}

static void audio_record_release(void)
{
#if defined(AUDIO_RECORDING_SERVICE_HOST_TEST)
    audio_record_test_release();
#else
    (void)resource_release(RESOURCE_SAI1, 0x0402U);
#endif
}

static int audio_record_acquire_sd_session(void)
{
    if (g_audio_record_sd_session_held != 0U)
    {
        return 0;
    }
#if !defined(AUDIO_RECORDING_SERVICE_HOST_TEST)
    if (sdcard_manager_session_acquire(AUDIO_RECORD_SD_SESSION_OWNER) !=
        SDCARD_MANAGER_OK)
    {
        return 1;
    }
#endif
    g_audio_record_sd_session_held = 1U;
    return 0;
}

static void audio_record_release_sd_session(void)
{
    if (g_audio_record_sd_session_held == 0U)
    {
        return;
    }
#if !defined(AUDIO_RECORDING_SERVICE_HOST_TEST)
    (void)sdcard_manager_session_release(AUDIO_RECORD_SD_SESSION_OWNER);
#endif
    g_audio_record_sd_session_held = 0U;
}

static int audio_record_open_file(const char *path)
{
#if defined(AUDIO_RECORDING_SERVICE_HOST_TEST)
    return audio_record_test_open_file(path);
#else
    return (f_open(&g_audio_record_file, path,
                   FA_CREATE_ALWAYS | FA_WRITE) == FR_OK) ? 0 : 1;
#endif
}

static int audio_record_write_file(const void *data, uint32_t size)
{
#if defined(AUDIO_RECORDING_SERVICE_HOST_TEST)
    return audio_record_test_write_file(data, size);
#else
    UINT written = 0U;

    return ((f_write(&g_audio_record_file, data, (UINT)size, &written) == FR_OK) &&
            (written == (UINT)size)) ? 0 : 1;
#endif
}

static int audio_record_seek_file(uint32_t position)
{
#if defined(AUDIO_RECORDING_SERVICE_HOST_TEST)
    return audio_record_test_seek_file(position);
#else
    return (f_lseek(&g_audio_record_file, (FSIZE_t)position) == FR_OK) ? 0 : 1;
#endif
}

static int audio_record_sync_file(void)
{
#if defined(AUDIO_RECORDING_SERVICE_HOST_TEST)
    return audio_record_test_sync_file();
#else
    return (f_sync(&g_audio_record_file) == FR_OK) ? 0 : 1;
#endif
}

static int audio_record_start_dma(void)
{
#if defined(AUDIO_RECORDING_SERVICE_HOST_TEST)
    if (audio_record_test_start_clock(g_audio_record_clock,
                                      AUDIO_RECORD_PCM_HALF_SAMPLES) != 0)
    {
        return 1;
    }
    return audio_record_test_start_rx(&g_audio_record_pcm[0][0],
                                      AUDIO_RECORD_PCM_HALF_SAMPLES * 2U);
#else
    uintptr_t start = (uintptr_t)g_audio_record_clock;
    uintptr_t end = start + sizeof(g_audio_record_clock);
    uint32_t aligned_start = (uint32_t)(start & ~(uintptr_t)31U);
    uint32_t aligned_end = (uint32_t)((end + 31U) & ~(uintptr_t)31U);

    SCB_CleanDCache_by_Addr((uint32_t *)aligned_start,
                            (int32_t)(aligned_end - aligned_start));
    CLEAR_BIT(hsai_BlockA1.Instance->CR1, SAI_xCR1_OSR);
    if (HAL_SAI_Transmit_DMA(&hsai_BlockA1,
                             (uint8_t *)g_audio_record_clock,
                             AUDIO_RECORD_PCM_HALF_SAMPLES) != HAL_OK)
    {
        return 1;
    }
    if (HAL_SAI_Receive_DMA(&hsai_BlockB1,
                            (uint8_t *)&g_audio_record_pcm[0][0],
                            AUDIO_RECORD_PCM_HALF_SAMPLES * 2U) != HAL_OK)
    {
        (void)HAL_SAI_DMAStop(&hsai_BlockA1);
        return 1;
    }
    return 0;
#endif
}

static void audio_record_stop_dma(void)
{
#if defined(AUDIO_RECORDING_SERVICE_HOST_TEST)
    (void)audio_record_test_stop_rx();
    (void)audio_record_test_stop_clock();
#else
    (void)HAL_SAI_DMAStop(&hsai_BlockB1);
    (void)HAL_SAI_DMAStop(&hsai_BlockA1);
#endif
}

static void audio_record_close_file(void)
{
    if (g_audio_record_file_open == 0U)
    {
        return;
    }
#if defined(AUDIO_RECORDING_SERVICE_HOST_TEST)
    (void)audio_record_test_close_file();
#else
    (void)f_close(&g_audio_record_file);
#endif
    g_audio_record_file_open = 0U;
}

static void audio_record_remove_file(void)
{
#if defined(AUDIO_RECORDING_SERVICE_HOST_TEST)
    (void)audio_record_test_remove_file();
#else
    (void)f_unlink(g_audio_record_path);
#endif
}

static void audio_record_put_u16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)(value & 0xFFU);
    data[1] = (uint8_t)(value >> 8);
}

static void audio_record_put_u32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)(value & 0xFFU);
    data[1] = (uint8_t)((value >> 8) & 0xFFU);
    data[2] = (uint8_t)((value >> 16) & 0xFFU);
    data[3] = (uint8_t)(value >> 24);
}

static void audio_record_make_wav_header(uint8_t *header,
                                         uint32_t data_bytes)
{
    memset(header, 0, AUDIO_RECORD_WAV_HEADER_SIZE);
    memcpy(&header[0], "RIFF", 4U);
    audio_record_put_u32(&header[4], 36U + data_bytes);
    memcpy(&header[8], "WAVE", 4U);
    memcpy(&header[12], "fmt ", 4U);
    audio_record_put_u32(&header[16], 16U);
    audio_record_put_u16(&header[20], 1U);
    audio_record_put_u16(&header[22], 2U);
    audio_record_put_u32(&header[24], 48000U);
    audio_record_put_u32(&header[28], 192000U);
    audio_record_put_u16(&header[32], 4U);
    audio_record_put_u16(&header[34], 16U);
    memcpy(&header[36], "data", 4U);
    audio_record_put_u32(&header[40], data_bytes);
}

static void audio_record_invalidate_cache(const int16_t *buffer,
                                          uint32_t samples)
{
#if !defined(AUDIO_RECORDING_SERVICE_HOST_TEST)
    uintptr_t start = (uintptr_t)buffer;
    uintptr_t end = start + ((uintptr_t)samples * sizeof(int16_t));
    uint32_t aligned_start = (uint32_t)(start & ~(uintptr_t)31U);
    uint32_t aligned_end = (uint32_t)((end + 31U) & ~(uintptr_t)31U);

    SCB_InvalidateDCache_by_Addr((uint32_t *)aligned_start,
                                 (int32_t)(aligned_end - aligned_start));
#else
    (void)buffer;
    (void)samples;
#endif
}

static int audio_record_write_header(uint32_t data_bytes)
{
    uint8_t header[AUDIO_RECORD_WAV_HEADER_SIZE];

    audio_record_make_wav_header(header, data_bytes);
    if ((audio_record_seek_file(0U) != 0) ||
        (audio_record_write_file(header, AUDIO_RECORD_WAV_HEADER_SIZE) != 0))
    {
        return 1;
    }
    return audio_record_sync_file();
}

static int audio_record_write_half(uint8_t half)
{
    uint32_t bytes = AUDIO_RECORD_PCM_HALF_SAMPLES *
                     AUDIO_RECORD_BYTES_PER_SAMPLE;

    if (g_audio_record_status.bytes_written >
        (AUDIO_RECORD_MAX_DATA_BYTES - bytes))
    {
        g_audio_record_status.last_result = AUDIO_RECORD_LIMIT;
        return 1;
    }
    audio_record_invalidate_cache(g_audio_record_pcm[half],
                                  AUDIO_RECORD_PCM_HALF_SAMPLES);
    if (audio_record_write_file(g_audio_record_pcm[half], bytes) != 0)
    {
        audio_record_increment(&g_audio_record_status.write_error_count);
        g_audio_record_status.last_result = AUDIO_RECORD_IO_ERROR;
        return 1;
    }
    g_audio_record_status.bytes_written += bytes;
    g_audio_record_status.sample_count += AUDIO_RECORD_PCM_HALF_SAMPLES;
    return 0;
}

void audio_recording_service_init(void)
{
    memset(&g_audio_record_status, 0, sizeof(g_audio_record_status));
    memset(g_audio_record_clock, 0, sizeof(g_audio_record_clock));
    g_audio_record_status.initialized = 1U;
    g_audio_record_status.state = AUDIO_RECORD_STATE_IDLE;
    g_audio_record_status.sample_rate = 48000U;
    g_audio_record_status.channels = 2U;
    g_audio_record_status.bits_per_sample = 16U;
    g_audio_record_status.last_result = AUDIO_RECORD_NOT_READY;
    g_audio_record_events = 0U;
    g_audio_record_file_open = 0U;
    g_audio_record_sd_session_held = 0U;
    g_audio_record_path[0] = '\0';
}

int audio_record_start(const char *path)
{
    size_t path_length;
    uint8_t header[AUDIO_RECORD_WAV_HEADER_SIZE];

    if ((path == NULL) || (path[0] == '\0'))
    {
        return AUDIO_RECORD_INVALID_ARG;
    }
    path_length = strlen(path);
    if (path_length >= AUDIO_RECORD_PATH_MAX)
    {
        return AUDIO_RECORD_INVALID_ARG;
    }
    if (g_audio_record_status.state == AUDIO_RECORD_STATE_RECORDING)
    {
        return AUDIO_RECORD_BUSY;
    }
    if (audio_record_acquire_sd_session() != 0)
    {
        g_audio_record_status.last_result = AUDIO_RECORD_BUSY;
        return AUDIO_RECORD_BUSY;
    }
    if (audio_record_acquire() != 0)
    {
        audio_record_release_sd_session();
        g_audio_record_status.last_result = AUDIO_RECORD_BUSY;
        return AUDIO_RECORD_BUSY;
    }
    if ((es8388_probe_readonly(10U) != ES8388_OK) ||
        (es8388_prepare_recording() != ES8388_OK))
    {
        audio_record_release();
        audio_record_release_sd_session();
        g_audio_record_status.last_result = AUDIO_RECORD_CODEC_ERROR;
        return AUDIO_RECORD_CODEC_ERROR;
    }
    if (audio_record_open_file(path) != 0)
    {
        audio_record_release();
        audio_record_release_sd_session();
        g_audio_record_status.last_result = AUDIO_RECORD_IO_ERROR;
        return AUDIO_RECORD_IO_ERROR;
    }
    memcpy(g_audio_record_path, path, path_length + 1U);
    g_audio_record_file_open = 1U;
    audio_record_make_wav_header(header, 0U);
    if (audio_record_write_file(header, AUDIO_RECORD_WAV_HEADER_SIZE) != 0)
    {
        audio_record_close_file();
        audio_record_remove_file();
        audio_record_release();
        audio_record_release_sd_session();
        g_audio_record_status.last_result = AUDIO_RECORD_IO_ERROR;
        return AUDIO_RECORD_IO_ERROR;
    }
    g_audio_record_events = 0U;
    g_audio_record_status.bytes_written = 0U;
    g_audio_record_status.sample_count = 0U;
    if (audio_record_start_dma() != 0)
    {
        audio_record_close_file();
        audio_record_remove_file();
        audio_record_release();
        audio_record_release_sd_session();
        g_audio_record_status.last_result = AUDIO_RECORD_DMA_ERROR;
        return AUDIO_RECORD_DMA_ERROR;
    }
    g_audio_record_status.state = AUDIO_RECORD_STATE_RECORDING;
    g_audio_record_status.last_result = AUDIO_RECORD_OK;
    return AUDIO_RECORD_OK;
}

int audio_record_stop(void)
{
    int result = AUDIO_RECORD_OK;

    if (g_audio_record_status.state != AUDIO_RECORD_STATE_RECORDING)
    {
        return AUDIO_RECORD_OK;
    }
    audio_record_stop_dma();
    if (audio_record_write_header(g_audio_record_status.bytes_written) != 0)
    {
        audio_record_increment(&g_audio_record_status.write_error_count);
        result = AUDIO_RECORD_IO_ERROR;
    }
    audio_record_close_file();
    audio_record_release();
    audio_record_release_sd_session();
    g_audio_record_status.state = AUDIO_RECORD_STATE_IDLE;
    g_audio_record_status.last_result = result;
    g_audio_record_events = 0U;
    return result;
}

void audio_record_poll(void)
{
    uint8_t events;

    if (g_audio_record_status.state != AUDIO_RECORD_STATE_RECORDING)
    {
        return;
    }
    events = g_audio_record_events;
    g_audio_record_events = 0U;
    g_audio_record_status.pending_events = events;
    if ((events & AUDIO_RECORD_EVENT_ERROR) != 0U)
    {
        audio_record_increment(&g_audio_record_status.dma_error_count);
        g_audio_record_status.state = AUDIO_RECORD_STATE_ERROR;
        g_audio_record_status.last_result = AUDIO_RECORD_DMA_ERROR;
        audio_record_stop_dma();
        audio_record_close_file();
        audio_record_release();
        audio_record_release_sd_session();
        return;
    }
    if ((events & (AUDIO_RECORD_EVENT_HALF | AUDIO_RECORD_EVENT_FULL)) != 0U)
    {
        if ((events & AUDIO_RECORD_EVENT_HALF) != 0U)
        {
            if (audio_record_write_half(0U) != 0)
            {
                int32_t result = g_audio_record_status.last_result;
                (void)audio_record_stop();
                g_audio_record_status.last_result = result;
                return;
            }
        }
        if ((events & AUDIO_RECORD_EVENT_FULL) != 0U)
        {
            if (audio_record_write_half(1U) != 0)
            {
                int32_t result = g_audio_record_status.last_result;
                (void)audio_record_stop();
                g_audio_record_status.last_result = result;
                return;
            }
        }
    }
    g_audio_record_status.pending_events = 0U;
}

void audio_record_get_status(audio_recording_status_t *status)
{
    if (status != NULL)
    {
        *status = g_audio_record_status;
    }
}

void audio_record_print_status(void)
{
    audio_recording_status_t status;

    audio_record_get_status(&status);
    printf("audio record status initialized=%u state=%u sample_rate=%lu channels=%u bits=%u bytes_written=%lu sample_count=%lu dma_errors=%lu write_errors=%lu overrun_count=%lu result=%ld\r\n",
           (unsigned int)status.initialized,
           (unsigned int)status.state,
           (unsigned long)status.sample_rate,
           (unsigned int)status.channels,
           (unsigned int)status.bits_per_sample,
           (unsigned long)status.bytes_written,
           (unsigned long)status.sample_count,
           (unsigned long)status.dma_error_count,
           (unsigned long)status.write_error_count,
           (unsigned long)status.overrun_count,
           (long)status.last_result);
}

void audio_record_on_rx_half_complete(void)
{
    if ((g_audio_record_events & AUDIO_RECORD_EVENT_HALF) != 0U)
    {
        audio_record_increment(&g_audio_record_status.overrun_count);
    }
    g_audio_record_events |= AUDIO_RECORD_EVENT_HALF;
}

void audio_record_on_rx_complete(void)
{
    if ((g_audio_record_events & AUDIO_RECORD_EVENT_FULL) != 0U)
    {
        audio_record_increment(&g_audio_record_status.overrun_count);
    }
    g_audio_record_events |= AUDIO_RECORD_EVENT_FULL;
}

void audio_record_on_rx_error(void)
{
    g_audio_record_events |= AUDIO_RECORD_EVENT_ERROR;
}

#if !defined(AUDIO_RECORDING_SERVICE_HOST_TEST)
void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
    if ((hsai != NULL) && (hsai->Instance == SAI1_Block_B))
    {
        audio_record_on_rx_half_complete();
    }
}

void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai)
{
    if ((hsai != NULL) && (hsai->Instance == SAI1_Block_B))
    {
        audio_record_on_rx_complete();
    }
}
#endif
