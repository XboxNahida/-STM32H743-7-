#ifndef PPC_DIAGNOSTICS_H
#define PPC_DIAGNOSTICS_H

#include <stdint.h>

typedef enum
{
    LOG_ERROR = 0,
    LOG_WARN,
    LOG_INFO,
    LOG_TRACE
} log_level_t;

typedef struct
{
    uint8_t valid;
    uint16_t module;
    int32_t code;
    uint32_t tick;
    uint32_t detail0;
    uint32_t detail1;
} fault_snapshot_t;

void diagnostics_init(void);
void diagnostics_set_log_level(log_level_t level);
log_level_t diagnostics_get_log_level(void);
void diagnostics_capture_first(uint16_t module,
                               int32_t code,
                               uint32_t detail0,
                               uint32_t detail1);
const fault_snapshot_t *diagnostics_get_first_fault(void);
void diagnostics_clear_first_fault(void);
void diagnostics_print_fault(void);

#endif
