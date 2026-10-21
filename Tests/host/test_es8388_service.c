#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "es8388_service.h"
#include "board_i2c.h"

static board_i2c_result_t g_ready_result;
static board_i2c_result_t g_write_result;
static uint32_t g_ready_calls;
static uint32_t g_write_calls;
static uint16_t g_last_reg;
static uint8_t g_last_value;
static uint16_t g_write_regs[32];
static uint8_t g_write_values[32];

board_i2c_result_t board_i2c_device_ready(uint8_t address_7bit,
                                          uint32_t timeout_ms)
{
    if ((address_7bit != 0x10U) || (timeout_ms == 0U))
    {
        return BOARD_I2C_INVALID_ARG;
    }
    g_ready_calls++;
    return g_ready_result;
}

board_i2c_result_t board_i2c_read_reg(uint8_t address_7bit,
                                      uint16_t reg,
                                      uint8_t reg_size,
                                      uint8_t *data,
                                      uint16_t length,
                                      uint32_t timeout_ms)
{
    (void)address_7bit;
    (void)reg;
    (void)reg_size;
    (void)data;
    (void)length;
    (void)timeout_ms;
    return BOARD_I2C_ERROR;
}

board_i2c_result_t board_i2c_command_read(uint8_t address_7bit,
                                          uint8_t command,
                                          uint8_t parameter,
                                          uint8_t *data,
                                          uint16_t length,
                                          uint32_t timeout_ms)
{
    (void)address_7bit;
    (void)command;
    (void)parameter;
    (void)data;
    (void)length;
    (void)timeout_ms;
    return BOARD_I2C_ERROR;
}

board_i2c_result_t board_i2c_write_reg(uint8_t address_7bit,
                                       uint16_t reg,
                                       uint8_t reg_size,
                                       const uint8_t *data,
                                       uint16_t length,
                                       uint32_t timeout_ms)
{
    if ((address_7bit != 0x10U) || (reg_size != 1U) ||
        (data == NULL) || (length != 1U) || (timeout_ms == 0U))
    {
        return BOARD_I2C_INVALID_ARG;
    }
    g_write_calls++;
    g_last_reg = reg;
    g_last_value = data[0];
    if (g_write_calls <= 32U)
    {
        g_write_regs[g_write_calls - 1U] = reg;
        g_write_values[g_write_calls - 1U] = data[0];
    }
    return g_write_result;
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
    es8388_service_init();
    g_ready_result = BOARD_I2C_OK;
    g_write_result = BOARD_I2C_OK;
    g_ready_calls = 0U;
    g_write_calls = 0U;
    g_last_reg = 0U;
    g_last_value = 0U;
    memset(g_write_regs, 0, sizeof(g_write_regs));
    memset(g_write_values, 0, sizeof(g_write_values));
}

static int test_probe_and_status(void)
{
    es8388_status_t status;
    int failures = 0;

    reset_fixture();
    failures += expect_i32("probe_ok", es8388_probe_readonly(10U),
                           ES8388_OK);
    es8388_get_status(&status);
    failures += expect_u32("probe_address_calls", g_ready_calls, 1U);
    failures += expect_u32("present", status.present, 1U);
    failures += expect_u32("ready", status.ready, 1U);
    failures += expect_i32("status_result", status.last_result, ES8388_OK);
    failures += expect_u32("probe_count", status.probe_count, 1U);

    reset_fixture();
    g_ready_result = BOARD_I2C_NO_ACK;
    failures += expect_i32("probe_no_ack", es8388_probe_readonly(10U),
                           ES8388_NO_ACK);
    es8388_get_status(&status);
    failures += expect_u32("no_ack_absent", status.present, 0U);
    failures += expect_u32("no_ack_not_ready", status.ready, 0U);
    return failures;
}

static int test_volume_mapping_and_validation(void)
{
    es8388_status_t status;
    int failures = 0;

    reset_fixture();
    (void)es8388_probe_readonly(10U);
    failures += expect_i32("volume_50", es8388_set_volume_percent(50U),
                           ES8388_OK);
    es8388_get_status(&status);
    failures += expect_u32("volume_cached", status.volume_percent, 50U);
    failures += expect_u32("volume_write_count", g_write_calls, 2U);
    failures += expect_u32("volume_last_reg", g_last_reg, 0x31U);
    failures += expect_u32("volume_last_value", g_last_value, 17U);
    failures += expect_i32("volume_invalid", es8388_set_volume_percent(101U),
                           ES8388_INVALID_ARG);
    failures += expect_u32("invalid_no_write", g_write_calls, 2U);
    return failures;
}

static int test_prepare_playback(void)
{
    es8388_status_t status;
    int failures = 0;

    reset_fixture();
    failures += expect_i32("prepare_before_probe",
                           es8388_prepare_playback(), ES8388_NOT_READY);
    failures += expect_u32("prepare_before_probe_no_write",
                           g_write_calls, 0U);

    (void)es8388_probe_readonly(10U);
    failures += expect_i32("prepare_ok", es8388_prepare_playback(),
                           ES8388_OK);
    es8388_get_status(&status);
    failures += expect_u32("playback_configured", status.playback_configured,
                           1U);
    failures += expect_u32("configure_count", status.configure_count, 1U);
    failures += expect_u32("prepare_write_count", g_write_calls, 18U);
    failures += expect_u32("dac_format_reg", g_write_regs[9], 0x17U);
    failures += expect_u32("dac_format_value", g_write_values[9], 0x18U);
    failures += expect_u32("output_reg", g_write_regs[16], 0x04U);
    failures += expect_u32("output_value", g_write_values[16], 0x3CU);
    failures += expect_u32("dac_power_reg", g_write_regs[17], 0x02U);
    failures += expect_u32("dac_power_value", g_write_values[17], 0x0AU);
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_probe_and_status();
    failures += test_volume_mapping_and_validation();
    failures += test_prepare_playback();
    if (failures != 0)
    {
        printf("FAIL es8388_service failures=%d\n", failures);
        return 1;
    }
    printf("PASS es8388_service\n");
    return 0;
}
