#include "wav_playback_service.h"

#include <stdint.h>
#include <string.h>

#include "audio_playback_service.h"
#include "ff.h"
#include "sdcard_manager.h"
#include "shared_buffer.h"

#define WAV_SD_OWNER 0x5741U
#define WAV_BUFFER_SIZE 4096U
#define WAV_HEADER_SCAN_LIMIT 65536UL

typedef struct
{
    FIL file;
    uint8_t *buffer;
    uint32_t data_remaining;
    uint32_t pending_bytes;
    uint8_t active;
    uint8_t file_open;
    uint8_t session_held;
    uint8_t buffer_held;
} wav_playback_context_t;

static wav_playback_context_t g_wav;

static uint16_t wav_u16(const uint8_t *data)
{ return (uint16_t)data[0] | ((uint16_t)data[1] << 8); }

static uint32_t wav_u32(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

static void wav_release(void)
{
    if (g_wav.file_open != 0U) { (void)f_close(&g_wav.file); g_wav.file_open = 0U; }
    if (g_wav.session_held != 0U)
    { (void)sdcard_manager_session_release(WAV_SD_OWNER); g_wav.session_held = 0U; }
    if (g_wav.buffer_held != 0U)
    { (void)shared_buffer_release(SHARED_BUFFER_OWNER_WAV_PLAYER); g_wav.buffer_held = 0U; }
    g_wav.active = 0U;
}

static int wav_read_exact(void *data, uint32_t size)
{
    UINT actual = 0U;
    return ((f_read(&g_wav.file, data, (UINT)size, &actual) == FR_OK) &&
            (actual == size)) ? 0 : 1;
}

int wav_playback_start(const char *path)
{
    uint8_t header[12];
    uint8_t chunk[8];
    uint8_t format[16];
    uint32_t scanned = 12U;
    uint32_t sample_rate = 0U;
    uint16_t channels = 0U;
    uint8_t format_ready = 0U;

    if ((path == NULL) || (path[0] == '\0') || (g_wav.active != 0U)) return 1;
    memset(&g_wav, 0, sizeof(g_wav));
    if (sdcard_manager_session_acquire(WAV_SD_OWNER) != SDCARD_MANAGER_OK) return 1;
    g_wav.session_held = 1U;
    if (shared_buffer_acquire(SHARED_BUFFER_OWNER_WAV_PLAYER,
                              WAV_BUFFER_SIZE, (void **)&g_wav.buffer) !=
        SHARED_BUFFER_OK) { wav_release(); return 1; }
    g_wav.buffer_held = 1U;
    if (f_open(&g_wav.file, path, FA_READ) != FR_OK) { wav_release(); return 1; }
    g_wav.file_open = 1U;
    if ((wav_read_exact(header, sizeof(header)) != 0) ||
        (memcmp(header, "RIFF", 4U) != 0) ||
        (memcmp(&header[8], "WAVE", 4U) != 0)) { wav_release(); return 1; }
    while (scanned < WAV_HEADER_SCAN_LIMIT)
    {
        uint32_t size;
        if (wav_read_exact(chunk, sizeof(chunk)) != 0) break;
        scanned += 8U;
        size = wav_u32(&chunk[4]);
        if (memcmp(chunk, "fmt ", 4U) == 0)
        {
            if ((size < sizeof(format)) || (wav_read_exact(format, sizeof(format)) != 0))
                break;
            if ((wav_u16(format) != 1U) ||
                ((channels = wav_u16(&format[2])) == 0U) || (channels > 2U) ||
                (wav_u16(&format[14]) != 16U)) break;
            sample_rate = wav_u32(&format[4]);
            if ((size > sizeof(format)) &&
                (f_lseek(&g_wav.file, f_tell(&g_wav.file) +
                         size - sizeof(format)) != FR_OK)) break;
            format_ready = 1U;
        }
        else if (memcmp(chunk, "data", 4U) == 0)
        {
            if ((format_ready == 0U) || (sample_rate == 0U)) break;
            g_wav.data_remaining = size;
            if (audio_stream_start(AUDIO_STREAM_FORMAT_PCM, sample_rate,
                                   (uint8_t)channels, 16U) != AUDIO_OK)
            { wav_release(); return 1; }
            g_wav.active = 1U;
            return 0;
        }
        else if (f_lseek(&g_wav.file, f_tell(&g_wav.file) + size) != FR_OK)
            break;
        if ((size & 1U) != 0U)
        {
            if (f_lseek(&g_wav.file, f_tell(&g_wav.file) + 1U) != FR_OK) break;
            size++;
        }
        scanned += size;
    }
    wav_release();
    return 1;
}

void wav_playback_poll(void)
{
    UINT actual = 0U;
    uint32_t requested;
    int result;

    if (g_wav.active == 0U) return;
    if (g_wav.pending_bytes != 0U)
    {
        result = audio_stream_push(g_wav.buffer, g_wav.pending_bytes);
        if (result == AUDIO_BUSY) return;
        if (result != AUDIO_OK) { (void)audio_stream_stop(); wav_release(); return; }
        g_wav.pending_bytes = 0U;
    }
    if (g_wav.data_remaining == 0U)
    {
        audio_stream_finish();
        wav_release();
        return;
    }
    requested = (g_wav.data_remaining < WAV_BUFFER_SIZE) ?
                g_wav.data_remaining : WAV_BUFFER_SIZE;
    if ((f_read(&g_wav.file, g_wav.buffer, (UINT)requested, &actual) != FR_OK) ||
        (actual == 0U)) { (void)audio_stream_stop(); wav_release(); return; }
    g_wav.data_remaining -= (uint32_t)actual;
    g_wav.pending_bytes = (uint32_t)actual;
}

void wav_playback_stop(void)
{
    if (g_wav.active != 0U) (void)audio_stream_stop();
    wav_release();
}
