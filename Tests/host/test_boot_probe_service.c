#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "boot_probe_service.h"

#define TEST_ENTRY_COUNT 5U

static uint32_t g_now_ms;
static uint32_t g_required_ok_duration_ms;
static uint32_t g_callback_count;
static uint32_t g_unsupported_bus_count;
static uint32_t g_diagnostics_capture_count;
static uint16_t g_diagnostics_module;
static int32_t g_diagnostics_code;
static uint32_t g_diagnostics_detail0;
static uint32_t g_diagnostics_detail1;
static uint32_t g_print_call_count;

int boot_probe_test_printf(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    va_end(args);
    g_print_call_count++;
    return 0;
}

uint32_t boot_probe_test_now_ms(void)
{
    return g_now_ms;
}

void diagnostics_capture_first(uint16_t module,
                               int32_t code,
                               uint32_t detail0,
                               uint32_t detail1)
{
    g_diagnostics_capture_count++;
    g_diagnostics_module = module;
    g_diagnostics_code = code;
    g_diagnostics_detail0 = detail0;
    g_diagnostics_detail1 = detail1;
}

static int probe_required_ok(boot_probe_status_t *status)
{
    g_callback_count++;
    g_now_ms += g_required_ok_duration_ms;
    status->present = 1U;
    status->ready = 1U;
    status->detail0 = 0x1234U;
    return BOOT_PROBE_OK;
}

static int probe_optional_absent(boot_probe_status_t *status)
{
    g_callback_count++;
    g_now_ms += g_required_ok_duration_ms;
    status->present = 0U;
    status->ready = 0U;
    return BOOT_PROBE_ABSENT;
}

static int probe_required_error(boot_probe_status_t *status)
{
    g_callback_count++;
    g_now_ms += g_required_ok_duration_ms;
    status->present = 1U;
    status->ready = 0U;
    status->detail0 = 0xDEADU;
    status->detail1 = 0xBEEFU;
    return BOOT_PROBE_ERROR;
}

static int probe_after_unsupported(boot_probe_status_t *status)
{
    g_callback_count++;
    g_now_ms += g_required_ok_duration_ms;
    status->present = 1U;
    status->ready = 1U;
    return BOOT_PROBE_OK;
}

static const boot_probe_entry_t g_entries[TEST_ENTRY_COUNT] =
{
    {"required_ok", BOOT_PROBE_KIND_INTERNAL, 1U, probe_required_ok},
    {"optional_absent", BOOT_PROBE_KIND_I2C, 0U, probe_optional_absent},
    {"required_error", BOOT_PROBE_KIND_STORAGE, 1U, probe_required_error},
    {"unsupported", BOOT_PROBE_KIND_SENSOR, 0U, NULL},
    {"after_unsupported", BOOT_PROBE_KIND_INTERNAL, 0U,
     probe_after_unsupported}
};

const boot_probe_entry_t *boot_probe_target_entries(uint32_t *count)
{
    if (count != NULL)
    {
        *count = TEST_ENTRY_COUNT;
    }
    return g_entries;
}

static int expect_u32(const char *name, uint32_t actual, uint32_t expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=%lu expected=%lu\n",
               name,
               (unsigned long)actual,
               (unsigned long)expected);
        return 1;
    }
    return 0;
}

static int expect_status(uint32_t index,
                         int32_t expected_result,
                         uint8_t expected_present,
                         uint8_t expected_ready)
{
    boot_probe_status_t status;
    int failures = 0;

    if (boot_probe_get_status(index, &status) != BOOT_PROBE_OK)
    {
        printf("FAIL get_status index=%lu\n", (unsigned long)index);
        return 1;
    }
    failures += expect_u32("status_result", (uint32_t)status.result,
                           (uint32_t)expected_result);
    failures += expect_u32("status_present", status.present,
                           expected_present);
    failures += expect_u32("status_ready", status.ready, expected_ready);
    return failures;
}

static void reset_fixture(void)
{
    g_now_ms = 0U;
    g_required_ok_duration_ms = 3U;
    g_callback_count = 0U;
    g_unsupported_bus_count = 0U;
    g_diagnostics_capture_count = 0U;
    g_diagnostics_module = 0U;
    g_diagnostics_code = 0;
    g_diagnostics_detail0 = 0U;
    g_diagnostics_detail1 = 0U;
    g_print_call_count = 0U;
}

