#include <stdint.h>
#include <stdio.h>

#include "nes_video.h"

static uint16_t g_buffer_a[NES_VIDEO_PIXEL_COUNT];
static uint16_t g_buffer_b[NES_VIDEO_PIXEL_COUNT];
static unsigned int g_start_count;
static unsigned int g_present_count;
static unsigned int g_stop_count;
static int g_start_result;
static int g_present_result;
static uint32_t g_last_generation;

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

static int direct_start(void *context)
{
    (void)context;
    g_start_count++;
    return g_start_result;
}

static int direct_present(void *context, const nes_video_frame_t *frame)
{
    (void)context;
    g_present_count++;
    g_last_generation = frame->generation;
    return g_present_result;
}

static void direct_stop(void *context)
{
    (void)context;
    g_stop_count++;
}

static void reset_backend_fixture(void)
{
    g_start_count = 0U;
    g_present_count = 0U;
    g_stop_count = 0U;
    g_start_result = 0;
    g_present_result = 0;
    g_last_generation = 0U;
}

static int test_init_validation(void)
{
    int failures = 0;

    failures += expect_u32("null_a",
        (uint32_t)nes_video_init(NULL, g_buffer_b, NES_VIDEO_PIXEL_COUNT),
        NES_VIDEO_INVALID_ARG);
    failures += expect_u32("null_b",
        (uint32_t)nes_video_init(g_buffer_a, NULL, NES_VIDEO_PIXEL_COUNT),
        NES_VIDEO_INVALID_ARG);
    failures += expect_u32("same_buffer",
        (uint32_t)nes_video_init(g_buffer_a, g_buffer_a,
                                 NES_VIDEO_PIXEL_COUNT),
        NES_VIDEO_INVALID_ARG);
    failures += expect_u32("small_buffer",
        (uint32_t)nes_video_init(g_buffer_a, g_buffer_b,
                                 NES_VIDEO_PIXEL_COUNT - 1U),
        NES_VIDEO_INVALID_ARG);
    return failures;
}

static int test_default_overlay_and_double_buffer(void)
{
    nes_video_status_t status;
    nes_video_frame_t frame;
    uint16_t *target = NULL;
    int failures = 0;

    failures += expect_u32("init",
        (uint32_t)nes_video_init(g_buffer_a, g_buffer_b,
                                 NES_VIDEO_PIXEL_COUNT),
        NES_VIDEO_OK);
    nes_video_get_status(&status);
    failures += expect_u32("default_mode", status.output_mode,
                           NES_VIDEO_OUTPUT_LVGL_OVERLAY);
    failures += expect_u32("no_frame",
        (uint32_t)nes_video_get_frame(&frame), NES_VIDEO_NOT_READY);
    failures += expect_u32("acquire_a",
        (uint32_t)nes_video_acquire_render_buffer(&target), NES_VIDEO_OK);
    failures += expect_u32("target_a", target == g_buffer_a, 1U);
    failures += expect_u32("double_acquire_busy",
        (uint32_t)nes_video_acquire_render_buffer(&target), NES_VIDEO_BUSY);
    g_buffer_a[0] = 0x1234U;
    failures += expect_u32("publish_a", (uint32_t)nes_video_publish(),
                           NES_VIDEO_OK);
    failures += expect_u32("get_a",
        (uint32_t)nes_video_get_frame(&frame), NES_VIDEO_OK);
    failures += expect_u32("frame_a", frame.pixels == g_buffer_a, 1U);
    failures += expect_u32("frame_width", frame.width, NES_VIDEO_WIDTH);
    failures += expect_u32("frame_height", frame.height, NES_VIDEO_HEIGHT);
    failures += expect_u32("frame_generation_1", frame.generation, 1U);
    failures += expect_u32("frame_pixel", frame.pixels[0], 0x1234U);
    failures += expect_u32("consume_a",
        (uint32_t)nes_video_mark_consumed(1U), NES_VIDEO_OK);

    failures += expect_u32("acquire_b",
        (uint32_t)nes_video_acquire_render_buffer(&target), NES_VIDEO_OK);
    failures += expect_u32("target_b", target == g_buffer_b, 1U);
    failures += expect_u32("publish_b", (uint32_t)nes_video_publish(),
                           NES_VIDEO_OK);
    failures += expect_u32("get_b",
        (uint32_t)nes_video_get_frame(&frame), NES_VIDEO_OK);
    failures += expect_u32("frame_b", frame.pixels == g_buffer_b, 1U);
    failures += expect_u32("frame_generation_2", frame.generation, 2U);
    failures += expect_u32("acquire_a_again",
        (uint32_t)nes_video_acquire_render_buffer(&target), NES_VIDEO_OK);
    failures += expect_u32("target_a_again", target == g_buffer_a, 1U);
    nes_video_deinit();
    return failures;
}

