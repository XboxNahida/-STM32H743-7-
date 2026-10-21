#include "mp3_decoder_service.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#define MP3_DECODER_PATH_MAX 128U
#define MP3_DECODER_INPUT_SIZE 8192U
#define MP3_DECODER_PCM_SIZE 2304U
#define MP3_SD_SESSION_OWNER 0x0501U
#define MP3_DECODER_MAIN_DATA_UNDERFLOW (-2)
#define MP3_DECODER_MAIN_DATA_RETRY_LIMIT 16U

static mp3_decoder_status_t g_mp3_decoder_status;
static uint8_t g_mp3_sd_session_held;
static char g_mp3_path[MP3_DECODER_PATH_MAX];
static uint8_t g_mp3_resume_warmup;

#if defined(MP3_DECODER_SERVICE_HOST_TEST)
static uint8_t g_host_frame_pending;
#else
#include "ff.h"
#include "mp3dec.h"
#include "sdcard_manager.h"

static FIL g_mp3_file;
static HMP3Decoder g_mp3_decoder;
static uint8_t g_mp3_input[MP3_DECODER_INPUT_SIZE];
static uint8_t *g_mp3_read_ptr;
static int g_mp3_bytes_left;
static uint8_t g_mp3_eof;
static uint64_t g_mp3_audio_bytes;
static int16_t g_mp3_pcm[MP3_DECODER_PCM_SIZE];
#endif

static int mp3_decoder_acquire_session(void)
{
    if (g_mp3_sd_session_held != 0U)
    {
        return 0;
    }
#if !defined(MP3_DECODER_SERVICE_HOST_TEST)
    if (sdcard_manager_session_acquire(MP3_SD_SESSION_OWNER) !=
        SDCARD_MANAGER_OK)
    {
        return 1;
    }
#endif
    g_mp3_sd_session_held = 1U;
    return 0;
}

static void mp3_decoder_release_session(void)
{
    if (g_mp3_sd_session_held == 0U)
    {
        return;
    }
#if !defined(MP3_DECODER_SERVICE_HOST_TEST)
    (void)sdcard_manager_session_release(MP3_SD_SESSION_OWNER);
#endif
    g_mp3_sd_session_held = 0U;
}

static void mp3_decoder_increment_saturated(uint32_t *value)
{
    if (*value != UINT32_MAX)
    {
        (*value)++;
    }
}

static int mp3_decoder_should_retry_main_data(int error,
                                              uint32_t retry_count)
{
    return ((error == MP3_DECODER_MAIN_DATA_UNDERFLOW) &&
            (retry_count < MP3_DECODER_MAIN_DATA_RETRY_LIMIT)) ? 1 : 0;
}

static int mp3_decoder_id3v2_offset(const uint8_t *header,
                                    uint32_t header_size,
                                    uint64_t file_size,
                                    uint32_t *offset)
{
    uint32_t tag_size;
    uint32_t data_offset;

    if ((header == NULL) || (offset == NULL) || (header_size < 3U))
    {
        return MP3_DECODER_INVALID_ARG;
    }
    *offset = 0U;
    if (memcmp(header, "ID3", 3U) != 0)
    {
        return MP3_DECODER_OK;
    }
    if ((header_size < 10U) || (header[3] < 2U) || (header[3] > 4U) ||
        (((header[6] | header[7] | header[8] | header[9]) & 0x80U) != 0U))
    {
        return MP3_DECODER_FORMAT_ERROR;
    }
    tag_size = ((uint32_t)header[6] << 21) |
               ((uint32_t)header[7] << 14) |
               ((uint32_t)header[8] << 7) |
               (uint32_t)header[9];
    data_offset = 10U + tag_size;
    if ((header[3] == 4U) && ((header[5] & 0x10U) != 0U))
    {
        data_offset += 10U;
    }
    if ((uint64_t)data_offset > file_size)
    {
        return MP3_DECODER_FORMAT_ERROR;
    }
    *offset = data_offset;
    return MP3_DECODER_OK;
}