static int test_one_callback_per_poll_and_summary(void)
{
    boot_probe_summary_t summary;
    boot_probe_status_t before_print;
    boot_probe_status_t after_print;
    uint32_t callbacks_before_print;
    uint32_t completed;
    uint32_t total;
    int failures = 0;

    reset_fixture();
    boot_probe_init();
    boot_probe_start(10U);
    boot_probe_get_progress(&completed, &total);
    failures += expect_u32("initial_progress_completed", completed, 0U);
    failures += expect_u32("initial_progress_total", total, TEST_ENTRY_COUNT);

    g_now_ms = 10U;
    boot_probe_poll(10U);
    failures += expect_u32("first_poll_callback_count", g_callback_count, 1U);
    boot_probe_get_progress(&completed, &total);
    failures += expect_u32("first_progress_completed", completed, 1U);
    failures += expect_status(0U, BOOT_PROBE_OK, 1U, 1U);
    if (boot_probe_get_status(0U, &before_print) == BOOT_PROBE_OK)
    {
        failures += expect_u32("callback_elapsed", before_print.elapsed_ms, 3U);
        failures += expect_u32("callback_detail", before_print.detail0, 0x1234U);
    }
    else
    {
        failures++;
    }

    boot_probe_poll(g_now_ms);
    failures += expect_u32("second_poll_callback_count", g_callback_count, 2U);
    failures += expect_status(1U, BOOT_PROBE_ABSENT, 0U, 0U);

    boot_probe_poll(g_now_ms);
    failures += expect_u32("third_poll_callback_count", g_callback_count, 3U);
    failures += expect_u32("required_error_diagnostic_count",
                           g_diagnostics_capture_count, 1U);
    failures += expect_u32("required_error_module", g_diagnostics_module,
                           BOOT_PROBE_DIAG_MODULE);
    failures += expect_u32("required_error_code",
                           (uint32_t)g_diagnostics_code,
                           (uint32_t)BOOT_PROBE_ERROR);
    failures += expect_u32("required_error_index", g_diagnostics_detail0, 2U);
    failures += expect_u32("required_error_detail", g_diagnostics_detail1,
                           0xDEADU);

    boot_probe_poll(g_now_ms);
    failures += expect_u32("unsupported_does_not_call_callback",
                           g_callback_count, 3U);
    failures += expect_u32("unsupported_does_not_touch_bus",
                           g_unsupported_bus_count, 0U);
    failures += expect_status(3U, BOOT_PROBE_UNSUPPORTED, 0U, 0U);

    boot_probe_poll(g_now_ms);
    failures += expect_u32("fifth_poll_callback_count", g_callback_count, 4U);
    failures += expect_u32("complete", boot_probe_is_complete(), 1U);

    boot_probe_get_summary(&summary);
    failures += expect_u32("summary_total", summary.total, TEST_ENTRY_COUNT);
    failures += expect_u32("summary_ready", summary.ready, 2U);
    failures += expect_u32("summary_absent", summary.absent, 1U);
    failures += expect_u32("summary_not_ready", summary.not_ready, 0U);
    failures += expect_u32("summary_timeout", summary.timeout, 0U);
    failures += expect_u32("summary_unsupported", summary.unsupported, 1U);
    failures += expect_u32("summary_errors", summary.errors, 1U);
    failures += expect_u32("summary_elapsed", summary.elapsed_ms, 12U);
    failures += expect_u32("summary_not_truncated", summary.truncated, 0U);
    boot_probe_get_progress(&completed, &total);
    failures += expect_u32("final_progress_completed", completed,
                           TEST_ENTRY_COUNT);
    failures += expect_u32("final_progress_total", total, TEST_ENTRY_COUNT);

    (void)boot_probe_get_status(0U, &before_print);
    callbacks_before_print = g_callback_count;
    boot_probe_print_status();
    boot_probe_print_next();
    (void)boot_probe_get_status(0U, &after_print);
    failures += expect_u32("print_does_not_probe", g_callback_count,
                           callbacks_before_print);
    failures += expect_u32("print_keeps_result",
                           (uint32_t)after_print.result,
                           (uint32_t)before_print.result);
    failures += expect_u32("print_keeps_detail", after_print.detail0,
                           before_print.detail0);
    return failures;
}

