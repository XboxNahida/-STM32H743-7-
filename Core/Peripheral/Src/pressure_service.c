#include "pressure_service.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#if defined(PRESSURE_SERVICE_HOST_TEST)
extern uint32_t pressure_test_now_ms(void);
extern void pressure_test_delay_ms(uint32_t delay_ms);

static uint32_t pressure_now_ms(void)
{
    return pressure_test_now_ms();
}

static void pressure_delay_ms(uint32_t delay_ms)
{
    pressure_test_delay_ms(delay_ms);
}
#else
#include "main.h"

static uint32_t pressure_now_ms(void)
{
    return HAL_GetTick();
}

static void pressure_delay_ms(uint32_t delay_ms)
{
    HAL_Delay(delay_ms);
}
#endif

#define PRESSURE_ADDRESS_7BIT       0x76U
#define PRESSURE_DATA_REG           0x00U
#define PRESSURE_PRS_CFG_REG        0x06U
#define PRESSURE_TMP_CFG_REG        0x07U
#define PRESSURE_MEAS_CFG_REG       0x08U
#define PRESSURE_CFG_REG            0x09U
#define PRESSURE_ID_REG             0x0DU
#define PRESSURE_COEF_REG           0x10U
#define PRESSURE_EXPECTED_ID        0x11U
#define PRESSURE_COEF_LENGTH        21U
#define PRESSURE_DATA_LENGTH        6U
#define PRESSURE_RATE_4_OSR_64      0x26U
#define PRESSURE_SHIFT_MASK         0x0CU
#define PRESSURE_BACKGROUND_BOTH    0x07U
#define PRESSURE_STANDBY            0x00U
#define PRESSURE_SENSOR_READY_MASK  0xC0U
#define PRESSURE_DATA_READY_MASK    0x30U
#define PRESSURE_SCALE_64           1040384.0f

typedef struct
{
    int32_t c0;
    int32_t c1;
    int32_t c00;
    int32_t c10;
    int32_t c01;
    int32_t c11;
    int32_t c20;
    int32_t c21;
    int32_t c30;
    int32_t c31;
    int32_t c40;
} pressure_calibration_t;

static pressure_status_t g_pressure_status;
static pressure_calibration_t g_pressure_calibration;

static int32_t pressure_sign_extend(uint32_t value, uint8_t bits)
{
    uint32_t sign_bit = 1UL << (bits - 1U);
    uint32_t mask = (1UL << bits) - 1UL;
    uint32_t raw = value & mask;

    if ((raw & sign_bit) != 0U)
    {
        return (int32_t)raw - (int32_t)(1UL << bits);
    }
    return (int32_t)raw;
}

static int pressure_map_board_result(board_i2c_result_t result)
{
    switch (result)
    {
        case BOARD_I2C_NO_ACK:
            return PRESSURE_RESULT_ABSENT;
        case BOARD_I2C_NOT_READY:
            return PRESSURE_RESULT_NOT_READY;
        case BOARD_I2C_TIMEOUT:
            return PRESSURE_RESULT_TIMEOUT;
        case BOARD_I2C_ERROR:
            return PRESSURE_RESULT_ERROR;
        case BOARD_I2C_OK:
            return PRESSURE_RESULT_OK;
        case BOARD_I2C_INVALID_ARG:
        default:
            return PRESSURE_RESULT_INVALID_ARG;
    }
}

static int pressure_read_register(uint8_t reg, uint8_t *data,
                                  uint16_t length, uint32_t timeout_ms)
{
    board_i2c_result_t bus_result;

    bus_result = board_i2c_read_reg(PRESSURE_ADDRESS_7BIT, reg, 1U,
                                    data, length, timeout_ms);
    return pressure_map_board_result(bus_result);
}

static int pressure_write_register(uint8_t reg, uint8_t value,
                                   uint32_t timeout_ms)
{
    board_i2c_result_t bus_result;

    bus_result = board_i2c_write_reg(PRESSURE_ADDRESS_7BIT, reg, 1U,
                                     &value, 1U, timeout_ms);
    return pressure_map_board_result(bus_result);
}

