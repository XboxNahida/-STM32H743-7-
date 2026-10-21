#include <stdint.h>
#include <stdio.h>

#include "piano_touch_mapper.h"

static int expect_u64(const char *name, uint64_t actual, uint64_t expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=0x%llX expected=0x%llX\n", name,
               (unsigned long long)actual, (unsigned long long)expected);
        return 1;
    }
    return 0;
}

static int expect_i32(const char *name, int32_t actual, int32_t expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=%ld expected=%ld\n", name,
               (long)actual, (long)expected);
        return 1;
    }
    return 0;
}

static uint64_t note_bit(uint8_t note)
{
    return 1ULL << note;
}

static int test_white_and_black_keys(void)
{
    piano_touch_point_t point;
    uint64_t mask = 0U;
    int failures = 0;

    point.x = 40;
    point.y = 500;
    failures += expect_i32("white_map_result",
        piano_touch_map(PIANO_TOUCH_MODE_KEYS, &point, 1U, 0, &mask),
        PIANO_TOUCH_OK);
    failures += expect_u64("white_c4", mask, note_bit(12U));

    point.x = 60;
    point.y = 100;
    failures += expect_i32("black_map_result",
        piano_touch_map(PIANO_TOUCH_MODE_KEYS, &point, 1U, 0, &mask),
        PIANO_TOUCH_OK);
    failures += expect_u64("black_cs4_priority", mask, note_bit(13U));

    point.y = 500;
    failures += expect_i32("same_x_lower_white_result",
        piano_touch_map(PIANO_TOUCH_MODE_KEYS, &point, 1U, 0, &mask),
        PIANO_TOUCH_OK);
    failures += expect_u64("same_x_lower_white", mask, note_bit(12U));
    return failures;
}

static int test_five_points_and_deduplication(void)
{
    static const piano_touch_point_t points[5] = {
        {40, 500}, {94, 500}, {148, 500}, {202, 500}, {256, 500}
    };
    static const piano_touch_point_t duplicate[2] = {
        {40, 500}, {42, 480}
    };
    uint64_t mask = 0U;
    int failures = 0;

    failures += expect_i32("five_map_result",
        piano_touch_map(PIANO_TOUCH_MODE_KEYS, points, 5U, 0, &mask),
        PIANO_TOUCH_OK);
    failures += expect_u64("five_white_notes", mask,
        note_bit(12U) | note_bit(14U) | note_bit(16U) |
        note_bit(17U) | note_bit(19U));
    failures += expect_i32("dedup_result",
        piano_touch_map(PIANO_TOUCH_MODE_KEYS, duplicate, 2U, 0, &mask),
        PIANO_TOUCH_OK);
    failures += expect_u64("dedup_same_key", mask, note_bit(12U));
    return failures;
}

static int test_matrix_and_transpose(void)
{
    static const piano_touch_point_t points[3] = {
        {80, 100}, {80, 280}, {680, 480}
    };
    uint64_t mask = 0U;
    int failures = 0;

    failures += expect_i32("matrix_result",
        piano_touch_map(PIANO_TOUCH_MODE_MATRIX, points, 3U, 0, &mask),
        PIANO_TOUCH_OK);
    failures += expect_u64("matrix_notes", mask,
        note_bit(12U) | note_bit(21U) | note_bit(36U));
    failures += expect_i32("transpose_result",
        piano_touch_map(PIANO_TOUCH_MODE_MATRIX, points, 3U, 2, &mask),
        PIANO_TOUCH_OK);
    failures += expect_u64("matrix_transposed", mask,
        note_bit(14U) | note_bit(23U) | note_bit(38U));
    return failures;
}

static int test_ignored_and_invalid_points(void)
{
    static const piano_touch_point_t control = {900, 300};
    uint64_t mask = 123U;
    int failures = 0;

    failures += expect_i32("control_ignored_result",
        piano_touch_map(PIANO_TOUCH_MODE_KEYS, &control, 1U, 0, &mask),
        PIANO_TOUCH_OK);
    failures += expect_u64("control_ignored", mask, 0U);
    failures += expect_i32("too_many_points",
        piano_touch_map(PIANO_TOUCH_MODE_KEYS, &control, 6U, 0, &mask),
        PIANO_TOUCH_INVALID_ARG);
    failures += expect_i32("invalid_transpose",
        piano_touch_map(PIANO_TOUCH_MODE_KEYS, &control, 1U, 13, &mask),
        PIANO_TOUCH_INVALID_ARG);
    failures += expect_i32("null_mask",
        piano_touch_map(PIANO_TOUCH_MODE_KEYS, &control, 1U, 0, NULL),
        PIANO_TOUCH_INVALID_ARG);
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_white_and_black_keys();
    failures += test_five_points_and_deduplication();
    failures += test_matrix_and_transpose();
    failures += test_ignored_and_invalid_points();
    if (failures != 0)
    {
        printf("FAIL piano_touch_mapper failures=%d\n", failures);
        return 1;
    }
    printf("PASS piano_touch_mapper\n");
    return 0;
}
