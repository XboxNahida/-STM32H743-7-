#include "adc_measurement_service.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#define ADC_MEASUREMENT_ADC1          1U
#define ADC_MEASUREMENT_ADC3          3U
#define ADC_MEASUREMENT_SAMPLE_COUNT 16U
#define ADC_MEASUREMENT_VREF_MV    3300U

#if defined(ADC_MEASUREMENT_HOST_TEST)
extern uint32_t adc_measurement_test_now_ms(void);
extern int adc_measurement_test_calibrate(uint8_t adc_index);
extern int adc_measurement_test_acquire_external(void);
extern void adc_measurement_test_release_external(void);
extern int adc_measurement_test_start(uint8_t adc_index);
extern int adc_measurement_test_poll(uint8_t adc_index, uint32_t timeout_ms);
extern uint32_t adc_measurement_test_get_value(uint8_t adc_index);
extern int adc_measurement_test_stop(uint8_t adc_index);
extern uint16_t adc_measurement_test_cal1(void);
extern uint16_t adc_measurement_test_cal2(void);
extern int32_t adc_measurement_test_cal2_temp(void);

#define ADC_MEASUREMENT_HAL_OK       0
#define ADC_MEASUREMENT_HAL_ERROR    1
#define ADC_MEASUREMENT_HAL_BUSY     2
#define ADC_MEASUREMENT_HAL_TIMEOUT  3

static uint32_t adc_measurement_now_ms(void)
{
    return adc_measurement_test_now_ms();
}

static int adc_measurement_hal_calibrate(uint8_t adc_index)
{
    return adc_measurement_test_calibrate(adc_index);
}

static int adc_measurement_acquire_external(void)
{
    return adc_measurement_test_acquire_external();
}

static void adc_measurement_release_external(void)
{
    adc_measurement_test_release_external();
}

static int adc_measurement_hal_start(uint8_t adc_index)
{
    return adc_measurement_test_start(adc_index);
}

static int adc_measurement_hal_poll(uint8_t adc_index, uint32_t timeout_ms)
{
    return adc_measurement_test_poll(adc_index, timeout_ms);
}

static uint32_t adc_measurement_hal_get_value(uint8_t adc_index)
{
    return adc_measurement_test_get_value(adc_index);
}

static int adc_measurement_hal_stop(uint8_t adc_index)
{
    return adc_measurement_test_stop(adc_index);
}

static uint16_t adc_measurement_cal1(void)
{
    return adc_measurement_test_cal1();
}

static uint16_t adc_measurement_cal2(void)
{
    return adc_measurement_test_cal2();
}

static int32_t adc_measurement_cal2_temp(void)
{
    return adc_measurement_test_cal2_temp();
}
#else
#include "adc.h"
#include "main.h"
#include "resource_manager.h"
#include "stm32h7xx_ll_adc.h"

#define ADC_MEASUREMENT_EXTERNAL_OWNER 0x0307U

#define ADC_MEASUREMENT_HAL_OK       HAL_OK
#define ADC_MEASUREMENT_HAL_ERROR    HAL_ERROR
#define ADC_MEASUREMENT_HAL_BUSY     HAL_BUSY
#define ADC_MEASUREMENT_HAL_TIMEOUT  HAL_TIMEOUT

static ADC_HandleTypeDef *adc_measurement_handle(uint8_t adc_index)
{
    return (adc_index == ADC_MEASUREMENT_ADC1) ? &hadc1 : &hadc3;
}

static uint32_t adc_measurement_now_ms(void)
{
    return HAL_GetTick();
}

static int adc_measurement_hal_calibrate(uint8_t adc_index)
{
    return (int)HAL_ADCEx_Calibration_Start(
        adc_measurement_handle(adc_index), ADC_CALIB_OFFSET,
        ADC_SINGLE_ENDED);
}

static int adc_measurement_acquire_external(void)
{
    return resource_acquire(RESOURCE_TIM2_PA5,
                            ADC_MEASUREMENT_EXTERNAL_OWNER);
}

static void adc_measurement_release_external(void)
{
    (void)resource_release(RESOURCE_TIM2_PA5,
                           ADC_MEASUREMENT_EXTERNAL_OWNER);
}

static int adc_measurement_hal_start(uint8_t adc_index)
{
    return (int)HAL_ADC_Start(adc_measurement_handle(adc_index));
}

static int adc_measurement_hal_poll(uint8_t adc_index, uint32_t timeout_ms)
{
    return (int)HAL_ADC_PollForConversion(
        adc_measurement_handle(adc_index), timeout_ms);
}

static uint32_t adc_measurement_hal_get_value(uint8_t adc_index)
{
    return HAL_ADC_GetValue(adc_measurement_handle(adc_index));
}

