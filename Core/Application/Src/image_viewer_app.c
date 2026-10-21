#include "image_viewer_app.h"
#include "sdram_layout.h"

#include <stdio.h>
#include <string.h>

#define IMAGE_VIEWER_SD_SESSION_OWNER 0x0503U

#if defined(IMAGE_VIEWER_HOST_TEST)
extern int image_viewer_platform_buffer_acquire(uint32_t size, void **address);
extern void image_viewer_platform_buffer_release(void);
extern int image_viewer_platform_file_open(const char *path);
extern int image_viewer_platform_file_size(uint32_t *size);
extern int image_viewer_platform_file_read(uint8_t *buffer,
                                           uint32_t request,
                                           uint32_t *bytes_read);
extern void image_viewer_platform_file_close(void);
extern int image_viewer_platform_decode(uint8_t *buffer,
                                        uint32_t size,
                                        uint16_t width,
                                        uint16_t height,
                                        uint32_t target);
extern int image_viewer_platform_dir_open(const char *path);
extern int image_viewer_platform_dir_read(char *name,
                                          uint32_t name_size,
                                          uint8_t *is_directory,
                                          uint8_t *end);
extern void image_viewer_platform_dir_close(void);
#else
#include "ff.h"
#include "mjpeg.h"
#include "sdcard_manager.h"
#include "shared_buffer.h"

static FIL g_image_file;
static DIR g_image_dir;
static FILINFO g_image_file_info;

static int image_viewer_platform_buffer_acquire(uint32_t size, void **address)
{
    shared_buffer_result_t result =
        shared_buffer_acquire(SHARED_BUFFER_OWNER_IMAGE_VIEWER,
                              size,
                              address);

    if (result == SHARED_BUFFER_OK)
    {
        return IMAGE_VIEWER_OK;
    }
    return (result == SHARED_BUFFER_BUSY) ?
           IMAGE_VIEWER_BUSY : IMAGE_VIEWER_IO_ERROR;
}

static void image_viewer_platform_buffer_release(void)
{
    (void)shared_buffer_release(SHARED_BUFFER_OWNER_IMAGE_VIEWER);
}

static int image_viewer_platform_file_open(const char *path)
{
    return (f_open(&g_image_file, path, FA_READ) == FR_OK) ?
           IMAGE_VIEWER_OK : IMAGE_VIEWER_IO_ERROR;
}

static int image_viewer_platform_file_size(uint32_t *size)
{
    if (size == NULL)
    {
        return IMAGE_VIEWER_INVALID_ARG;
    }
    *size = (uint32_t)f_size(&g_image_file);
    return IMAGE_VIEWER_OK;
}

static int image_viewer_platform_file_read(uint8_t *buffer,
                                           uint32_t request,
                                           uint32_t *bytes_read)
{
    UINT actual = 0U;
    FRESULT result;

    if ((buffer == NULL) || (bytes_read == NULL))
    {
        return IMAGE_VIEWER_INVALID_ARG;
    }
    result = f_read(&g_image_file, buffer, (UINT)request, &actual);
    *bytes_read = (uint32_t)actual;
    return (result == FR_OK) ? IMAGE_VIEWER_OK : IMAGE_VIEWER_IO_ERROR;
}

static void image_viewer_platform_file_close(void)
{
    (void)f_close(&g_image_file);
}

static int image_viewer_platform_dir_open(const char *path)
{
    return (f_opendir(&g_image_dir, path) == FR_OK) ?
           IMAGE_VIEWER_OK : IMAGE_VIEWER_IO_ERROR;
}

static int image_viewer_platform_dir_read(char *name,
                                          uint32_t name_size,
                                          uint8_t *is_directory,
                                          uint8_t *end)
{
    size_t length;

    if ((name == NULL) || (name_size == 0U) ||
        (is_directory == NULL) || (end == NULL))
    {
        return IMAGE_VIEWER_INVALID_ARG;
    }
    if (f_readdir(&g_image_dir, &g_image_file_info) != FR_OK)
    {
        return IMAGE_VIEWER_IO_ERROR;
    }
    if (g_image_file_info.fname[0] == '\0')
    {
        name[0] = '\0';
        *is_directory = 0U;
        *end = 1U;
        return IMAGE_VIEWER_OK;
    }

    length = strlen(g_image_file_info.fname);
    if (length >= name_size)
    {
        return IMAGE_VIEWER_PATH_TOO_LONG;
    }
    memcpy(name, g_image_file_info.fname, length + 1U);
    *is_directory = ((g_image_file_info.fattrib & AM_DIR) != 0U) ? 1U : 0U;
    *end = 0U;
    return IMAGE_VIEWER_OK;
}

