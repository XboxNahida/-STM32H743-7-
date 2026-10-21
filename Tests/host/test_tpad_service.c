#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "tpad_service.h"

static uint32_t g_now_ms;
static uint32_t g_samples[32];
static uint32_t g_sample_count;
static uint32_t g_sample_index;
static int g_acquire_result;
static int g_capture_result;
static int g_restore_result;
static uint32_t g_acquire_count;
static uint32_t g_release_count;
static uint32_t g_restore_count;

uint32_t tpad_test_now_ms(void)
{
    return g_now_ms;
}

void tpad_test_delay_ms(uint32_t delay_ms)
{
    g_now_ms += delay_ms;
}

int tpad_test_acquire(void)
{
    g_acquire_count++;
    return g_acquire_result;
}

void tpad_test_release(void)
{
    g_release_count++;
}

int tpad_test_capture(uint32_t timeout_ms, uint32_t *raw)
{
    (void)timeout_ms;
    if (g_capture_result != 0)
    {
        return g_capture_result;
    }
    if (g_sample_index >= g_sample_count)
    {
        return 2;
    }
    *raw = g_samples[g_sample_index++];
    return 0;
}

int tpad_test_restore_analog(void)
{
    g_restore_count++;
    return g_restore_result;
}

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

static void reset_fixture(void)
{
    memset(g_samples, 0, sizeof(g_samples));
    g_now_ms = 100U;
    g_sample_count = 0U;
    g_sample_index = 0U;
    g_acquire_result = 0;
    g_capture_result = 0;
    g_restore_result = 0;
    g_acquire_count = 0U;
    g_release_count = 0U;
    g_restore_count = 0U;
    tpad_service_init();
}

static int test_calibration_and_touch_threshold(void)
{
    static const uint32_t calibration[10] =
        {100U, 90U, 110U, 95U, 105U, 100U, 98U, 102U, 500U, 1U};
    tpad_status_t status;
    int failures = 0;

    reset_fixture();
    memcpy(g_samples, calibration, sizeof(calibration));
    g_sample_count = 10U;
    failures += expect_i32("calibrate_result", tpad_calibrate(300U),
                           TPAD_OK);
    tpad_get_status(&status);
    failures += expect_u32("calibrated", status.calibrated, 1U);
    failures += expect_u32("baseline", status.baseline, 100U);
    failures += expect_u32("calibrate_count", status.calibrate_count, 1U);
    failures += expect_u32("calibrate_restore", g_restore_count, 1U);
    failures += expect_u32("calibrate_release", g_release_count, 1U);

    g_samples[0] = 150U;
    g_samples[1] = 180U;
    g_samples[2] = 170U;
    g_sample_count = 3U;
    g_sample_index = 0U;
    failures += expect_i32("read_not_touched", tpad_read(100U), TPAD_OK);
    tpad_get_status(&status);
    failures += expect_u32("not_touched_raw", status.last_raw, 180U);
    failures += expect_u32("not_touched", status.touched, 0U);

    g_samples[0] = 180U;
    g_samples[1] = 201U;
    g_samples[2] = 190U;
    g_sample_index = 0U;
    failures += expect_i32("read_touched", tpad_read(100U), TPAD_OK);
    tpad_get_status(&status);
    failures += expect_u32("touched_raw", status.last_raw, 201U);
    failures += expect_u32("touched", status.touched, 1U);
    return failures;
}

static int test_errors_restore_and_release(void)
{
    tpad_status_t status;
    int failures = 0;

    reset_fixture();
    g_acquire_result = 1;
    failures += expect_i32("resource_busy", tpad_calibrate(300U),
                           TPAD_NOT_READY);
    failures += expect_u32("busy_no_restore", g_restore_count, 0U);
    failures += expect_u32("busy_no_release", g_release_count, 0U);

    reset_fixture();
    g_capture_result = 2;
    failures += expect_i32("capture_timeout", tpad_calibrate(300U),
                           TPAD_TIMEOUT);
    failures += expect_u32("timeout_restore", g_restore_count, 1U);
    failures += expect_u32("timeout_release", g_release_count, 1U);

    reset_fixture();
    g_samples[0] = 100U;
    g_sample_count = 10U;
    g_restore_result = 1;
    failures += expect_i32("restore_error", tpad_calibrate(300U),
                           TPAD_RESTORE_ERROR);

    reset_fixture();
    failures += expect_i32("read_not_calibrated", tpad_read(100U),
                           TPAD_NOT_CALIBRATED);
    failures += expect_i32("zero_timeout", tpad_calibrate(0U),
                           TPAD_INVALID_ARG);
    tpad_get_status(&status);
    failures += expect_u32("zero_timeout_no_acquire", g_acquire_count, 0U);
    tpad_get_status(NULL);
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_calibration_and_touch_threshold();
    failures += test_errors_restore_and_release();
    if (failures != 0)
    {
        printf("FAIL tpad_service failures=%d\n", failures);
        return 1;
    }
    printf("PASS tpad_service\n");
    return 0;
}
