#include "ir_remote_service.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#define IR_REMOTE_LEADER_MIN_US 4200U
#define IR_REMOTE_LEADER_MAX_US 4700U
#define IR_REMOTE_ZERO_MIN_US    300U
#define IR_REMOTE_ZERO_MAX_US    800U
#define IR_REMOTE_ONE_MIN_US    1400U
#define IR_REMOTE_ONE_MAX_US    1800U
#define IR_REMOTE_REPEAT_MIN_US 2200U
#define IR_REMOTE_REPEAT_MAX_US 2600U
#define IR_REMOTE_DATA_BITS       32U
#define IR_REMOTE_OWNER         0x0309U

#if defined(IR_REMOTE_SERVICE_HOST_TEST)
static uint32_t ir_remote_enter_critical(void)
{
    return 0U;
}

static void ir_remote_exit_critical(uint32_t state)
{
    (void)state;
}
#else
#include "main.h"
#include "resource_manager.h"
#include "tim.h"

static uint32_t ir_remote_enter_critical(void)
{
    uint32_t state = __get_PRIMASK();
    __disable_irq();
    return state;
}

static void ir_remote_exit_critical(uint32_t state)
{
    if (state == 0U)
    {
        __enable_irq();
    }
}
#endif

typedef enum
{
    IR_REMOTE_WAIT_LEADER = 0,
    IR_REMOTE_RECEIVING
} ir_remote_decode_state_t;

static volatile ir_remote_status_t g_ir_remote_status;
static volatile uint32_t g_ir_remote_data;
static volatile uint8_t g_ir_remote_bit_count;
static volatile uint8_t g_ir_remote_capture_seen;
static volatile uint8_t g_ir_remote_waiting_falling;
static volatile ir_remote_decode_state_t g_ir_remote_decode_state;

#if !defined(IR_REMOTE_SERVICE_HOST_TEST)
static int ir_remote_start_hardware(void)
{
    GPIO_InitTypeDef gpio = {0};
    TIM_IC_InitTypeDef input = {0};

    gpio.Pin = GPIO_PIN_8;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = GPIO_AF1_TIM1;
    HAL_GPIO_Init(GPIOA, &gpio);

    input.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
    input.ICSelection = TIM_ICSELECTION_DIRECTTI;
    input.ICPrescaler = TIM_ICPSC_DIV1;
    input.ICFilter = 3U;
    if ((HAL_TIM_IC_ConfigChannel(&htim1, &input, TIM_CHANNEL_1) != HAL_OK) ||
        (HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_1) != HAL_OK))
    {
        return 1;
    }
    __HAL_TIM_ENABLE_IT(&htim1, TIM_IT_UPDATE);
    return 0;
}

static void ir_remote_stop_hardware(void)
{
    __HAL_TIM_DISABLE_IT(&htim1, TIM_IT_UPDATE);
    (void)HAL_TIM_IC_Stop_IT(&htim1, TIM_CHANNEL_1);
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_8);
}
#endif

static void ir_remote_increment_saturated(volatile uint32_t *value)
{
    if (*value != UINT32_MAX)
    {
        (*value)++;
    }
}

static void ir_remote_reset_decoder(void)
{
    g_ir_remote_data = 0U;
    g_ir_remote_bit_count = 0U;
    g_ir_remote_decode_state = IR_REMOTE_WAIT_LEADER;
}

void ir_remote_service_init(void)
{
    uint32_t critical_state = ir_remote_enter_critical();

    memset((void *)&g_ir_remote_status, 0, sizeof(g_ir_remote_status));
    g_ir_remote_status.initialized = 1U;
    g_ir_remote_capture_seen = 0U;
    g_ir_remote_waiting_falling = 0U;
    ir_remote_reset_decoder();
    ir_remote_exit_critical(critical_state);

#if !defined(IR_REMOTE_SERVICE_HOST_TEST)
    (void)resource_configure(RESOURCE_TIM1_PA8, IR_REMOTE_OWNER,
                             NULL, NULL);
    (void)ir_remote_start_hardware();
#endif
}

