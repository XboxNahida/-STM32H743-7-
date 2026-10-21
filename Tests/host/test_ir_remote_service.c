#include <stdint.h>
#include <stdio.h>

#include "ir_remote_service.h"

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

static void feed_frame(uint8_t address, uint8_t command)
{
    uint8_t bytes[4];
    uint32_t index;
    uint8_t bit;

    bytes[0] = address;
    bytes[1] = (uint8_t)~address;
    bytes[2] = command;
    bytes[3] = (uint8_t)~command;
    ir_remote_capture_pulse(4500U);
    for (index = 0U; index < 4U; index++)
    {
        for (bit = 0U; bit < 8U; bit++)
        {
            ir_remote_capture_pulse(
                ((bytes[index] & (uint8_t)(1U << bit)) != 0U) ?
                1680U : 560U);
        }
    }
}

static int test_valid_frame_and_repeat(void)
{
    ir_remote_status_t status;
    int failures = 0;

    ir_remote_service_init();
    feed_frame(0x12U, 0xA5U);
    ir_remote_get_status(&status);
    failures += expect_u32("valid", status.frame_valid, 1U);
    failures += expect_u32("address", status.address, 0x12U);
    failures += expect_u32("command", status.command, 0xA5U);
    failures += expect_u32("frame_count", status.frame_count, 1U);
    failures += expect_u32("repeat_count_before", status.repeat_count, 0U);

    ir_remote_capture_pulse(2500U);
    ir_remote_get_status(&status);
    failures += expect_u32("repeat_count_after", status.repeat_count, 1U);
    failures += expect_u32("repeat_keeps_frame", status.command, 0xA5U);
    return failures;
}

static int test_periodic_updates_do_not_abort_frame(void)
{
    static const uint8_t bytes[4] = {0x12U, 0xEDU, 0xA5U, 0x5AU};
    ir_remote_status_t status;
    uint32_t index;
    uint8_t bit;
    int failures = 0;

    ir_remote_service_init();
    ir_remote_capture_pulse(4500U);
    ir_remote_timeout();
    for (index = 0U; index < 4U; index++)
    {
        for (bit = 0U; bit < 8U; bit++)
        {
            ir_remote_capture_pulse(
                ((bytes[index] & (uint8_t)(1U << bit)) != 0U) ?
                1680U : 560U);
            ir_remote_timeout();
        }
    }
    ir_remote_get_status(&status);
    failures += expect_u32("periodic_valid", status.frame_valid, 1U);
    failures += expect_u32("periodic_frame_count", status.frame_count, 1U);
    failures += expect_u32("periodic_invalid_count", status.invalid_count, 0U);
    return failures;
}

static int test_rejects_bad_frame_and_unknown_width(void)
{
    ir_remote_status_t status;
    int failures = 0;

    ir_remote_service_init();
    ir_remote_capture_pulse(4500U);
    ir_remote_capture_pulse(560U);
    ir_remote_capture_pulse(999U);
    ir_remote_get_status(&status);
    failures += expect_u32("unknown_invalid", status.invalid_count, 1U);
    failures += expect_u32("unknown_not_valid", status.frame_valid, 0U);

    ir_remote_service_init();
    ir_remote_capture_pulse(4500U);
    {
        uint8_t bytes[4] = {0x12U, 0x12U, 0xA5U, 0x5AU};
        uint32_t index;
        uint8_t bit;
        for (index = 0U; index < 4U; index++)
        {
            for (bit = 0U; bit < 8U; bit++)
            {
                ir_remote_capture_pulse(
                    ((bytes[index] & (uint8_t)(1U << bit)) != 0U) ?
                    1680U : 560U);
            }
        }
    }
    ir_remote_get_status(&status);
    failures += expect_u32("bad_complement_invalid", status.invalid_count, 1U);
    failures += expect_u32("bad_complement_not_valid", status.frame_valid, 0U);
    return failures;
}

static int test_timeout_snapshot_and_clear(void)
{
    ir_remote_status_t status;
    int failures = 0;

    ir_remote_service_init();
    ir_remote_capture_pulse(4500U);
    ir_remote_capture_pulse(560U);
    ir_remote_timeout();
    ir_remote_timeout();
    ir_remote_get_status(&status);
    failures += expect_u32("timeout_invalid", status.invalid_count, 1U);
    failures += expect_u32("timeout_not_valid", status.frame_valid, 0U);

    feed_frame(0x01U, 0x02U);
    ir_remote_get_status(&status);
    failures += expect_u32("snapshot_valid", status.frame_valid, 1U);
    ir_remote_clear();
    ir_remote_get_status(&status);
    failures += expect_u32("clear_valid", status.frame_valid, 0U);
    failures += expect_u32("clear_frames", status.frame_count, 0U);
    failures += expect_u32("clear_invalid", status.invalid_count, 0U);
    return failures;
}

static int test_suspend_and_resume(void)
{
    ir_remote_status_t status;
    int failures = 0;

    ir_remote_service_init();
    failures += expect_u32("suspend", ir_remote_suspend(), IR_REMOTE_OK);
    failures += expect_u32("suspend_idempotent",
                           ir_remote_suspend(), IR_REMOTE_OK);
    ir_remote_capture_pulse(4500U);
    ir_remote_capture_pulse(1680U);
    ir_remote_get_status(&status);
    failures += expect_u32("suspended_flag", status.suspended, 1U);
    failures += expect_u32("suspended_ignores_pulse", status.frame_count, 0U);
    failures += expect_u32("resume", ir_remote_resume(), IR_REMOTE_OK);
    failures += expect_u32("resume_idempotent", ir_remote_resume(), IR_REMOTE_OK);
    ir_remote_get_status(&status);
    failures += expect_u32("resumed_flag", status.suspended, 0U);
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_valid_frame_and_repeat();
    failures += test_periodic_updates_do_not_abort_frame();
    failures += test_rejects_bad_frame_and_unknown_width();
    failures += test_timeout_snapshot_and_clear();
    failures += test_suspend_and_resume();
    if (failures != 0)
    {
        printf("FAIL ir_remote_service failures=%d\n", failures);
        return 1;
    }
    printf("PASS ir_remote_service\n");
    return 0;
}