static int pressure_identify(pressure_status_t *snapshot,
                             uint32_t timeout_ms)
{
    int result;

    result = pressure_read_register(PRESSURE_ID_REG, &snapshot->chip_id,
                                    1U, timeout_ms);
    if (result != PRESSURE_RESULT_OK)
    {
        return result;
    }

    snapshot->present = 1U;
    if (snapshot->chip_id != PRESSURE_EXPECTED_ID)
    {
        return PRESSURE_RESULT_ID_MISMATCH;
    }

    result = pressure_read_register(PRESSURE_MEAS_CFG_REG,
                                    &snapshot->meas_cfg, 1U, timeout_ms);
    if (result != PRESSURE_RESULT_OK)
    {
        return result;
    }
    if ((snapshot->meas_cfg & PRESSURE_SENSOR_READY_MASK) !=
        PRESSURE_SENSOR_READY_MASK)
    {
        return PRESSURE_RESULT_NOT_READY;
    }

    snapshot->ready = 1U;
    return PRESSURE_RESULT_OK;
}

static int pressure_calibration_is_invalid(const uint8_t *data)
{
    uint32_t index;
    uint8_t any_nonzero = 0U;
    uint8_t any_not_ff = 0U;

    for (index = 0U; index < PRESSURE_COEF_LENGTH; index++)
    {
        any_nonzero |= data[index];
        any_not_ff |= (uint8_t)(data[index] ^ 0xFFU);
    }
    return ((any_nonzero == 0U) || (any_not_ff == 0U)) ? 1 : 0;
}

static void pressure_decode_calibration(const uint8_t *data,
                                        pressure_calibration_t *calib)
{
    calib->c0 = pressure_sign_extend(((uint32_t)data[0] << 4) |
                                     ((uint32_t)data[1] >> 4), 12U);
    calib->c1 = pressure_sign_extend(((uint32_t)(data[1] & 0x0FU) << 8) |
                                     data[2], 12U);
    calib->c00 = pressure_sign_extend(((uint32_t)data[3] << 12) |
                                      ((uint32_t)data[4] << 4) |
                                      ((uint32_t)data[5] >> 4), 20U);
    calib->c10 = pressure_sign_extend(((uint32_t)(data[5] & 0x0FU) << 16) |
                                      ((uint32_t)data[6] << 8) |
                                      data[7], 20U);
    calib->c01 = pressure_sign_extend(((uint32_t)data[8] << 8) |
                                      data[9], 16U);
    calib->c11 = pressure_sign_extend(((uint32_t)data[10] << 8) |
                                      data[11], 16U);
    calib->c20 = pressure_sign_extend(((uint32_t)data[12] << 8) |
                                      data[13], 16U);
    calib->c21 = pressure_sign_extend(((uint32_t)data[14] << 8) |
                                      data[15], 16U);
    calib->c30 = pressure_sign_extend(((uint32_t)data[16] << 8) |
                                      data[17], 16U);
    calib->c31 = pressure_sign_extend(((uint32_t)data[18] << 4) |
                                      ((uint32_t)data[19] >> 4), 12U);
    calib->c40 = pressure_sign_extend(((uint32_t)(data[19] & 0x0FU) << 8) |
                                      data[20], 12U);
}

static int pressure_load_calibration(uint32_t timeout_ms)
{
    uint8_t data[PRESSURE_COEF_LENGTH];
    int result;

    result = pressure_read_register(PRESSURE_COEF_REG, data,
                                    PRESSURE_COEF_LENGTH, timeout_ms);
    if (result != PRESSURE_RESULT_OK)
    {
        return result;
    }
    if (pressure_calibration_is_invalid(data) != 0)
    {
        return PRESSURE_RESULT_CALIB_INVALID;
    }
    pressure_decode_calibration(data, &g_pressure_calibration);
    return PRESSURE_RESULT_OK;
}

