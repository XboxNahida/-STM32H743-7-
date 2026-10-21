#include "drawing_history.h"

#include <stddef.h>
#include <string.h>

static uint8_t drawing_history_is_ready(const drawing_history_t *history)
{
    return (history != NULL) && (history->strokes != NULL) &&
           (history->points != NULL) &&
           (history->stroke_capacity != 0U) &&
           (history->point_capacity != 0U);
}

static void drawing_history_discard_redo(drawing_history_t *history)
{
    if (history->visible_count >= history->stroke_count)
    {
        return;
    }
    if (history->visible_count == 0U)
    {
        history->point_count = 0U;
    }
    else
    {
        const drawing_stroke_t *last =
            &history->strokes[history->visible_count - 1U];
        history->point_count = last->point_start + last->point_count;
    }
    history->stroke_count = history->visible_count;
}

int drawing_history_init(drawing_history_t *history,
                         drawing_stroke_t *strokes,
                         uint32_t stroke_capacity,
                         drawing_point_t *points,
                         uint32_t point_capacity)
{
    if ((history == NULL) || (strokes == NULL) || (points == NULL) ||
        (stroke_capacity == 0U) || (point_capacity == 0U))
    {
        return DRAWING_HISTORY_INVALID_ARG;
    }
    (void)memset(history, 0, sizeof(*history));
    history->strokes = strokes;
    history->points = points;
    history->stroke_capacity = stroke_capacity;
    history->point_capacity = point_capacity;
    return DRAWING_HISTORY_OK;
}

int drawing_history_begin(drawing_history_t *history, int16_t x, int16_t y,
                          uint16_t color_rgb565, uint8_t width,
                          uint8_t eraser)
{
    drawing_stroke_t *stroke;

    if ((drawing_history_is_ready(history) == 0U) || (width == 0U) ||
        (width > 32U) || (eraser > 1U))
    {
        return DRAWING_HISTORY_INVALID_ARG;
    }
    if (history->active != 0U)
    {
        return DRAWING_HISTORY_BUSY;
    }
    drawing_history_discard_redo(history);
    if ((history->stroke_count >= history->stroke_capacity) ||
        (history->point_count >= history->point_capacity))
    {
        return DRAWING_HISTORY_FULL;
    }

    stroke = &history->strokes[history->stroke_count];
    stroke->point_start = history->point_count;
    stroke->point_count = 1U;
    stroke->color_rgb565 = color_rgb565;
    stroke->width = width;
    stroke->eraser = eraser;
    history->points[history->point_count].x = x;
    history->points[history->point_count].y = y;
    history->point_count++;
    history->active_stroke_index = history->stroke_count;
    history->stroke_count++;
    history->visible_count = history->stroke_count;
    history->active = 1U;
    return DRAWING_HISTORY_OK;
}

int drawing_history_append(drawing_history_t *history, int16_t x, int16_t y)
{
    drawing_stroke_t *stroke;
    const drawing_point_t *last;

    if (drawing_history_is_ready(history) == 0U)
    {
        return DRAWING_HISTORY_INVALID_ARG;
    }
    if (history->active == 0U)
    {
        return DRAWING_HISTORY_EMPTY;
    }
    stroke = &history->strokes[history->active_stroke_index];
    last = &history->points[stroke->point_start + stroke->point_count - 1U];
    if ((last->x == x) && (last->y == y))
    {
        return DRAWING_HISTORY_OK;
    }
    if (history->point_count >= history->point_capacity)
    {
        return DRAWING_HISTORY_FULL;
    }
    history->points[history->point_count].x = x;
    history->points[history->point_count].y = y;
    history->point_count++;
    stroke->point_count++;
    return DRAWING_HISTORY_OK;
}

int drawing_history_end(drawing_history_t *history)
{
    if (drawing_history_is_ready(history) == 0U)
    {
        return DRAWING_HISTORY_INVALID_ARG;
    }
    if (history->active == 0U)
    {
        return DRAWING_HISTORY_EMPTY;
    }
    history->active = 0U;
    return DRAWING_HISTORY_OK;
}

int drawing_history_undo(drawing_history_t *history)
{
    if (drawing_history_is_ready(history) == 0U)
    {
        return DRAWING_HISTORY_INVALID_ARG;
    }
    if (history->active != 0U)
    {
        return DRAWING_HISTORY_BUSY;
    }
    if (history->visible_count == 0U)
    {
        return DRAWING_HISTORY_EMPTY;
    }
    history->visible_count--;
    return DRAWING_HISTORY_OK;
}

int drawing_history_redo(drawing_history_t *history)
{
    if (drawing_history_is_ready(history) == 0U)
    {
        return DRAWING_HISTORY_INVALID_ARG;
    }
    if (history->active != 0U)
    {
        return DRAWING_HISTORY_BUSY;
    }
    if (history->visible_count >= history->stroke_count)
    {
        return DRAWING_HISTORY_EMPTY;
    }
    history->visible_count++;
    return DRAWING_HISTORY_OK;
}

void drawing_history_clear(drawing_history_t *history)
{
    if (drawing_history_is_ready(history) == 0U)
    {
        return;
    }
    history->stroke_count = 0U;
    history->visible_count = 0U;
    history->point_count = 0U;
    history->active_stroke_index = 0U;
    history->active = 0U;
}

uint32_t drawing_history_visible_count(const drawing_history_t *history)
{
    return (drawing_history_is_ready(history) != 0U) ?
           history->visible_count : 0U;
}

const drawing_stroke_t *drawing_history_get_stroke(
    const drawing_history_t *history, uint32_t index)
{
    if ((drawing_history_is_ready(history) == 0U) ||
        (index >= history->visible_count))
    {
        return NULL;
    }
    return &history->strokes[index];
}

const drawing_point_t *drawing_history_get_point(
    const drawing_history_t *history, const drawing_stroke_t *stroke,
    uint32_t point_index)
{
    uint32_t absolute_index;

    if ((drawing_history_is_ready(history) == 0U) || (stroke == NULL) ||
        (point_index >= stroke->point_count))
    {
        return NULL;
    }
    absolute_index = stroke->point_start + point_index;
    if ((absolute_index < stroke->point_start) ||
        (absolute_index >= history->point_count))
    {
        return NULL;
    }
    return &history->points[absolute_index];
}
