#include "drawing_bmp_reader.h"

#include <stddef.h>

#include "ff.h"
#include "sdcard_manager.h"

#define DRAWING_BMP_READ_OWNER 0x0507U
#define DRAWING_BMP_HEADER_SIZE 54U
#define DRAWING_BMP_MAX_ROW 3072U

static uint8_t g_bmp_read_row[DRAWING_BMP_MAX_ROW];

static uint16_t bmp_u16(const uint8_t *data)
{ return (uint16_t)data[0] | ((uint16_t)data[1] << 8); }

static uint32_t bmp_u32(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

int drawing_bmp_read_rgb565(const char *path, uint8_t *destination,
                            uint16_t width, uint16_t height,
                            uint32_t stride)
{
    FIL file;
    uint8_t header[DRAWING_BMP_HEADER_SIZE];
    UINT actual = 0U;
    uint32_t file_width;
    uint32_t file_height;
    uint32_t row_size;
    uint32_t row;
    int result = 1;

    if ((path == NULL) || (destination == NULL) ||
        (stride < (uint32_t)width * 2U)) return 1;
    if (sdcard_manager_session_acquire(DRAWING_BMP_READ_OWNER) !=
        SDCARD_MANAGER_OK) return 1;
    if (f_open(&file, path, FA_READ) != FR_OK) goto release;
    if ((f_read(&file, header, sizeof(header), &actual) != FR_OK) ||
        (actual != sizeof(header)) || (header[0] != 'B') || (header[1] != 'M') ||
        (bmp_u16(&header[26]) != 1U) || (bmp_u16(&header[28]) != 24U) ||
        (bmp_u32(&header[30]) != 0U)) goto close;
    file_width = bmp_u32(&header[18]);
    file_height = bmp_u32(&header[22]);
    row_size = ((file_width * 3U) + 3U) & ~3U;
    if ((file_width != width) || (file_height != height) ||
        (row_size > sizeof(g_bmp_read_row)) ||
        (f_lseek(&file, bmp_u32(&header[10])) != FR_OK)) goto close;
    for (row = 0U; row < file_height; row++)
    {
        uint8_t *target = destination + ((file_height - row - 1U) * stride);
        uint32_t x;
        if ((f_read(&file, g_bmp_read_row, row_size, &actual) != FR_OK) ||
            (actual != row_size)) goto close;
        for (x = 0U; x < file_width; x++)
        {
            uint8_t blue = g_bmp_read_row[x * 3U];
            uint8_t green = g_bmp_read_row[x * 3U + 1U];
            uint8_t red = g_bmp_read_row[x * 3U + 2U];
            uint16_t color = (uint16_t)(((uint16_t)(red >> 3) << 11) |
                ((uint16_t)(green >> 2) << 5) | (blue >> 3));
            target[x * 2U] = (uint8_t)color;
            target[x * 2U + 1U] = (uint8_t)(color >> 8);
        }
    }
    result = 0;
close:
    (void)f_close(&file);
release:
    (void)sdcard_manager_session_release(DRAWING_BMP_READ_OWNER);
    return result;
}