void ir_remote_capture_pulse(uint32_t width_us)
{
    uint32_t data;
    uint8_t address;
    uint8_t address_inverse;
    uint8_t command;
    uint8_t command_inverse;

    if (g_ir_remote_status.suspended != 0U)
    {
        return;
    }

    g_ir_remote_capture_seen = 1U;
    if ((width_us >= IR_REMOTE_LEADER_MIN_US) &&
        (width_us <= IR_REMOTE_LEADER_MAX_US))
    {
        g_ir_remote_data = 0U;
        g_ir_remote_bit_count = 0U;
        g_ir_remote_decode_state = IR_REMOTE_RECEIVING;
        return;
    }

    if ((width_us >= IR_REMOTE_REPEAT_MIN_US) &&
        (width_us <= IR_REMOTE_REPEAT_MAX_US) &&
        ((g_ir_remote_decode_state == IR_REMOTE_WAIT_LEADER) ||
         (g_ir_remote_bit_count == 0U)))
    {
        if (g_ir_remote_status.frame_valid != 0U)
        {
            ir_remote_increment_saturated(&g_ir_remote_status.repeat_count);
        }
        else
        {
            ir_remote_increment_saturated(&g_ir_remote_status.invalid_count);
        }
        ir_remote_reset_decoder();
        return;
    }

    if (g_ir_remote_decode_state != IR_REMOTE_RECEIVING)
    {
        return;
    }

    if ((width_us >= IR_REMOTE_ZERO_MIN_US) &&
        (width_us <= IR_REMOTE_ZERO_MAX_US))
    {
        data = 0U;
    }
    else if ((width_us >= IR_REMOTE_ONE_MIN_US) &&
             (width_us <= IR_REMOTE_ONE_MAX_US))
    {
        data = 1U;
    }
    else
    {
        ir_remote_increment_saturated(&g_ir_remote_status.invalid_count);
        ir_remote_reset_decoder();
        return;
    }

    if (g_ir_remote_bit_count >= IR_REMOTE_DATA_BITS)
    {
        ir_remote_increment_saturated(&g_ir_remote_status.overflow_count);
        ir_remote_reset_decoder();
        return;
    }
    g_ir_remote_data |= data << g_ir_remote_bit_count;
    g_ir_remote_bit_count++;

    if (g_ir_remote_bit_count == IR_REMOTE_DATA_BITS)
    {
        address = (uint8_t)(g_ir_remote_data & 0xFFU);
        address_inverse = (uint8_t)((g_ir_remote_data >> 8) & 0xFFU);
        command = (uint8_t)((g_ir_remote_data >> 16) & 0xFFU);
        command_inverse = (uint8_t)((g_ir_remote_data >> 24) & 0xFFU);
        if (((uint8_t)~address == address_inverse) &&
            ((uint8_t)~command == command_inverse))
        {
            g_ir_remote_status.address = address;
            g_ir_remote_status.command = command;
            g_ir_remote_status.frame_valid = 1U;
            ir_remote_increment_saturated(&g_ir_remote_status.frame_count);
        }
        else
        {
            ir_remote_increment_saturated(&g_ir_remote_status.invalid_count);
        }
        ir_remote_reset_decoder();
    }
}

void ir_remote_timeout(void)
{
    if (g_ir_remote_status.suspended != 0U)
    {
        return;
    }
    if (g_ir_remote_decode_state == IR_REMOTE_RECEIVING)
    {
        if (g_ir_remote_capture_seen != 0U)
        {
            g_ir_remote_capture_seen = 0U;
            return;
        }
        ir_remote_increment_saturated(&g_ir_remote_status.invalid_count);
        ir_remote_reset_decoder();
    }
}

void ir_remote_get_status(ir_remote_status_t *status)
{
    uint32_t critical_state;

    if (status == NULL)
    {
        return;
    }
    critical_state = ir_remote_enter_critical();
    memcpy(status, (const void *)&g_ir_remote_status, sizeof(*status));
    ir_remote_exit_critical(critical_state);
}

