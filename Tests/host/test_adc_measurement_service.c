#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "adc_measurement_service.h"

#define TEST_HAL_OK       0
#define TEST_HAL_ERROR    1
#define TEST_HAL_BUSY     2
#define TEST_HAL_TIMEOUT  3

static uint16_t g_samples_adc1[16];
static uint16_t g_samples_adc3[16];
static uint32_t g_sample_index_adc1;
static uint32_t g_sample_index_adc3;
static int g_calibration_result_adc1;
static int g_calibration_result_adc3;
static int g_start_result;
static int g_poll_result;
static int g_stop_result;
static int g_external_resource_result;
static uint32_t g_now_ms;
static uint32_t g_poll_advance_ms;
static uint32_t g_calibrate_count_adc1;
static uint32_t g_calibrate_count_adc3;
static uint32_t g_start_count;
static uint32_t g_poll_count;
static uint32_t g_stop_count;
static uint32_t g_external_acquire_count;
static uint32_t g_external_release_count;
static uint32_t g_last_poll_timeout;
static uint16_t g_cal1;
static uint16_t g_cal2;
static int32_t g_cal2_temp;

uint32_t adc_measurement_test_now_ms(void)
{
    return g_now_ms;
}

int adc_measurement_test_calibrate(uint8_t adc_index)
{
    if (adc_index == 1U)
    {
        g_calibrate_count_adc1++;
        return g_calibration_result_adc1;
    }
    g_calibrate_count_adc3++;
    return g_calibration_result_adc3;
}

int adc_measurement_test_start(uint8_t adc_index)
{
    (void)adc_index;
    g_start_count++;
    return g_start_result;
}

int adc_measurement_test_poll(uint8_t adc_index, uint32_t timeout_ms)
{
    (void)adc_index;
    g_poll_count++;
    g_last_poll_timeout = timeout_ms;
    g_now_ms += g_poll_advance_ms;
    return g_poll_result;
}

uint32_t adc_measurement_test_get_value(uint8_t adc_index)
{
    if (adc_index == 1U)
    {
        return g_samples_adc1[g_sample_index_adc1++ % 16U];
    }
    return g_samples_adc3[g_sample_index_adc3++ % 16U];
}

int adc_measurement_test_stop(uint8_t adc_index)
{
    (void)adc_index;
    g_stop_count++;
    return g_stop_result;
}

int adc_measurement_test_acquire_external(void)
{
    g_external_acquire_count++;
    return g_external_resource_result;
}

void adc_measurement_test_release_external(void)
{
    g_external_release_count++;
}

uint16_t adc_measurement_test_cal1(void)
{
    return g_cal1;
}

uint16_t adc_measurement_test_cal2(void)
{
    return g_cal2;
}

int32_t adc_measurement_test_cal2_temp(void)
{
    return g_cal2_temp;
}

static int expect_i32(const char *name, int32_t actual, int32_t expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=%ld expected=%ld\n",
               name, (long)actual, (long)expected);
        return 1;
    }
    return 0;
}

static int expect_u32(const char *name, uint32_t actual, uint32_t expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=%lu expected=%lu\n",
               name, (unsigned long)actual, (unsigned long)expected);
        return 1;
    }
    return 0;
}

static void fill_samples(uint16_t *samples, uint16_t value)
{
    uint32_t index;

    for (index = 0U; index < 16U; index++)
    {
        samples[index] = value;
    }
}

static void reset_fixture(void)
{
    memset(g_samples_adc1, 0, sizeof(g_samples_adc1));
    memset(g_samples_adc3, 0, sizeof(g_samples_adc3));
    g_sample_index_adc1 = 0U;
    g_sample_index_adc3 = 0U;
    g_calibration_result_adc1 = TEST_HAL_OK;
    g_calibration_result_adc3 = TEST_HAL_OK;
    g_start_result = TEST_HAL_OK;
    g_poll_result = TEST_HAL_OK;
    g_stop_result = TEST_HAL_OK;
    g_external_resource_result = 0;
    g_now_ms = 100U;
    g_poll_advance_ms = 0U;
    g_calibrate_count_adc1 = 0U;
    g_calibrate_count_adc3 = 0U;
    g_start_count = 0U;
    g_poll_count = 0U;
    g_stop_count = 0U;
    g_external_acquire_count = 0U;
    g_external_release_count = 0U;
    g_last_poll_timeout = 0U;
    g_cal1 = 10000U;
    g_cal2 = 30000U;
    g_cal2_temp = 130;
    adc_measurement_service_init();
}