static void image_viewer_platform_dir_close(void)
{
    (void)f_closedir(&g_image_dir);
}

static int image_viewer_platform_decode(uint8_t *buffer,
                                        uint32_t size,
                                        uint16_t width,
                                        uint16_t height,
                                        uint32_t target)
{
    uint32_t cache_address;
    uint32_t cache_size;
    mjpeg_result_t result;

    cache_address = ((uint32_t)buffer) & ~0x1FUL;
    cache_size = ((((uint32_t)buffer) & 0x1FUL) + size + 31U) & ~0x1FUL;
    SCB_CleanInvalidateDCache_by_Addr((uint32_t *)cache_address, cache_size);
    __DSB();

    result = mjpeg_session_open(width, height);
    if (result != MJPEG_OK)
    {
        return IMAGE_VIEWER_DECODE_ERROR;
    }
    result = mjpeg_session_decode(buffer, size, target, 1000U);
    mjpeg_session_close();
    return (result == MJPEG_OK) ?
           IMAGE_VIEWER_OK : IMAGE_VIEWER_DECODE_ERROR;
}
#endif

static uint8_t g_image_viewer_sd_session_held;

static int image_viewer_acquire_sd_session(void)
{
    if (g_image_viewer_sd_session_held != 0U)
    {
        return IMAGE_VIEWER_OK;
    }
#if !defined(IMAGE_VIEWER_HOST_TEST)
    if (sdcard_manager_session_acquire(IMAGE_VIEWER_SD_SESSION_OWNER) !=
        SDCARD_MANAGER_OK)
    {
        return IMAGE_VIEWER_BUSY;
    }
#endif
    g_image_viewer_sd_session_held = 1U;
    return IMAGE_VIEWER_OK;
}

static void image_viewer_release_sd_session(void)
{
    if (g_image_viewer_sd_session_held == 0U)
    {
        return;
    }
#if !defined(IMAGE_VIEWER_HOST_TEST)
    (void)sdcard_manager_session_release(IMAGE_VIEWER_SD_SESSION_OWNER);
#endif
    g_image_viewer_sd_session_held = 0U;
}

typedef struct
{
    image_viewer_status_t status;
    char request_path[IMAGE_VIEWER_PATH_SIZE];
    uint8_t *buffer;
    uint8_t album_mode;
    uint8_t buffer_acquired;
    uint8_t file_open;
    uint8_t dir_open;
    uint8_t album_count;
    uint8_t album_index;
    char scan_name[IMAGE_VIEWER_PATH_SIZE];
    char album_paths[64U][IMAGE_VIEWER_PATH_SIZE];
} image_viewer_context_t;

static image_viewer_context_t g_image_viewer;

static const char *image_viewer_state_name(image_viewer_state_t state)
{
    switch (state)
    {
        case IMAGE_VIEWER_IDLE:     return "idle";
        case IMAGE_VIEWER_SCANNING: return "scanning";
        case IMAGE_VIEWER_OPENING:  return "opening";
        case IMAGE_VIEWER_READING:  return "reading";
        case IMAGE_VIEWER_DECODING: return "decoding";
        case IMAGE_VIEWER_READY:    return "ready";
        case IMAGE_VIEWER_STOPPING: return "stopping";
        case IMAGE_VIEWER_ERROR:    return "error";
        default:                    return "invalid";
    }
}

static const char *image_viewer_basename(const char *path)
{
    const char *name = path;

    while (*path != '\0')
    {
        if ((*path == '/') || (*path == '\\'))
        {
            name = path + 1;
        }
        path++;
    }
    return name;
}

static int image_viewer_store_request(const char *path)
{
    size_t length;

    if ((path == NULL) || (path[0] == '\0'))
    {
        g_image_viewer.status.result = IMAGE_VIEWER_INVALID_ARG;
        return IMAGE_VIEWER_INVALID_ARG;
    }
    length = strlen(path);
    if (length >= IMAGE_VIEWER_PATH_SIZE)
    {
        g_image_viewer.status.result = IMAGE_VIEWER_PATH_TOO_LONG;
        return IMAGE_VIEWER_PATH_TOO_LONG;
    }
    memcpy(g_image_viewer.request_path, path, length + 1U);
    g_image_viewer.status.display_name =
        image_viewer_basename(g_image_viewer.request_path);
    g_image_viewer.status.result = IMAGE_VIEWER_OK;
    return IMAGE_VIEWER_OK;
}

