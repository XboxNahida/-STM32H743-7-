#include <stdio.h>
/**
 ********************************************************************
 * @file    pcf8574.c
 * @brief   PCF8574 I2C GPIO扩展芯片驱动
 * @note    使用 HAL I2C (hi2c2) 替代原子的软件I2C
 *          移植自正点原子 pcf8574.c
 ********************************************************************
 */
#include "pcf8574.h"

#define PCF8574_WRITE_TIMEOUT_MS 10U

static uint8_t g_pcf8574_output_shadow = 0xFFU;

#ifdef PCF8574_HOST_TEST
extern int pcf8574_test_is_ready(uint32_t timeout_ms);
extern int pcf8574_test_transmit(uint8_t data, uint32_t timeout_ms);

static int pcf8574_device_is_ready(uint32_t timeout_ms)
{
    return pcf8574_test_is_ready(timeout_ms);
}

static int pcf8574_transmit(uint8_t data, uint32_t timeout_ms)
{
    return pcf8574_test_transmit(data, timeout_ms);
}
#else
static int pcf8574_device_is_ready(uint32_t timeout_ms)
{
    HAL_StatusTypeDef result;

    result = HAL_I2C_IsDeviceReady(&hi2c2, PCF8574_ADDR, 1U, timeout_ms);
    return (result == HAL_OK) ? 0 : 1;
}

static int pcf8574_transmit(uint8_t data, uint32_t timeout_ms)
{
    HAL_StatusTypeDef result;

    result = HAL_I2C_Master_Transmit(&hi2c2, PCF8574_ADDR,
                                     &data, 1U, timeout_ms);
    return (result == HAL_OK) ? 0 : 1;
}
#endif

static int pcf8574_commit_output(uint8_t data, uint8_t allow_beep)
{
    uint8_t next = data;

    if (allow_beep == 0U)
    {
        next |= (uint8_t)(1U << BEEP_IO);
    }
    if (pcf8574_transmit(next, PCF8574_WRITE_TIMEOUT_MS) != 0)
    {
        return 1;
    }
    g_pcf8574_output_shadow = next;
    return 0;
}

uint8_t pcf8574_probe_readonly(uint32_t timeout_ms)
{
#ifndef PCF8574_HOST_TEST
    HAL_StatusTypeDef result;
#endif

    if (timeout_ms == 0U)
    {
        return 1U;
    }
#ifdef PCF8574_HOST_TEST
    return (pcf8574_test_is_ready(timeout_ms) == 0) ? 0U : 1U;
#else
    result = HAL_I2C_IsDeviceReady(&hi2c2, PCF8574_ADDR, 1U, timeout_ms);
    return (result == HAL_OK) ? 0U : 1U;
#endif
}

/**
 * @brief  检测PCF8574是否在位, 并初始化所有IO为高电平
 * @retval 0=成功(芯片在位), 1=失败(未检测到)
 */
uint8_t pcf8574_init(void)
{
    /* 发送地址检测芯片是否在位 */
    if (pcf8574_device_is_ready(5U) != 0)
    {
#ifndef PCF8574_HOST_TEST
        printf("[PCF8574] Not found!\r\n");
#endif
        return 1;
    }

    /* 默认所有IO输出高电平 */
    if (pcf8574_commit_output(0xFFU, 0U) != 0)
    {
        return 1;
    }
#ifndef PCF8574_HOST_TEST
    printf("[PCF8574] OK.\r\n");
#endif
    return 0;
}

/**
 * @brief  读取PCF8574的8位IO值
 * @retval 8位IO数据
 */
uint8_t pcf8574_read_byte(void)
{
#ifdef PCF8574_HOST_TEST
    return g_pcf8574_output_shadow;
#else
    uint8_t data = 0;
    HAL_I2C_Master_Receive(&hi2c2, PCF8574_ADDR, &data, 1, 100);
    return data;
#endif
}

/**
 * @brief  向PCF8574写入8位IO值
 * @param  data: 8位数据
 */
void pcf8574_write_byte(uint8_t data)
{
    (void)pcf8574_commit_output(data, 0U);
}

/**
 * @brief  设置PCF8574某个IO的高低电平
 * @param  bit: IO编号(0~7)
 * @param  sta: 0=低电平, 1=高电平
 */
void pcf8574_write_bit(uint8_t bit, uint8_t sta)
{
    (void)pcf8574_write_bit_safe(bit, sta);
}

/**
 * @brief  读取PCF8574某个IO的状态
 * @param  bit: IO编号(0~7)
 * @retval 0或1
 */
uint8_t pcf8574_read_bit(uint8_t bit)
{
    uint8_t data = pcf8574_read_byte();
    return (data & (1 << bit)) ? 1 : 0;
}

int pcf8574_write_bit_safe(uint8_t bit, uint8_t state)
{
    uint8_t next;

    if (bit > 7U)
    {
        return 1;
    }
    next = g_pcf8574_output_shadow;
    if (state != 0U)
    {
        next |= (uint8_t)(1U << bit);
    }
    else
    {
        next &= (uint8_t)~(1U << bit);
    }
    return pcf8574_commit_output(next, 0U);
}

uint8_t pcf8574_get_output_shadow(void)
{
    return g_pcf8574_output_shadow;
}

int pcf8574_beep_set(uint8_t enabled)
{
    uint8_t next = g_pcf8574_output_shadow;

    if (enabled != 0U)
    {
        next &= (uint8_t)~(1U << BEEP_IO);
    }
    else
    {
        next |= (uint8_t)(1U << BEEP_IO);
    }
    return pcf8574_commit_output(next, 1U);
}
