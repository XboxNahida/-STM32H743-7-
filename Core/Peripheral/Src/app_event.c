#include "app_event.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#if defined(APP_EVENT_HOST_TEST)
static uint32_t app_event_enter_critical(void)
{
    return 0U;
}

static void app_event_exit_critical(uint32_t state)
{
    (void)state;
}

static uint32_t app_event_now(void)
{
    return 0U;
}
#else
#include "main.h"

static uint32_t app_event_enter_critical(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void app_event_exit_critical(uint32_t state)
{
    if (state == 0U)
    {
        __enable_irq();
    }
}

static uint32_t app_event_now(void)
{
    return HAL_GetTick();
}
#endif

static app_event_t g_app_event_queue[APP_EVENT_QUEUE_CAPACITY];
static volatile uint32_t g_app_event_read_index;
static volatile uint32_t g_app_event_write_index;
static volatile uint32_t g_app_event_drop_count;

void app_event_init(void)
{
    uint32_t critical_state = app_event_enter_critical();

    g_app_event_read_index = 0U;
    g_app_event_write_index = 0U;
    g_app_event_drop_count = 0U;

    app_event_exit_critical(critical_state);
}

static int app_event_publish_internal(uint16_t source,
                                      uint16_t type,
                                      uint32_t value)
{
    uint32_t critical_state;
    uint32_t write_index;
    uint32_t next_index;

    critical_state = app_event_enter_critical();
    write_index = g_app_event_write_index;
    next_index = (write_index + 1U) % APP_EVENT_QUEUE_CAPACITY;

    if (next_index == g_app_event_read_index)
    {
        if (g_app_event_drop_count < UINT32_MAX)
        {
            g_app_event_drop_count++;
        }
        app_event_exit_critical(critical_state);
        return APP_EVENT_FULL;
    }

    g_app_event_queue[write_index].source = source;
    g_app_event_queue[write_index].type = type;
    g_app_event_queue[write_index].value = value;
    g_app_event_queue[write_index].tick = app_event_now();
    g_app_event_write_index = next_index;

    app_event_exit_critical(critical_state);
    return APP_EVENT_OK;
}

int app_event_publish_from_isr(uint16_t source, uint16_t type, uint32_t value)
{
    return app_event_publish_internal(source, type, value);
}

int app_event_publish(uint16_t source, uint16_t type, uint32_t value)
{
    return app_event_publish_internal(source, type, value);
}

int app_event_take(app_event_t *event)
{
    uint32_t critical_state;
    uint32_t read_index;

    if (event == NULL)
    {
        return APP_EVENT_INVALID_ARG;
    }

    critical_state = app_event_enter_critical();
    read_index = g_app_event_read_index;

    if (read_index == g_app_event_write_index)
    {
        app_event_exit_critical(critical_state);
        return APP_EVENT_EMPTY;
    }

    *event = g_app_event_queue[read_index];
    g_app_event_read_index = (read_index + 1U) % APP_EVENT_QUEUE_CAPACITY;

    app_event_exit_critical(critical_state);
    return APP_EVENT_OK;
}

uint32_t app_event_get_drop_count(void)
{
    uint32_t critical_state;
    uint32_t drop_count;

    critical_state = app_event_enter_critical();
    drop_count = g_app_event_drop_count;
    app_event_exit_critical(critical_state);
    return drop_count;
}

int app_event_run_selftest(void)
{
    app_event_t backup_queue[APP_EVENT_QUEUE_CAPACITY];
    app_event_t event;
    uint32_t backup_read_index;
    uint32_t backup_write_index;
    uint32_t backup_drop_count;
    uint32_t critical_state;
    uint32_t index;
    int failures = 0;

    critical_state = app_event_enter_critical();
    memcpy(backup_queue, g_app_event_queue, sizeof(backup_queue));
    backup_read_index = g_app_event_read_index;
    backup_write_index = g_app_event_write_index;
    backup_drop_count = g_app_event_drop_count;
    g_app_event_read_index = 0U;
    g_app_event_write_index = 0U;
    g_app_event_drop_count = 0U;
    app_event_exit_critical(critical_state);

    for (index = 0U; index < (APP_EVENT_QUEUE_CAPACITY - 1U); index++)
    {
        if (app_event_publish(1U,
                              (uint16_t)index,
                              index + 100U) != APP_EVENT_OK)
        {
            failures++;
            break;
        }
    }

    if (app_event_publish(2U, 0U, 0U) != APP_EVENT_FULL)
    {
        failures++;
    }

    if (app_event_get_drop_count() != 1U)
    {
        failures++;
    }

    for (index = 0U; index < (APP_EVENT_QUEUE_CAPACITY - 1U); index++)
    {
        if (app_event_take(&event) != APP_EVENT_OK)
        {
            failures++;
            break;
        }

        if ((event.source != 1U) ||
            (event.type != (uint16_t)index) ||
            (event.value != (index + 100U)))
        {
            failures++;
        }
    }

    if (app_event_take(&event) != APP_EVENT_EMPTY)
    {
        failures++;
    }

    critical_state = app_event_enter_critical();
    memcpy(g_app_event_queue, backup_queue, sizeof(g_app_event_queue));
    g_app_event_read_index = backup_read_index;
    g_app_event_write_index = backup_write_index;
    g_app_event_drop_count = backup_drop_count;
    app_event_exit_critical(critical_state);

    if (failures == 0)
    {
        printf("OK test=app_event failures=0\r\n");
        return 0;
    }

    printf("ERROR test=app_event failures=%d\r\n", failures);
    return failures;
}