static int image_viewer_is_idle(void)
{
    return (g_image_viewer.status.state == IMAGE_VIEWER_IDLE) ? 1 : 0;
}

static int image_viewer_is_jpeg_name(const char *name)
{
    size_t length;
    const char *extension;

    length = strlen(name);
    if (length < 4U)
    {
        return 0;
    }
    extension = name + length - 4U;
    if ((extension[0] == '.') &&
        ((extension[1] == 'j') || (extension[1] == 'J')) &&
        ((extension[2] == 'p') || (extension[2] == 'P')) &&
        ((extension[3] == 'g') || (extension[3] == 'G')))
    {
        return 1;
    }
    if (length < 5U)
    {
        return 0;
    }
    extension = name + length - 5U;
    return ((extension[0] == '.') &&
            ((extension[1] == 'j') || (extension[1] == 'J')) &&
            ((extension[2] == 'p') || (extension[2] == 'P')) &&
            ((extension[3] == 'e') || (extension[3] == 'E')) &&
            ((extension[4] == 'g') || (extension[4] == 'G'))) ? 1 : 0;
}

static void image_viewer_release_resources(void)
{
    if (g_image_viewer.file_open != 0U)
    {
        image_viewer_platform_file_close();
        g_image_viewer.file_open = 0U;
    }
    if (g_image_viewer.dir_open != 0U)
    {
        image_viewer_platform_dir_close();
        g_image_viewer.dir_open = 0U;
    }
    if (g_image_viewer.buffer_acquired != 0U)
    {
        image_viewer_platform_buffer_release();
        g_image_viewer.buffer_acquired = 0U;
    }
    g_image_viewer.buffer = NULL;
    image_viewer_release_sd_session();
}

static void image_viewer_reset(void)
{
    memset(&g_image_viewer, 0, sizeof(g_image_viewer));
    g_image_viewer.status.state = IMAGE_VIEWER_IDLE;
    g_image_viewer.status.result = IMAGE_VIEWER_OK;
    g_image_viewer.status.display_name = "";
}

static uint32_t image_viewer_decode_target(void)
{
    if (g_image_viewer.status.rgb_address == SDRAM_VIDEO_RGB_A_ADDR)
    {
        return SDRAM_VIDEO_RGB_B_ADDR;
    }
    return SDRAM_VIDEO_RGB_A_ADDR;
}

static void image_viewer_fail(image_viewer_result_t result)
{
    image_viewer_release_resources();
    g_image_viewer.status.result = result;
    g_image_viewer.status.state = IMAGE_VIEWER_ERROR;
}

static void image_viewer_finish_album_scan(void)
{
    size_t length;

    if (g_image_viewer.dir_open != 0U)
    {
        image_viewer_platform_dir_close();
        g_image_viewer.dir_open = 0U;
    }
    if (g_image_viewer.album_count == 0U)
    {
        image_viewer_fail(IMAGE_VIEWER_EMPTY_ALBUM);
        return;
    }

    g_image_viewer.album_index = 0U;
    length = strlen(g_image_viewer.album_paths[0]);
    memcpy(g_image_viewer.request_path,
           g_image_viewer.album_paths[0],
           length + 1U);
    g_image_viewer.status.display_name =
        image_viewer_basename(g_image_viewer.request_path);
    g_image_viewer.status.state = IMAGE_VIEWER_OPENING;
}

static void image_viewer_select_album(uint8_t index)
{
    size_t length;

    g_image_viewer.album_index = index;
    length = strlen(g_image_viewer.album_paths[index]);
    memcpy(g_image_viewer.request_path,
           g_image_viewer.album_paths[index],
           length + 1U);
    g_image_viewer.status.display_name =
        image_viewer_basename(g_image_viewer.request_path);
    g_image_viewer.status.file_size = 0U;
    g_image_viewer.status.bytes_read = 0U;
    g_image_viewer.status.result = IMAGE_VIEWER_OK;
    g_image_viewer.status.state = IMAGE_VIEWER_OPENING;
}

