#ifndef TPAD_SERVICE_H
#define TPAD_SERVICE_H

#include <stdint.h>

typedef enum
{
    TPAD_OK = 0,
    TPAD_NOT_READY,
    TPAD_TIMEOUT,
    TPAD_CAPTURE_ERROR,
    TPAD_RESTORE_ERROR,
    TPAD_NOT_CALIBRATED,
    TPAD_INVALID_ARG
} tpad_result_t;

typedef struct
{
    uint8_t initialized;
    uint8_t calibrated;
    uint8_t touched;
    uint8_t reserved;
    uint32_t baseline;
    uint32_t last_raw;
    int32_t result;
    uint32_t elapsed_ms;
    uint32_t calibrate_count;
    uint32_t read_count;
} tpad_status_t;

void tpad_service_init(void);
int tpad_calibrate(uint32_t timeout_ms);
int tpad_read(uint32_t timeout_ms);
void tpad_get_status(tpad_status_t *status);
void tpad_print_status(void);

#endif
