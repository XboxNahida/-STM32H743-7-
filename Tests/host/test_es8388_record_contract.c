#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "board_i2c.h"
#include "es8388_service.h"

static uint32_t g_write_count;
static uint32_t g_fail_at;
static uint16_t g_regs[32];
static uint8_t g_values[32];

board_i2c_result_t board_i2c_device_ready(uint8_t address_7bit,
                                          uint32_t timeout_ms)
{
    return ((address_7bit == 0x10U) && (timeout_ms != 0U)) ?
           BOARD_I2C_OK : BOARD_I2C_INVALID_ARG;
}

board_i2c_result_t board_i2c_read_reg(uint8_t address_7bit, uint16_t reg,
                                      uint8_t reg_size, uint8_t *data,
                                      uint16_t length, uint32_t timeout_ms)
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
    if ((address_7bit != 0x10U) || (reg_size != 1U) || (data == NULL) ||
        (length != 1U) || (timeout_ms == 0U))
    {
        return BOARD_I2C_INVALID_ARG;
    }
    g_write_count++;
    if (g_write_count <= 32U)
    {
        g_regs[g_write_count - 1U] = reg;
        g_values[g_write_count - 1U] = data[0];
    }
    return (g_write_count == g_fail_at) ? BOARD_I2C_ERROR : BOARD_I2C_OK;
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
    g_write_count = 0U;
    g_fail_at = 0U;
    memset(g_regs, 0, sizeof(g_regs));
    memset(g_values, 0, sizeof(g_values));
}

int main(void)
{
    es8388_status_t status;
    int failures = 0;

    reset_fixture();
    failures += expect_i32("before_probe", es8388_prepare_recording(),
                           ES8388_NOT_READY);
    failures += expect_u32("before_probe_no_write", g_write_count, 0U);

    (void)es8388_probe_readonly(10U);
    failures += expect_i32("prepare", es8388_prepare_recording(), ES8388_OK);
    es8388_get_status(&status);
    failures += expect_u32("recording_configured",
                           status.recording_configured, 1U);
    failures += expect_u32("recording_configure_count",
                           status.recording_configure_count, 1U);
    failures += expect_u32("write_count", g_write_count, 21U);
    failures += expect_u32("adc_power_reg", g_regs[14], 0x02U);
    failures += expect_u32("adc_power_value", g_values[14], 0x05U);
    failures += expect_u32("mic_input_reg", g_regs[15], 0x0AU);
    failures += expect_u32("mic_input_value", g_values[15], 0x00U);
    failures += expect_u32("alc_reg", g_regs[16], 0x12U);
    failures += expect_u32("alc_value", g_values[16], 0xE4U);
    failures += expect_i32("prepare_idempotent",
                           es8388_prepare_recording(), ES8388_OK);
    failures += expect_u32("idempotent_no_write", g_write_count, 21U);

    reset_fixture();
    (void)es8388_probe_readonly(10U);
    g_fail_at = 5U;
    failures += expect_i32("write_failure", es8388_prepare_recording(),
                           ES8388_ERROR);
    es8388_get_status(&status);
    failures += expect_u32("failure_not_configured",
                           status.recording_configured, 0U);
    failures += expect_u32("failure_stops", g_write_count, 5U);

    if (failures != 0)
    {
        printf("FAIL es8388_record_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS es8388_record_contract\n");
    return 0;
}
