#ifndef IMU_SERVICE_H
#define IMU_SERVICE_H

#include <stdint.h>

#include "board_i2c.h"
#include "sh3001_service.h"

typedef enum
{
    IMU_RESULT_OK = 0,
    IMU_RESULT_ABSENT,
    IMU_RESULT_NOT_READY,
    IMU_RESULT_TIMEOUT,
    IMU_RESULT_ID_MISMATCH,
    IMU_RESULT_ERROR,
    IMU_RESULT_INVALID_ARG
} imu_result_t;

typedef enum
{
    IMU_MODEL_NONE = 0,
    IMU_MODEL_SH3001,
    IMU_MODEL_QMI8658A
} imu_model_t;

typedef struct
{
    uint8_t present;
    uint8_t ready;
    uint8_t model;
    uint8_t id0;
    uint8_t id1;
    int32_t result;
    uint32_t elapsed_ms;
    uint32_t probe_count;
} imu_status_t;

void imu_service_init(void);
int imu_probe_readonly(uint32_t timeout_ms);
void imu_get_status(imu_status_t *status);
void imu_print_status(void);

#endif
