#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ap3216c_service.h"

static uint8_t g_registers[256];
static board_i2c_result_t g_read_result;
static board_i2c_result_t g_write_result;
static uint32_t g_now_ms;
static uint32_t g_delay_total;
static uint32_t g_read_count;
static uint32_t g_write_count;
static uint8_t g_last_read_reg;
static uint16_t g_last_read_length;
static uint8_t g_write_regs[4];
static uint8_t g_write_values[4];

board_i2c_result_t board_i2c_read_reg(uint8_t address_7bit,
                                     uint16_t reg,
                                     uint8_t reg_size,
                                     uint8_t *data,
                                     uint16_t length,
                                     uint32_t timeout_ms)
{
    uint16_t index;

    g_read_count++;
    g_last_read_reg = (uint8_t)reg;
    g_last_read_length = length;
    if ((address_7bit != 0x1EU) || (reg_size != 1U) ||
        (data == NULL) || (length == 0U) || (timeout_ms == 0U))
    {
        return BOARD_I2C_INVALID_ARG;
    }
    if (g_read_result != BOARD_I2C_OK)
    {
        return g_read_result;
    }
    for (index = 0U; index < length; index++)
    {
        data[index] = g_registers[(uint8_t)(reg + index)];
    }
    return BOARD_I2C_OK;
}

board_i2c_result_t board_i2c_write_reg(uint8_t address_7bit,
                                      uint16_t reg,
                                      uint8_t reg_size,
                                      const uint8_t *data,
                                      uint16_t length,
                                      uint32_t timeout_ms)
{
    if ((address_7bit != 0x1EU) || (reg_size != 1U) ||
        (data == NULL) || (length != 1U) || (timeout_ms == 0U))
    {
        return BOARD_I2C_INVALID_ARG;
    }
    if (g_write_count < 4U)
    {
        g_write_regs[g_write_count] = (uint8_t)reg;
        g_write_values[g_write_count] = data[0];
    }
    g_write_count++;
    if (g_write_result != BOARD_I2C_OK)
    {
        return g_write_result;
    }
    g_registers[(uint8_t)reg] = data[0];
    return BOARD_I2C_OK;
}

uint32_t ap3216c_test_now_ms(void)
{
    return g_now_ms;
}

