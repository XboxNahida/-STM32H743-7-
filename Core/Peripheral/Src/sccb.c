#include "sccb.h"

#include <stddef.h>

#if defined(SCCB_HOST_TEST)
extern int sccb_test_init_bus(void);
extern void sccb_test_start(void);
extern void sccb_test_stop(void);
extern int sccb_test_write_byte(uint8_t value);
extern int sccb_test_read_byte(uint8_t nack, uint8_t *value);

static void sccb_bus_init(void)
{
    (void)sccb_test_init_bus();
}

#define sccb_bus_start      sccb_test_start
#define sccb_bus_stop       sccb_test_stop
#define sccb_bus_write_byte sccb_test_write_byte
#define sccb_bus_read_byte  sccb_test_read_byte
#else
#include "main.h"

static void sccb_delay(void)
{
    volatile uint32_t cycles = 400U;

    while (cycles > 0U)
    {
        cycles--;
        __NOP();
    }
}

static void sccb_scl(uint8_t high)
{
    HAL_GPIO_WritePin(SCCB_SCL_GPIO_Port, SCCB_SCL_Pin,
                      (high != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void sccb_sda(uint8_t high)
{
    HAL_GPIO_WritePin(SCCB_SDA_GPIO_Port, SCCB_SDA_Pin,
                      (high != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void sccb_bus_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    gpio.Pin = SCCB_SCL_Pin | SCCB_SDA_Pin;
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);
    sccb_scl(1U);
    sccb_sda(1U);
}

static void sccb_bus_start(void)
{
    sccb_sda(1U);
    sccb_scl(1U);
    sccb_delay();
    sccb_sda(0U);
    sccb_delay();
    sccb_scl(0U);
    sccb_delay();
}

static void sccb_bus_stop(void)
{
    sccb_sda(0U);
    sccb_delay();
    sccb_scl(1U);
    sccb_delay();
    sccb_sda(1U);
    sccb_delay();
}

static int sccb_bus_write_byte(uint8_t value)
{
    uint8_t bit;

    for (bit = 0U; bit < 8U; bit++)
    {
        sccb_sda((uint8_t)((value & 0x80U) != 0U));
        sccb_delay();
        sccb_scl(1U);
        sccb_delay();
        sccb_scl(0U);
        value <<= 1;
    }
    sccb_sda(1U);
    sccb_delay();
    sccb_scl(1U);
    sccb_delay();
    if (HAL_GPIO_ReadPin(SCCB_SDA_GPIO_Port, SCCB_SDA_Pin) != GPIO_PIN_RESET)
    {
        sccb_scl(0U);
        sccb_delay();
        return 1;
    }
    sccb_scl(0U);
    sccb_delay();
    return 0;
}

static int sccb_bus_read_byte(uint8_t nack, uint8_t *value)
{
    uint8_t bit;
    uint8_t data = 0U;

    if (value == NULL)
    {
        return 1;
    }
    sccb_sda(1U);
    for (bit = 0U; bit < 8U; bit++)
    {
        data <<= 1;
        sccb_scl(1U);
        sccb_delay();
        if (HAL_GPIO_ReadPin(SCCB_SDA_GPIO_Port, SCCB_SDA_Pin) != GPIO_PIN_RESET)
        {
            data++;
        }
        sccb_scl(0U);
        sccb_delay();
    }
    sccb_sda((nack != 0U) ? 1U : 0U);
    sccb_scl(1U);
    sccb_delay();
    sccb_scl(0U);
    sccb_sda(1U);
    sccb_delay();
    *value = data;
    return 0;
}
#endif

void sccb_init(void)
{
    sccb_bus_init();
}

int sccb_write_reg16(uint8_t device, uint16_t reg, uint8_t value)
{
    uint8_t write_device = (uint8_t)(device & 0xFEU);

    sccb_bus_start();
    if ((sccb_bus_write_byte(write_device) != 0) ||
        (sccb_bus_write_byte((uint8_t)(reg >> 8)) != 0) ||
        (sccb_bus_write_byte((uint8_t)reg) != 0) ||
        (sccb_bus_write_byte(value) != 0))
    {
        sccb_bus_stop();
        return SCCB_NAK;
    }
    sccb_bus_stop();
    return SCCB_OK;
}

int sccb_read_reg16(uint8_t device, uint16_t reg, uint8_t *value)
{
    uint8_t write_device = (uint8_t)(device & 0xFEU);

    if (value == NULL)
    {
        return SCCB_INVALID_ARG;
    }
    sccb_bus_start();
    if ((sccb_bus_write_byte(write_device) != 0) ||
        (sccb_bus_write_byte((uint8_t)(reg >> 8)) != 0) ||
        (sccb_bus_write_byte((uint8_t)reg) != 0))
    {
        sccb_bus_stop();
        return SCCB_NAK;
    }
    sccb_bus_stop();
    sccb_bus_start();
    if (sccb_bus_write_byte((uint8_t)(write_device | 0x01U)) != 0)
    {
        sccb_bus_stop();
        return SCCB_NAK;
    }
    if (sccb_bus_read_byte(1U, value) != 0)
    {
        sccb_bus_stop();
        return SCCB_BUS_ERROR;
    }
    sccb_bus_stop();
    return SCCB_OK;
}