static int adc_measurement_hal_stop(uint8_t adc_index)
{
    return (int)HAL_ADC_Stop(adc_measurement_handle(adc_index));
}

static uint16_t adc_measurement_cal1(void)
{
    return *TEMPSENSOR_CAL1_ADDR;
}

static uint16_t adc_measurement_cal2(void)
{
    return *TEMPSENSOR_CAL2_ADDR;
}

static int32_t adc_measurement_cal2_temp(void)
{
    return (int32_t)TEMPSENSOR_CAL2_TEMP;
}
#endif

static adc_measurement_status_t g_adc_measurement_status;

static void adc_measurement_increment_saturated(uint32_t *value)
{
    if (*value != UINT32_MAX)
    {
        (*value)++;
    }
}

static int adc_measurement_map_conversion_result(int hal_result)
{
    if (hal_result == ADC_MEASUREMENT_HAL_OK)
    {
        return ADC_MEASUREMENT_OK;
    }
    if (hal_result == ADC_MEASUREMENT_HAL_BUSY)
    {
        return ADC_MEASUREMENT_NOT_READY;
    }
    if (hal_result == ADC_MEASUREMENT_HAL_TIMEOUT)
    {
        return ADC_MEASUREMENT_TIMEOUT;
    }
    return ADC_MEASUREMENT_CONVERSION_ERROR;
}

static int adc_measurement_map_calibration_result(int hal_result)
{
    if (hal_result == ADC_MEASUREMENT_HAL_OK)
    {
        return ADC_MEASUREMENT_OK;
    }
    if (hal_result == ADC_MEASUREMENT_HAL_BUSY)
    {
        return ADC_MEASUREMENT_NOT_READY;
    }
    if (hal_result == ADC_MEASUREMENT_HAL_TIMEOUT)
    {
        return ADC_MEASUREMENT_TIMEOUT;
    }
    return ADC_MEASUREMENT_CALIBRATION_ERROR;
}

static uint32_t adc_measurement_remaining(uint32_t start_ms,
                                          uint32_t timeout_ms)
{
    uint32_t elapsed = adc_measurement_now_ms() - start_ms;

    return (elapsed < timeout_ms) ? (timeout_ms - elapsed) : 0U;
}

static int adc_measurement_calibrate(uint8_t adc_index,
                                     uint8_t *calibrated,
                                     uint32_t start_ms,
                                     uint32_t timeout_ms)
{
    int result;

    if (*calibrated != 0U)
    {
        return ADC_MEASUREMENT_OK;
    }
    if (adc_measurement_remaining(start_ms, timeout_ms) == 0U)
    {
        return ADC_MEASUREMENT_TIMEOUT;
    }
    result = adc_measurement_map_calibration_result(
        adc_measurement_hal_calibrate(adc_index));
    if (result != ADC_MEASUREMENT_OK)
    {
        return result;
    }
    if (adc_measurement_remaining(start_ms, timeout_ms) == 0U)
    {
        return ADC_MEASUREMENT_TIMEOUT;
    }
    *calibrated = 1U;
    return ADC_MEASUREMENT_OK;
}

static int adc_measurement_average(uint8_t adc_index,
                                   uint32_t start_ms,
                                   uint32_t timeout_ms,
                                   uint16_t *average)
{
    uint32_t sum = 0U;
    uint32_t remaining;
    uint32_t index;
    int result;
    int stop_result;

    for (index = 0U; index < ADC_MEASUREMENT_SAMPLE_COUNT; index++)
    {
        if (adc_measurement_remaining(start_ms, timeout_ms) == 0U)
        {
            return ADC_MEASUREMENT_TIMEOUT;
        }
        result = adc_measurement_map_conversion_result(
            adc_measurement_hal_start(adc_index));
        if (result != ADC_MEASUREMENT_OK)
        {
            (void)adc_measurement_hal_stop(adc_index);
            return result;
        }

        remaining = adc_measurement_remaining(start_ms, timeout_ms);
        if (remaining == 0U)
        {
            (void)adc_measurement_hal_stop(adc_index);
            return ADC_MEASUREMENT_TIMEOUT;
        }
        result = adc_measurement_map_conversion_result(
            adc_measurement_hal_poll(adc_index, remaining));
        if (result == ADC_MEASUREMENT_OK)
        {
            sum += adc_measurement_hal_get_value(adc_index);
        }
        stop_result = adc_measurement_map_conversion_result(
            adc_measurement_hal_stop(adc_index));
        if (result != ADC_MEASUREMENT_OK)
        {
            return result;
        }
        if (stop_result != ADC_MEASUREMENT_OK)
        {
            return stop_result;
        }
    }

    *average = (uint16_t)(sum / ADC_MEASUREMENT_SAMPLE_COUNT);
    return ADC_MEASUREMENT_OK;
}

