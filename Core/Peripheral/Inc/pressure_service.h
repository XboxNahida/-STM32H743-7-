#ifndef PRESSURE_SERVICE_H
#define PRESSURE_SERVICE_H

#include <stdint.h>

#include "board_i2c.h"

typedef enum
{
    PRESSURE_RESULT_OK = 0,
    PRESSURE_RESULT_ABSENT,
    PRESSURE_RESULT_NOT_READY,
    PRESSURE_RESULT_TIMEOUT,
    PRESSURE_RESULT_ID_MISMATCH,
    PRESSURE_RESULT_CALIB_INVALID,
    PRESSURE_RESULT_RANGE_ERROR,
    PRESSURE_RESULT_ERROR,
    PRESSURE_RESULT_INVALID_ARG
} pressure_result_t;

typedef struct
{
    uint8_t present;
    uint8_t ready;
    uint8_t configured;
    uint8_t chip_id;
    uint8_t meas_cfg;
    int32_t raw_pressure;
    int32_t raw_temperature;
    int32_t temperature_centi_c;
    int32_t pressure_pa;
    int32_t result;
    uint32_t elapsed_ms;
    uint32_t probe_count;
    uint32_t read_count;
} pressure_status_t;

void pressure_service_init(void);
int pressure_probe_readonly(uint32_t timeout_ms);
int pressure_read_once(uint32_t timeout_ms);
void pressure_get_status(pressure_status_t *status);
void pressure_print_status(void);

#endif
