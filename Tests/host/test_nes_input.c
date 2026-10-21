#include <stdint.h>
#include <stdio.h>

#include "nes_input.h"

static int expect_u32(const char *name, uint32_t actual, uint32_t expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=0x%08lX expected=0x%08lX\n", name,
               (unsigned long)actual, (unsigned long)expected);
        return 1;
    }
    return 0;
}

static int test_official_button_layout(void)
{
    int failures = 0;

    failures += expect_u32("button_a", NES_BUTTON_A, 0x01U);
    failures += expect_u32("button_b", NES_BUTTON_B, 0x02U);
    failures += expect_u32("button_select", NES_BUTTON_SELECT, 0x04U);
    failures += expect_u32("button_start", NES_BUTTON_START, 0x08U);
    failures += expect_u32("button_up", NES_BUTTON_UP, 0x10U);
    failures += expect_u32("button_down", NES_BUTTON_DOWN, 0x20U);
    failures += expect_u32("button_left", NES_BUTTON_LEFT, 0x40U);
    failures += expect_u32("button_right", NES_BUTTON_RIGHT, 0x80U);
    return failures;
}

static int test_source_merge_and_release(void)
{
    int failures = 0;

    nes_input_init();
    failures += expect_u32("initial_buttons", nes_input_get_buttons(), 0U);
    failures += expect_u32("touch_set_result",
        (uint32_t)nes_input_set_source(NES_INPUT_SOURCE_TOUCH,
                                      NES_BUTTON_LEFT | NES_BUTTON_A),
        NES_INPUT_OK);
    failures += expect_u32("external_set_result",
        (uint32_t)nes_input_set_source(NES_INPUT_SOURCE_EXTERNAL,
                                      NES_BUTTON_UP | NES_BUTTON_B),
        NES_INPUT_OK);
    failures += expect_u32("merged_sources", nes_input_get_buttons(),
        NES_BUTTON_LEFT | NES_BUTTON_UP | NES_BUTTON_A | NES_BUTTON_B);
    failures += expect_u32("touch_snapshot",
        nes_input_get_source(NES_INPUT_SOURCE_TOUCH),
        NES_BUTTON_LEFT | NES_BUTTON_A);
    failures += expect_u32("touch_clear_result",
        (uint32_t)nes_input_set_source(NES_INPUT_SOURCE_TOUCH, 0U),
        NES_INPUT_OK);
    failures += expect_u32("external_survives", nes_input_get_buttons(),
        NES_BUTTON_UP | NES_BUTTON_B);
    failures += expect_u32("invalid_source_set",
        (uint32_t)nes_input_set_source(NES_INPUT_SOURCE_COUNT, 0U),
        NES_INPUT_INVALID_ARG);
    failures += expect_u32("invalid_source_get",
        nes_input_get_source(NES_INPUT_SOURCE_COUNT), 0U);
    return failures;
}

static int test_five_point_touch_mapping(void)
{
    static const nes_touch_region_t regions[] = {
        {100, 400, 70U, 70U, NES_BUTTON_UP},
        {30, 470, 70U, 70U, NES_BUTTON_LEFT},
        {170, 470, 70U, 70U, NES_BUTTON_RIGHT},
        {100, 540, 70U, 60U, NES_BUTTON_DOWN},
        {760, 430, 90U, 90U, NES_BUTTON_B},
        {870, 390, 90U, 90U, NES_BUTTON_A},
        {430, 530, 80U, 50U, NES_BUTTON_SELECT},
        {530, 530, 80U, 50U, NES_BUTTON_START}
    };
    static const nes_touch_point_t points[NES_INPUT_MAX_TOUCH_POINTS] = {
        {110, 410}, {40, 480}, {780, 450}, {900, 420}, {550, 550}
    };
    static const nes_touch_point_t outside[] = {{-1, 450}, {1023, 20}};
    nes_button_mask_t buttons = 0U;
    int failures = 0;

    failures += expect_u32("five_touch_result",
        (uint32_t)nes_input_map_touch(points, NES_INPUT_MAX_TOUCH_POINTS,
                                      regions,
                                      (uint8_t)(sizeof(regions) /
                                                sizeof(regions[0])),
                                      &buttons),
        NES_INPUT_OK);
    failures += expect_u32("five_touch_buttons", buttons,
        NES_BUTTON_UP | NES_BUTTON_LEFT | NES_BUTTON_B |
        NES_BUTTON_A | NES_BUTTON_START);
    failures += expect_u32("outside_touch_result",
        (uint32_t)nes_input_map_touch(outside, 2U, regions,
                                      (uint8_t)(sizeof(regions) /
                                                sizeof(regions[0])),
                                      &buttons),
        NES_INPUT_OK);
    failures += expect_u32("outside_touch_buttons", buttons, 0U);
    failures += expect_u32("empty_touch_result",
        (uint32_t)nes_input_map_touch(NULL, 0U, NULL, 0U, &buttons),
        NES_INPUT_OK);
    failures += expect_u32("empty_touch_buttons", buttons, 0U);
    return failures;
}