#if defined(MP3_DECODER_SERVICE_HOST_TEST)
int mp3_decoder_test_id3v2_offset(const uint8_t *header,
                                  uint32_t header_size,
                                  uint32_t file_size,
                                  uint32_t *offset)
{
    return mp3_decoder_id3v2_offset(header, header_size,
                                    (uint64_t)file_size, offset);
}

int mp3_decoder_test_should_retry_main_data(int error,
                                            uint32_t retry_count)
{
    return mp3_decoder_should_retry_main_data(error, retry_count);
}
#endif

void mp3_decoder_service_init(void)
{
    memset(&g_mp3_decoder_status, 0, sizeof(g_mp3_decoder_status));
    g_mp3_decoder_status.initialized = 1U;
    g_mp3_decoder_status.last_result = MP3_DECODER_NOT_READY;
    g_mp3_sd_session_held = 0U;
    g_mp3_path[0] = '\0';
    g_mp3_resume_warmup = 0U;
#if defined(MP3_DECODER_SERVICE_HOST_TEST)
    g_host_frame_pending = 1U;
#else
    g_mp3_decoder = NULL;
    g_mp3_read_ptr = g_mp3_input;
    g_mp3_bytes_left = 0;
    g_mp3_eof = 0U;
    g_mp3_audio_bytes = 0U;
#endif
}

int mp3_decoder_open(const char *path)
{
    size_t length;

    if (path == NULL)
    {
        return MP3_DECODER_INVALID_ARG;
    }
    length = strlen(path);
    if ((length == 0U) || (length >= MP3_DECODER_PATH_MAX))
    {
        return MP3_DECODER_INVALID_ARG;
    }
    if (mp3_decoder_acquire_session() != 0)
    {
        g_mp3_decoder_status.last_result = MP3_DECODER_IO_ERROR;
        return MP3_DECODER_IO_ERROR;
    }

#if defined(MP3_DECODER_SERVICE_HOST_TEST)
    memcpy(g_mp3_path, path, length + 1U);
    g_host_frame_pending = 1U;
    g_mp3_decoder_status.opened = 1U;
    g_mp3_decoder_status.suspended = 0U;
    g_mp3_decoder_status.resume_offset = 0U;
    g_mp3_decoder_status.channels = 2U;
    g_mp3_decoder_status.sample_rate = 48000U;
    g_mp3_decoder_status.bitrate = 128000U;
    g_mp3_decoder_status.duration_ms = 120000U;
    g_mp3_decoder_status.last_result = MP3_DECODER_OK;
    return MP3_DECODER_OK;
#else
    uint8_t header[10];
    UINT bytes_read = 0U;
    uint32_t data_offset = 0U;
    int offset_result;

    if (f_open(&g_mp3_file, path, FA_READ) != FR_OK)
    {
        mp3_decoder_release_session();
        g_mp3_decoder_status.last_result = MP3_DECODER_IO_ERROR;
        return MP3_DECODER_IO_ERROR;
    }
    if (f_read(&g_mp3_file, header, sizeof(header), &bytes_read) != FR_OK)
    {
        (void)f_close(&g_mp3_file);
        mp3_decoder_release_session();
        g_mp3_decoder_status.last_result = MP3_DECODER_IO_ERROR;
        return MP3_DECODER_IO_ERROR;
    }
    offset_result = mp3_decoder_id3v2_offset(header, (uint32_t)bytes_read,
                                              (uint64_t)f_size(&g_mp3_file),
                                              &data_offset);
    if (offset_result != MP3_DECODER_OK)
    {
        (void)f_close(&g_mp3_file);
        mp3_decoder_release_session();
        g_mp3_decoder_status.last_result = MP3_DECODER_FORMAT_ERROR;
        return MP3_DECODER_FORMAT_ERROR;
    }
    if (f_lseek(&g_mp3_file, (FSIZE_t)data_offset) != FR_OK)
    {
        (void)f_close(&g_mp3_file);
        mp3_decoder_release_session();
        g_mp3_decoder_status.last_result = MP3_DECODER_IO_ERROR;
        return MP3_DECODER_IO_ERROR;
    }
    g_mp3_decoder_status.channels = 0U;
    g_mp3_decoder_status.sample_rate = 0U;
    g_mp3_decoder_status.bitrate = 0U;
    g_mp3_decoder_status.duration_ms = 0U;
    g_mp3_decoder_status.frame_count = 0U;
    g_mp3_audio_bytes = (uint64_t)f_size(&g_mp3_file) - data_offset;
    g_mp3_decoder = MP3InitDecoder();
    if (g_mp3_decoder == NULL)
    {
        (void)f_close(&g_mp3_file);
        mp3_decoder_release_session();
        g_mp3_decoder_status.last_result = MP3_DECODER_ERROR;
        return MP3_DECODER_ERROR;
    }
    g_mp3_read_ptr = g_mp3_input;
    g_mp3_bytes_left = 0;
    g_mp3_eof = 0U;
    g_mp3_decoder_status.opened = 1U;
    g_mp3_decoder_status.suspended = 0U;
    g_mp3_decoder_status.resume_offset = data_offset;
    g_mp3_resume_warmup = 0U;
    memcpy(g_mp3_path, path, length + 1U);
    g_mp3_decoder_status.last_result = MP3_DECODER_OK;
    return MP3_DECODER_OK;
#endif
}

