#ifndef APP_EVENT_H
#define APP_EVENT_H

#include <stdint.h>

#define APP_EVENT_QUEUE_CAPACITY 32U

typedef struct
{
    uint16_t source;
    uint16_t type;
    uint32_t value;
    uint32_t tick;
} app_event_t;

typedef enum
{
    APP_EVENT_OK = 0,
    APP_EVENT_INVALID_ARG,
    APP_EVENT_FULL,
    APP_EVENT_EMPTY
} app_event_result_t;

void app_event_init(void);
int app_event_publish_from_isr(uint16_t source, uint16_t type, uint32_t value);
int app_event_publish(uint16_t source, uint16_t type, uint32_t value);
int app_event_take(app_event_t *event);
uint32_t app_event_get_drop_count(void);
int app_event_run_selftest(void);

#endif
