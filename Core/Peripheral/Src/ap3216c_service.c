#include "ap3216c_service.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#if defined(AP3216C_SERVICE_HOST_TEST)
extern uint32_t ap3216c_test_now_ms(void);
extern void ap3216c_test_delay_ms(uint32_t delay_ms);

static uint32_t ap3216c_now_ms(void)
{
    return ap3216c_test_now_ms();
}

static void ap3216c_delay_ms(uint32_t delay_ms)
{
    ap3216c_test_delay_ms(delay_ms);
}
#else
#include "main.h"

static uint32_t ap3216c_now_ms(void)
{
    return HAL_GetTick();
}

static void ap3216c_delay_ms(uint32_t delay_ms)
{
    HAL_Delay(delay_ms);
}
#endif

#define AP3216C_ADDRESS_7BIT       0x1EU
#define AP3216C_SYSTEM_CFG_REG     0x00U
#define AP3216C_DATA_REG           0x0AU
#define AP3216C_DATA_LENGTH        6U
#define AP3216C_RESERVED_MASK      0xF8U
#define AP3216C_MODE_MASK          0x07U
#define AP3216C_MODE_POWER_DOWN    0x00U
#define AP3216C_MODE_RESET         0x04U
#define AP3216C_MODE_ALL_ONCE      0x07U
#define AP3216C_RESET_DELAY_MS     10U
#define AP3216C_ONCE_DELAY_MS      240U
#define AP3216C_LUX_MILLI_PER_BIT  350U

static ap3216c_status_t g_ap3216c_status;

static int ap3216c_map_board_result(board_i2c_result_t result)
{
    switch (result)
    {
        case BOARD_I2C_NO_ACK:
            return AP3216C_RESULT_ABSENT;
        case BOARD_I2C_NOT_READY:
            return AP3216C_RESULT_NOT_READY;
        case BOARD_I2C_TIMEOUT:
            return AP3216C_RESULT_TIMEOUT;
        case BOARD_I2C_ERROR:
            return AP3216C_RESULT_ERROR;
        case BOARD_I2C_OK:
            return AP3216C_RESULT_OK;
        case BOARD_I2C_INVALID_ARG:
        default:
            return AP3216C_RESULT_INVALID_ARG;
    }
}

static int ap3216c_read_register(uint8_t reg, uint8_t *data,
                                 uint16_t length, uint32_t timeout_ms)
{
    board_i2c_result_t bus_result;

    bus_result = board_i2c_read_reg(AP3216C_ADDRESS_7BIT, reg, 1U,
                                    data, length, timeout_ms);
    return ap3216c_map_board_result(bus_result);
}

static int ap3216c_write_register(uint8_t reg, uint8_t value,
                                  uint32_t timeout_ms)
{
    board_i2c_result_t bus_result;

    bus_result = board_i2c_write_reg(AP3216C_ADDRESS_7BIT, reg, 1U,
                                     &value, 1U, timeout_ms);
    return ap3216c_map_board_result(bus_result);
}

static void ap3216c_increment_saturated(uint32_t *value)
{
    if (*value != UINT32_MAX)
    {
        (*value)++;
    }
}

static void ap3216c_decode_data(ap3216c_status_t *snapshot,
                                const uint8_t *data)
{
    uint8_t overflow;
    uint8_t object_low;
    uint8_t object_high;

    snapshot->ir_raw = (uint16_t)(((uint16_t)data[1] << 2) |
                                  (data[0] & 0x03U));
    snapshot->als_raw = (uint16_t)(((uint16_t)data[3] << 8) | data[2]);
    snapshot->proximity_raw =
        (uint16_t)(((uint16_t)(data[5] & 0x3FU) << 4) |
                   (data[4] & 0x0FU));
    snapshot->lux_milli = (uint32_t)snapshot->als_raw *
                          AP3216C_LUX_MILLI_PER_BIT;

    overflow = (((data[0] & 0x80U) != 0U) ||
                ((data[4] & 0x40U) != 0U) ||
                ((data[5] & 0x40U) != 0U)) ? 1U : 0U;
    object_low = ((data[4] & 0x80U) != 0U) ? 1U : 0U;
    object_high = ((data[5] & 0x80U) != 0U) ? 1U : 0U;

    snapshot->ir_valid = (overflow == 0U) ? 1U : 0U;
    snapshot->proximity_valid =
        ((overflow == 0U) && (object_low == object_high)) ? 1U : 0U;
    snapshot->proximity_near =
        (snapshot->proximity_valid != 0U) ? object_low : 0U;
}

void ap3216c_service_init(void)
{
    memset(&g_ap3216c_status, 0, sizeof(g_ap3216c_status));
    g_ap3216c_status.result = AP3216C_RESULT_NOT_READY;
}

