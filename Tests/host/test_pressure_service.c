#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pressure_service.h"

static uint8_t g_registers[256];
static board_i2c_result_t g_read_result;
static board_i2c_result_t g_write_result;
static uint32_t g_now_ms;
static uint32_t g_read_count;
static uint32_t g_write_count;
static uint8_t g_hold_data_not_ready;

board_i2c_result_t board_i2c_read_reg(uint8_t address_7bit,
                                     uint16_t reg,
                                     uint8_t reg_size,
                                     uint8_t *data,
                                     uint16_t length,
                                     uint32_t timeout_ms)
{
    uint16_t index;

    g_read_count++;
    if ((address_7bit != 0x76U) || (reg_size != 1U) ||
        (data == NULL) || (length == 0U) || (timeout_ms == 0U))
    {
        return BOARD_I2C_INVALID_ARG;
    }
    if (g_read_result != BOARD_I2C_OK)
    {
        return g_read_result;
    }
    if ((reg == 0x08U) && (g_hold_data_not_ready != 0U))
    {
        data[0] = 0xC7U;
        return BOARD_I2C_OK;
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
    uint16_t index;

    g_write_count++;
    if ((address_7bit != 0x76U) || (reg_size != 1U) ||
        (data == NULL) || (length == 0U) || (timeout_ms == 0U))
    {
        return BOARD_I2C_INVALID_ARG;
    }
    if (g_write_result != BOARD_I2C_OK)
    {
        return g_write_result;
    }
    for (index = 0U; index < length; index++)
    {
        g_registers[(uint8_t)(reg + index)] = data[index];
    }
    if ((reg == 0x08U) && (data[0] == 0x07U) &&
        (g_hold_data_not_ready == 0U))
    {
        g_registers[0x08U] = 0xF7U;
    }
    return BOARD_I2C_OK;
}

uint32_t pressure_test_now_ms(void)
{
    return g_now_ms;
}

void pressure_test_delay_ms(uint32_t delay_ms)
{
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

static void set_u16(uint8_t reg, int16_t value)
{
    uint16_t raw = (uint16_t)value;

    g_registers[reg] = (uint8_t)(raw >> 8);
    g_registers[(uint8_t)(reg + 1U)] = (uint8_t)raw;
}

static void set_calibration(void)
{
    uint32_t c0 = 40U;
    uint32_t c1 = 10U;
    uint32_t c00 = 100000U;
    uint32_t c10 = 1000U;
    uint32_t c31 = 1U;
    uint32_t c40 = 1U;

    g_registers[0x10U] = (uint8_t)(c0 >> 4);
    g_registers[0x11U] = (uint8_t)((c0 << 4) | (c1 >> 8));
    g_registers[0x12U] = (uint8_t)c1;
    g_registers[0x13U] = (uint8_t)(c00 >> 12);
    g_registers[0x14U] = (uint8_t)(c00 >> 4);
    g_registers[0x15U] = (uint8_t)((c00 << 4) | (c10 >> 16));
    g_registers[0x16U] = (uint8_t)(c10 >> 8);
    g_registers[0x17U] = (uint8_t)c10;
    set_u16(0x18U, 20);
    set_u16(0x1AU, 2);
    set_u16(0x1CU, 100);
    set_u16(0x1EU, 1);
    set_u16(0x20U, 10);
    g_registers[0x22U] = (uint8_t)(c31 >> 4);
    g_registers[0x23U] = (uint8_t)((c31 << 4) | (c40 >> 8));
    g_registers[0x24U] = (uint8_t)c40;
}

static void reset_fixture(void)
{
    memset(g_registers, 0, sizeof(g_registers));
    g_registers[0x0DU] = 0x11U;
    g_registers[0x08U] = 0xC0U;
    set_calibration();
    g_registers[0x00U] = 0x0FU;
    g_registers[0x01U] = 0xE0U;
    g_registers[0x02U] = 0x00U;
    g_registers[0x03U] = 0x0FU;
    g_registers[0x04U] = 0xE0U;
    g_registers[0x05U] = 0x00U;
    g_read_result = BOARD_I2C_OK;
    g_write_result = BOARD_I2C_OK;
    g_now_ms = 100U;
    g_read_count = 0U;
    g_write_count = 0U;
    g_hold_data_not_ready = 0U;
    pressure_service_init();
}

static int test_probe_is_readonly(void)
{
    pressure_status_t status;
    int failures = 0;

    reset_fixture();
    failures += expect_i32("probe_result", pressure_probe_readonly(10U),
                           PRESSURE_RESULT_OK);
    pressure_get_status(&status);
    failures += expect_u32("probe_present", status.present, 1U);
    failures += expect_u32("probe_ready", status.ready, 1U);
    failures += expect_u32("probe_id", status.chip_id, 0x11U);
    failures += expect_u32("probe_writes", g_write_count, 0U);
    failures += expect_u32("probe_count", status.probe_count, 1U);
    return failures;
}

static int test_full_compensation_and_configuration(void)
{
    pressure_status_t status;
    int failures = 0;

    reset_fixture();
    failures += expect_i32("read_result", pressure_read_once(500U),
                           PRESSURE_RESULT_OK);
    pressure_get_status(&status);
    failures += expect_i32("raw_pressure", status.raw_pressure, 1040384);
    failures += expect_i32("raw_temperature", status.raw_temperature, 1040384);
    failures += expect_i32("temperature_centi", status.temperature_centi_c, 3000);
    failures += expect_i32("pressure_pa", status.pressure_pa, 101135);
    failures += expect_u32("pressure_cfg", g_registers[0x06U], 0x26U);
    failures += expect_u32("temperature_cfg", g_registers[0x07U], 0x26U);
    failures += expect_u32("shift_cfg", g_registers[0x09U] & 0x0CU, 0x0CU);
    failures += expect_u32("standby_after_read", g_registers[0x08U], 0x00U);
    failures += expect_u32("configured", status.configured, 1U);
    failures += expect_u32("read_count", status.read_count, 1U);
    return failures;
}

static int test_negative_sign_extension(void)
{
    pressure_status_t status;
    int failures = 0;

    reset_fixture();
    g_registers[0x00U] = 0xF0U;
    g_registers[0x01U] = 0x20U;
    g_registers[0x02U] = 0x00U;
    g_registers[0x03U] = 0xF0U;
    g_registers[0x04U] = 0x20U;
    g_registers[0x05U] = 0x00U;
    failures += expect_i32("negative_read", pressure_read_once(500U),
                           PRESSURE_RESULT_OK);
    pressure_get_status(&status);
    failures += expect_i32("negative_raw_pressure", status.raw_pressure, -1040384);
    failures += expect_i32("negative_raw_temperature", status.raw_temperature, -1040384);
    failures += expect_i32("negative_temperature", status.temperature_centi_c, 1000);
    failures += expect_i32("negative_pressure", status.pressure_pa, 99073);
    return failures;
}

static int test_error_and_timeout_mapping(void)
{
    pressure_status_t status;
    int failures = 0;

    reset_fixture();
    g_read_result = BOARD_I2C_NO_ACK;
    failures += expect_i32("absent", pressure_probe_readonly(10U),
                           PRESSURE_RESULT_ABSENT);
    pressure_get_status(&status);
    failures += expect_u32("absent_present", status.present, 0U);

    reset_fixture();
    g_registers[0x0DU] = 0x10U;
    failures += expect_i32("id_mismatch", pressure_probe_readonly(10U),
                           PRESSURE_RESULT_ID_MISMATCH);

    reset_fixture();
    g_hold_data_not_ready = 1U;
    failures += expect_i32("data_timeout", pressure_read_once(5U),
                           PRESSURE_RESULT_TIMEOUT);
    pressure_get_status(&status);
    failures += expect_u32("timeout_elapsed", status.elapsed_ms, 5U);
    failures += expect_u32("timeout_standby", g_registers[0x08U], 0x00U);

    reset_fixture();
    failures += expect_i32("zero_timeout", pressure_read_once(0U),
                           PRESSURE_RESULT_INVALID_ARG);
    failures += expect_u32("zero_timeout_reads", g_read_count, 0U);
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_probe_is_readonly();
    failures += test_full_compensation_and_configuration();
    failures += test_negative_sign_extension();
    failures += test_error_and_timeout_mapping();

    if (failures != 0)
    {
        printf("FAIL pressure_service failures=%d\n", failures);
        return 1;
    }
    printf("PASS pressure_service\n");
    return 0;
}
