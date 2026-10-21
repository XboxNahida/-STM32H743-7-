#include "tpad_service.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#define TPAD_CALIBRATION_SAMPLES 10U
#define TPAD_READ_SAMPLES         3U
#define TPAD_GATE_VALUE         100U

#if defined(TPAD_SERVICE_HOST_TEST)
extern uint32_t tpad_test_now_ms(void);
extern int tpad_test_acquire(void);
extern void tpad_test_release(void);
extern int tpad_test_capture(uint32_t timeout_ms, uint32_t *raw);
extern int tpad_test_restore_analog(void);

static uint32_t tpad_now_ms(void)
{
    return tpad_test_now_ms();
}

static int tpad_platform_acquire(void)
{
    return tpad_test_acquire();
}

static void tpad_platform_release(void)
{
    tpad_test_release();
}

static int tpad_platform_capture(uint32_t timeout_ms, uint32_t *raw)
{
    return tpad_test_capture(timeout_ms, raw);
}

static int tpad_platform_restore(void)
{
    return tpad_test_restore_analog();
}
#else
#include "main.h"
#include "resource_manager.h"
#include "tim.h"

#define TPAD_RESOURCE_OWNER 0x0308U

static uint32_t tpad_now_ms(void)
{
    return HAL_GetTick();
}

static int tpad_platform_acquire(void)
{
    return resource_acquire(RESOURCE_TIM2_PA5, TPAD_RESOURCE_OWNER);
}

static void tpad_platform_release(void)
{
    (void)resource_release(RESOURCE_TIM2_PA5, TPAD_RESOURCE_OWNER);
}

static int tpad_platform_capture(uint32_t timeout_ms, uint32_t *raw)
{
    GPIO_InitTypeDef gpio = {0};
    TIM_IC_InitTypeDef input = {0};
    uint32_t start_ms;

    if ((raw == NULL) || (timeout_ms == 0U))
    {
        return 1;
    }

    __HAL_RCC_GPIOA_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_5;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_PULLDOWN;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_Delay(5U);

    input.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
    input.ICSelection = TIM_ICSELECTION_DIRECTTI;
    input.ICPrescaler = TIM_ICPSC_DIV1;
    input.ICFilter = 0U;
    if (HAL_TIM_IC_ConfigChannel(&htim2, &input, TIM_CHANNEL_1) != HAL_OK)
    {
        return 1;
    }

    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_CC1);
    __HAL_TIM_SET_COUNTER(&htim2, 0U);
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &gpio);
    if (HAL_TIM_IC_Start(&htim2, TIM_CHANNEL_1) != HAL_OK)
    {
        return 1;
    }

    start_ms = HAL_GetTick();
    while (__HAL_TIM_GET_FLAG(&htim2, TIM_FLAG_CC1) == RESET)
    {
        if ((HAL_GetTick() - start_ms) >= timeout_ms)
        {
            (void)HAL_TIM_IC_Stop(&htim2, TIM_CHANNEL_1);
            return 2;
        }
    }
    *raw = HAL_TIM_ReadCapturedValue(&htim2, TIM_CHANNEL_1);
    (void)HAL_TIM_IC_Stop(&htim2, TIM_CHANNEL_1);
    return 0;
}

static int tpad_platform_restore(void)
{
    GPIO_InitTypeDef gpio = {0};

    (void)HAL_TIM_IC_Stop(&htim2, TIM_CHANNEL_1);
    gpio.Pin = GPIO_PIN_5;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);
    return 0;
}
#endif

static tpad_status_t g_tpad_status;

static void tpad_increment_saturated(uint32_t *value)
{
    if (*value != UINT32_MAX)
    {
        (*value)++;
    }
}

static uint32_t tpad_remaining(uint32_t start_ms, uint32_t timeout_ms)
{
    uint32_t elapsed = tpad_now_ms() - start_ms;
    return (elapsed < timeout_ms) ? (timeout_ms - elapsed) : 0U;
}

static int tpad_capture_samples(uint32_t *values, uint32_t count,
                                uint32_t start_ms, uint32_t timeout_ms)
{
    uint32_t index;
    uint32_t remaining;
    int platform_result;

    for (index = 0U; index < count; index++)
    {
        remaining = tpad_remaining(start_ms, timeout_ms);
        if (remaining == 0U)
        {
            return TPAD_TIMEOUT;
        }
        platform_result = tpad_platform_capture(remaining, &values[index]);
        if (platform_result == 2)
        {
            return TPAD_TIMEOUT;
        }
        if (platform_result != 0)
        {
            return TPAD_CAPTURE_ERROR;
        }
    }
    return TPAD_OK;
}

