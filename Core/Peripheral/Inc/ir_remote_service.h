#ifndef IR_REMOTE_SERVICE_H
#define IR_REMOTE_SERVICE_H

#include <stdint.h>

typedef enum
{
    IR_REMOTE_OK = 0,
    IR_REMOTE_NOT_READY,
    IR_REMOTE_INVALID_ARG
} ir_remote_result_t;

typedef struct
{
    uint8_t initialized;
    uint8_t suspended;
    uint8_t frame_valid;
    uint8_t address;
    uint8_t command;
    uint32_t frame_count;
    uint32_t repeat_count;
    uint32_t invalid_count;
    uint32_t overflow_count;
} ir_remote_status_t;

void ir_remote_service_init(void);
void ir_remote_capture_pulse(uint32_t width_us);
void ir_remote_timeout(void);
void ir_remote_get_status(ir_remote_status_t *status);
void ir_remote_print_status(void);
void ir_remote_clear(void);
int ir_remote_suspend(void);
int ir_remote_resume(void);

#endif
