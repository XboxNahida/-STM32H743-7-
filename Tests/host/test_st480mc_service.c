#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "st480mc_service.h"

static board_i2c_result_t g_i2c_result;
static uint8_t g_reply[3];
static uint32_t g_i2c_calls;
static uint8_t g_last_address;
static uint8_t g_last_command;
static uint8_t g_last_parameter;
static uint16_t g_last_length;
static uint32_t g_last_timeout;
static uint32_t g_now_ms;

board_i2c_result_t board_i2c_command_read(uint8_t address_7bit,
                                          uint8_t command,
                                          uint8_t parameter,
                                          uint8_t *data,
                                          uint16_t length,
                                          uint32_t timeout_ms)
{
    g_i2c_calls++;
    g_last_address = address_7bit;
    g_last_command = command;
    g_last_parameter = parameter;
    g_last_length = length;
    g_last_timeout = timeout_ms;
    g_now_ms = 123U;
    if ((g_i2c_result == BOARD_I2C_OK) && (data != NULL) &&
        (length == sizeof(g_reply)))
    {
        memcpy(data, g_reply, sizeof(g_reply));
    }
    return g_i2c_result;
}

uint32_t st480mc_test_now_ms(void)
{
    return g_now_ms;
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
    g_i2c_result = BOARD_I2C_OK;
    g_reply[0] = 0x04U;
    g_reply[1] = 0x12U;
    g_reply[2] = 0x34U;
    g_i2c_calls = 0U;
    g_last_address = 0U;
    g_last_command = 0U;
    g_last_parameter = 0U;
    g_last_length = 0U;
    g_last_timeout = 0U;
    g_now_ms = 100U;
    st480mc_service_init();
}

static int test_success_and_transaction_contract(void)
{
    st480mc_status_t status;
    int failures = 0;

    reset_fixture();
    failures += expect_u32("success_result",
                           st480mc_probe_readonly(10U),
                           ST480MC_RESULT_OK);
    st480mc_get_status(&status);
    failures += expect_u32("success_present", status.present, 1U);
    failures += expect_u32("success_ready", status.ready, 1U);
    failures += expect_u32("success_status", status.status_byte, 0x04U);
    failures += expect_u32("success_reg0", status.reg0_value, 0x1234U);
    failures += expect_u32("success_elapsed", status.elapsed_ms, 23U);
    failures += expect_u32("success_count", status.probe_count, 1U);
    failures += expect_u32("read_calls", g_i2c_calls, 1U);
    failures += expect_u32("read_address", g_last_address, 0x0CU);
    failures += expect_u32("read_command", g_last_command, 0x50U);
    failures += expect_u32("read_parameter", g_last_parameter, 0x00U);
    failures += expect_u32("read_length", g_last_length, 3U);
    failures += expect_u32("read_timeout", g_last_timeout, 10U);
    return failures;
}

static int test_bus_error_mapping(void)
{
    st480mc_status_t status;
    int failures = 0;

    reset_fixture();
    g_i2c_result = BOARD_I2C_NO_ACK;
    failures += expect_u32("absent_result", st480mc_probe_readonly(10U),
                           ST480MC_RESULT_ABSENT);
    st480mc_get_status(&status);
    failures += expect_u32("absent_present", status.present, 0U);

    reset_fixture();
    g_i2c_result = BOARD_I2C_NOT_READY;
    failures += expect_u32("busy_result", st480mc_probe_readonly(10U),
                           ST480MC_RESULT_NOT_READY);

    reset_fixture();
    g_i2c_result = BOARD_I2C_TIMEOUT;
    failures += expect_u32("timeout_result", st480mc_probe_readonly(10U),
                           ST480MC_RESULT_TIMEOUT);

    reset_fixture();
    g_i2c_result = BOARD_I2C_ERROR;
    failures += expect_u32("hal_error_result", st480mc_probe_readonly(10U),
                           ST480MC_RESULT_ERROR);
    return failures;
}

static int test_status_validation(void)
{
    st480mc_status_t status;
    int failures = 0;

    reset_fixture();
    g_reply[0] = 0x10U;
    failures += expect_u32("device_error", st480mc_probe_readonly(10U),
                           ST480MC_RESULT_DEVICE_ERROR);
    st480mc_get_status(&status);
    failures += expect_u32("device_error_present", status.present, 1U);
    failures += expect_u32("device_error_ready", status.ready, 0U);

    reset_fixture();
    g_reply[0] = 0xC0U;
    failures += expect_u32("multiple_modes", st480mc_probe_readonly(10U),
                           ST480MC_RESULT_PROTOCOL_ERROR);

    reset_fixture();
    g_reply[0] = 0x01U;
    failures += expect_u32("rr_data_count_bits_ignored",
                           st480mc_probe_readonly(10U),
                           ST480MC_RESULT_OK);

    reset_fixture();
    g_reply[0] = 0x08U;
    failures += expect_u32("sed_informational", st480mc_probe_readonly(10U),
                           ST480MC_RESULT_OK);
    return failures;
}

static int test_invalid_and_status_no_bus_access(void)
{
    st480mc_status_t before;
    st480mc_status_t after;
    int failures = 0;

    reset_fixture();
    failures += expect_u32("zero_timeout", st480mc_probe_readonly(0U),
                           ST480MC_RESULT_INVALID_ARG);
    failures += expect_u32("zero_timeout_calls", g_i2c_calls, 0U);
    st480mc_get_status(&before);
    st480mc_print_status();
    st480mc_get_status(&after);
    failures += expect_u32("status_no_i2c", g_i2c_calls, 0U);
    failures += expect_u32("status_unchanged", after.result, before.result);
    st480mc_get_status(NULL);
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_success_and_transaction_contract();
    failures += test_bus_error_mapping();
    failures += test_status_validation();
    failures += test_invalid_and_status_no_bus_access();
    if (failures != 0)
    {
        printf("FAIL st480mc_service failures=%d\n", failures);
        return 1;
    }
    printf("PASS st480mc_service\n");
    return 0;
}
