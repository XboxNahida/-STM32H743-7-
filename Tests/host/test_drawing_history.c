#include <stdint.h>
#include <stdio.h>

#include "drawing_history.h"

static int expect_u32(const char *name, uint32_t actual, uint32_t expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=%lu expected=%lu\n", name,
               (unsigned long)actual, (unsigned long)expected);
        return 1;
    }
    return 0;
}

static int expect_i32(const char *name, int actual, int expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=%d expected=%d\n", name, actual, expected);
        return 1;
    }
    return 0;
}

int main(void)
{
    drawing_history_t history;
    drawing_stroke_t strokes[3];
    drawing_point_t points[6];
    const drawing_stroke_t *stroke;
    const drawing_point_t *point;
    int failures = 0;

    failures += expect_i32("init", drawing_history_init(
        &history, strokes, 3U, points, 6U), DRAWING_HISTORY_OK);
    failures += expect_u32("initial_visible",
                           drawing_history_visible_count(&history), 0U);

    failures += expect_i32("begin_first", drawing_history_begin(
        &history, 10, 20, 0x1234U, 8U, 0U), DRAWING_HISTORY_OK);
    failures += expect_i32("skip_duplicate", drawing_history_append(
        &history, 10, 20), DRAWING_HISTORY_OK);
    failures += expect_i32("append_first", drawing_history_append(
        &history, 12, 23), DRAWING_HISTORY_OK);
    failures += expect_i32("end_first", drawing_history_end(&history),
                           DRAWING_HISTORY_OK);
    failures += expect_u32("first_visible",
                           drawing_history_visible_count(&history), 1U);
    stroke = drawing_history_get_stroke(&history, 0U);
    failures += expect_u32("first_points",
                           (stroke != NULL) ? stroke->point_count : 0U, 2U);
    failures += expect_u32("first_color",
                           (stroke != NULL) ? stroke->color_rgb565 : 0U,
                           0x1234U);
    point = drawing_history_get_point(&history, stroke, 1U);
    failures += expect_i32("point_x", (point != NULL) ? point->x : -1, 12);
    failures += expect_i32("point_y", (point != NULL) ? point->y : -1, 23);

    failures += expect_i32("begin_second", drawing_history_begin(
        &history, 30, 40, 0xFFFFU, 16U, 1U), DRAWING_HISTORY_OK);
    failures += expect_i32("end_second", drawing_history_end(&history),
                           DRAWING_HISTORY_OK);
    failures += expect_i32("undo", drawing_history_undo(&history),
                           DRAWING_HISTORY_OK);
    failures += expect_u32("undo_visible",
                           drawing_history_visible_count(&history), 1U);
    failures += expect_i32("redo", drawing_history_redo(&history),
                           DRAWING_HISTORY_OK);
    failures += expect_u32("redo_visible",
                           drawing_history_visible_count(&history), 2U);

    failures += expect_i32("undo_for_branch", drawing_history_undo(&history),
                           DRAWING_HISTORY_OK);
    failures += expect_i32("begin_branch", drawing_history_begin(
        &history, 50, 60, 0x07E0U, 4U, 0U), DRAWING_HISTORY_OK);
    failures += expect_i32("end_branch", drawing_history_end(&history),
                           DRAWING_HISTORY_OK);
    failures += expect_i32("redo_removed", drawing_history_redo(&history),
                           DRAWING_HISTORY_EMPTY);
    stroke = drawing_history_get_stroke(&history, 1U);
    failures += expect_u32("branch_start",
                           (stroke != NULL) ? stroke->point_start : UINT32_MAX,
                           2U);

    failures += expect_i32("begin_capacity", drawing_history_begin(
        &history, 1, 1, 0U, 1U, 0U), DRAWING_HISTORY_OK);
    failures += expect_i32("append_capacity_1", drawing_history_append(
        &history, 2, 2), DRAWING_HISTORY_OK);
    failures += expect_i32("append_capacity_2", drawing_history_append(
        &history, 3, 3), DRAWING_HISTORY_OK);
    failures += expect_i32("append_full", drawing_history_append(
        &history, 4, 4), DRAWING_HISTORY_FULL);
    failures += expect_i32("end_capacity", drawing_history_end(&history),
                           DRAWING_HISTORY_OK);
    failures += expect_u32("capacity_visible",
                           drawing_history_visible_count(&history), 3U);

    drawing_history_clear(&history);
    failures += expect_u32("clear_visible",
                           drawing_history_visible_count(&history), 0U);
    failures += expect_i32("undo_empty", drawing_history_undo(&history),
                           DRAWING_HISTORY_EMPTY);
    failures += expect_i32("invalid_width", drawing_history_begin(
        &history, 0, 0, 0U, 0U, 0U), DRAWING_HISTORY_INVALID_ARG);

    if (failures != 0)
    {
        printf("FAIL drawing_history failures=%d\n", failures);
        return 1;
    }
    printf("PASS drawing_history\n");
    return 0;
}