static int pressure_configure(uint32_t timeout_ms)
{
    uint8_t cfg;
    int result;

    result = pressure_write_register(PRESSURE_PRS_CFG_REG,
                                     PRESSURE_RATE_4_OSR_64, timeout_ms);
    if (result != PRESSURE_RESULT_OK)
    {
        return result;
    }
    result = pressure_write_register(PRESSURE_TMP_CFG_REG,
                                     PRESSURE_RATE_4_OSR_64, timeout_ms);
    if (result != PRESSURE_RESULT_OK)
    {
        return result;
    }
    result = pressure_read_register(PRESSURE_CFG_REG, &cfg, 1U, timeout_ms);
    if (result != PRESSURE_RESULT_OK)
    {
        return result;
    }
    cfg |= PRESSURE_SHIFT_MASK;
    return pressure_write_register(PRESSURE_CFG_REG, cfg, timeout_ms);
}

static int pressure_wait_data_ready(pressure_status_t *snapshot,
                                    uint32_t start_ms,
                                    uint32_t timeout_ms)
{
    int result;

    for (;;)
    {
        result = pressure_read_register(PRESSURE_MEAS_CFG_REG,
                                        &snapshot->meas_cfg, 1U, timeout_ms);
        if (result != PRESSURE_RESULT_OK)
        {
            return result;
        }
        if ((snapshot->meas_cfg & PRESSURE_DATA_READY_MASK) ==
            PRESSURE_DATA_READY_MASK)
        {
            return PRESSURE_RESULT_OK;
        }
        if ((pressure_now_ms() - start_ms) >= timeout_ms)
        {
            return PRESSURE_RESULT_TIMEOUT;
        }
        pressure_delay_ms(1U);
    }
}

static int32_t pressure_decode_raw24(const uint8_t *data)
{
    return pressure_sign_extend(((uint32_t)data[0] << 16) |
                                ((uint32_t)data[1] << 8) |
                                data[2], 24U);
}

static int pressure_compensate(pressure_status_t *snapshot)
{
    float tsc;
    float psc;
    float p2;
    float p3;
    float p4;
    float temperature;
    float pressure;
    float temperature_centi;

    tsc = (float)snapshot->raw_temperature / PRESSURE_SCALE_64;
    psc = (float)snapshot->raw_pressure / PRESSURE_SCALE_64;
    p2 = psc * psc;
    p3 = p2 * psc;
    p4 = p3 * psc;
    temperature = ((float)g_pressure_calibration.c0 * 0.5f) +
                  ((float)g_pressure_calibration.c1 * tsc);
    pressure = (float)g_pressure_calibration.c00 +
               ((float)g_pressure_calibration.c10 * psc) +
               ((float)g_pressure_calibration.c20 * p2) +
               ((float)g_pressure_calibration.c30 * p3) +
               ((float)g_pressure_calibration.c40 * p4) +
               tsc * ((float)g_pressure_calibration.c01 +
                      ((float)g_pressure_calibration.c11 * psc) +
                      ((float)g_pressure_calibration.c21 * p2) +
                      ((float)g_pressure_calibration.c31 * p3));
    temperature_centi = temperature * 100.0f;

    if ((temperature != temperature) || (pressure != pressure) ||
        (temperature_centi < -4000.0f) || (temperature_centi > 8500.0f) ||
        (pressure < 10000.0f) || (pressure > 130000.0f))
    {
        return PRESSURE_RESULT_RANGE_ERROR;
    }

    snapshot->temperature_centi_c = (int32_t)(temperature_centi +
        ((temperature_centi >= 0.0f) ? 0.5f : -0.5f));
    snapshot->pressure_pa = (int32_t)(pressure + 0.5f);
    return PRESSURE_RESULT_OK;
}

void pressure_service_init(void)
{
    memset(&g_pressure_status, 0, sizeof(g_pressure_status));
    memset(&g_pressure_calibration, 0, sizeof(g_pressure_calibration));
    g_pressure_status.result = PRESSURE_RESULT_NOT_READY;
}