static int tpad_finish(int result, uint32_t start_ms)
{
    if (tpad_platform_restore() != 0)
    {
        result = TPAD_RESTORE_ERROR;
    }
    tpad_platform_release();
    g_tpad_status.result = result;
    g_tpad_status.elapsed_ms = tpad_now_ms() - start_ms;
    return result;
}

void tpad_service_init(void)
{
    memset(&g_tpad_status, 0, sizeof(g_tpad_status));
    g_tpad_status.initialized = 1U;
    g_tpad_status.result = TPAD_NOT_READY;
}

int tpad_calibrate(uint32_t timeout_ms)
{
    uint32_t values[TPAD_CALIBRATION_SAMPLES];
    uint32_t start_ms;
    uint32_t sum = 0U;
    uint32_t index;
    uint32_t swap_index;
    uint32_t temp;
    int result;

    if (timeout_ms == 0U)
    {
        return TPAD_INVALID_ARG;
    }
    start_ms = tpad_now_ms();
    if (tpad_platform_acquire() != 0)
    {
        g_tpad_status.result = TPAD_NOT_READY;
        return TPAD_NOT_READY;
    }
    result = tpad_capture_samples(values, TPAD_CALIBRATION_SAMPLES,
                                  start_ms, timeout_ms);
    if (result == TPAD_OK)
    {
        for (index = 0U; index < TPAD_CALIBRATION_SAMPLES - 1U; index++)
        {
            for (swap_index = index + 1U;
                 swap_index < TPAD_CALIBRATION_SAMPLES; swap_index++)
            {
                if (values[index] > values[swap_index])
                {
                    temp = values[index];
                    values[index] = values[swap_index];
                    values[swap_index] = temp;
                }
            }
        }
        for (index = 2U; index < 8U; index++)
        {
            sum += values[index];
        }
        g_tpad_status.baseline = sum / 6U;
        g_tpad_status.calibrated = 1U;
        tpad_increment_saturated(&g_tpad_status.calibrate_count);
    }
    return tpad_finish(result, start_ms);
}

int tpad_read(uint32_t timeout_ms)
{
    uint32_t values[TPAD_READ_SAMPLES];
    uint32_t maximum = 0U;
    uint32_t start_ms;
    uint32_t index;
    int result;

    if (timeout_ms == 0U)
    {
        return TPAD_INVALID_ARG;
    }
    if (g_tpad_status.calibrated == 0U)
    {
        return TPAD_NOT_CALIBRATED;
    }
    start_ms = tpad_now_ms();
    if (tpad_platform_acquire() != 0)
    {
        g_tpad_status.result = TPAD_NOT_READY;
        return TPAD_NOT_READY;
    }
    result = tpad_capture_samples(values, TPAD_READ_SAMPLES,
                                  start_ms, timeout_ms);
    if (result == TPAD_OK)
    {
        for (index = 0U; index < TPAD_READ_SAMPLES; index++)
        {
            if (values[index] > maximum)
            {
                maximum = values[index];
            }
        }
        g_tpad_status.last_raw = maximum;
        g_tpad_status.touched =
            (maximum > (g_tpad_status.baseline + TPAD_GATE_VALUE)) ? 1U : 0U;
        tpad_increment_saturated(&g_tpad_status.read_count);
    }
    return tpad_finish(result, start_ms);
}

void tpad_get_status(tpad_status_t *status)
{
    if (status != NULL)
    {
        *status = g_tpad_status;
    }
}

void tpad_print_status(void)
{
    printf("tpad status initialized=%u calibrated=%u baseline=%lu last_raw=%lu touched=%u result=%ld elapsed_ms=%lu calibrate_count=%lu read_count=%lu\r\n",
           (unsigned int)g_tpad_status.initialized,
           (unsigned int)g_tpad_status.calibrated,
           (unsigned long)g_tpad_status.baseline,
           (unsigned long)g_tpad_status.last_raw,
           (unsigned int)g_tpad_status.touched,
           (long)g_tpad_status.result,
           (unsigned long)g_tpad_status.elapsed_ms,
           (unsigned long)g_tpad_status.calibrate_count,
           (unsigned long)g_tpad_status.read_count);
}
