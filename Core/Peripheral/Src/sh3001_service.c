#include "sh3001_service.h"

#include <stdio.h>
#include <string.h>

#if defined(SH3001_SERVICE_HOST_TEST)
extern uint32_t sh3001_test_now_ms(void);

static uint32_t sh3001_now_ms(void)
{
    return sh3001_test_now_ms();
}
#else
#include "main.h"

static uint32_t sh3001_now_ms(void)
{
    return HAL_GetTick();
}
#endif

#define SH3001_ADDRESS_7BIT  0x36U
#define SH3001_CHIP_ID_REG   0x0FU
#define SH3001_EXPECTED_ID   0x61U

static sh3001_status_t g_sh3001_status;

void sh3001_service_init(void)
{
    memset(&g_sh3001_status, 0, sizeof(g_sh3001_status));
    g_sh3001_status.result = SH3001_RESULT_NOT_READY;
}

static int sh3001_map_result(board_i2c_result_t result)
{
    switch (result)
    {
        case BOARD_I2C_NO_ACK:
            return SH3001_RESULT_ABSENT;
        case BOARD_I2C_NOT_READY:
            return SH3001_RESULT_NOT_READY;
        case BOARD_I2C_TIMEOUT:
            return SH3001_RESULT_TIMEOUT;
        case BOARD_I2C_ERROR:
            return SH3001_RESULT_ERROR;
        case BOARD_I2C_OK:
            return SH3001_RESULT_OK;
        case BOARD_I2C_INVALID_ARG:
        default:
            return SH3001_RESULT_INVALID_ARG;
    }
}

int sh3001_probe_readonly(uint32_t timeout_ms)
{
    sh3001_status_t snapshot;
    uint8_t chip_id = 0U;
    uint32_t start_ms;
    board_i2c_result_t bus_result;
    int result;

    if (timeout_ms == 0U)
    {
        return SH3001_RESULT_INVALID_ARG;
    }

    snapshot = g_sh3001_status;
    snapshot.present = 0U;
    snapshot.ready = 0U;
    snapshot.chip_id = 0U;
    snapshot.result = SH3001_RESULT_NOT_READY;
    start_ms = sh3001_now_ms();

    bus_result = board_i2c_read_reg(SH3001_ADDRESS_7BIT,
                                    SH3001_CHIP_ID_REG,
                                    1U,
                                    &chip_id,
                                    1U,
                                    timeout_ms);
    result = sh3001_map_result(bus_result);
    if (result == SH3001_RESULT_OK)
    {
        snapshot.present = 1U;
        snapshot.chip_id = chip_id;
        if (chip_id == SH3001_EXPECTED_ID)
        {
            snapshot.ready = 1U;
        }
        else
        {
            result = SH3001_RESULT_ID_MISMATCH;
        }
    }
    else if (result != SH3001_RESULT_ABSENT)
    {
        snapshot.present = 1U;
    }

    snapshot.result = result;
    snapshot.elapsed_ms = sh3001_now_ms() - start_ms;
    if (snapshot.probe_count != UINT32_MAX)
    {
        snapshot.probe_count++;
    }
    g_sh3001_status = snapshot;
    return result;
}

void sh3001_get_status(sh3001_status_t *status)
{
    if (status != NULL)
    {
        *status = g_sh3001_status;
    }
}

void sh3001_print_status(void)
{
    printf("sh3001 status present=%u ready=%u chip_id=0x%02X result=%ld elapsed_ms=%lu probe_count=%lu\r\n",
           (unsigned int)g_sh3001_status.present,
           (unsigned int)g_sh3001_status.ready,
           (unsigned int)g_sh3001_status.chip_id,
           (long)g_sh3001_status.result,
           (unsigned long)g_sh3001_status.elapsed_ms,
           (unsigned long)g_sh3001_status.probe_count);
}
