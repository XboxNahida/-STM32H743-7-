#include "piano_touch_mapper.h"

#include <stddef.h>

#define PIANO_TOUCH_BASE_NOTE  12

static const uint8_t g_white_note_offset[14] = {
    0U, 2U, 4U, 5U, 7U, 9U, 11U,
    12U, 14U, 16U, 17U, 19U, 21U, 23U
};

static const uint8_t g_black_key_after[10] = {
    0U, 1U, 3U, 4U, 5U, 7U, 8U, 10U, 11U, 12U
};

static const uint8_t g_black_note_offset[10] = {
    1U, 3U, 6U, 8U, 10U, 13U, 15U, 18U, 20U, 22U
};

static const uint8_t g_matrix_note_offset[15] = {
    0U, 2U, 4U, 5U, 7U,
    9U, 11U, 12U, 14U, 16U,
    17U, 19U, 21U, 23U, 24U
};

static int16_t piano_touch_map_keys(const piano_touch_point_t *point)
{
    uint8_t index;

    if ((point->x < 0) || (point->x >= 786) ||
        (point->y < 67) || (point->y >= 549))
    {
        return -1;
    }
    if (point->y < 347)
    {
        for (index = 0U; index < 10U; index++)
        {
            int16_t black_x = (int16_t)(14 +
                ((int16_t)g_black_key_after[index] + 1) * 54 - 16);

            if ((point->x >= black_x) && (point->x < (black_x + 32)))
            {
                return (int16_t)g_black_note_offset[index];
            }
        }
    }
    for (index = 0U; index < 14U; index++)
    {
        int16_t white_x = (int16_t)(14 + (int16_t)index * 54);

        if ((point->x >= white_x) && (point->x < (white_x + 52)))
        {
            return (int16_t)g_white_note_offset[index];
        }
    }
    return -1;
}

static int16_t piano_touch_map_matrix(const piano_touch_point_t *point)
{
    uint8_t row;
    uint8_t column;

    if ((point->x < 0) || (point->x >= 786) ||
        (point->y < 35) || (point->y >= 600))
    {
        return -1;
    }
    for (row = 0U; row < 3U; row++)
    {
        int16_t pad_y = (int16_t)(53 + (int16_t)row * 176);

        if ((point->y < pad_y) || (point->y >= (pad_y + 158)))
        {
            continue;
        }
        for (column = 0U; column < 5U; column++)
        {
            int16_t pad_x = (int16_t)(18 + (int16_t)column * 150);

            if ((point->x >= pad_x) && (point->x < (pad_x + 135)))
            {
                return (int16_t)g_matrix_note_offset[
                    (uint8_t)(row * 5U + column)];
            }
        }
    }
    return -1;
}

int piano_touch_map(piano_touch_mode_t mode,
                    const piano_touch_point_t *points,
                    uint8_t point_count,
                    int8_t transpose,
                    uint64_t *note_mask)
{
    uint64_t mask = 0U;
    uint8_t index;

    if ((note_mask == NULL) || (point_count > PIANO_TOUCH_MAX_POINTS) ||
        ((point_count != 0U) && (points == NULL)) ||
        ((mode != PIANO_TOUCH_MODE_KEYS) &&
         (mode != PIANO_TOUCH_MODE_MATRIX)) ||
        (transpose < -12) || (transpose > 12))
    {
        return PIANO_TOUCH_INVALID_ARG;
    }
    for (index = 0U; index < point_count; index++)
    {
        int16_t offset = (mode == PIANO_TOUCH_MODE_KEYS) ?
            piano_touch_map_keys(&points[index]) :
            piano_touch_map_matrix(&points[index]);

        if (offset >= 0)
        {
            int16_t note = (int16_t)(PIANO_TOUCH_BASE_NOTE + offset +
                                     transpose);

            if ((note >= 0) && (note < 48))
            {
                mask |= 1ULL << (uint8_t)note;
            }
        }
    }
    *note_mask = mask;
    return PIANO_TOUCH_OK;
}