static int test_touch_validation_and_overlap(void)
{
    static const nes_touch_point_t point = {120, 120};
    static const nes_touch_region_t overlap[] = {
        {100, 100, 50U, 50U, NES_BUTTON_A},
        {100, 100, 50U, 50U, NES_BUTTON_B}
    };
    static const nes_touch_region_t zero_width = {
        100, 100, 0U, 50U, NES_BUTTON_A
    };
    nes_button_mask_t buttons = 0U;
    int failures = 0;

    failures += expect_u32("overlap_result",
        (uint32_t)nes_input_map_touch(&point, 1U, overlap, 2U, &buttons),
        NES_INPUT_OK);
    failures += expect_u32("first_overlap_wins", buttons, NES_BUTTON_A);
    failures += expect_u32("too_many_points",
        (uint32_t)nes_input_map_touch(&point,
                                      NES_INPUT_MAX_TOUCH_POINTS + 1U,
                                      overlap, 2U, &buttons),
        NES_INPUT_INVALID_ARG);
    failures += expect_u32("too_many_regions",
        (uint32_t)nes_input_map_touch(&point, 1U, overlap,
                                      NES_INPUT_MAX_TOUCH_REGIONS + 1U,
                                      &buttons),
        NES_INPUT_INVALID_ARG);
    failures += expect_u32("null_points",
        (uint32_t)nes_input_map_touch(NULL, 1U, overlap, 2U, &buttons),
        NES_INPUT_INVALID_ARG);
    failures += expect_u32("null_regions",
        (uint32_t)nes_input_map_touch(&point, 1U, NULL, 1U, &buttons),
        NES_INPUT_INVALID_ARG);
    failures += expect_u32("null_output",
        (uint32_t)nes_input_map_touch(&point, 1U, overlap, 2U, NULL),
        NES_INPUT_INVALID_ARG);
    failures += expect_u32("zero_width",
        (uint32_t)nes_input_map_touch(&point, 1U, &zero_width, 1U,
                                      &buttons),
        NES_INPUT_INVALID_ARG);
    return failures;
}