static int test_external_calibrates_once_and_averages(void)
{
    adc_measurement_status_t status;
    int failures = 0;

    reset_fixture();
    fill_samples(g_samples_adc1, 32768U);
    failures += expect_i32("external_result",
                           adc_measurement_read_external(50U),
                           ADC_MEASUREMENT_OK);
    adc_measurement_get_status(&status);
    failures += expect_u32("external_calibrated", status.adc1_calibrated, 1U);
    failures += expect_u32("external_valid", status.external_valid, 1U);
    failures += expect_u32("external_raw", status.external_raw, 32768U);
    failures += expect_u32("external_mv", status.external_mv, 1650U);
    failures += expect_u32("external_calibrate_count", g_calibrate_count_adc1, 1U);
    failures += expect_u32("external_start_count", g_start_count, 16U);
    failures += expect_u32("external_poll_count", g_poll_count, 16U);
    failures += expect_u32("external_stop_count", g_stop_count, 16U);
    failures += expect_u32("external_read_count", status.read_count, 1U);
    failures += expect_u32("external_acquire", g_external_acquire_count, 1U);
    failures += expect_u32("external_release", g_external_release_count, 1U);

    failures += expect_i32("external_second_result",
                           adc_measurement_read_external(50U),
                           ADC_MEASUREMENT_OK);
    failures += expect_u32("external_calibrate_once", g_calibrate_count_adc1, 1U);
    failures += expect_u32("external_second_start_count", g_start_count, 32U);
    failures += expect_u32("external_second_acquire",
                           g_external_acquire_count, 2U);
    failures += expect_u32("external_second_release",
                           g_external_release_count, 2U);
    return failures;
}

static int test_temperature_uses_factory_calibration(void)
{
    adc_measurement_status_t status;
    int failures = 0;

    reset_fixture();
    fill_samples(g_samples_adc3, 15000U);
    failures += expect_i32("temperature_result",
                           adc_measurement_read_temperature(50U),
                           ADC_MEASUREMENT_OK);
    adc_measurement_get_status(&status);
    failures += expect_u32("temperature_calibrated", status.adc3_calibrated, 1U);
    failures += expect_u32("temperature_valid", status.temperature_valid, 1U);
    failures += expect_u32("temperature_raw", status.temperature_raw, 15000U);
    failures += expect_i32("temperature_centi", status.temperature_centi_c, 5500);
    failures += expect_u32("temperature_calibrate_count", g_calibrate_count_adc3, 1U);
    failures += expect_u32("temperature_stop_count", g_stop_count, 16U);
    failures += expect_u32("temperature_no_external_resource",
                           g_external_acquire_count, 0U);
    return failures;
}

static int test_read_all_shares_one_timeout_budget(void)
{
    adc_measurement_status_t status;
    int failures = 0;

    reset_fixture();
    fill_samples(g_samples_adc1, 10000U);
    fill_samples(g_samples_adc3, 15000U);
    g_poll_advance_ms = 1U;
    failures += expect_i32("all_result", adc_measurement_read_all(50U),
                           ADC_MEASUREMENT_OK);
    adc_measurement_get_status(&status);
    failures += expect_u32("all_elapsed", status.elapsed_ms, 32U);
    failures += expect_u32("all_read_count", status.read_count, 1U);
    failures += expect_u32("all_start_count", g_start_count, 32U);
    failures += expect_u32("all_last_remaining", g_last_poll_timeout, 19U);
    failures += expect_u32("all_external_acquire",
                           g_external_acquire_count, 1U);
    failures += expect_u32("all_external_release",
                           g_external_release_count, 1U);

    reset_fixture();
    fill_samples(g_samples_adc1, 10000U);
    fill_samples(g_samples_adc3, 15000U);
    g_poll_advance_ms = 1U;
    failures += expect_i32("all_timeout", adc_measurement_read_all(20U),
                           ADC_MEASUREMENT_TIMEOUT);
    adc_measurement_get_status(&status);
    failures += expect_u32("all_timeout_elapsed", status.elapsed_ms, 20U);
    failures += expect_u32("all_timeout_stops", g_stop_count, 20U);
    failures += expect_u32("all_timeout_count", status.read_count, 1U);
    return failures;
}

static int test_errors_stop_adc_and_preserve_data(void)
{
    adc_measurement_status_t before;
    adc_measurement_status_t after;
    int failures = 0;

    reset_fixture();
    fill_samples(g_samples_adc1, 20000U);
    failures += expect_i32("seed_external", adc_measurement_read_external(50U),
                           ADC_MEASUREMENT_OK);
    adc_measurement_get_status(&before);

    g_poll_result = TEST_HAL_TIMEOUT;
    failures += expect_i32("poll_timeout", adc_measurement_read_external(50U),
                           ADC_MEASUREMENT_TIMEOUT);
    adc_measurement_get_status(&after);
    failures += expect_u32("timeout_data_preserved", after.external_raw,
                           before.external_raw);
    failures += expect_u32("timeout_valid_preserved", after.external_valid, 1U);
    failures += expect_u32("timeout_stop_called", g_stop_count, 17U);

    reset_fixture();
    g_external_resource_result = 1;
    failures += expect_i32("external_resource_busy",
                           adc_measurement_read_external(50U),
                           ADC_MEASUREMENT_NOT_READY);
    failures += expect_u32("resource_busy_no_calibrate",
                           g_calibrate_count_adc1, 0U);
    failures += expect_u32("resource_busy_no_start", g_start_count, 0U);
    failures += expect_u32("resource_busy_no_release",
                           g_external_release_count, 0U);

    reset_fixture();
    g_calibration_result_adc1 = TEST_HAL_BUSY;
    failures += expect_i32("calibration_busy",
                           adc_measurement_read_external(50U),
                           ADC_MEASUREMENT_NOT_READY);
    failures += expect_u32("busy_no_start", g_start_count, 0U);

    reset_fixture();
    g_start_result = TEST_HAL_ERROR;
    failures += expect_i32("start_error",
                           adc_measurement_read_external(50U),
                           ADC_MEASUREMENT_CONVERSION_ERROR);
    failures += expect_u32("start_error_stop", g_stop_count, 1U);
    return failures;
}