static void adc_measurement_finish_failure(
    const adc_measurement_status_t *original,
    const adc_measurement_status_t *working,
    int result, uint32_t start_ms)
{
    adc_measurement_status_t failed = *original;

    failed.adc1_calibrated = working->adc1_calibrated;
    failed.adc3_calibrated = working->adc3_calibrated;
    failed.temperature_attempt_valid = working->temperature_attempt_valid;
    failed.temperature_attempt_raw = working->temperature_attempt_raw;
    failed.temperature_cal1 = working->temperature_cal1;
    failed.temperature_cal2 = working->temperature_cal2;
    failed.temperature_cal2_temp = working->temperature_cal2_temp;
    failed.result = result;
    failed.elapsed_ms = adc_measurement_now_ms() - start_ms;
    adc_measurement_increment_saturated(&failed.read_count);
    g_adc_measurement_status = failed;
}

static int adc_measurement_read_external_into(
    adc_measurement_status_t *working, uint32_t start_ms,
    uint32_t timeout_ms)
{
    uint16_t average;
    int result;

    result = adc_measurement_calibrate(ADC_MEASUREMENT_ADC1,
                                       &working->adc1_calibrated,
                                       start_ms, timeout_ms);
    if (result != ADC_MEASUREMENT_OK)
    {
        return result;
    }
    result = adc_measurement_average(ADC_MEASUREMENT_ADC1, start_ms,
                                     timeout_ms, &average);
    if (result != ADC_MEASUREMENT_OK)
    {
        return result;
    }
    working->external_raw = average;
    working->external_mv =
        ((uint32_t)average * ADC_MEASUREMENT_VREF_MV + 32767U) / 65535U;
    working->external_valid = 1U;
    return ADC_MEASUREMENT_OK;
}

static int adc_measurement_read_temperature_into(
    adc_measurement_status_t *working, uint32_t start_ms,
    uint32_t timeout_ms)
{
    uint16_t cal1 = adc_measurement_cal1();
    uint16_t cal2 = adc_measurement_cal2();
    int32_t cal2_temp = adc_measurement_cal2_temp();
    int64_t numerator;
    int32_t temperature;
    uint16_t average;
    int result;

    working->temperature_attempt_valid = 0U;
    working->temperature_attempt_raw = 0U;
    working->temperature_cal1 = cal1;
    working->temperature_cal2 = cal2;
    working->temperature_cal2_temp = cal2_temp;
    if ((cal1 == 0U) || (cal1 == 0xFFFFU) ||
        (cal2 == 0U) || (cal2 == 0xFFFFU) ||
        (cal1 == cal2) || (cal2_temp <= 30))
    {
        return ADC_MEASUREMENT_CALIBRATION_ERROR;
    }
    result = adc_measurement_calibrate(ADC_MEASUREMENT_ADC3,
                                       &working->adc3_calibrated,
                                       start_ms, timeout_ms);
    if (result != ADC_MEASUREMENT_OK)
    {
        return result;
    }
    result = adc_measurement_average(ADC_MEASUREMENT_ADC3, start_ms,
                                     timeout_ms, &average);
    if (result != ADC_MEASUREMENT_OK)
    {
        return result;
    }
    working->temperature_attempt_raw = average;
    working->temperature_attempt_valid = 1U;

    numerator = (int64_t)((int32_t)average - (int32_t)cal1) *
                (int64_t)(cal2_temp - 30) * 100;
    temperature = 3000 +
                  (int32_t)(numerator /
                            ((int32_t)cal2 - (int32_t)cal1));
    if ((temperature < -4000) || (temperature > 15000))
    {
        return ADC_MEASUREMENT_RANGE_ERROR;
    }
    working->temperature_raw = average;
    working->temperature_centi_c = temperature;
    working->temperature_valid = 1U;
    return ADC_MEASUREMENT_OK;
}

static void adc_measurement_finish_success(adc_measurement_status_t *working,
                                           uint32_t start_ms)
{
    working->result = ADC_MEASUREMENT_OK;
    working->elapsed_ms = adc_measurement_now_ms() - start_ms;
    adc_measurement_increment_saturated(&working->read_count);
    g_adc_measurement_status = *working;
}

void adc_measurement_service_init(void)
{
    memset(&g_adc_measurement_status, 0, sizeof(g_adc_measurement_status));
    g_adc_measurement_status.result = ADC_MEASUREMENT_NOT_READY;
}