int mp3_decoder_decode_next(int16_t *pcm,
                            uint32_t capacity,
                            uint32_t *samples,
                            uint8_t *channels)
{
    if ((pcm == NULL) || (samples == NULL) || (channels == NULL) ||
        (capacity == 0U))
    {
        return MP3_DECODER_INVALID_ARG;
    }
    if (g_mp3_decoder_status.opened == 0U)
    {
        return MP3_DECODER_NOT_READY;
    }

#if defined(MP3_DECODER_SERVICE_HOST_TEST)
    if (g_host_frame_pending == 0U)
    {
        return MP3_DECODER_EOF;
    }
    if (capacity < 4U)
    {
        return MP3_DECODER_INVALID_ARG;
    }
    pcm[0] = 0;
    pcm[1] = 0;
    pcm[2] = 0;
    pcm[3] = 0;
    *samples = 4U;
    *channels = 2U;
    g_host_frame_pending = 0U;
    g_mp3_decoder_status.channels = *channels;
    g_mp3_decoder_status.sample_rate = 48000U;
    mp3_decoder_increment_saturated(&g_mp3_decoder_status.frame_count);
    g_mp3_decoder_status.last_result = MP3_DECODER_OK;
    return MP3_DECODER_OK;
#else
    {
        MP3FrameInfo frame_info;
        int offset;
        int error;
        UINT bytes_read;
        uint32_t main_data_retry_count = 0U;

        for (;;)
        {
            while (g_mp3_bytes_left <
                   (int)(MP3_DECODER_INPUT_SIZE / 2U))
            {
                if (g_mp3_eof != 0U)
                {
                    break;
                }
                if (g_mp3_bytes_left > 0)
                {
                    memmove(g_mp3_input, g_mp3_read_ptr,
                            (size_t)g_mp3_bytes_left);
                }
                g_mp3_read_ptr = g_mp3_input;
                if (f_read(&g_mp3_file,
                           g_mp3_input + g_mp3_bytes_left,
                           MP3_DECODER_INPUT_SIZE -
                           (UINT)g_mp3_bytes_left,
                           &bytes_read) != FR_OK)
                {
                    g_mp3_decoder_status.last_result =
                        MP3_DECODER_IO_ERROR;
                    return MP3_DECODER_IO_ERROR;
                }
                g_mp3_bytes_left += (int)bytes_read;
                if (bytes_read == 0U)
                {
                    g_mp3_eof = 1U;
                    break;
                }
            }

            offset = MP3FindSyncWord(g_mp3_read_ptr, g_mp3_bytes_left);
            if (offset < 0)
            {
                g_mp3_decoder_status.last_result = g_mp3_eof ?
                                                    MP3_DECODER_EOF :
                                                    MP3_DECODER_FORMAT_ERROR;
                return g_mp3_decoder_status.last_result;
            }
            g_mp3_read_ptr += offset;
            g_mp3_bytes_left -= offset;
            error = MP3Decode(g_mp3_decoder, &g_mp3_read_ptr,
                              &g_mp3_bytes_left, g_mp3_pcm, 0);
            if (error == 0)
            {
                g_mp3_resume_warmup = 0U;
                break;
            }
            if ((g_mp3_resume_warmup != 0U) &&
                (mp3_decoder_should_retry_main_data(
                     error, main_data_retry_count) != 0))
            {
                main_data_retry_count++;
                continue;
            }
            g_mp3_decoder_status.last_result = MP3_DECODER_FORMAT_ERROR;
            return MP3_DECODER_FORMAT_ERROR;
        }
        MP3GetLastFrameInfo(g_mp3_decoder, &frame_info);
        if (frame_info.samprate == 0)
        {
            g_mp3_decoder_status.last_result = MP3_DECODER_FORMAT_ERROR;
            return MP3_DECODER_FORMAT_ERROR;
        }
        if (frame_info.nChans == 1)
        {
            uint32_t index;
            uint32_t output_samples = (uint32_t)frame_info.outputSamps * 2U;
            if (capacity < output_samples)
            {
                return MP3_DECODER_INVALID_ARG;
            }
            for (index = 0U; index < (uint32_t)frame_info.outputSamps;
                 index++)
            {
                pcm[index * 2U] = g_mp3_pcm[index];
                pcm[index * 2U + 1U] = g_mp3_pcm[index];
            }
            *samples = output_samples;
            *channels = 2U;
        }
        else
        {
            uint32_t output_samples = (uint32_t)frame_info.outputSamps;
            if (capacity < output_samples)
            {
                return MP3_DECODER_INVALID_ARG;
            }
            memcpy(pcm, g_mp3_pcm, output_samples * sizeof(int16_t));
            *samples = output_samples;
            *channels = 2U;
        }
        g_mp3_decoder_status.channels = *channels;
        g_mp3_decoder_status.sample_rate = (uint32_t)frame_info.samprate;
        if (frame_info.bitrate > 0)
        {
            uint64_t duration_ms =
                ((uint64_t)g_mp3_audio_bytes * 8000ULL) /
                (uint32_t)frame_info.bitrate;

            g_mp3_decoder_status.bitrate = (uint32_t)frame_info.bitrate;
            g_mp3_decoder_status.duration_ms =
                (duration_ms > UINT32_MAX) ? UINT32_MAX :
                (uint32_t)duration_ms;
        }
        mp3_decoder_increment_saturated(&g_mp3_decoder_status.frame_count);
        g_mp3_decoder_status.last_result = MP3_DECODER_OK;
        return MP3_DECODER_OK;
    }
#endif
}

