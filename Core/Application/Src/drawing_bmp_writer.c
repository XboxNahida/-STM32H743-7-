#include "drawing_bmp_writer.h"

#include <stdio.h>
#include <string.h>

#define DRAWING_BMP_OWNER          0x0506U
#define DRAWING_BMP_MAX_WIDTH      1024U
#define DRAWING_BMP_MAX_HEIGHT     600U
#define DRAWING_BMP_HEADER_SIZE    54U
#define DRAWING_BMP_MAX_ROW_SIZE   (DRAWING_BMP_MAX_WIDTH * 3U)
#define DRAWING_BMP_STAT_EXISTS    0
#define DRAWING_BMP_STAT_ABSENT    1

#ifndef DRAWING_BMP_WRITER_HOST_TEST
#include "ff.h"
#include "sdcard_manager.h"

static FIL g_drawing_bmp_file;

static int drawing_bmp_platform_session_acquire(void)
{
    return (sdcard_manager_session_acquire(DRAWING_BMP_OWNER) ==
            SDCARD_MANAGER_OK) ? 0 : 1;
}

static void drawing_bmp_platform_session_release(void)
{
    (void)sdcard_manager_session_release(DRAWING_BMP_OWNER);
}

static int drawing_bmp_platform_mkdir(const char *path)
{
    FRESULT result = f_mkdir(path);
    return ((result == FR_OK) || (result == FR_EXIST)) ? 0 : 1;
}

static int drawing_bmp_platform_stat(const char *path)
{
    FILINFO info;
    FRESULT result = f_stat(path, &info);

    if (result == FR_OK) return DRAWING_BMP_STAT_EXISTS;
    if (result == FR_NO_FILE) return DRAWING_BMP_STAT_ABSENT;
    return 2;
}

static int drawing_bmp_platform_open(const char *path)
{
    return (f_open(&g_drawing_bmp_file, path,
                   FA_WRITE | FA_CREATE_NEW) == FR_OK) ? 0 : 1;
}

static int drawing_bmp_platform_write(const void *data, uint32_t size,
                                      uint32_t *written)
{
    UINT count = 0U;
    FRESULT result = f_write(&g_drawing_bmp_file, data, (UINT)size, &count);

    *written = (uint32_t)count;
    return (result == FR_OK) ? 0 : 1;
}

static int drawing_bmp_platform_close(void)
{
    return (f_close(&g_drawing_bmp_file) == FR_OK) ? 0 : 1;
}

static void drawing_bmp_platform_remove(const char *path)
{
    (void)f_unlink(path);
}
#else
extern int drawing_bmp_test_session_acquire(void);
extern void drawing_bmp_test_session_release(void);
extern int drawing_bmp_test_mkdir(const char *path);
extern int drawing_bmp_test_stat(const char *path);
extern int drawing_bmp_test_open(const char *path);
extern int drawing_bmp_test_write(const void *data, uint32_t size,
                                  uint32_t *written);
extern int drawing_bmp_test_close(void);
extern void drawing_bmp_test_remove(const char *path);

#define drawing_bmp_platform_session_acquire drawing_bmp_test_session_acquire
#define drawing_bmp_platform_session_release drawing_bmp_test_session_release
#define drawing_bmp_platform_mkdir           drawing_bmp_test_mkdir
#define drawing_bmp_platform_stat            drawing_bmp_test_stat
#define drawing_bmp_platform_open            drawing_bmp_test_open
#define drawing_bmp_platform_write           drawing_bmp_test_write
#define drawing_bmp_platform_close           drawing_bmp_test_close
#define drawing_bmp_platform_remove          drawing_bmp_test_remove
#endif

static uint8_t g_drawing_bmp_row[DRAWING_BMP_MAX_ROW_SIZE];

static void drawing_bmp_write_le16(uint8_t *target, uint16_t value)
{
    target[0] = (uint8_t)value;
    target[1] = (uint8_t)(value >> 8);
}