int adc_measurement_read_external(uint32_t timeout_ms)
{
    adc_measurement_status_t original;
    adc_measurement_status_t working;
    uint32_t start_ms;
    int result;

    if (timeout_ms == 0U)
    {
        return ADC_MEASUREMENT_INVALID_ARG;
    }
    original = g_adc_measurement_status;
    working = original;
    start_ms = adc_measurement_now_ms();
    if (adc_measurement_acquire_external() != 0)
    {
        adc_measurement_finish_failure(&original, &working,
                                       ADC_MEASUREMENT_NOT_READY, start_ms);
        return ADC_MEASUREMENT_NOT_READY;
    }
    result = adc_measurement_read_external_into(&working, start_ms,
                                                timeout_ms);
    adc_measurement_release_external();
    if (result != ADC_MEASUREMENT_OK)
    {
        adc_measurement_finish_failure(&original, &working, result, start_ms);
        return result;
    }
    adc_measurement_finish_success(&working, start_ms);
    return ADC_MEASUREMENT_OK;
}

int adc_measurement_read_temperature(uint32_t timeout_ms)
{
    adc_measurement_status_t original;
    adc_measurement_status_t working;
    uint32_t start_ms;
    int result;

    if (timeout_ms == 0U)
    {
        return ADC_MEASUREMENT_INVALID_ARG;
    }
    original = g_adc_measurement_status;
    working = original;
    start_ms = adc_measurement_now_ms();
    result = adc_measurement_read_temperature_into(&working, start_ms,
                                                   timeout_ms);
    if (result != ADC_MEASUREMENT_OK)
    {
        adc_measurement_finish_failure(&original, &working, result, start_ms);
        return result;
    }
    adc_measurement_finish_success(&working, start_ms);
    return ADC_MEASUREMENT_OK;
}

int adc_measurement_read_all(uint32_t timeout_ms)
{
    adc_measurement_status_t original;
    adc_measurement_status_t working;
    uint32_t start_ms;
    int result;

    if (timeout_ms == 0U)
    {
        return ADC_MEASUREMENT_INVALID_ARG;
    }
    original = g_adc_measurement_status;
    working = original;
    start_ms = adc_measurement_now_ms();
    if (adc_measurement_acquire_external() != 0)
    {
        adc_measurement_finish_failure(&original, &working,
                                       ADC_MEASUREMENT_NOT_READY, start_ms);
        return ADC_MEASUREMENT_NOT_READY;
    }
    result = adc_measurement_read_external_into(&working, start_ms,
                                                timeout_ms);
    adc_measurement_release_external();
    if (result == ADC_MEASUREMENT_OK)
    {
        result = adc_measurement_read_temperature_into(&working, start_ms,
                                                       timeout_ms);
    }
    if (result != ADC_MEASUREMENT_OK)
    {
        adc_measurement_finish_failure(&original, &working, result, start_ms);
        return result;
    }
    adc_measurement_finish_success(&working, start_ms);
    return ADC_MEASUREMENT_OK;
}

void adc_measurement_get_status(adc_measurement_status_t *status)
{
    if (status != NULL)
    {
        *status = g_adc_measurement_status;
    }
}

void adc_measurement_print_status(void)
{
    printf("adc status adc1_calibrated=%u adc3_calibrated=%u external_valid=%u external_raw=%u external_mv=%lu temperature_valid=%u temperature_raw=%u temperature_centi_c=%ld temperature_attempt_valid=%u temperature_attempt_raw=%u temperature_cal1=%u temperature_cal2=%u temperature_cal2_temp=%ld result=%ld elapsed_ms=%lu read_count=%lu\r\n",
           (unsigned int)g_adc_measurement_status.adc1_calibrated,
           (unsigned int)g_adc_measurement_status.adc3_calibrated,
           (unsigned int)g_adc_measurement_status.external_valid,
           (unsigned int)g_adc_measurement_status.external_raw,
           (unsigned long)g_adc_measurement_status.external_mv,
           (unsigned int)g_adc_measurement_status.temperature_valid,
           (unsigned int)g_adc_measurement_status.temperature_raw,
           (long)g_adc_measurement_status.temperature_centi_c,
           (unsigned int)g_adc_measurement_status.temperature_attempt_valid,
           (unsigned int)g_adc_measurement_status.temperature_attempt_raw,
           (unsigned int)g_adc_measurement_status.temperature_cal1,
           (unsigned int)g_adc_measurement_status.temperature_cal2,
           (long)g_adc_measurement_status.temperature_cal2_temp,
           (long)g_adc_measurement_status.result,
           (unsigned long)g_adc_measurement_status.elapsed_ms,
           (unsigned long)g_adc_measurement_status.read_count);
}