static void image_viewer_scan_one_entry(void)
{
    size_t directory_length;
    size_t name_length;
    size_t position;
    uint8_t is_directory;
    uint8_t end;
    int result;

    if (g_image_viewer.dir_open == 0U)
    {
        result = image_viewer_platform_dir_open(g_image_viewer.request_path);
        if (result != IMAGE_VIEWER_OK)
        {
            image_viewer_fail(IMAGE_VIEWER_IO_ERROR);
            return;
        }
        g_image_viewer.dir_open = 1U;
    }
    if (g_image_viewer.album_count >= 64U)
    {
        image_viewer_finish_album_scan();
        return;
    }

    result = image_viewer_platform_dir_read(g_image_viewer.scan_name,
                                            sizeof(g_image_viewer.scan_name),
                                            &is_directory,
                                            &end);
    if (result != IMAGE_VIEWER_OK)
    {
        image_viewer_fail((image_viewer_result_t)result);
        return;
    }
    if (end != 0U)
    {
        image_viewer_finish_album_scan();
        return;
    }
    if ((is_directory != 0U) ||
        (image_viewer_is_jpeg_name(g_image_viewer.scan_name) == 0))
    {
        return;
    }

    directory_length = strlen(g_image_viewer.request_path);
    name_length = strlen(g_image_viewer.scan_name);
    position = directory_length;
    if ((position != 0U) &&
        (g_image_viewer.request_path[position - 1U] != '/') &&
        (g_image_viewer.request_path[position - 1U] != '\\'))
    {
        position++;
    }
    if ((position + name_length) >= IMAGE_VIEWER_PATH_SIZE)
    {
        return;
    }

    memcpy(g_image_viewer.album_paths[g_image_viewer.album_count],
           g_image_viewer.request_path,
           directory_length);
    if (position != directory_length)
    {
        g_image_viewer.album_paths[g_image_viewer.album_count][directory_length] = '/';
    }
    memcpy(g_image_viewer.album_paths[g_image_viewer.album_count] + position,
           g_image_viewer.scan_name,
           name_length + 1U);
    g_image_viewer.album_count++;
}

static image_viewer_result_t image_viewer_parse_jpeg(uint8_t *buffer,
                                                     uint32_t size,
                                                     uint16_t *width,
                                                     uint16_t *height,
                                                     uint16_t *aligned_width,
                                                     uint16_t *aligned_height)
{
    uint32_t offset;
    uint32_t segment_length;
    uint32_t aligned;
    uint8_t marker;

    if ((buffer == NULL) || (width == NULL) || (height == NULL) ||
        (aligned_width == NULL) || (aligned_height == NULL) ||
        (size < 4U) || (buffer[0] != 0xFFU) || (buffer[1] != 0xD8U))
    {
        return IMAGE_VIEWER_DECODE_ERROR;
    }

    offset = 2U;
    while (offset < size)
    {
        while ((offset < size) && (buffer[offset] != 0xFFU))
        {
            offset++;
        }
        if (offset >= size)
        {
            break;
        }
        while ((offset < size) && (buffer[offset] == 0xFFU))
        {
            offset++;
        }
        if (offset >= size)
        {
            break;
        }

        marker = buffer[offset++];
        if ((marker == 0xD8U) || (marker == 0x01U) ||
            ((marker >= 0xD0U) && (marker <= 0xD7U)))
        {
            continue;
        }
        if ((marker == 0xD9U) || (marker == 0xDAU))
        {
            break;
        }
        if ((size - offset) < 2U)
        {
            return IMAGE_VIEWER_DECODE_ERROR;
        }

        segment_length = ((uint32_t)buffer[offset] << 8) |
                         (uint32_t)buffer[offset + 1U];
        if ((segment_length < 2U) || (segment_length > (size - offset)))
        {
            return IMAGE_VIEWER_DECODE_ERROR;
        }

        if ((marker == 0xC0U) || (marker == 0xC1U) || (marker == 0xC2U))
        {
            if ((marker == 0xC2U) || (segment_length < 8U))
            {
                return (marker == 0xC2U) ?
                       IMAGE_VIEWER_UNSUPPORTED : IMAGE_VIEWER_DECODE_ERROR;
            }

            *height = (uint16_t)(((uint16_t)buffer[offset + 3U] << 8) |
                                 (uint16_t)buffer[offset + 4U]);
            *width = (uint16_t)(((uint16_t)buffer[offset + 5U] << 8) |
                                (uint16_t)buffer[offset + 6U]);
            if ((*width == 0U) || (*height == 0U) ||
                (*width > 2048U) || (*height > 2048U))
            {
                return IMAGE_VIEWER_DECODE_ERROR;
            }

            aligned = ((uint32_t)(*width) + 15U) & ~15UL;
            *aligned_width = (uint16_t)aligned;
            aligned = ((uint32_t)(*height) + 15U) & ~15UL;
            *aligned_height = (uint16_t)aligned;
            if (((uint32_t)(*aligned_width) * (uint32_t)(*aligned_height)) >
                ((SDRAM_VIDEO_RGB_B_ADDR - SDRAM_VIDEO_RGB_A_ADDR) / 2UL))
            {
                return IMAGE_VIEWER_DECODE_ERROR;
            }

            buffer[offset + 3U] = (uint8_t)(*aligned_height >> 8);
            buffer[offset + 4U] = (uint8_t)(*aligned_height);
            buffer[offset + 5U] = (uint8_t)(*aligned_width >> 8);
            buffer[offset + 6U] = (uint8_t)(*aligned_width);
            return IMAGE_VIEWER_OK;
        }

        offset += segment_length;
    }

    return IMAGE_VIEWER_DECODE_ERROR;
}