static void drawing_bmp_write_le32(uint8_t *target, uint32_t value)
{
    target[0] = (uint8_t)value;
    target[1] = (uint8_t)(value >> 8);
    target[2] = (uint8_t)(value >> 16);
    target[3] = (uint8_t)(value >> 24);
}

static uint8_t drawing_bmp_expand5(uint16_t value)
{
    return (uint8_t)((value << 3) | (value >> 2));
}

static uint8_t drawing_bmp_expand6(uint16_t value)
{
    return (uint8_t)((value << 2) | (value >> 4));
}

static int drawing_bmp_write_all(const void *data, uint32_t size)
{
    uint32_t written = 0U;

    if ((drawing_bmp_platform_write(data, size, &written) != 0) ||
        (written != size))
    {
        return 1;
    }
    return 0;
}

static int drawing_bmp_select_path(char *saved_path,
                                   size_t saved_path_capacity)
{
    uint32_t index;

    for (index = 1U; index <= 9999U; index++)
    {
        int length = snprintf(saved_path, saved_path_capacity,
                              "0:/PICTURE/DRAW%04lu.BMP",
                              (unsigned long)index);
        int state;

        if ((length < 0) || ((size_t)length >= saved_path_capacity))
        {
            return DRAWING_BMP_NO_NAME;
        }
        state = drawing_bmp_platform_stat(saved_path);
        if (state == DRAWING_BMP_STAT_ABSENT)
        {
            return DRAWING_BMP_OK;
        }
        if (state != DRAWING_BMP_STAT_EXISTS)
        {
            return DRAWING_BMP_IO_ERROR;
        }
    }
    return DRAWING_BMP_NO_NAME;
}

static void drawing_bmp_build_header(uint8_t *header, uint16_t width,
                                     uint16_t height, uint32_t row_size)
{
    uint32_t image_size = row_size * (uint32_t)height;

    (void)memset(header, 0, DRAWING_BMP_HEADER_SIZE);
    header[0] = 'B';
    header[1] = 'M';
    drawing_bmp_write_le32(&header[2], DRAWING_BMP_HEADER_SIZE + image_size);
    drawing_bmp_write_le32(&header[10], DRAWING_BMP_HEADER_SIZE);
    drawing_bmp_write_le32(&header[14], 40U);
    drawing_bmp_write_le32(&header[18], width);
    drawing_bmp_write_le32(&header[22], height);
    drawing_bmp_write_le16(&header[26], 1U);
    drawing_bmp_write_le16(&header[28], 24U);
    drawing_bmp_write_le32(&header[34], image_size);
    drawing_bmp_write_le32(&header[38], 2835U);
    drawing_bmp_write_le32(&header[42], 2835U);
}

static void drawing_bmp_convert_row(const uint8_t *source, uint16_t width,
                                    uint32_t row_size)
{
    uint32_t x;

    (void)memset(g_drawing_bmp_row, 0, row_size);
    for (x = 0U; x < width; x++)
    {
        uint16_t color = (uint16_t)source[x * 2U] |
                         ((uint16_t)source[x * 2U + 1U] << 8);
        g_drawing_bmp_row[x * 3U] = drawing_bmp_expand5(color & 0x1FU);
        g_drawing_bmp_row[x * 3U + 1U] =
            drawing_bmp_expand6((color >> 5) & 0x3FU);
        g_drawing_bmp_row[x * 3U + 2U] =
            drawing_bmp_expand5((color >> 11) & 0x1FU);
    }
}