static int test_external_time_does_not_consume_probe_budget(void)
{
    boot_probe_summary_t summary;
    uint32_t index;
    int failures = 0;

    reset_fixture();
    boot_probe_init();
    boot_probe_start(100U);
    g_now_ms = 100U;
    g_required_ok_duration_ms = 5U;
    boot_probe_get_summary(&summary);
    failures += expect_u32("restart_ready_reset", summary.ready, 0U);
    failures += expect_u32("restart_not_ready", summary.not_ready,
                           TEST_ENTRY_COUNT);
    failures += expect_u32("restart_not_complete", boot_probe_is_complete(),
                           0U);
    for (index = 0U; index < TEST_ENTRY_COUNT; index++)
    {
        g_now_ms += 100U;
        boot_probe_poll(g_now_ms);
    }
    failures += expect_u32("external_gap_all_callbacks", g_callback_count, 4U);
    failures += expect_u32("external_gap_complete", boot_probe_is_complete(),
                           1U);
    boot_probe_get_summary(&summary);
    failures += expect_u32("external_gap_not_truncated", summary.truncated, 0U);
    failures += expect_u32("external_gap_callback_elapsed", summary.elapsed_ms,
                           20U);
    failures += expect_u32("external_gap_no_remaining", summary.not_ready, 0U);
    return failures;
}

static int test_invalid_status_arguments(void)
{
    boot_probe_status_t status;
    int failures = 0;

    failures += expect_u32("status_null_rejected",
                           (uint32_t)boot_probe_get_status(0U, NULL),
                           (uint32_t)BOOT_PROBE_ERROR);
    failures += expect_u32("status_index_rejected",
                           (uint32_t)boot_probe_get_status(TEST_ENTRY_COUNT,
                                                          &status),
                           (uint32_t)BOOT_PROBE_ERROR);
    boot_probe_get_progress(NULL, NULL);
    return failures;
}

static int test_quiet_and_detailed_output_modes(void)
{
    uint32_t index;
    int failures = 0;

    reset_fixture();
    boot_probe_init();
    boot_probe_start_quiet(0U);
    for (index = 0U; index < TEST_ENTRY_COUNT; index++)
    {
        boot_probe_poll(g_now_ms);
    }
    boot_probe_print_next();
    boot_probe_print_next();
    failures += expect_u32("quiet_print_count", g_print_call_count, 0U);

    boot_probe_start(100U);
    g_now_ms = 100U;
    for (index = 0U; index < TEST_ENTRY_COUNT; index++)
    {
        boot_probe_poll(g_now_ms);
    }
    boot_probe_print_next();
    failures += expect_u32("detail_print_restored", g_print_call_count, 1U);
    return failures;
}

static int test_callback_crossing_total_timeout_truncates(void)
{
    boot_probe_summary_t summary;
    int failures = 0;

    reset_fixture();
    boot_probe_init();
    boot_probe_start(0U);
    g_required_ok_duration_ms = BOOT_PROBE_TOTAL_TIMEOUT_MS + 10U;
    boot_probe_poll(0U);

    failures += expect_u32("callback_timeout_one_callback", g_callback_count,
                           1U);
    failures += expect_u32("callback_timeout_complete",
                           boot_probe_is_complete(), 1U);
    boot_probe_get_summary(&summary);
    failures += expect_u32("callback_timeout_truncated", summary.truncated,
                           1U);
    failures += expect_u32("callback_timeout_elapsed", summary.elapsed_ms,
                           BOOT_PROBE_TOTAL_TIMEOUT_MS);
    failures += expect_u32("callback_timeout_remaining_not_ready",
                           summary.not_ready, TEST_ENTRY_COUNT - 1U);
    failures += expect_status(0U, BOOT_PROBE_OK, 1U, 1U);
    failures += expect_status(1U, BOOT_PROBE_NOT_READY, 0U, 0U);
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_one_callback_per_poll_and_summary();
    failures += test_external_time_does_not_consume_probe_budget();
    failures += test_invalid_status_arguments();
    failures += test_callback_crossing_total_timeout_truncates();
    failures += test_quiet_and_detailed_output_modes();

    if (failures != 0)
    {
        printf("FAIL boot_probe_service failures=%d\n", failures);
        return 1;
    }

    printf("PASS boot_probe_service\n");
    return 0;
}
