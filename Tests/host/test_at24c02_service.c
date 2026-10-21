#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "at24c02_service.h"

static uint8_t g_eeprom[256];
static board_i2c_result_t g_ready_result;
static board_i2c_result_t g_read_result;
static board_i2c_result_t g_write_result;
static uint32_t g_now_ms;
static uint32_t g_ready_count;
static uint32_t g_read_count;
static uint32_t g_write_count;
static uint16_t g_write_regs[16];
static uint16_t g_write_lengths[16];
static uint32_t g_ready_busy_count;
static uint8_t g_last_address;
static uint16_t g_last_reg;
static uint8_t g_last_reg_size;
static uint16_t g_last_length;
static uint32_t g_last_timeout;

board_i2c_result_t board_i2c_device_ready(uint8_t address_7bit,
                                          uint32_t timeout_ms)
{
    g_ready_count++;
    g_last_address = address_7bit;
    g_last_timeout = timeout_ms;
    if (g_ready_busy_count != 0U)
    {
        g_ready_busy_count--;
        g_now_ms++;
        return BOARD_I2C_NO_ACK;
    }
    return g_ready_result;
}

board_i2c_result_t board_i2c_read_reg(uint8_t address_7bit,
                                     uint16_t reg,
                                     uint8_t reg_size,
                                     uint8_t *data,
                                     uint16_t length,
                                     uint32_t timeout_ms)
{
    uint16_t index;

    g_read_count++;
    g_last_address = address_7bit;
    g_last_reg = reg;
    g_last_reg_size = reg_size;
    g_last_length = length;
    g_last_timeout = timeout_ms;
    if (g_read_result != BOARD_I2C_OK)
    {
        return g_read_result;
    }
    for (index = 0U; index < length; index++)
    {
        data[index] = g_eeprom[reg + index];
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

    g_last_address = address_7bit;
    g_last_reg = reg;
    g_last_reg_size = reg_size;
    g_last_length = length;
    g_last_timeout = timeout_ms;
    if (g_write_count < 16U)
    {
        g_write_regs[g_write_count] = reg;
        g_write_lengths[g_write_count] = length;
    }
    g_write_count++;
    if (g_write_result != BOARD_I2C_OK)
    {
        return g_write_result;
    }
    for (index = 0U; index < length; index++)
    {
        g_eeprom[reg + index] = data[index];
    }
    return g_write_result;
}

uint32_t at24c02_test_now_ms(void)
{
    return g_now_ms;
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
    uint16_t index;

    for (index = 0U; index < sizeof(g_eeprom); index++)
    {
        g_eeprom[index] = (uint8_t)index;
    }
    g_ready_result = BOARD_I2C_OK;
    g_read_result = BOARD_I2C_OK;
    g_write_result = BOARD_I2C_OK;
    g_now_ms = 100U;
    g_ready_count = 0U;
    g_read_count = 0U;
    g_write_count = 0U;
    memset(g_write_regs, 0, sizeof(g_write_regs));
    memset(g_write_lengths, 0, sizeof(g_write_lengths));
    g_ready_busy_count = 0U;
    g_last_address = 0U;
    g_last_reg = 0U;
    g_last_reg_size = 0U;
    g_last_length = 0U;
    g_last_timeout = 0U;
    at24c02_service_init();
}

static int test_write_splits_at_eight_byte_page_boundaries(void)
{
    at24c02_status_t status;
    uint8_t data[12];
    uint16_t index;
    int failures = 0;

    reset_fixture();
    for (index = 0U; index < sizeof(data); index++)
    {
        data[index] = (uint8_t)(0xA0U + index);
    }
    failures += expect_i32("write_result",
                           at24c02_write(6U, data, sizeof(data), 20U),
                           AT24C02_RESULT_OK);
    at24c02_get_status(&status);
    failures += expect_u32("write_calls", g_write_count, 3U);
    failures += expect_u32("write0_reg", g_write_regs[0], 6U);
    failures += expect_u32("write0_len", g_write_lengths[0], 2U);
    failures += expect_u32("write1_reg", g_write_regs[1], 8U);
    failures += expect_u32("write1_len", g_write_lengths[1], 8U);
    failures += expect_u32("write2_reg", g_write_regs[2], 16U);
    failures += expect_u32("write2_len", g_write_lengths[2], 2U);
    failures += expect_u32("write_ready_polls", g_ready_count, 3U);
    failures += expect_u32("write_status_count", status.write_count, 1U);
    failures += expect_u32("write_last_offset", status.last_offset, 6U);
    failures += expect_u32("write_last_length", status.last_length, 12U);
    failures += expect_u32("write_first_value", g_eeprom[6], 0xA0U);
    failures += expect_u32("write_last_value", g_eeprom[17], 0xABU);
    return failures;
}

static int test_write_waits_for_ack_and_times_out(void)
{
    uint8_t data = 0x5AU;
    int failures = 0;

    reset_fixture();
    g_ready_busy_count = 2U;
    failures += expect_i32("write_ack_retry",
                           at24c02_write(31U, &data, 1U, 10U),
                           AT24C02_RESULT_OK);
    failures += expect_u32("write_ack_retry_count", g_ready_count, 3U);

    reset_fixture();
    g_ready_busy_count = 10U;
    failures += expect_i32("write_ack_timeout",
                           at24c02_write(31U, &data, 1U, 2U),
                           AT24C02_RESULT_TIMEOUT);
    failures += expect_u32("write_timeout_calls", g_write_count, 1U);
    return failures;
}

static int test_invalid_writes_do_not_touch_bus(void)
{
    uint8_t data[65] = {0};
    int failures = 0;

    reset_fixture();
    failures += expect_i32("write_zero_length",
                           at24c02_write(0U, data, 0U, 10U),
                           AT24C02_RESULT_INVALID_ARG);
    failures += expect_i32("write_too_long",
                           at24c02_write(0U, data, 65U, 10U),
                           AT24C02_RESULT_INVALID_ARG);
    failures += expect_i32("write_past_end",
                           at24c02_write(250U, data, 7U, 10U),
                           AT24C02_RESULT_INVALID_ARG);
    failures += expect_i32("write_null",
                           at24c02_write(0U, NULL, 1U, 10U),
                           AT24C02_RESULT_INVALID_ARG);
    failures += expect_i32("write_zero_timeout",
                           at24c02_write(0U, data, 1U, 0U),
                           AT24C02_RESULT_INVALID_ARG);
    failures += expect_u32("invalid_write_calls", g_write_count, 0U);
    failures += expect_u32("invalid_write_ready_calls", g_ready_count, 0U);
    return failures;
}

static int test_probe_only_checks_device_ready(void)
{
    at24c02_status_t status;
    int failures = 0;

    reset_fixture();
    g_now_ms = 103U;
    failures += expect_i32("probe_result", at24c02_probe_readonly(10U),
                           AT24C02_RESULT_OK);
    at24c02_get_status(&status);
    failures += expect_u32("probe_address", g_last_address, 0x50U);
    failures += expect_u32("probe_timeout", g_last_timeout, 10U);
    failures += expect_u32("probe_ready_calls", g_ready_count, 1U);
    failures += expect_u32("probe_read_calls", g_read_count, 0U);
    failures += expect_u32("probe_write_calls", g_write_count, 0U);
    failures += expect_u32("probe_present", status.present, 1U);
    failures += expect_u32("probe_ready", status.ready, 1U);
    failures += expect_u32("probe_count", status.probe_count, 1U);
    return failures;
}

static int test_read_uses_one_byte_internal_address(void)
{
    at24c02_status_t status;
    uint8_t data[64];
    int failures = 0;

    reset_fixture();
    failures += expect_i32("read_result", at24c02_read(16U, data, 64U, 20U),
                           AT24C02_RESULT_OK);
    at24c02_get_status(&status);
    failures += expect_u32("read_address", g_last_address, 0x50U);
    failures += expect_u32("read_reg", g_last_reg, 16U);
    failures += expect_u32("read_reg_size", g_last_reg_size, 1U);
    failures += expect_u32("read_length", g_last_length, 64U);
    failures += expect_u32("read_timeout", g_last_timeout, 20U);
    failures += expect_u32("read_calls", g_read_count, 1U);
    failures += expect_u32("read_write_calls", g_write_count, 0U);
    failures += expect_u32("read_first", data[0], 16U);
    failures += expect_u32("read_last", data[63], 79U);
    failures += expect_u32("read_present", status.present, 1U);
    failures += expect_u32("read_ready", status.ready, 1U);
    failures += expect_u32("read_count_status", status.read_count, 1U);
    failures += expect_u32("last_offset", status.last_offset, 16U);
    failures += expect_u32("last_length", status.last_length, 64U);
    return failures;
}

static int test_last_byte_is_valid(void)
{
    uint8_t data = 0U;
    int failures = 0;

    reset_fixture();
    failures += expect_i32("last_byte_result",
                           at24c02_read(255U, &data, 1U, 10U),
                           AT24C02_RESULT_OK);
    failures += expect_u32("last_byte_value", data, 255U);
    failures += expect_u32("last_byte_reg", g_last_reg, 255U);
    return failures;
}

static int test_invalid_requests_do_not_touch_bus(void)
{
    uint8_t data[65];
    int failures = 0;

    reset_fixture();
    failures += expect_i32("zero_length",
                           at24c02_read(0U, data, 0U, 10U),
                           AT24C02_RESULT_INVALID_ARG);
    failures += expect_i32("too_long",
                           at24c02_read(0U, data, 65U, 10U),
                           AT24C02_RESULT_INVALID_ARG);
    failures += expect_i32("past_end",
                           at24c02_read(250U, data, 7U, 10U),
                           AT24C02_RESULT_INVALID_ARG);
    failures += expect_i32("null_data",
                           at24c02_read(0U, NULL, 1U, 10U),
                           AT24C02_RESULT_INVALID_ARG);
    failures += expect_i32("zero_read_timeout",
                           at24c02_read(0U, data, 1U, 0U),
                           AT24C02_RESULT_INVALID_ARG);
    failures += expect_i32("zero_probe_timeout",
                           at24c02_probe_readonly(0U),
                           AT24C02_RESULT_INVALID_ARG);
    failures += expect_u32("invalid_ready_calls", g_ready_count, 0U);
    failures += expect_u32("invalid_read_calls", g_read_count, 0U);
    failures += expect_u32("invalid_write_calls", g_write_count, 0U);
    return failures;
}

static int test_bus_errors_are_mapped(void)
{
    at24c02_status_t status;
    uint8_t data[4];
    int failures = 0;

    reset_fixture();
    g_ready_result = BOARD_I2C_NO_ACK;
    failures += expect_i32("absent", at24c02_probe_readonly(10U),
                           AT24C02_RESULT_ABSENT);
    at24c02_get_status(&status);
    failures += expect_u32("absent_present", status.present, 0U);
    failures += expect_u32("absent_ready", status.ready, 0U);

    reset_fixture();
    g_read_result = BOARD_I2C_NOT_READY;
    failures += expect_i32("busy", at24c02_read(0U, data, 4U, 10U),
                           AT24C02_RESULT_NOT_READY);

    reset_fixture();
    g_read_result = BOARD_I2C_TIMEOUT;
    failures += expect_i32("timeout", at24c02_read(0U, data, 4U, 10U),
                           AT24C02_RESULT_TIMEOUT);
    failures += expect_u32("error_write_calls", g_write_count, 0U);
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_probe_only_checks_device_ready();
    failures += test_write_splits_at_eight_byte_page_boundaries();
    failures += test_write_waits_for_ack_and_times_out();
    failures += test_invalid_writes_do_not_touch_bus();
    failures += test_read_uses_one_byte_internal_address();
    failures += test_last_byte_is_valid();
    failures += test_invalid_requests_do_not_touch_bus();
    failures += test_bus_errors_are_mapped();

    if (failures != 0)
    {
        printf("FAIL at24c02_service failures=%d\n", failures);
        return 1;
    }
    printf("PASS at24c02_service\n");
    return 0;
}