int drawing_bmp_save_next(const uint8_t *rgb565, uint16_t width,
                          uint16_t height, uint32_t stride_bytes,
                          char *saved_path, size_t saved_path_capacity)
{
    uint8_t header[DRAWING_BMP_HEADER_SIZE];
    uint32_t row_size;
    uint32_t row;
    int result;
    uint8_t file_open = 0U;

    if ((rgb565 == NULL) || (saved_path == NULL) ||
        (saved_path_capacity < DRAWING_BMP_PATH_CAPACITY) ||
        (width == 0U) || (width > DRAWING_BMP_MAX_WIDTH) ||
        (height == 0U) || (height > DRAWING_BMP_MAX_HEIGHT) ||
        (stride_bytes < ((uint32_t)width * 2U)))
    {
        return DRAWING_BMP_INVALID_ARG;
    }
    saved_path[0] = '\0';
    if (drawing_bmp_platform_session_acquire() != 0)
    {
        return DRAWING_BMP_NOT_READY;
    }
    if (drawing_bmp_platform_mkdir("0:/PICTURE") != 0)
    {
        result = DRAWING_BMP_IO_ERROR;
        goto release_session;
    }
    result = drawing_bmp_select_path(saved_path, saved_path_capacity);
    if (result != DRAWING_BMP_OK)
    {
        goto release_session;
    }
    if (drawing_bmp_platform_open(saved_path) != 0)
    {
        result = DRAWING_BMP_IO_ERROR;
        goto release_session;
    }
    file_open = 1U;

    row_size = (((uint32_t)width * 3U) + 3U) & ~3U;
    drawing_bmp_build_header(header, width, height, row_size);
    if (drawing_bmp_write_all(header, sizeof(header)) != 0)
    {
        result = DRAWING_BMP_IO_ERROR;
        goto close_file;
    }
    for (row = height; row > 0U; row--)
    {
        const uint8_t *source = rgb565 + ((row - 1U) * stride_bytes);
        drawing_bmp_convert_row(source, width, row_size);
        if (drawing_bmp_write_all(g_drawing_bmp_row, row_size) != 0)
        {
            result = DRAWING_BMP_IO_ERROR;
            goto close_file;
        }
    }
    result = DRAWING_BMP_OK;

close_file:
    if (file_open != 0U)
    {
        if ((drawing_bmp_platform_close() != 0) &&
            (result == DRAWING_BMP_OK))
        {
            result = DRAWING_BMP_IO_ERROR;
        }
        if (result != DRAWING_BMP_OK)
        {
            drawing_bmp_platform_remove(saved_path);
        }
    }
release_session:
    drawing_bmp_platform_session_release();
    return result;
}

int drawing_bmp_save_overwrite(const uint8_t *rgb565, uint16_t width,
                               uint16_t height, uint32_t stride_bytes,
                               const char *path)
{
#ifdef DRAWING_BMP_WRITER_HOST_TEST
    (void)rgb565; (void)width; (void)height; (void)stride_bytes; (void)path;
    return DRAWING_BMP_IO_ERROR;
#else
    uint8_t header[DRAWING_BMP_HEADER_SIZE];
    uint32_t row_size;
    uint32_t row;
    uint8_t file_open = 0U;
    int result = DRAWING_BMP_IO_ERROR;

    if ((rgb565 == NULL) || (path == NULL) || (path[0] == '\0') ||
        (width == 0U) || (width > DRAWING_BMP_MAX_WIDTH) ||
        (height == 0U) || (height > DRAWING_BMP_MAX_HEIGHT) ||
        (stride_bytes < ((uint32_t)width * 2U))) return DRAWING_BMP_INVALID_ARG;
    if (drawing_bmp_platform_session_acquire() != 0) return DRAWING_BMP_NOT_READY;
    if (f_open(&g_drawing_bmp_file, path,
               FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) goto release_session;
    file_open = 1U;
    row_size = (((uint32_t)width * 3U) + 3U) & ~3U;
    drawing_bmp_build_header(header, width, height, row_size);
    if (drawing_bmp_write_all(header, sizeof(header)) != 0) goto close_file;
    for (row = height; row > 0U; row--)
    {
        const uint8_t *source = rgb565 + ((row - 1U) * stride_bytes);
        drawing_bmp_convert_row(source, width, row_size);
        if (drawing_bmp_write_all(g_drawing_bmp_row, row_size) != 0)
            goto close_file;
    }
    result = DRAWING_BMP_OK;
close_file:
    if ((file_open != 0U) && (drawing_bmp_platform_close() != 0))
        result = DRAWING_BMP_IO_ERROR;
release_session:
    drawing_bmp_platform_session_release();
    return result;
#endif
}
