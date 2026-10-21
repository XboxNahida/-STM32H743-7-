#ifndef DRAWING_HISTORY_H
#define DRAWING_HISTORY_H

#include <stdint.h>

typedef enum
{
    DRAWING_HISTORY_OK = 0,
    DRAWING_HISTORY_INVALID_ARG,
    DRAWING_HISTORY_FULL,
    DRAWING_HISTORY_EMPTY,
    DRAWING_HISTORY_BUSY
} drawing_history_result_t;

typedef struct
{
    int16_t x;
    int16_t y;
} drawing_point_t;

typedef struct
{
    uint32_t point_start;
    uint32_t point_count;
    uint16_t color_rgb565;
    uint8_t width;
    uint8_t eraser;
} drawing_stroke_t;

typedef struct
{
    drawing_stroke_t *strokes;
    drawing_point_t *points;
    uint32_t stroke_capacity;
    uint32_t point_capacity;
    uint32_t stroke_count;
    uint32_t visible_count;
    uint32_t point_count;
    uint32_t active_stroke_index;
    uint8_t active;
} drawing_history_t;

int drawing_history_init(drawing_history_t *history,
                         drawing_stroke_t *strokes,
                         uint32_t stroke_capacity,
                         drawing_point_t *points,
                         uint32_t point_capacity);
int drawing_history_begin(drawing_history_t *history, int16_t x, int16_t y,
                          uint16_t color_rgb565, uint8_t width,
                          uint8_t eraser);
int drawing_history_append(drawing_history_t *history, int16_t x, int16_t y);
int drawing_history_end(drawing_history_t *history);
int drawing_history_undo(drawing_history_t *history);
int drawing_history_redo(drawing_history_t *history);
void drawing_history_clear(drawing_history_t *history);
uint32_t drawing_history_visible_count(const drawing_history_t *history);
const drawing_stroke_t *drawing_history_get_stroke(
    const drawing_history_t *history, uint32_t index);
const drawing_point_t *drawing_history_get_point(
    const drawing_history_t *history, const drawing_stroke_t *stroke,
    uint32_t point_index);

#endif
