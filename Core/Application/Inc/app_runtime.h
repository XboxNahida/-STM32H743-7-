#ifndef APP_RUNTIME_H
#define APP_RUNTIME_H

#include <stdint.h>

#define APP_RUNTIME_MAX_APPS 8U

typedef enum
{
    APP_STATE_IDLE = 0,
    APP_STATE_STARTING,
    APP_STATE_RUNNING,
    APP_STATE_STOPPING,
    APP_STATE_ERROR
} app_state_t;

typedef struct
{
    int (*start)(void *context);
    void (*poll)(void *context);
    int (*stop)(void *context);
    int (*recover)(void *context);
    void (*print_status)(const void *context);
} app_ops_t;

typedef enum
{
    APP_RUNTIME_OK = 0,
    APP_RUNTIME_INVALID_ARG,
    APP_RUNTIME_FULL,
    APP_RUNTIME_DUPLICATE,
    APP_RUNTIME_BUSY,
    APP_RUNTIME_NOT_FOUND
} app_runtime_result_t;

void app_runtime_init(void);
int app_runtime_register(uint16_t app_id,
                         const app_ops_t *ops,
                         void *context);
int app_runtime_request_start(uint16_t app_id);
int app_runtime_request_stop(uint16_t app_id);
void app_runtime_poll(void);
app_state_t app_runtime_get_state(uint16_t app_id);
void app_runtime_print_status(void);

#endif