static int test_invalid_calibration_and_arguments(void)
{
    adc_measurement_status_t status;
    int failures = 0;

    reset_fixture();
    g_cal1 = 0U;
    failures += expect_i32("cal1_zero",
                           adc_measurement_read_temperature(50U),
                           ADC_MEASUREMENT_CALIBRATION_ERROR);
    failures += expect_u32("cal1_zero_no_start", g_start_count, 0U);

    reset_fixture();
    g_cal2 = 0xFFFFU;
    failures += expect_i32("cal2_erased",
                           adc_measurement_read_temperature(50U),
                           ADC_MEASUREMENT_CALIBRATION_ERROR);

    reset_fixture();
    g_cal2 = g_cal1;
    failures += expect_i32("cal_equal",
                           adc_measurement_read_temperature(50U),
                           ADC_MEASUREMENT_CALIBRATION_ERROR);

    reset_fixture();
    fill_samples(g_samples_adc3, 65535U);
    failures += expect_i32("temperature_range",
                           adc_measurement_read_temperature(50U),
                           ADC_MEASUREMENT_RANGE_ERROR);

    reset_fixture();
    failures += expect_i32("zero_timeout",
                           adc_measurement_read_external(0U),
                           ADC_MEASUREMENT_INVALID_ARG);
    adc_measurement_get_status(&status);
    failures += expect_i32("init_result", status.result,
                           ADC_MEASUREMENT_NOT_READY);
    failures += expect_u32("zero_timeout_no_calibrate", g_calibrate_count_adc1, 0U);
    adc_measurement_get_status(NULL);
    return failures;
}

static int test_temperature_failure_records_attempt_diagnostics(void)
{
    adc_measurement_status_t before;
    adc_measurement_status_t after;
    int failures = 0;

    reset_fixture();
    fill_samples(g_samples_adc3, 15000U);
    failures += expect_i32("diagnostic_seed",
                           adc_measurement_read_temperature(50U),
                           ADC_MEASUREMENT_OK);
    adc_measurement_get_status(&before);

    fill_samples(g_samples_adc3, 65535U);
    failures += expect_i32("diagnostic_range",
                           adc_measurement_read_temperature(50U),
                           ADC_MEASUREMENT_RANGE_ERROR);
    adc_measurement_get_status(&after);
    failures += expect_u32("diagnostic_attempt_valid",
                           after.temperature_attempt_valid, 1U);
    failures += expect_u32("diagnostic_attempt_raw",
                           after.temperature_attempt_raw, 65535U);
    failures += expect_u32("diagnostic_cal1", after.temperature_cal1,
                           10000U);
    failures += expect_u32("diagnostic_cal2", after.temperature_cal2,
                           30000U);
    failures += expect_i32("diagnostic_cal2_temp",
                           after.temperature_cal2_temp, 130);
    failures += expect_u32("diagnostic_valid_preserved",
                           after.temperature_valid, 1U);
    failures += expect_u32("diagnostic_raw_preserved",
                           after.temperature_raw, before.temperature_raw);
    failures += expect_i32("diagnostic_temperature_preserved",
                           after.temperature_centi_c,
                           before.temperature_centi_c);

    reset_fixture();
    g_cal1 = 0U;
    failures += expect_i32("diagnostic_invalid_calibration",
                           adc_measurement_read_temperature(50U),
                           ADC_MEASUREMENT_CALIBRATION_ERROR);
    adc_measurement_get_status(&after);
    failures += expect_u32("diagnostic_no_attempt",
                           after.temperature_attempt_valid, 0U);
    failures += expect_u32("diagnostic_invalid_cal1",
                           after.temperature_cal1, 0U);
    failures += expect_u32("diagnostic_invalid_cal2",
                           after.temperature_cal2, 30000U);
    failures += expect_i32("diagnostic_invalid_cal2_temp",
                           after.temperature_cal2_temp, 130);
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_external_calibrates_once_and_averages();
    failures += test_temperature_uses_factory_calibration();
    failures += test_read_all_shares_one_timeout_budget();
    failures += test_errors_stop_adc_and_preserve_data();
    failures += test_invalid_calibration_and_arguments();
    failures += test_temperature_failure_records_attempt_diagnostics();

    if (failures != 0)
    {
        printf("FAIL adc_measurement_service failures=%d\n", failures);
        return 1;
    }
    printf("PASS adc_measurement_service\n");
    return 0;
}
