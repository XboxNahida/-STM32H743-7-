#include "es8388_service.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "board_i2c.h"

#define ES8388_I2C_ADDRESS 0x10U
#define ES8388_SPEAKER_VOLUME_MAX 33U
#define ES8388_SPEAKER_LEFT_REG 0x30U
#define ES8388_SPEAKER_RIGHT_REG 0x31U
#define ES8388_I2C_TIMEOUT_MS 10U

static es8388_status_t g_es8388_status;

static void es8388_increment_saturated(uint32_t *value)
{
    if (*value != UINT32_MAX)
    {
        (*value)++;
    }
}

static int es8388_map_i2c_result(board_i2c_result_t result)
{
    switch (result)
    {
        case BOARD_I2C_OK:       return ES8388_OK;
        case BOARD_I2C_NO_ACK:   return ES8388_NO_ACK;
        case BOARD_I2C_NOT_READY:return ES8388_NOT_READY;
        case BOARD_I2C_TIMEOUT:  return ES8388_TIMEOUT;
        default:                 return ES8388_ERROR;
    }
}

static uint8_t es8388_volume_register(uint8_t percent)
{
    return (uint8_t)(((uint32_t)percent * ES8388_SPEAKER_VOLUME_MAX +
                      50U) / 100U);
}

void es8388_service_init(void)
{
    memset(&g_es8388_status, 0, sizeof(g_es8388_status));
    g_es8388_status.initialized = 1U;
    g_es8388_status.volume_percent = 50U;
    g_es8388_status.last_result = ES8388_NOT_READY;
}

int es8388_probe_readonly(uint32_t timeout_ms)
{
    board_i2c_result_t bus_result;
    int result;

    if (timeout_ms == 0U)
    {
        g_es8388_status.last_result = ES8388_INVALID_ARG;
        return ES8388_INVALID_ARG;
    }
    bus_result = board_i2c_device_ready(ES8388_I2C_ADDRESS, timeout_ms);
    result = es8388_map_i2c_result(bus_result);
    g_es8388_status.present = (result == ES8388_OK) ? 1U : 0U;
    g_es8388_status.ready = (result == ES8388_OK) ? 1U : 0U;
    g_es8388_status.last_result = result;
    es8388_increment_saturated(&g_es8388_status.probe_count);
    return result;
}

int es8388_prepare_playback(void)
{
    static const uint8_t registers[][2] =
    {
        {0x01U, 0x58U}, {0x01U, 0x50U}, {0x02U, 0xF3U},
        {0x02U, 0xF0U}, {0x03U, 0x09U}, {0x00U, 0x06U},
        {0x04U, 0x00U}, {0x08U, 0x00U}, {0x2BU, 0x80U},
        {0x17U, 0x18U}, {0x18U, 0x02U}, {0x1AU, 0x00U},
        {0x1BU, 0x00U}, {0x27U, 0xB8U}, {0x2AU, 0xB8U},
        {0x0AU, 0x00U}, {0x04U, 0x3CU}, {0x02U, 0x0AU}
    };
    uint32_t index;
    board_i2c_result_t bus_result;
    int result;

    if (g_es8388_status.ready == 0U)
    {
        g_es8388_status.last_result = ES8388_NOT_READY;
        return ES8388_NOT_READY;
    }
    if (g_es8388_status.playback_configured != 0U)
    {
        return ES8388_OK;
    }
    for (index = 0U; index < (sizeof(registers) / sizeof(registers[0]));
         index++)
    {
        bus_result = board_i2c_write_reg(ES8388_I2C_ADDRESS,
                                         registers[index][0], 1U,
                                         &registers[index][1], 1U,
                                         ES8388_I2C_TIMEOUT_MS);
        result = es8388_map_i2c_result(bus_result);
        if (result != ES8388_OK)
        {
            g_es8388_status.last_result = result;
            return result;
        }
        es8388_increment_saturated(&g_es8388_status.write_count);
    }
    g_es8388_status.playback_configured = 1U;
    g_es8388_status.recording_configured = 0U;
    es8388_increment_saturated(&g_es8388_status.configure_count);
    g_es8388_status.last_result = ES8388_OK;
    return ES8388_OK;
}