int mp3_decoder_suspend(void)
{
    if (g_mp3_decoder_status.opened == 0U)
    {
        return (g_mp3_decoder_status.suspended != 0U) ?
               MP3_DECODER_OK : MP3_DECODER_NOT_READY;
    }
#if defined(MP3_DECODER_SERVICE_HOST_TEST)
    g_mp3_decoder_status.resume_offset =
        (g_mp3_decoder_status.frame_count + 1U) * 417U;
#else
    {
        FSIZE_t file_position = f_tell(&g_mp3_file);

        if ((g_mp3_bytes_left > 0) &&
            (file_position >= (FSIZE_t)g_mp3_bytes_left))
        {
            file_position -= (FSIZE_t)g_mp3_bytes_left;
        }
        g_mp3_decoder_status.resume_offset =
            (file_position > UINT32_MAX) ? UINT32_MAX :
            (uint32_t)file_position;
    }
    MP3FreeDecoder(g_mp3_decoder);
    g_mp3_decoder = NULL;
    (void)f_close(&g_mp3_file);
#endif
    mp3_decoder_release_session();
    g_mp3_decoder_status.opened = 0U;
    g_mp3_decoder_status.suspended = 1U;
    g_mp3_resume_warmup = 0U;
    g_mp3_decoder_status.last_result = MP3_DECODER_OK;
    return MP3_DECODER_OK;
}

