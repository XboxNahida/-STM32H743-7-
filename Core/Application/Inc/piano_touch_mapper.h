#ifndef PIANO_TOUCH_MAPPER_H
#define PIANO_TOUCH_MAPPER_H

#include <stdint.h>

#define PIANO_TOUCH_MAX_POINTS  5U

typedef enum
{
    PIANO_TOUCH_OK = 0,
    PIANO_TOUCH_INVALID_ARG
} piano_touch_result_t;

typedef enum
{
    PIANO_TOUCH_MODE_KEYS = 0,
    PIANO_TOUCH_MODE_MATRIX
} piano_touch_mode_t;

typedef struct
{
    int16_t x;
    int16_t y;
} piano_touch_point_t;

int piano_touch_map(piano_touch_mode_t mode,
                    const piano_touch_point_t *points,
                    uint8_t point_count,
                    int8_t transpose,
                    uint64_t *note_mask);

#endif