int es8388_prepare_recording(void)
{
    static const uint8_t registers[][2] =
    {
        {0x01U, 0x58U}, {0x01U, 0x50U}, {0x02U, 0xF3U},
        {0x02U, 0xF0U}, {0x03U, 0x09U}, {0x00U, 0x06U},
        {0x04U, 0x00U}, {0x08U, 0x00U}, {0x2BU, 0x80U},
        {0x09U, 0x88U}, {0x0CU, 0x4CU}, {0x0DU, 0x02U},
        {0x10U, 0x00U}, {0x11U, 0x00U}, {0x02U, 0x05U},
        {0x0AU, 0x00U}, {0x12U, 0xE4U}, {0x04U, 0x00U},
        {0x30U, 0x00U}, {0x31U, 0x00U}, {0x17U, 0x18U}
    };
    uint32_t index;
    board_i2c_result_t bus_result;
    int result;

    if (g_es8388_status.ready == 0U)
    {
        g_es8388_status.last_result = ES8388_NOT_READY;
        return ES8388_NOT_READY;
    }
    if (g_es8388_status.recording_configured != 0U)
    {
        return ES8388_OK;
    }
    for (index = 0U; index < (sizeof(registers) / sizeof(registers[0]));
         index++)
    {
        bus_result = board_i2c_write_reg(ES8388_I2C_ADDRESS,
                                         registers[index][0], 1U,
                                         &registers[index][1], 1U,
                                         ES8388_I2C_TIMEOUT_MS);
        result = es8388_map_i2c_result(bus_result);
        if (result != ES8388_OK)
        {
            g_es8388_status.last_result = result;
            return result;
        }
        es8388_increment_saturated(&g_es8388_status.write_count);
    }
    g_es8388_status.recording_configured = 1U;
    g_es8388_status.playback_configured = 0U;
    es8388_increment_saturated(&g_es8388_status.recording_configure_count);
    g_es8388_status.last_result = ES8388_OK;
    return ES8388_OK;
}

int es8388_set_volume_percent(uint8_t percent)
{
    uint8_t value;
    board_i2c_result_t left_result;
    board_i2c_result_t right_result;
    int result;

    if (percent > 100U)
    {
        return ES8388_INVALID_ARG;
    }
    g_es8388_status.volume_percent = percent;
    if (g_es8388_status.ready == 0U)
    {
        g_es8388_status.last_result = ES8388_NOT_READY;
        return ES8388_NOT_READY;
    }

    value = es8388_volume_register(percent);
    left_result = board_i2c_write_reg(ES8388_I2C_ADDRESS,
                                      ES8388_SPEAKER_LEFT_REG, 1U,
                                      &value, 1U, ES8388_I2C_TIMEOUT_MS);
    if (left_result != BOARD_I2C_OK)
    {
        result = es8388_map_i2c_result(left_result);
        g_es8388_status.last_result = result;
        return result;
    }
    es8388_increment_saturated(&g_es8388_status.write_count);

    right_result = board_i2c_write_reg(ES8388_I2C_ADDRESS,
                                       ES8388_SPEAKER_RIGHT_REG, 1U,
                                       &value, 1U, ES8388_I2C_TIMEOUT_MS);
    result = es8388_map_i2c_result(right_result);
    if (result == ES8388_OK)
    {
        es8388_increment_saturated(&g_es8388_status.write_count);
    }
    g_es8388_status.last_result = result;
    return result;
}

void es8388_get_status(es8388_status_t *status)
{
    if (status != NULL)
    {
        *status = g_es8388_status;
    }
}

void es8388_print_status(void)
{
    es8388_status_t status;

    es8388_get_status(&status);
    printf("es8388 status initialized=%u present=%u ready=%u playback_configured=%u recording_configured=%u volume_percent=%u result=%ld probe_count=%lu write_count=%lu configure_count=%lu recording_configure_count=%lu\r\n",
           (unsigned int)status.initialized,
           (unsigned int)status.present,
           (unsigned int)status.ready,
           (unsigned int)status.playback_configured,
           (unsigned int)status.recording_configured,
           (unsigned int)status.volume_percent,
           (long)status.last_result,
           (unsigned long)status.probe_count,
           (unsigned long)status.write_count,
           (unsigned long)status.configure_count,
           (unsigned long)status.recording_configure_count);
}