static int test_consumption_and_lag_detection(void)
{
    static const nes_video_backend_ops_t ops = {
        direct_start, direct_present, direct_stop
    };
    uint16_t *target;
    int failures = 0;

    reset_backend_fixture();
    (void)nes_video_init(g_buffer_a, g_buffer_b, NES_VIDEO_PIXEL_COUNT);
    failures += expect_u32("no_backend_no_suggestion",
                           nes_video_should_use_direct(0U), 0U);
    failures += expect_u32("register_backend",
        (uint32_t)nes_video_register_direct_backend(&ops, NULL), NES_VIDEO_OK);
    (void)nes_video_acquire_render_buffer(&target);
    (void)nes_video_publish();
    (void)nes_video_acquire_render_buffer(&target);
    (void)nes_video_publish();
    failures += expect_u32("both_buffers_unconsumed_busy",
        (uint32_t)nes_video_acquire_render_buffer(&target), NES_VIDEO_BUSY);
    failures += expect_u32("lag_above_one",
                           nes_video_should_use_direct(1U), 1U);
    failures += expect_u32("consume_generation_1",
        (uint32_t)nes_video_mark_consumed(1U), NES_VIDEO_OK);
    failures += expect_u32("consumed_buffer_reusable",
        (uint32_t)nes_video_acquire_render_buffer(&target), NES_VIDEO_OK);
    failures += expect_u32("lag_equal_one",
                           nes_video_should_use_direct(1U), 0U);
    failures += expect_u32("future_generation_rejected",
        (uint32_t)nes_video_mark_consumed(3U), NES_VIDEO_INVALID_ARG);
    failures += expect_u32("consume_generation_2",
        (uint32_t)nes_video_mark_consumed(2U), NES_VIDEO_OK);
    nes_video_deinit();
    return failures;
}

static int test_direct_mode_lifecycle(void)
{
    static const nes_video_backend_ops_t ops = {
        direct_start, direct_present, direct_stop
    };
    nes_video_status_t status;
    uint16_t *target;
    int failures = 0;

    reset_backend_fixture();
    (void)nes_video_init(g_buffer_a, g_buffer_b, NES_VIDEO_PIXEL_COUNT);
    failures += expect_u32("direct_without_backend",
        (uint32_t)nes_video_set_output_mode(NES_VIDEO_OUTPUT_DIRECT),
        NES_VIDEO_NOT_READY);
    failures += expect_u32("register_direct",
        (uint32_t)nes_video_register_direct_backend(&ops, NULL), NES_VIDEO_OK);
    failures += expect_u32("switch_direct",
        (uint32_t)nes_video_set_output_mode(NES_VIDEO_OUTPUT_DIRECT),
        NES_VIDEO_OK);
    failures += expect_u32("direct_started", g_start_count, 1U);
    (void)nes_video_acquire_render_buffer(&target);
    failures += expect_u32("direct_publish", (uint32_t)nes_video_publish(),
                           NES_VIDEO_OK);
    failures += expect_u32("direct_presented", g_present_count, 1U);
    failures += expect_u32("direct_generation", g_last_generation, 1U);
    nes_video_get_status(&status);
    failures += expect_u32("direct_consumed", status.consumed_generation, 1U);
    failures += expect_u32("switch_overlay",
        (uint32_t)nes_video_set_output_mode(NES_VIDEO_OUTPUT_LVGL_OVERLAY),
        NES_VIDEO_OK);
    failures += expect_u32("direct_stopped", g_stop_count, 1U);
    nes_video_deinit();
    return failures;
}

static int test_backend_failures_and_deinit(void)
{
    static const nes_video_backend_ops_t ops = {
        direct_start, direct_present, direct_stop
    };
    nes_video_status_t status;
    uint16_t *target;
    int failures = 0;

    reset_backend_fixture();
    (void)nes_video_init(g_buffer_a, g_buffer_b, NES_VIDEO_PIXEL_COUNT);
    (void)nes_video_register_direct_backend(&ops, NULL);
    g_start_result = 1;
    failures += expect_u32("start_failure",
        (uint32_t)nes_video_set_output_mode(NES_VIDEO_OUTPUT_DIRECT),
        NES_VIDEO_BACKEND_ERROR);
    nes_video_get_status(&status);
    failures += expect_u32("start_failure_keeps_overlay", status.output_mode,
                           NES_VIDEO_OUTPUT_LVGL_OVERLAY);
    g_start_result = 0;
    (void)nes_video_set_output_mode(NES_VIDEO_OUTPUT_DIRECT);
    g_present_result = 1;
    (void)nes_video_acquire_render_buffer(&target);
    failures += expect_u32("present_failure", (uint32_t)nes_video_publish(),
                           NES_VIDEO_BACKEND_ERROR);
    nes_video_get_status(&status);
    failures += expect_u32("dropped_frame", status.dropped_frames, 1U);
    failures += expect_u32("failed_frame_still_published",
                           status.generation, 1U);
    nes_video_deinit();
    failures += expect_u32("deinit_stops_direct", g_stop_count, 1U);
    failures += expect_u32("acquire_after_deinit",
        (uint32_t)nes_video_acquire_render_buffer(&target),
        NES_VIDEO_NOT_READY);
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += expect_u32("native_width", NES_VIDEO_WIDTH, 512U);
    failures += expect_u32("native_height", NES_VIDEO_HEIGHT, 480U);
    failures += test_init_validation();
    failures += test_default_overlay_and_double_buffer();
    failures += test_consumption_and_lag_detection();
    failures += test_direct_mode_lifecycle();
    failures += test_backend_failures_and_deinit();
    if (failures != 0)
    {
        printf("FAIL nes_video failures=%d\n", failures);
        return 1;
    }
    printf("PASS nes_video\n");
    return 0;
}
