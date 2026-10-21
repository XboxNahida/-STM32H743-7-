#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "board_i2c.h"

enum
{
    TEST_HAL_OK = 0,
    TEST_HAL_ERROR = 1,
    TEST_HAL_BUSY = 2,
    TEST_HAL_TIMEOUT = 3
};

#define TEST_HAL_ERROR_AF 0x00000004UL
#define TEST_HAL_ERROR_TIMEOUT 0x00000020UL

static int g_hal_status;
static uint32_t g_hal_error;
static uint32_t g_ready_calls;
static uint32_t g_read_calls;
static uint32_t g_write_calls;
static uint16_t g_last_address;
static uint16_t g_last_register;
static uint8_t g_last_reg_size;
static uint16_t g_last_length;
static uint32_t g_last_timeout;
static uint8_t g_read_value;

int board_i2c_test_device_ready(uint16_t address,
                                uint32_t trials,
                                uint32_t timeout_ms)
{
    (void)trials;
    g_ready_calls++;
    g_last_address = address;
    g_last_timeout = timeout_ms;
    return g_hal_status;
}

int board_i2c_test_mem_read(uint16_t address,
                            uint16_t reg,
                            uint8_t reg_size,
                            uint8_t *data,
                            uint16_t length,
                            uint32_t timeout_ms)
{
    g_read_calls++;
    g_last_address = address;
    g_last_register = reg;
    g_last_reg_size = reg_size;
    g_last_length = length;
    g_last_timeout = timeout_ms;
    if ((g_hal_status == TEST_HAL_OK) && (data != NULL) && (length != 0U))
    {
        data[0] = g_read_value;
    }
    return g_hal_status;
}

int board_i2c_test_mem_write(uint16_t address,
                             uint16_t reg,
                             uint8_t reg_size,
                             const uint8_t *data,
                             uint16_t length,
                             uint32_t timeout_ms)
{
    (void)data;
    g_write_calls++;
    g_last_address = address;
    g_last_register = reg;
    g_last_reg_size = reg_size;
    g_last_length = length;
    g_last_timeout = timeout_ms;
    return g_hal_status;
}

uint32_t board_i2c_test_error(void)
{
    return g_hal_error;
}

static int expect_u32(const char *name, uint32_t actual, uint32_t expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=%lu expected=%lu\n",
               name,
               (unsigned long)actual,
               (unsigned long)expected);
        return 1;
    }
    return 0;
}

static void reset_fixture(void)
{
    g_hal_status = TEST_HAL_OK;
    g_hal_error = 0U;
    g_ready_calls = 0U;
    g_read_calls = 0U;
    g_write_calls = 0U;
    g_last_address = 0U;
    g_last_register = 0U;
    g_last_reg_size = 0U;
    g_last_length = 0U;
    g_last_timeout = 0U;
    g_read_value = 0x61U;
}

static int test_read_and_write_address_contract(void)
{
    uint8_t value = 0U;
    uint8_t write_value = 0xA5U;
    int failures = 0;

    reset_fixture();
    failures += expect_u32("read_result",
                           board_i2c_read_reg(0x36U, 0x0FU, 1U,
                                              &value, 1U, 10U),
                           BOARD_I2C_OK);
    failures += expect_u32("read_hal_address", g_last_address, 0x6CU);
    failures += expect_u32("read_register", g_last_register, 0x0FU);
    failures += expect_u32("read_reg_size", g_last_reg_size, 1U);
    failures += expect_u32("read_length", g_last_length, 1U);
    failures += expect_u32("read_timeout", g_last_timeout, 10U);
    failures += expect_u32("read_value", value, 0x61U);

    failures += expect_u32("write_result",
                           board_i2c_write_reg(0x36U, 0x1234U, 2U,
                                               &write_value, 1U, 20U),
                           BOARD_I2C_OK);
    failures += expect_u32("write_hal_address", g_last_address, 0x6CU);
    failures += expect_u32("write_register", g_last_register, 0x1234U);
    failures += expect_u32("write_reg_size", g_last_reg_size, 2U);
    failures += expect_u32("write_length", g_last_length, 1U);
    failures += expect_u32("write_timeout", g_last_timeout, 20U);
    failures += expect_u32("one_read_call", g_read_calls, 1U);
    failures += expect_u32("one_write_call", g_write_calls, 1U);
    return failures;
}

