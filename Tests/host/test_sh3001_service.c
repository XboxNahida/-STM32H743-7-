#include <stdint.h>
#include <stdio.h>

#include "sh3001_service.h"

static board_i2c_result_t g_i2c_result;
static uint8_t g_i2c_id;
static uint32_t g_i2c_calls;
static uint8_t g_last_address;
static uint16_t g_last_register;
static uint8_t g_last_reg_size;
static uint16_t g_last_length;
static uint32_t g_last_timeout;
static uint32_t g_now_ms;

board_i2c_result_t board_i2c_read_reg(uint8_t address_7bit,
                                      uint16_t reg,
                                      uint8_t reg_size,
                                      uint8_t *data,
                                      uint16_t length,
                                      uint32_t timeout_ms)
{
    g_i2c_calls++;
    g_last_address = address_7bit;
    g_last_register = reg;
    g_last_reg_size = reg_size;
    g_last_length = length;
    g_last_timeout = timeout_ms;
    g_now_ms = 123U;
    if ((g_i2c_result == BOARD_I2C_OK) && (data != NULL) && (length != 0U))
    {
        data[0] = g_i2c_id;
    }
    return g_i2c_result;
}

uint32_t sh3001_test_now_ms(void)
{
    return g_now_ms;
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
    g_i2c_result = BOARD_I2C_OK;
    g_i2c_id = 0x61U;
    g_i2c_calls = 0U;
    g_last_address = 0U;
    g_last_register = 0U;
    g_last_reg_size = 0U;
    g_last_length = 0U;
    g_last_timeout = 0U;
    g_now_ms = 100U;
    sh3001_service_init();
}

static int test_success_and_read_contract(void)
{
    sh3001_status_t status;
    int failures = 0;

    reset_fixture();
    failures += expect_u32("success_result",
                           sh3001_probe_readonly(10U),
                           SH3001_RESULT_OK);
    sh3001_get_status(&status);
    failures += expect_u32("success_present", status.present, 1U);
    failures += expect_u32("success_ready", status.ready, 1U);
    failures += expect_u32("success_id", status.chip_id, 0x61U);
    failures += expect_u32("success_elapsed", status.elapsed_ms, 23U);
    failures += expect_u32("success_count", status.probe_count, 1U);
    failures += expect_u32("read_calls", g_i2c_calls, 1U);
    failures += expect_u32("read_address", g_last_address, 0x36U);
    failures += expect_u32("read_register", g_last_register, 0x0FU);
    failures += expect_u32("read_reg_size", g_last_reg_size, 1U);
    failures += expect_u32("read_length", g_last_length, 1U);
    failures += expect_u32("read_timeout", g_last_timeout, 10U);
    return failures;
}

static int test_error_mapping(void)
{
    sh3001_status_t status;
    int failures = 0;

    reset_fixture();
    g_i2c_id = 0x62U;
    failures += expect_u32("id_mismatch_result",
                           sh3001_probe_readonly(10U),
                           SH3001_RESULT_ID_MISMATCH);
    sh3001_get_status(&status);
    failures += expect_u32("id_mismatch_present", status.present, 1U);
    failures += expect_u32("id_mismatch_ready", status.ready, 0U);

    reset_fixture();
    g_i2c_result = BOARD_I2C_NO_ACK;
    failures += expect_u32("absent_result",
                           sh3001_probe_readonly(10U),
                           SH3001_RESULT_ABSENT);
    sh3001_get_status(&status);
    failures += expect_u32("absent_present", status.present, 0U);

    reset_fixture();
    g_i2c_result = BOARD_I2C_TIMEOUT;
    failures += expect_u32("timeout_result",
                           sh3001_probe_readonly(10U),
                           SH3001_RESULT_TIMEOUT);

    reset_fixture();
    g_i2c_result = BOARD_I2C_NOT_READY;
    failures += expect_u32("not_ready_result",
                           sh3001_probe_readonly(10U),
                           SH3001_RESULT_NOT_READY);

    reset_fixture();
    g_i2c_result = BOARD_I2C_ERROR;
    failures += expect_u32("error_result",
                           sh3001_probe_readonly(10U),
                           SH3001_RESULT_ERROR);
    return failures;
}

static int test_status_has_no_bus_side_effect(void)
{
    sh3001_status_t before;
    sh3001_status_t after;
    int failures = 0;

    reset_fixture();
    sh3001_get_status(&before);
    sh3001_print_status();
    sh3001_get_status(&after);
    failures += expect_u32("status_no_i2c", g_i2c_calls, 0U);
    failures += expect_u32("status_result_unchanged",
                           after.result, before.result);
    sh3001_get_status(NULL);
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_success_and_read_contract();
    failures += test_error_mapping();
    failures += test_status_has_no_bus_side_effect();
    if (failures != 0)
    {
        printf("FAIL sh3001_service failures=%d\n", failures);
        return 1;
    }
    printf("PASS sh3001_service\n");
    return 0;
}
