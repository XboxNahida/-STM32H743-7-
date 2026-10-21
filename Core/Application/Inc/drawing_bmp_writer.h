#ifndef DRAWING_BMP_WRITER_H
#define DRAWING_BMP_WRITER_H

#include <stddef.h>
#include <stdint.h>

#define DRAWING_BMP_PATH_CAPACITY 32U

typedef enum
{
    DRAWING_BMP_OK = 0,
    DRAWING_BMP_INVALID_ARG,
    DRAWING_BMP_NOT_READY,
    DRAWING_BMP_NO_NAME,
    DRAWING_BMP_IO_ERROR
} drawing_bmp_result_t;

int drawing_bmp_save_next(const uint8_t *rgb565, uint16_t width,
                          uint16_t height, uint32_t stride_bytes,
                          char *saved_path, size_t saved_path_capacity);
int drawing_bmp_save_overwrite(const uint8_t *rgb565, uint16_t width,
                               uint16_t height, uint32_t stride_bytes,
                               const char *path);

#endif