void ap3216c_test_delay_ms(uint32_t delay_ms)
{
    g_delay_total += delay_ms;
    g_now_ms += delay_ms;
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

static void reset_fixture(void)
{
    memset(g_registers, 0, sizeof(g_registers));
    memset(g_write_regs, 0, sizeof(g_write_regs));
    memset(g_write_values, 0, sizeof(g_write_values));
    g_read_result = BOARD_I2C_OK;
    g_write_result = BOARD_I2C_OK;
    g_now_ms = 100U;
    g_delay_total = 0U;
    g_read_count = 0U;
    g_write_count = 0U;
    g_last_read_reg = 0U;
    g_last_read_length = 0U;
    ap3216c_service_init();
}

static void set_normal_data(void)
{
    g_registers[0x0AU] = 0x02U;
    g_registers[0x0BU] = 0x12U;
    g_registers[0x0CU] = 0x34U;
    g_registers[0x0DU] = 0x12U;
    g_registers[0x0EU] = 0x8BU;
    g_registers[0x0FU] = 0x95U;
}

static int test_probe_is_readonly(void)
{
    ap3216c_status_t status;
    int failures = 0;

    reset_fixture();
    g_registers[0x00U] = 0x00U;
    failures += expect_i32("probe_result", ap3216c_probe_readonly(10U),
                           AP3216C_RESULT_OK);
    ap3216c_get_status(&status);
    failures += expect_u32("probe_present", status.present, 1U);
    failures += expect_u32("probe_ready", status.ready, 1U);
    failures += expect_u32("probe_mode", status.system_mode, 0U);
    failures += expect_u32("probe_writes", g_write_count, 0U);
    failures += expect_u32("probe_reads", g_read_count, 1U);
    failures += expect_u32("probe_count", status.probe_count, 1U);
    return failures;
}

static int test_first_combined_measurement(void)
{
    ap3216c_status_t status;
    int failures = 0;

    reset_fixture();
    set_normal_data();
    failures += expect_i32("read_result", ap3216c_read_once(300U),
                           AP3216C_RESULT_OK);
    ap3216c_get_status(&status);
    failures += expect_u32("write_count", g_write_count, 2U);
    failures += expect_u32("reset_reg", g_write_regs[0], 0x00U);
    failures += expect_u32("reset_value", g_write_values[0], 0x04U);
    failures += expect_u32("once_reg", g_write_regs[1], 0x00U);
    failures += expect_u32("once_value", g_write_values[1], 0x07U);
    failures += expect_u32("delay_total", g_delay_total, 250U);
    failures += expect_u32("burst_reg", g_last_read_reg, 0x0AU);
    failures += expect_u32("burst_length", g_last_read_length, 6U);
    failures += expect_u32("ir_raw", status.ir_raw, 74U);
    failures += expect_u32("als_raw", status.als_raw, 4660U);
    failures += expect_u32("lux_milli", status.lux_milli, 1631000U);
    failures += expect_u32("proximity_raw", status.proximity_raw, 347U);
    failures += expect_u32("ir_valid", status.ir_valid, 1U);
    failures += expect_u32("proximity_valid", status.proximity_valid, 1U);
    failures += expect_u32("proximity_near", status.proximity_near, 1U);
    failures += expect_u32("initialized", status.initialized, 1U);
    failures += expect_u32("elapsed", status.elapsed_ms, 250U);
    failures += expect_u32("read_count", status.read_count, 1U);
    return failures;
}

static int test_second_measurement_skips_reset(void)
{
    int failures = 0;

    reset_fixture();
    set_normal_data();
    failures += expect_i32("first_read", ap3216c_read_once(300U),
                           AP3216C_RESULT_OK);
    g_write_count = 0U;
    g_delay_total = 0U;
    failures += expect_i32("second_read", ap3216c_read_once(300U),
                           AP3216C_RESULT_OK);
    failures += expect_u32("second_write_count", g_write_count, 1U);
    failures += expect_u32("second_once_value", g_write_values[0], 0x07U);
    failures += expect_u32("second_delay", g_delay_total, 240U);
    return failures;
}

static int test_invalid_flags_preserve_raw_data(void)
{
    ap3216c_status_t status;
    int failures = 0;

    reset_fixture();
    set_normal_data();
    g_registers[0x0AU] |= 0x80U;
    g_registers[0x0EU] |= 0x40U;
    g_registers[0x0FU] |= 0x40U;
    failures += expect_i32("overflow_read", ap3216c_read_once(300U),
                           AP3216C_RESULT_OK);
    ap3216c_get_status(&status);
    failures += expect_u32("overflow_ir_raw", status.ir_raw, 74U);
    failures += expect_u32("overflow_ps_raw", status.proximity_raw, 347U);
    failures += expect_u32("overflow_ir_valid", status.ir_valid, 0U);
    failures += expect_u32("overflow_ps_valid", status.proximity_valid, 0U);

    reset_fixture();
    set_normal_data();
    g_registers[0x0FU] &= 0x7FU;
    failures += expect_i32("obj_mismatch_read", ap3216c_read_once(300U),
                           AP3216C_RESULT_OK);
    ap3216c_get_status(&status);
    failures += expect_u32("obj_mismatch_valid", status.proximity_valid, 0U);
    failures += expect_u32("obj_mismatch_near", status.proximity_near, 0U);
    return failures;
}

static int test_errors_are_bounded(void)
{
    ap3216c_status_t status;
    int failures = 0;

    reset_fixture();
    g_read_result = BOARD_I2C_NO_ACK;
    failures += expect_i32("absent", ap3216c_probe_readonly(10U),
                           AP3216C_RESULT_ABSENT);
    ap3216c_get_status(&status);
    failures += expect_u32("absent_present", status.present, 0U);

    reset_fixture();
    g_registers[0x00U] = 0x80U;
    failures += expect_i32("reserved_mode", ap3216c_probe_readonly(10U),
                           AP3216C_RESULT_PROTOCOL_ERROR);

    reset_fixture();
    set_normal_data();
    failures += expect_i32("short_timeout", ap3216c_read_once(249U),
                           AP3216C_RESULT_TIMEOUT);
    failures += expect_u32("short_timeout_writes", g_write_count, 0U);
    failures += expect_u32("short_timeout_delay", g_delay_total, 0U);

    reset_fixture();
    failures += expect_i32("zero_timeout", ap3216c_probe_readonly(0U),
                           AP3216C_RESULT_INVALID_ARG);
    failures += expect_u32("zero_timeout_reads", g_read_count, 0U);
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_probe_is_readonly();
    failures += test_first_combined_measurement();
    failures += test_second_measurement_skips_reset();
    failures += test_invalid_flags_preserve_raw_data();
    failures += test_errors_are_bounded();

    if (failures != 0)
    {
        printf("FAIL ap3216c_service failures=%d\n", failures);
        return 1;
    }
    printf("PASS ap3216c_service\n");
    return 0;
}
