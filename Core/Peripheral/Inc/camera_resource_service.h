#ifndef CAMERA_RESOURCE_SERVICE_H
#define CAMERA_RESOURCE_SERVICE_H

#include <stdint.h>

typedef enum
{
    CAMERA_RESOURCE_OK = 0,
    CAMERA_RESOURCE_INVALID_ARG,
    CAMERA_RESOURCE_BUSY,
    CAMERA_RESOURCE_RESOURCE_ERROR,
    CAMERA_RESOURCE_SENSOR_ERROR
} camera_resource_result_t;

typedef enum
{
    CAMERA_RESOURCE_IDLE = 0,
    CAMERA_RESOURCE_SD_SUSPENDED,
    CAMERA_RESOURCE_PINS_ACQUIRED,
    CAMERA_RESOURCE_DMA_ACQUIRED,
    CAMERA_RESOURCE_GPIO_CONFIGURED,
    CAMERA_RESOURCE_SENSOR_POWERED,
    CAMERA_RESOURCE_ACTIVE
} camera_resource_stage_t;

typedef struct
{
    camera_resource_stage_t stage;
    uint8_t acquired;
    uint8_t release_errors;
    int last_result;
} camera_resource_status_t;

int camera_resource_service_init(void);
int camera_resource_acquire(void);
int camera_resource_release(void);
void camera_resource_get_status(camera_resource_status_t *status);

#endif
