#include "imu_service.h"

#include <stdio.h>
#include <string.h>

#if defined(IMU_SERVICE_HOST_TEST)
extern uint32_t imu_test_now_ms(void);

static uint32_t imu_now_ms(void)
{
    return imu_test_now_ms();
}
#else
#include "main.h"

static uint32_t imu_now_ms(void)
{
    return HAL_GetTick();
}
#endif

#define QMI8658_ADDRESS_7BIT    0x6AU
#define QMI8658_WHOAMI_REG      0x00U
#define QMI8658_REVISION_REG    0x01U
#define QMI8658_EXPECTED_ID     0x05U
#define QMI8658_EXPECTED_REV    0x7CU

static imu_status_t g_imu_status;

void imu_service_init(void)
{
    memset(&g_imu_status, 0, sizeof(g_imu_status));
    g_imu_status.result = IMU_RESULT_NOT_READY;
}

static int imu_map_board_result(board_i2c_result_t result)
{
    switch (result)
    {
        case BOARD_I2C_NO_ACK:
            return IMU_RESULT_ABSENT;
        case BOARD_I2C_NOT_READY:
            return IMU_RESULT_NOT_READY;
        case BOARD_I2C_TIMEOUT:
            return IMU_RESULT_TIMEOUT;
        case BOARD_I2C_ERROR:
            return IMU_RESULT_ERROR;
        case BOARD_I2C_OK:
            return IMU_RESULT_OK;
        case BOARD_I2C_INVALID_ARG:
        default:
            return IMU_RESULT_INVALID_ARG;
    }
}

static int imu_map_sh3001_result(int32_t result)
{
    switch (result)
    {
        case SH3001_RESULT_OK:
            return IMU_RESULT_OK;
        case SH3001_RESULT_ABSENT:
            return IMU_RESULT_ABSENT;
        case SH3001_RESULT_NOT_READY:
            return IMU_RESULT_NOT_READY;
        case SH3001_RESULT_TIMEOUT:
            return IMU_RESULT_TIMEOUT;
        case SH3001_RESULT_ID_MISMATCH:
            return IMU_RESULT_ID_MISMATCH;
        case SH3001_RESULT_ERROR:
            return IMU_RESULT_ERROR;
        case SH3001_RESULT_INVALID_ARG:
        default:
            return IMU_RESULT_INVALID_ARG;
    }
}

static int imu_probe_qmi8658(imu_status_t *snapshot, uint32_t timeout_ms)
{
    board_i2c_result_t bus_result;

    bus_result = board_i2c_read_reg(QMI8658_ADDRESS_7BIT,
                                    QMI8658_WHOAMI_REG,
                                    1U,
                                    &snapshot->id0,
                                    1U,
                                    timeout_ms);
    if (bus_result != BOARD_I2C_OK)
    {
        return imu_map_board_result(bus_result);
    }

    snapshot->present = 1U;
    snapshot->model = IMU_MODEL_QMI8658A;
    if (snapshot->id0 != QMI8658_EXPECTED_ID)
    {
        return IMU_RESULT_ID_MISMATCH;
    }

    bus_result = board_i2c_read_reg(QMI8658_ADDRESS_7BIT,
                                    QMI8658_REVISION_REG,
                                    1U,
                                    &snapshot->id1,
                                    1U,
                                    timeout_ms);
    if (bus_result != BOARD_I2C_OK)
    {
        return imu_map_board_result(bus_result);
    }
    if (snapshot->id1 != QMI8658_EXPECTED_REV)
    {
        return IMU_RESULT_ID_MISMATCH;
    }

    snapshot->ready = 1U;
    return IMU_RESULT_OK;
}

static int imu_probe_sh3001(imu_status_t *snapshot, uint32_t timeout_ms)
{
    sh3001_status_t sh3001;
    int result;

    memset(&sh3001, 0, sizeof(sh3001));
    result = sh3001_probe_readonly(timeout_ms);
    sh3001_get_status(&sh3001);
    snapshot->present = sh3001.present;
    snapshot->ready = sh3001.ready;
    snapshot->id0 = sh3001.chip_id;
    if (sh3001.present != 0U)
    {
        snapshot->model = IMU_MODEL_SH3001;
    }
    return imu_map_sh3001_result(result);
}

int imu_probe_readonly(uint32_t timeout_ms)
{
    imu_status_t snapshot;
    uint32_t start_ms;
    int result;

    if (timeout_ms == 0U)
    {
        return IMU_RESULT_INVALID_ARG;
    }

    snapshot = g_imu_status;
    snapshot.present = 0U;
    snapshot.ready = 0U;
    snapshot.model = IMU_MODEL_NONE;
    snapshot.id0 = 0U;
    snapshot.id1 = 0U;
    start_ms = imu_now_ms();

    result = imu_probe_qmi8658(&snapshot, timeout_ms);
    if (result == IMU_RESULT_ABSENT)
    {
        result = imu_probe_sh3001(&snapshot, timeout_ms);
    }

    snapshot.result = result;
    snapshot.elapsed_ms = imu_now_ms() - start_ms;
    if (snapshot.probe_count != UINT32_MAX)
    {
        snapshot.probe_count++;
    }
    g_imu_status = snapshot;
    return result;
}

void imu_get_status(imu_status_t *status)
{
    if (status != NULL)
    {
        *status = g_imu_status;
    }
}

void imu_print_status(void)
{
    static const char *const model_names[] =
    {
        "none", "sh3001", "qmi8658a"
    };
    uint32_t model = g_imu_status.model;

    if (model >= (sizeof(model_names) / sizeof(model_names[0])))
    {
        model = IMU_MODEL_NONE;
    }
    printf("imu status model=%s present=%u ready=%u id0=0x%02X id1=0x%02X result=%ld elapsed_ms=%lu probe_count=%lu\r\n",
           model_names[model],
           (unsigned int)g_imu_status.present,
           (unsigned int)g_imu_status.ready,
           (unsigned int)g_imu_status.id0,
           (unsigned int)g_imu_status.id1,
           (long)g_imu_status.result,
           (unsigned long)g_imu_status.elapsed_ms,
           (unsigned long)g_imu_status.probe_count);
}