int mp3_decoder_resume(void)
{
    if ((g_mp3_decoder_status.suspended == 0U) ||
        (g_mp3_path[0] == '\0'))
    {
        return MP3_DECODER_NOT_READY;
    }
    if (mp3_decoder_acquire_session() != 0)
    {
        g_mp3_decoder_status.last_result = MP3_DECODER_IO_ERROR;
        return MP3_DECODER_IO_ERROR;
    }
#if defined(MP3_DECODER_SERVICE_HOST_TEST)
    g_host_frame_pending = 1U;
#else
    if (f_open(&g_mp3_file, g_mp3_path, FA_READ) != FR_OK)
    {
        mp3_decoder_release_session();
        g_mp3_decoder_status.last_result = MP3_DECODER_IO_ERROR;
        return MP3_DECODER_IO_ERROR;
    }
    if ((g_mp3_decoder_status.resume_offset >= f_size(&g_mp3_file)) ||
        (f_lseek(&g_mp3_file,
                 (FSIZE_t)g_mp3_decoder_status.resume_offset) != FR_OK))
    {
        (void)f_close(&g_mp3_file);
        mp3_decoder_release_session();
        g_mp3_decoder_status.last_result = MP3_DECODER_IO_ERROR;
        return MP3_DECODER_IO_ERROR;
    }
    g_mp3_decoder = MP3InitDecoder();
    if (g_mp3_decoder == NULL)
    {
        (void)f_close(&g_mp3_file);
        mp3_decoder_release_session();
        g_mp3_decoder_status.last_result = MP3_DECODER_ERROR;
        return MP3_DECODER_ERROR;
    }
    g_mp3_read_ptr = g_mp3_input;
    g_mp3_bytes_left = 0;
    g_mp3_eof = 0U;
#endif
    g_mp3_decoder_status.opened = 1U;
    g_mp3_decoder_status.suspended = 0U;
    g_mp3_resume_warmup = 1U;
    g_mp3_decoder_status.last_result = MP3_DECODER_OK;
    return MP3_DECODER_OK;
}

int mp3_decoder_close(void)
{
    if (g_mp3_decoder_status.opened != 0U)
    {
#if !defined(MP3_DECODER_SERVICE_HOST_TEST)
        MP3FreeDecoder(g_mp3_decoder);
        g_mp3_decoder = NULL;
        (void)f_close(&g_mp3_file);
#endif
    }
    mp3_decoder_release_session();
    g_mp3_decoder_status.opened = 0U;
    g_mp3_decoder_status.suspended = 0U;
    g_mp3_decoder_status.resume_offset = 0U;
    g_mp3_path[0] = '\0';
    g_mp3_resume_warmup = 0U;
    g_mp3_decoder_status.last_result = MP3_DECODER_OK;
    return MP3_DECODER_OK;
}

void mp3_decoder_get_status(mp3_decoder_status_t *status)
{
    if (status != NULL)
    {
        *status = g_mp3_decoder_status;
    }
}

void mp3_decoder_print_status(void)
{
    mp3_decoder_status_t status;

    mp3_decoder_get_status(&status);
    printf("mp3 status initialized=%u opened=%u channels=%u sample_rate=%lu bitrate=%lu duration_ms=%lu frame_count=%lu result=%ld\r\n",
           (unsigned int)status.initialized,
           (unsigned int)status.opened,
           (unsigned int)status.channels,
           (unsigned long)status.sample_rate,
           (unsigned long)status.bitrate,
           (unsigned long)status.duration_ms,
           (unsigned long)status.frame_count,
           (long)status.last_result);
}
