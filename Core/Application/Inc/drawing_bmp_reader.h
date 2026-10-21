#ifndef DRAWING_BMP_READER_H
#define DRAWING_BMP_READER_H

#include <stdint.h>

int drawing_bmp_read_rgb565(const char *path, uint8_t *destination,
                            uint16_t width, uint16_t height,
                            uint32_t stride);

#endif
