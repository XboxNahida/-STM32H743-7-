#include "app_runtime.h"

#include <stdio.h>
#include <string.h>

typedef struct
{
    uint16_t app_id;
    const app_ops_t *ops;
    void *context;
    app_state_t state;
    uint8_t registered;
} app_runtime_entry_t;

static app_runtime_entry_t g_apps[APP_RUNTIME_MAX_APPS];
static int16_t g_active_index = -1;

static int app_runtime_find(uint16_t app_id)
{
    uint32_t index;

    for (index = 0U; index < APP_RUNTIME_MAX_APPS; index++)
    {
        if ((g_apps[index].registered != 0U) &&
            (g_apps[index].app_id == app_id))
        {
            return (int)index;
        }
    }
    return -1;
}

static const char *app_runtime_state_name(app_state_t state)
{
    switch (state)
    {
        case APP_STATE_IDLE:     return "idle";
        case APP_STATE_STARTING: return "starting";
        case APP_STATE_RUNNING:  return "running";
        case APP_STATE_STOPPING: return "stopping";
        case APP_STATE_ERROR:    return "error";
        default:                 return "invalid";
    }
}

static void app_runtime_set_idle(int16_t index)
{
    g_apps[index].state = APP_STATE_IDLE;
    g_active_index = -1;
}

void app_runtime_init(void)
{
    memset(g_apps, 0, sizeof(g_apps));
    g_active_index = -1;
}

int app_runtime_register(uint16_t app_id,
                         const app_ops_t *ops,
                         void *context)
{
    uint32_t index;

    if ((app_id == 0U) || (ops == NULL) ||
        (ops->start == NULL) || (ops->poll == NULL) ||
        (ops->stop == NULL))
    {
        return APP_RUNTIME_INVALID_ARG;
    }
    if (app_runtime_find(app_id) >= 0)
    {
        return APP_RUNTIME_DUPLICATE;
    }

    for (index = 0U; index < APP_RUNTIME_MAX_APPS; index++)
    {
        if (g_apps[index].registered == 0U)
        {
            g_apps[index].app_id = app_id;
            g_apps[index].ops = ops;
            g_apps[index].context = context;
            g_apps[index].state = APP_STATE_IDLE;
            g_apps[index].registered = 1U;
            return APP_RUNTIME_OK;
        }
    }
    return APP_RUNTIME_FULL;
}

int app_runtime_request_start(uint16_t app_id)
{
    int index = app_runtime_find(app_id);

    if (index < 0)
    {
        return APP_RUNTIME_NOT_FOUND;
    }
    if ((g_active_index >= 0) ||
        (g_apps[index].state != APP_STATE_IDLE))
    {
        return APP_RUNTIME_BUSY;
    }

    g_apps[index].state = APP_STATE_STARTING;
    g_active_index = (int16_t)index;
    return APP_RUNTIME_OK;
}

int app_runtime_request_stop(uint16_t app_id)
{
    int index = app_runtime_find(app_id);

    if (index < 0)
    {
        return APP_RUNTIME_NOT_FOUND;
    }
    if (g_apps[index].state == APP_STATE_IDLE)
    {
        return APP_RUNTIME_OK;
    }
    if (g_apps[index].state == APP_STATE_STOPPING)
    {
        return APP_RUNTIME_OK;
    }
    if (g_active_index != (int16_t)index)
    {
        return APP_RUNTIME_BUSY;
    }

    g_apps[index].state = APP_STATE_STOPPING;
    return APP_RUNTIME_OK;
}

void app_runtime_poll(void)
{
    app_runtime_entry_t *entry;

    if (g_active_index < 0)
    {
        return;
    }
    entry = &g_apps[g_active_index];

    switch (entry->state)
    {
        case APP_STATE_STARTING:
            entry->state = (entry->ops->start(entry->context) == 0) ?
                           APP_STATE_RUNNING : APP_STATE_ERROR;
            break;

        case APP_STATE_RUNNING:
            entry->ops->poll(entry->context);
            break;

        case APP_STATE_STOPPING:
            if (entry->ops->stop(entry->context) == 0)
            {
                app_runtime_set_idle(g_active_index);
            }
            else
            {
                entry->state = APP_STATE_ERROR;
            }
            break;

        case APP_STATE_ERROR:
            if ((entry->ops->recover != NULL) &&
                (entry->ops->recover(entry->context) == 0))
            {
                app_runtime_set_idle(g_active_index);
            }
            break;

        case APP_STATE_IDLE:
            g_active_index = -1;
            break;

        default:
            entry->state = APP_STATE_ERROR;
            break;
    }
}

app_state_t app_runtime_get_state(uint16_t app_id)
{
    int index = app_runtime_find(app_id);

    return (index >= 0) ? g_apps[index].state : APP_STATE_ERROR;
}

void app_runtime_print_status(void)
{
    uint32_t index;

    printf("app active_id=%u registered_max=%u\r\n",
           (g_active_index >= 0) ?
           (unsigned int)g_apps[g_active_index].app_id : 0U,
           (unsigned int)APP_RUNTIME_MAX_APPS);

    for (index = 0U; index < APP_RUNTIME_MAX_APPS; index++)
    {
        if (g_apps[index].registered != 0U)
        {
            printf("app id=%u state=%s\r\n",
                   (unsigned int)g_apps[index].app_id,
                   app_runtime_state_name(g_apps[index].state));
            if (g_apps[index].ops->print_status != NULL)
            {
                g_apps[index].ops->print_status(g_apps[index].context);
            }
        }
    }
}