void image_viewer_app_init(void)
{
    image_viewer_release_resources();
    image_viewer_reset();
}

int image_viewer_app_open_album(const char *directory)
{
    int result;

    if (image_viewer_is_idle() == 0)
    {
        return IMAGE_VIEWER_BUSY;
    }
    result = image_viewer_acquire_sd_session();
    if (result != IMAGE_VIEWER_OK)
    {
        return result;
    }
    result = image_viewer_store_request(directory);
    if (result != IMAGE_VIEWER_OK)
    {
        image_viewer_release_sd_session();
        return result;
    }
    g_image_viewer.album_mode = 1U;
    g_image_viewer.album_count = 0U;
    g_image_viewer.album_index = 0U;
    g_image_viewer.status.state = IMAGE_VIEWER_SCANNING;
    return IMAGE_VIEWER_OK;
}

int image_viewer_app_open_file(const char *path)
{
    int result;

    if (image_viewer_is_idle() == 0)
    {
        return IMAGE_VIEWER_BUSY;
    }
    result = image_viewer_acquire_sd_session();
    if (result != IMAGE_VIEWER_OK)
    {
        return result;
    }
    result = image_viewer_store_request(path);
    if (result != IMAGE_VIEWER_OK)
    {
        image_viewer_release_sd_session();
        return result;
    }
    g_image_viewer.album_mode = 0U;
    g_image_viewer.status.state = IMAGE_VIEWER_OPENING;
    return IMAGE_VIEWER_OK;
}

int image_viewer_app_next(void)
{
    uint8_t index;

    if ((g_image_viewer.album_mode == 0U) ||
        (g_image_viewer.album_count == 0U) ||
        (g_image_viewer.status.state != IMAGE_VIEWER_READY))
    {
        return IMAGE_VIEWER_NOT_READY;
    }
    index = g_image_viewer.album_index + 1U;
    if (index >= g_image_viewer.album_count)
    {
        index = 0U;
    }
    image_viewer_select_album(index);
    return IMAGE_VIEWER_OK;
}

int image_viewer_app_previous(void)
{
    uint8_t index;

    if ((g_image_viewer.album_mode == 0U) ||
        (g_image_viewer.album_count == 0U) ||
        (g_image_viewer.status.state != IMAGE_VIEWER_READY))
    {
        return IMAGE_VIEWER_NOT_READY;
    }
    index = (g_image_viewer.album_index == 0U) ?
            (uint8_t)(g_image_viewer.album_count - 1U) :
            (uint8_t)(g_image_viewer.album_index - 1U);
    image_viewer_select_album(index);
    return IMAGE_VIEWER_OK;
}

int image_viewer_app_stop(void)
{
    if (g_image_viewer.status.state == IMAGE_VIEWER_IDLE)
    {
        return IMAGE_VIEWER_OK;
    }
    g_image_viewer.status.state = IMAGE_VIEWER_STOPPING;
    return IMAGE_VIEWER_OK;
}

