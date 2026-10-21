#include <stdint.h>
#include <stdio.h>

#include "imu_service.h"

static board_i2c_result_t g_qmi_result;
static uint8_t g_qmi_whoami;
static uint8_t g_qmi_revision;
static uint32_t g_qmi_calls;
static uint32_t g_sh3001_calls;
static sh3001_status_t g_sh3001_status;
static uint32_t g_now_ms;

board_i2c_result_t board_i2c_read_reg(uint8_t address_7bit,
                                      uint16_t reg,
                                      uint8_t reg_size,
                                      uint8_t *data,
                                      uint16_t length,
                                      uint32_t timeout_ms)
{
    (void)timeout_ms;
    g_qmi_calls++;
    g_now_ms++;
    if ((address_7bit != 0x6AU) || (reg_size != 1U) ||
        (data == NULL) || (length != 1U))
    {
        return BOARD_I2C_INVALID_ARG;
    }
    if (g_qmi_result == BOARD_I2C_OK)
    {
        *data = (reg == 0U) ? g_qmi_whoami : g_qmi_revision;
    }
    return g_qmi_result;
}

int sh3001_probe_readonly(uint32_t timeout_ms)
{
    (void)timeout_ms;
    g_sh3001_calls++;
    g_now_ms++;
    return (int)g_sh3001_status.result;
}

void sh3001_get_status(sh3001_status_t *status)
{
    if (status != NULL)
    {
        *status = g_sh3001_status;
    }
}

uint32_t imu_test_now_ms(void)
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
    g_qmi_result = BOARD_I2C_OK;
    g_qmi_whoami = 0x05U;
    g_qmi_revision = 0x7CU;
    g_qmi_calls = 0U;
    g_sh3001_calls = 0U;
    g_now_ms = 100U;
    g_sh3001_status.present = 0U;
    g_sh3001_status.ready = 0U;
    g_sh3001_status.chip_id = 0U;
    g_sh3001_status.result = SH3001_RESULT_ABSENT;
    g_sh3001_status.elapsed_ms = 0U;
    g_sh3001_status.probe_count = 0U;
    imu_service_init();
}

static int test_qmi8658_selected_first(void)
{
    imu_status_t status;
    int failures = 0;

    reset_fixture();
    failures += expect_u32("qmi_result", imu_probe_readonly(10U),
                           IMU_RESULT_OK);
    imu_get_status(&status);
    failures += expect_u32("qmi_model", status.model, IMU_MODEL_QMI8658A);
    failures += expect_u32("qmi_present", status.present, 1U);
    failures += expect_u32("qmi_ready", status.ready, 1U);
    failures += expect_u32("qmi_whoami", status.id0, 0x05U);
    failures += expect_u32("qmi_revision", status.id1, 0x7CU);
    failures += expect_u32("qmi_reads", g_qmi_calls, 2U);
    failures += expect_u32("qmi_no_sh_fallback", g_sh3001_calls, 0U);
    return failures;
}

static int test_sh3001_fallback(void)
{
    imu_status_t status;
    int failures = 0;

    reset_fixture();
    g_qmi_result = BOARD_I2C_NO_ACK;
    g_sh3001_status.present = 1U;
    g_sh3001_status.ready = 1U;
    g_sh3001_status.chip_id = 0x61U;
    g_sh3001_status.result = SH3001_RESULT_OK;
    failures += expect_u32("sh_result", imu_probe_readonly(10U),
                           IMU_RESULT_OK);
    imu_get_status(&status);
    failures += expect_u32("sh_model", status.model, IMU_MODEL_SH3001);
    failures += expect_u32("sh_id", status.id0, 0x61U);
    failures += expect_u32("sh_qmi_reads", g_qmi_calls, 1U);
    failures += expect_u32("sh_fallback_calls", g_sh3001_calls, 1U);
    return failures;
}

static int test_absent_and_id_mismatch(void)
{
    imu_status_t status;
    int failures = 0;

    reset_fixture();
    g_qmi_result = BOARD_I2C_NO_ACK;
    failures += expect_u32("absent_result", imu_probe_readonly(10U),
                           IMU_RESULT_ABSENT);
    imu_get_status(&status);
    failures += expect_u32("absent_present", status.present, 0U);

    reset_fixture();
    g_qmi_whoami = 0x04U;
    failures += expect_u32("mismatch_result", imu_probe_readonly(10U),
                           IMU_RESULT_ID_MISMATCH);
    imu_get_status(&status);
    failures += expect_u32("mismatch_present", status.present, 1U);
    failures += expect_u32("mismatch_model", status.model,
                           IMU_MODEL_QMI8658A);
    failures += expect_u32("mismatch_no_fallback", g_sh3001_calls, 0U);
    return failures;
}

static int test_status_has_no_bus_side_effect(void)
{
    imu_status_t status;
    int failures = 0;

    reset_fixture();
    imu_get_status(&status);
    imu_print_status();
    imu_get_status(NULL);
    failures += expect_u32("status_qmi_calls", g_qmi_calls, 0U);
    failures += expect_u32("status_sh_calls", g_sh3001_calls, 0U);
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_qmi8658_selected_first();
    failures += test_sh3001_fallback();
    failures += test_absent_and_id_mismatch();
    failures += test_status_has_no_bus_side_effect();
    if (failures != 0)
    {
        printf("FAIL imu_service failures=%d\n", failures);
        return 1;
    }
    printf("PASS imu_service\n");
    return 0;
}