int pressure_probe_readonly(uint32_t timeout_ms)
{
    pressure_status_t snapshot;
    uint32_t start_ms;
    int result;

    if (timeout_ms == 0U)
    {
        return PRESSURE_RESULT_INVALID_ARG;
    }

    snapshot = g_pressure_status;
    snapshot.present = 0U;
    snapshot.ready = 0U;
    snapshot.chip_id = 0U;
    snapshot.meas_cfg = 0U;
    start_ms = pressure_now_ms();
    result = pressure_identify(&snapshot, timeout_ms);
    snapshot.result = result;
    snapshot.elapsed_ms = pressure_now_ms() - start_ms;
    if (snapshot.probe_count != UINT32_MAX)
    {
        snapshot.probe_count++;
    }
    g_pressure_status = snapshot;
    return result;
}

int pressure_read_once(uint32_t timeout_ms)
{
    pressure_status_t snapshot;
    uint8_t data[PRESSURE_DATA_LENGTH];
    uint32_t start_ms;
    int result;
    int standby_result;
    uint8_t measurement_started = 0U;

    if (timeout_ms == 0U)
    {
        return PRESSURE_RESULT_INVALID_ARG;
    }

    snapshot = g_pressure_status;
    snapshot.present = 0U;
    snapshot.ready = 0U;
    snapshot.configured = 0U;
    start_ms = pressure_now_ms();

    result = pressure_identify(&snapshot, timeout_ms);
    if (result == PRESSURE_RESULT_OK)
    {
        result = pressure_load_calibration(timeout_ms);
    }
    if (result == PRESSURE_RESULT_OK)
    {
        result = pressure_configure(timeout_ms);
    }
    if (result == PRESSURE_RESULT_OK)
    {
        snapshot.configured = 1U;
        result = pressure_write_register(PRESSURE_MEAS_CFG_REG,
                                         PRESSURE_BACKGROUND_BOTH,
                                         timeout_ms);
        if (result == PRESSURE_RESULT_OK)
        {
            measurement_started = 1U;
        }
    }
    if (result == PRESSURE_RESULT_OK)
    {
        result = pressure_wait_data_ready(&snapshot, start_ms, timeout_ms);
    }
    if (result == PRESSURE_RESULT_OK)
    {
        result = pressure_read_register(PRESSURE_DATA_REG, data,
                                        PRESSURE_DATA_LENGTH, timeout_ms);
    }
    if (result == PRESSURE_RESULT_OK)
    {
        snapshot.raw_pressure = pressure_decode_raw24(&data[0]);
        snapshot.raw_temperature = pressure_decode_raw24(&data[3]);
        result = pressure_compensate(&snapshot);
    }

    if (measurement_started != 0U)
    {
        standby_result = pressure_write_register(PRESSURE_MEAS_CFG_REG,
                                                 PRESSURE_STANDBY,
                                                 timeout_ms);
        if ((result == PRESSURE_RESULT_OK) &&
            (standby_result != PRESSURE_RESULT_OK))
        {
            result = standby_result;
        }
    }

    snapshot.result = result;
    snapshot.elapsed_ms = pressure_now_ms() - start_ms;
    if (snapshot.read_count != UINT32_MAX)
    {
        snapshot.read_count++;
    }
    g_pressure_status = snapshot;
    return result;
}

void pressure_get_status(pressure_status_t *status)
{
    if (status != NULL)
    {
        *status = g_pressure_status;
    }
}

void pressure_print_status(void)
{
    printf("pressure status present=%u ready=%u configured=%u id=0x%02X meas=0x%02X raw_pressure=%ld raw_temperature=%ld temperature_centi_c=%ld pressure_pa=%ld result=%ld elapsed_ms=%lu probe_count=%lu read_count=%lu\r\n",
           (unsigned int)g_pressure_status.present,
           (unsigned int)g_pressure_status.ready,
           (unsigned int)g_pressure_status.configured,
           (unsigned int)g_pressure_status.chip_id,
           (unsigned int)g_pressure_status.meas_cfg,
           (long)g_pressure_status.raw_pressure,
           (long)g_pressure_status.raw_temperature,
           (long)g_pressure_status.temperature_centi_c,
           (long)g_pressure_status.pressure_pa,
           (long)g_pressure_status.result,
           (unsigned long)g_pressure_status.elapsed_ms,
           (unsigned long)g_pressure_status.probe_count,
           (unsigned long)g_pressure_status.read_count);
}
