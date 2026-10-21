#include "st480mc_service.h"

#include <stdio.h>
#include <string.h>

#if defined(ST480MC_SERVICE_HOST_TEST)
extern uint32_t st480mc_test_now_ms(void);

static uint32_t st480mc_now_ms(void)
{
    return st480mc_test_now_ms();
}
#else
#include "main.h"

static uint32_t st480mc_now_ms(void)
{
    return HAL_GetTick();
}
#endif

#define ST480MC_ADDRESS_7BIT       0x0CU
#define ST480MC_READ_REGISTER      0x50U
#define ST480MC_REGISTER0_ADDRESS  0x00U
#define ST480MC_STATUS_MODE_MASK   0xE0U
#define ST480MC_STATUS_ERROR       0x10U

static st480mc_status_t g_st480mc_status;

void st480mc_service_init(void)
{
    memset(&g_st480mc_status, 0, sizeof(g_st480mc_status));
    g_st480mc_status.result = ST480MC_RESULT_NOT_READY;
}

static int st480mc_map_board_result(board_i2c_result_t result)
{
    switch (result)
    {
        case BOARD_I2C_NO_ACK:
            return ST480MC_RESULT_ABSENT;
        case BOARD_I2C_NOT_READY:
            return ST480MC_RESULT_NOT_READY;
        case BOARD_I2C_TIMEOUT:
            return ST480MC_RESULT_TIMEOUT;
        case BOARD_I2C_ERROR:
            return ST480MC_RESULT_ERROR;
        case BOARD_I2C_OK:
            return ST480MC_RESULT_OK;
        case BOARD_I2C_INVALID_ARG:
        default:
            return ST480MC_RESULT_INVALID_ARG;
    }
}

static int st480mc_validate_status(uint8_t status_byte)
{
    uint8_t mode_bits = status_byte & ST480MC_STATUS_MODE_MASK;

    if ((status_byte & ST480MC_STATUS_ERROR) != 0U)
    {
        return ST480MC_RESULT_DEVICE_ERROR;
    }
    if ((mode_bits != 0U) && ((mode_bits & (mode_bits - 1U)) != 0U))
    {
        return ST480MC_RESULT_PROTOCOL_ERROR;
    }
    return ST480MC_RESULT_OK;
}

int st480mc_probe_readonly(uint32_t timeout_ms)
{
    st480mc_status_t snapshot;
    board_i2c_result_t bus_result;
    uint8_t reply[3] = {0U, 0U, 0U};
    uint32_t start_ms;
    int result;

    if (timeout_ms == 0U)
    {
        return ST480MC_RESULT_INVALID_ARG;
    }

    snapshot = g_st480mc_status;
    snapshot.present = 0U;
    snapshot.ready = 0U;
    snapshot.status_byte = 0U;
    snapshot.reg0_value = 0U;
    start_ms = st480mc_now_ms();

    bus_result = board_i2c_command_read(ST480MC_ADDRESS_7BIT,
                                        ST480MC_READ_REGISTER,
                                        ST480MC_REGISTER0_ADDRESS,
                                        reply, sizeof(reply), timeout_ms);
    result = st480mc_map_board_result(bus_result);
    if (result == ST480MC_RESULT_OK)
    {
        snapshot.present = 1U;
        snapshot.status_byte = reply[0];
        snapshot.reg0_value = ((uint16_t)reply[1] << 8) | reply[2];
        result = st480mc_validate_status(snapshot.status_byte);
        if (result == ST480MC_RESULT_OK)
        {
            snapshot.ready = 1U;
        }
    }
    else if (result != ST480MC_RESULT_ABSENT)
    {
        snapshot.present = 1U;
    }

    snapshot.result = result;
    snapshot.elapsed_ms = st480mc_now_ms() - start_ms;
    if (snapshot.probe_count != UINT32_MAX)
    {
        snapshot.probe_count++;
    }
    g_st480mc_status = snapshot;
    return result;
}

void st480mc_get_status(st480mc_status_t *status)
{
    if (status != NULL)
    {
        *status = g_st480mc_status;
    }
}

void st480mc_print_status(void)
{
    printf("st480mc status protocol=st480mc present=%u ready=%u status=0x%02X reg0=0x%04X result=%ld elapsed_ms=%lu probe_count=%lu\r\n",
           (unsigned int)g_st480mc_status.present,
           (unsigned int)g_st480mc_status.ready,
           (unsigned int)g_st480mc_status.status_byte,
           (unsigned int)g_st480mc_status.reg0_value,
           (long)g_st480mc_status.result,
           (unsigned long)g_st480mc_status.elapsed_ms,
           (unsigned long)g_st480mc_status.probe_count);
}