void ir_remote_print_status(void)
{
    ir_remote_status_t status;

    ir_remote_get_status(&status);
    printf("ir status initialized=%u suspended=%u frame_valid=%u address=0x%02X command=0x%02X frame_count=%lu repeat_count=%lu invalid_count=%lu overflow_count=%lu\r\n",
           (unsigned int)status.initialized,
           (unsigned int)status.suspended,
           (unsigned int)status.frame_valid,
           (unsigned int)status.address,
           (unsigned int)status.command,
           (unsigned long)status.frame_count,
           (unsigned long)status.repeat_count,
           (unsigned long)status.invalid_count,
           (unsigned long)status.overflow_count);
}

void ir_remote_clear(void)
{
    uint32_t critical_state = ir_remote_enter_critical();

    g_ir_remote_status.frame_valid = 0U;
    g_ir_remote_status.address = 0U;
    g_ir_remote_status.command = 0U;
    g_ir_remote_status.frame_count = 0U;
    g_ir_remote_status.repeat_count = 0U;
    g_ir_remote_status.invalid_count = 0U;
    g_ir_remote_status.overflow_count = 0U;
    g_ir_remote_capture_seen = 0U;
    g_ir_remote_waiting_falling = 0U;
    ir_remote_reset_decoder();
#if !defined(IR_REMOTE_SERVICE_HOST_TEST)
    __HAL_TIM_SET_CAPTUREPOLARITY(&htim1, TIM_CHANNEL_1,
                                  TIM_INPUTCHANNELPOLARITY_RISING);
    __HAL_TIM_SET_COUNTER(&htim1, 0U);
#endif
    ir_remote_exit_critical(critical_state);
}

int ir_remote_suspend(void)
{
    uint32_t critical_state;

    if (g_ir_remote_status.suspended != 0U)
    {
        return IR_REMOTE_OK;
    }
#if !defined(IR_REMOTE_SERVICE_HOST_TEST)
    ir_remote_stop_hardware();
    if (resource_release(RESOURCE_TIM1_PA8, IR_REMOTE_OWNER) != RESOURCE_OK)
    {
        (void)ir_remote_start_hardware();
        return IR_REMOTE_NOT_READY;
    }
#endif
    critical_state = ir_remote_enter_critical();
    g_ir_remote_status.suspended = 1U;
    g_ir_remote_capture_seen = 0U;
    g_ir_remote_waiting_falling = 0U;
    ir_remote_reset_decoder();
    ir_remote_exit_critical(critical_state);
    return IR_REMOTE_OK;
}

int ir_remote_resume(void)
{
    uint32_t critical_state;

    if (g_ir_remote_status.suspended == 0U)
    {
        return IR_REMOTE_OK;
    }
#if !defined(IR_REMOTE_SERVICE_HOST_TEST)
    if (resource_acquire(RESOURCE_TIM1_PA8, IR_REMOTE_OWNER) != RESOURCE_OK)
    {
        return IR_REMOTE_NOT_READY;
    }
    if (ir_remote_start_hardware() != 0)
    {
        (void)resource_release(RESOURCE_TIM1_PA8, IR_REMOTE_OWNER);
        return IR_REMOTE_NOT_READY;
    }
#endif
    critical_state = ir_remote_enter_critical();
    g_ir_remote_status.suspended = 0U;
    g_ir_remote_capture_seen = 0U;
    g_ir_remote_waiting_falling = 0U;
    ir_remote_reset_decoder();
    ir_remote_exit_critical(critical_state);
    return IR_REMOTE_OK;
}

#if !defined(IR_REMOTE_SERVICE_HOST_TEST)
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if ((htim != NULL) && (htim->Instance == TIM1))
    {
        if (g_ir_remote_waiting_falling == 0U)
        {
            __HAL_TIM_SET_CAPTUREPOLARITY(
                htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_FALLING);
            __HAL_TIM_SET_COUNTER(htim, 0U);
            g_ir_remote_waiting_falling = 1U;
        }
        else
        {
            uint32_t width_us = HAL_TIM_ReadCapturedValue(htim,
                                                          TIM_CHANNEL_1);
            __HAL_TIM_SET_CAPTUREPOLARITY(
                htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
            g_ir_remote_waiting_falling = 0U;
            ir_remote_capture_pulse(width_us);
        }
    }
}
#endif