static int test_ir_mapping_and_release(void)
{
    static const nes_ir_binding_t bindings[] = {
        {0x00U, 0x18U, NES_BUTTON_UP},
        {0x00U, 0x16U, NES_BUTTON_A},
        {0x00U, 0x0DU, NES_BUTTON_START}
    };
    nes_ir_snapshot_t snapshot = {1U, 0x00U, 0x16U, 1U, 0U};
    int failures = 0;

    nes_input_init();
    failures += expect_u32("ir_map_result",
        (uint32_t)nes_input_set_ir_map(bindings,
            (uint8_t)(sizeof(bindings) / sizeof(bindings[0]))),
        NES_INPUT_OK);
    failures += expect_u32("ir_new_frame",
        (uint32_t)nes_input_update_ir(&snapshot, 100U), NES_INPUT_OK);
    failures += expect_u32("ir_a_pressed", nes_input_get_buttons(),
                           NES_BUTTON_A);
    failures += expect_u32("ir_same_frame",
        (uint32_t)nes_input_update_ir(&snapshot, 200U), NES_INPUT_OK);
    failures += expect_u32("same_frame_does_not_refresh",
        (uint32_t)nes_input_update_ir(&snapshot, 281U), NES_INPUT_OK);
    failures += expect_u32("ir_released_after_timeout",
                           nes_input_get_buttons(), 0U);

    snapshot.repeat_count = 1U;
    failures += expect_u32("ir_repeat",
        (uint32_t)nes_input_update_ir(&snapshot, 300U), NES_INPUT_OK);
    failures += expect_u32("repeat_restores_last_button",
                           nes_input_get_buttons(), NES_BUTTON_A);
    failures += expect_u32("repeat_hold_before_timeout",
        (uint32_t)nes_input_update_ir(&snapshot,
                                      300U + NES_INPUT_IR_RELEASE_MS),
        NES_INPUT_OK);
    failures += expect_u32("ir_still_held_at_boundary",
                           nes_input_get_buttons(), NES_BUTTON_A);
    failures += expect_u32("repeat_timeout",
        (uint32_t)nes_input_update_ir(&snapshot,
                                      301U + NES_INPUT_IR_RELEASE_MS),
        NES_INPUT_OK);
    failures += expect_u32("repeat_released", nes_input_get_buttons(), 0U);

    snapshot.command = 0x18U;
    snapshot.frame_count = 2U;
    failures += expect_u32("ir_command_change",
        (uint32_t)nes_input_update_ir(&snapshot, 600U), NES_INPUT_OK);
    failures += expect_u32("ir_up_pressed", nes_input_get_buttons(),
                           NES_BUTTON_UP);
    snapshot.frame_valid = 0U;
    failures += expect_u32("ir_invalid_frame",
        (uint32_t)nes_input_update_ir(&snapshot, 601U), NES_INPUT_OK);
    failures += expect_u32("invalid_frame_releases", nes_input_get_buttons(),
                           0U);
    return failures;
}

static int test_ir_validation_and_tick_wrap(void)
{
    static const nes_ir_binding_t binding = {
        0x00U, 0x16U, NES_BUTTON_A
    };
    static const nes_ir_binding_t duplicate[2] = {
        {0x00U, 0x16U, NES_BUTTON_A},
        {0x00U, 0x16U, NES_BUTTON_B}
    };
    nes_ir_snapshot_t snapshot = {1U, 0x00U, 0x16U, 1U, 0U};
    int failures = 0;

    nes_input_init();
    failures += expect_u32("null_ir_map",
        (uint32_t)nes_input_set_ir_map(NULL, 1U), NES_INPUT_INVALID_ARG);
    failures += expect_u32("duplicate_ir_map",
        (uint32_t)nes_input_set_ir_map(duplicate, 2U),
        NES_INPUT_INVALID_ARG);
    failures += expect_u32("valid_wrap_map",
        (uint32_t)nes_input_set_ir_map(&binding, 1U), NES_INPUT_OK);
    failures += expect_u32("null_ir_snapshot",
        (uint32_t)nes_input_update_ir(NULL, 0U), NES_INPUT_INVALID_ARG);
    failures += expect_u32("wrap_press",
        (uint32_t)nes_input_update_ir(&snapshot, UINT32_MAX - 20U),
        NES_INPUT_OK);
    failures += expect_u32("wrap_held",
        (uint32_t)nes_input_update_ir(&snapshot, 100U), NES_INPUT_OK);
    failures += expect_u32("wrap_still_pressed", nes_input_get_buttons(),
                           NES_BUTTON_A);
    failures += expect_u32("wrap_release",
        (uint32_t)nes_input_update_ir(&snapshot, 200U), NES_INPUT_OK);
    failures += expect_u32("wrap_released", nes_input_get_buttons(), 0U);
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_official_button_layout();
    failures += test_source_merge_and_release();
    failures += test_five_point_touch_mapping();
    failures += test_touch_validation_and_overlap();
    failures += test_ir_mapping_and_release();
    failures += test_ir_validation_and_tick_wrap();
    if (failures != 0)
    {
        printf("FAIL nes_input failures=%d\n", failures);
        return 1;
    }
    printf("PASS nes_input\n");
    return 0;
}