static int test_command_read_contract(void)
{
    uint8_t data[3] = {0U, 0U, 0U};
    int failures = 0;

    reset_fixture();
    failures += expect_u32("command_read_result",
                           board_i2c_command_read(0x0CU, 0x50U, 0x00U,
                                                  data, 3U, 10U),
                           BOARD_I2C_OK);
    failures += expect_u32("command_read_hal_address",
                           g_last_address, 0x18U);
    failures += expect_u32("command_read_combined",
                           g_last_register, 0x5000U);
    failures += expect_u32("command_read_reg_size",
                           g_last_reg_size, 2U);
    failures += expect_u32("command_read_length", g_last_length, 3U);
    failures += expect_u32("command_read_timeout", g_last_timeout, 10U);
    failures += expect_u32("command_read_calls", g_read_calls, 1U);

    reset_fixture();
    failures += expect_u32("command_read_invalid_address",
                           board_i2c_command_read(0x80U, 0x50U, 0x00U,
                                                  data, 3U, 10U),
                           BOARD_I2C_INVALID_ARG);
    failures += expect_u32("command_read_null",
                           board_i2c_command_read(0x0CU, 0x50U, 0x00U,
                                                  NULL, 3U, 10U),
                           BOARD_I2C_INVALID_ARG);
    failures += expect_u32("command_read_zero_length",
                           board_i2c_command_read(0x0CU, 0x50U, 0x00U,
                                                  data, 0U, 10U),
                           BOARD_I2C_INVALID_ARG);
    failures += expect_u32("command_read_zero_timeout",
                           board_i2c_command_read(0x0CU, 0x50U, 0x00U,
                                                  data, 3U, 0U),
                           BOARD_I2C_INVALID_ARG);
    failures += expect_u32("command_read_invalid_calls", g_read_calls, 0U);
    return failures;
}

static int test_invalid_arguments_do_not_call_hal(void)
{
    uint8_t value = 0U;
    int failures = 0;

    reset_fixture();
    failures += expect_u32("invalid_address",
                           board_i2c_read_reg(0x80U, 0U, 1U,
                                              &value, 1U, 1U),
                           BOARD_I2C_INVALID_ARG);
    failures += expect_u32("invalid_reg_size",
                           board_i2c_read_reg(0x36U, 0U, 3U,
                                              &value, 1U, 1U),
                           BOARD_I2C_INVALID_ARG);
    failures += expect_u32("null_data",
                           board_i2c_read_reg(0x36U, 0U, 1U,
                                              NULL, 1U, 1U),
                           BOARD_I2C_INVALID_ARG);
    failures += expect_u32("zero_length",
                           board_i2c_read_reg(0x36U, 0U, 1U,
                                              &value, 0U, 1U),
                           BOARD_I2C_INVALID_ARG);
    failures += expect_u32("zero_timeout",
                           board_i2c_read_reg(0x36U, 0U, 1U,
                                              &value, 1U, 0U),
                           BOARD_I2C_INVALID_ARG);
    failures += expect_u32("invalid_calls", g_read_calls, 0U);
    return failures;
}

static int test_error_mapping(void)
{
    uint8_t value = 0U;
    int failures = 0;

    reset_fixture();
    g_hal_status = TEST_HAL_BUSY;
    failures += expect_u32("busy_mapping",
                           board_i2c_read_reg(0x36U, 0U, 1U,
                                              &value, 1U, 1U),
                           BOARD_I2C_NOT_READY);

    reset_fixture();
    g_hal_status = TEST_HAL_TIMEOUT;
    failures += expect_u32("timeout_mapping",
                           board_i2c_device_ready(0x36U, 1U),
                           BOARD_I2C_TIMEOUT);

    reset_fixture();
    g_hal_status = TEST_HAL_ERROR;
    g_hal_error = TEST_HAL_ERROR_AF;
    failures += expect_u32("no_ack_mapping",
                           board_i2c_device_ready(0x36U, 1U),
                           BOARD_I2C_NO_ACK);

    reset_fixture();
    g_hal_status = TEST_HAL_ERROR;
    g_hal_error = TEST_HAL_ERROR_TIMEOUT;
    failures += expect_u32("ready_trial_exhausted_mapping",
                           board_i2c_device_ready(0x36U, 1U),
                           BOARD_I2C_NO_ACK);

    reset_fixture();
    g_hal_status = TEST_HAL_ERROR;
    g_hal_error = TEST_HAL_ERROR_TIMEOUT;
    failures += expect_u32("read_timeout_error_stays_error",
                           board_i2c_read_reg(0x36U, 0U, 1U,
                                              &value, 1U, 1U),
                           BOARD_I2C_ERROR);

    reset_fixture();
    g_hal_status = TEST_HAL_ERROR;
    g_hal_error = 0x80000000UL;
    failures += expect_u32("error_mapping",
                           board_i2c_device_ready(0x36U, 1U),
                           BOARD_I2C_ERROR);
    failures += expect_u32("ready_call_count", g_ready_calls, 1U);
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_read_and_write_address_contract();
    failures += test_command_read_contract();
    failures += test_invalid_arguments_do_not_call_hal();
    failures += test_error_mapping();
    if (failures != 0)
    {
        printf("FAIL board_i2c failures=%d\n", failures);
        return 1;
    }
    printf("PASS board_i2c\n");
    return 0;
}