int ap3216c_probe_readonly(uint32_t timeout_ms)
{
    ap3216c_status_t snapshot;
    uint8_t system_cfg = 0U;
    uint32_t start_ms;
    int result;

    if (timeout_ms == 0U)
    {
        return AP3216C_RESULT_INVALID_ARG;
    }

    snapshot = g_ap3216c_status;
    snapshot.present = 0U;
    snapshot.ready = 0U;
    start_ms = ap3216c_now_ms();
    result = ap3216c_read_register(AP3216C_SYSTEM_CFG_REG, &system_cfg,
                                   1U, timeout_ms);
    if (result == AP3216C_RESULT_OK)
    {
        snapshot.present = 1U;
        snapshot.system_mode = system_cfg & AP3216C_MODE_MASK;
        if ((system_cfg & AP3216C_RESERVED_MASK) != 0U)
        {
            result = AP3216C_RESULT_PROTOCOL_ERROR;
        }
        else
        {
            snapshot.ready = 1U;
        }
    }

    snapshot.result = result;
    snapshot.elapsed_ms = ap3216c_now_ms() - start_ms;
    ap3216c_increment_saturated(&snapshot.probe_count);
    g_ap3216c_status = snapshot;
    return result;
}

int ap3216c_read_once(uint32_t timeout_ms)
{
    ap3216c_status_t snapshot;
    uint8_t data[AP3216C_DATA_LENGTH];
    uint32_t required_delay;
    uint32_t start_ms;
    int result;

    if (timeout_ms == 0U)
    {
        return AP3216C_RESULT_INVALID_ARG;
    }

    snapshot = g_ap3216c_status;
    required_delay = AP3216C_ONCE_DELAY_MS;
    if (snapshot.initialized == 0U)
    {
        required_delay += AP3216C_RESET_DELAY_MS;
    }
    if (timeout_ms < required_delay)
    {
        snapshot.result = AP3216C_RESULT_TIMEOUT;
        snapshot.elapsed_ms = 0U;
        ap3216c_increment_saturated(&snapshot.read_count);
        g_ap3216c_status = snapshot;
        return AP3216C_RESULT_TIMEOUT;
    }

    start_ms = ap3216c_now_ms();
    result = AP3216C_RESULT_OK;
    if (snapshot.initialized == 0U)
    {
        result = ap3216c_write_register(AP3216C_SYSTEM_CFG_REG,
                                        AP3216C_MODE_RESET, timeout_ms);
        if (result == AP3216C_RESULT_OK)
        {
            snapshot.present = 1U;
            ap3216c_delay_ms(AP3216C_RESET_DELAY_MS);
            snapshot.initialized = 1U;
        }
    }
    if (result == AP3216C_RESULT_OK)
    {
        result = ap3216c_write_register(AP3216C_SYSTEM_CFG_REG,
                                        AP3216C_MODE_ALL_ONCE, timeout_ms);
        if (result == AP3216C_RESULT_OK)
        {
            snapshot.present = 1U;
            snapshot.ready = 1U;
            snapshot.system_mode = AP3216C_MODE_ALL_ONCE;
            ap3216c_delay_ms(AP3216C_ONCE_DELAY_MS);
        }
    }
    if (result == AP3216C_RESULT_OK)
    {
        result = ap3216c_read_register(AP3216C_DATA_REG, data,
                                       AP3216C_DATA_LENGTH, timeout_ms);
    }
    if (result == AP3216C_RESULT_OK)
    {
        ap3216c_decode_data(&snapshot, data);
        snapshot.system_mode = AP3216C_MODE_POWER_DOWN;
    }

    snapshot.result = result;
    snapshot.elapsed_ms = ap3216c_now_ms() - start_ms;
    ap3216c_increment_saturated(&snapshot.read_count);
    g_ap3216c_status = snapshot;
    return result;
}

void ap3216c_get_status(ap3216c_status_t *status)
{
    if (status != NULL)
    {
        *status = g_ap3216c_status;
    }
}

void ap3216c_print_status(void)
{
    printf("ap3216c status present=%u ready=%u initialized=%u mode=%u als_raw=%u lux_milli=%lu ir_raw=%u ir_valid=%u proximity_raw=%u proximity_valid=%u proximity_near=%u result=%ld elapsed_ms=%lu probe_count=%lu read_count=%lu\r\n",
           (unsigned int)g_ap3216c_status.present,
           (unsigned int)g_ap3216c_status.ready,
           (unsigned int)g_ap3216c_status.initialized,
           (unsigned int)g_ap3216c_status.system_mode,
           (unsigned int)g_ap3216c_status.als_raw,
           (unsigned long)g_ap3216c_status.lux_milli,
           (unsigned int)g_ap3216c_status.ir_raw,
           (unsigned int)g_ap3216c_status.ir_valid,
           (unsigned int)g_ap3216c_status.proximity_raw,
           (unsigned int)g_ap3216c_status.proximity_valid,
           (unsigned int)g_ap3216c_status.proximity_near,
           (long)g_ap3216c_status.result,
           (unsigned long)g_ap3216c_status.elapsed_ms,
           (unsigned long)g_ap3216c_status.probe_count,
           (unsigned long)g_ap3216c_status.read_count);
}