void image_viewer_app_poll(void)
{
    uint32_t request;
    uint32_t actual;
    uint16_t width;
    uint16_t height;
    uint16_t aligned_width;
    uint16_t aligned_height;
    uint32_t decode_target;
    int result;

    switch (g_image_viewer.status.state)
    {
        case IMAGE_VIEWER_SCANNING:
            image_viewer_scan_one_entry();
            break;

        case IMAGE_VIEWER_OPENING:
            result = image_viewer_platform_buffer_acquire(
                IMAGE_VIEWER_MAX_FILE_SIZE,
                (void **)&g_image_viewer.buffer);
            if (result != IMAGE_VIEWER_OK)
            {
                image_viewer_fail((image_viewer_result_t)result);
                break;
            }
            g_image_viewer.buffer_acquired = 1U;

            result = image_viewer_platform_file_open(
                g_image_viewer.request_path);
            if (result != IMAGE_VIEWER_OK)
            {
                image_viewer_fail(IMAGE_VIEWER_IO_ERROR);
                break;
            }
            g_image_viewer.file_open = 1U;

            result = image_viewer_platform_file_size(
                &g_image_viewer.status.file_size);
            if ((result != IMAGE_VIEWER_OK) ||
                (g_image_viewer.status.file_size == 0U))
            {
                image_viewer_fail(IMAGE_VIEWER_IO_ERROR);
                break;
            }
            if (g_image_viewer.status.file_size >
                IMAGE_VIEWER_MAX_FILE_SIZE)
            {
                image_viewer_fail(IMAGE_VIEWER_FILE_TOO_LARGE);
                break;
            }
            g_image_viewer.status.bytes_read = 0U;
            g_image_viewer.status.state = IMAGE_VIEWER_READING;
            break;

        case IMAGE_VIEWER_READING:
            request = g_image_viewer.status.file_size -
                      g_image_viewer.status.bytes_read;
            if (request > IMAGE_VIEWER_READ_CHUNK_SIZE)
            {
                request = IMAGE_VIEWER_READ_CHUNK_SIZE;
            }
            actual = 0U;
            result = image_viewer_platform_file_read(
                g_image_viewer.buffer + g_image_viewer.status.bytes_read,
                request,
                &actual);
            if ((result != IMAGE_VIEWER_OK) ||
                (actual == 0U) || (actual > request))
            {
                image_viewer_fail(IMAGE_VIEWER_IO_ERROR);
                break;
            }
            g_image_viewer.status.bytes_read += actual;
            if (g_image_viewer.status.bytes_read ==
                g_image_viewer.status.file_size)
            {
                image_viewer_platform_file_close();
                g_image_viewer.file_open = 0U;
                g_image_viewer.status.state = IMAGE_VIEWER_DECODING;
            }
            break;

        case IMAGE_VIEWER_DECODING:
            decode_target = image_viewer_decode_target();
            result = image_viewer_parse_jpeg(g_image_viewer.buffer,
                                             g_image_viewer.status.file_size,
                                             &width,
                                             &height,
                                             &aligned_width,
                                             &aligned_height);
            if (result != IMAGE_VIEWER_OK)
            {
                image_viewer_fail((image_viewer_result_t)result);
                break;
            }
            result = image_viewer_platform_decode(g_image_viewer.buffer,
                                                  g_image_viewer.status.file_size,
                                                  aligned_width,
                                                  aligned_height,
                                                  decode_target);
            if (result != IMAGE_VIEWER_OK)
            {
                image_viewer_fail(IMAGE_VIEWER_DECODE_ERROR);
                break;
            }

            g_image_viewer.status.width = width;
            g_image_viewer.status.height = height;
            g_image_viewer.status.rgb_address = decode_target;
            g_image_viewer.status.rgb_stride = (uint32_t)aligned_width * 2U;
            image_viewer_release_resources();
            g_image_viewer.status.result = IMAGE_VIEWER_OK;
            g_image_viewer.status.state = IMAGE_VIEWER_READY;
            break;

        case IMAGE_VIEWER_STOPPING:
            image_viewer_release_resources();
            image_viewer_reset();
            break;

        default:
            break;
    }
}

void image_viewer_app_get_status(image_viewer_status_t *status)
{
    if (status != NULL)
    {
        *status = g_image_viewer.status;
    }
}

void image_viewer_app_print_status(void)
{
    printf("image state=%s result=%ld file_size=%lu bytes_read=%lu width=%u height=%u rgb=0x%08lX stride=%lu name=%s\r\n",
           image_viewer_state_name(g_image_viewer.status.state),
           (long)g_image_viewer.status.result,
           (unsigned long)g_image_viewer.status.file_size,
           (unsigned long)g_image_viewer.status.bytes_read,
           (unsigned int)g_image_viewer.status.width,
           (unsigned int)g_image_viewer.status.height,
           (unsigned long)g_image_viewer.status.rgb_address,
           (unsigned long)g_image_viewer.status.rgb_stride,
           g_image_viewer.status.display_name);
}
