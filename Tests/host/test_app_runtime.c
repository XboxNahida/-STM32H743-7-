#include <stdint.h>
#include <stdio.h>

#include "app_runtime.h"

typedef struct
{
    int start_result;
    int stop_result;
    int recover_result;
    uint32_t start_calls;
    uint32_t poll_calls;
    uint32_t stop_calls;
    uint32_t recover_calls;
} app_probe_t;

static int probe_start(void *context)
{
    app_probe_t *probe = (app_probe_t *)context;
    probe->start_calls++;
    return probe->start_result;
}

static void probe_poll(void *context)
{
    app_probe_t *probe = (app_probe_t *)context;
    probe->poll_calls++;
}

static int probe_stop(void *context)
{
    app_probe_t *probe = (app_probe_t *)context;
    probe->stop_calls++;
    return probe->stop_result;
}

static int probe_recover(void *context)
{
    app_probe_t *probe = (app_probe_t *)context;
    probe->recover_calls++;
    return probe->recover_result;
}

int main(void)
{
    static const app_ops_t probe_ops = {
        probe_start,
        probe_poll,
        probe_stop,
        probe_recover,
        NULL
    };
    app_probe_t first = {0};
    app_probe_t second = {-1, 0, 0, 0U, 0U, 0U, 0U};
    int failures = 0;

    app_runtime_init();

    if (app_runtime_register(1U, &probe_ops, &first) != APP_RUNTIME_OK)
    {
        printf("FAIL register_first\n");
        failures++;
    }
    if (app_runtime_register(1U, &probe_ops, &first) != APP_RUNTIME_DUPLICATE)
    {
        printf("FAIL duplicate_not_rejected\n");
        failures++;
    }
    if (app_runtime_register(2U, &probe_ops, &second) != APP_RUNTIME_OK)
    {
        printf("FAIL register_second\n");
        failures++;
    }

    if (app_runtime_request_start(1U) != APP_RUNTIME_OK)
    {
        printf("FAIL request_first_start\n");
        failures++;
    }
    if ((app_runtime_get_state(1U) != APP_STATE_STARTING) ||
        (first.start_calls != 0U))
    {
        printf("FAIL start_not_deferred\n");
        failures++;
    }
    if (app_runtime_request_start(2U) != APP_RUNTIME_BUSY)
    {
        printf("FAIL second_start_not_busy\n");
        failures++;
    }

    app_runtime_poll();
    if ((app_runtime_get_state(1U) != APP_STATE_RUNNING) ||
        (first.start_calls != 1U))
    {
        printf("FAIL first_start_transition\n");
        failures++;
    }
    app_runtime_poll();
    if (first.poll_calls != 1U)
    {
        printf("FAIL running_poll\n");
        failures++;
    }

    if ((app_runtime_request_stop(1U) != APP_RUNTIME_OK) ||
        (app_runtime_request_stop(1U) != APP_RUNTIME_OK) ||
        (app_runtime_get_state(1U) != APP_STATE_STOPPING))
    {
        printf("FAIL stop_not_idempotent\n");
        failures++;
    }
    app_runtime_poll();
    if ((app_runtime_get_state(1U) != APP_STATE_IDLE) ||
        (first.stop_calls != 1U))
    {
        printf("FAIL first_stop_transition\n");
        failures++;
    }
    if (app_runtime_request_stop(1U) != APP_RUNTIME_OK)
    {
        printf("FAIL idle_stop_not_idempotent\n");
        failures++;
    }

    if (app_runtime_request_start(2U) != APP_RUNTIME_OK)
    {
        printf("FAIL request_second_start\n");
        failures++;
    }
    app_runtime_poll();
    if ((app_runtime_get_state(2U) != APP_STATE_ERROR) ||
        (second.start_calls != 1U))
    {
        printf("FAIL start_failure_state\n");
        failures++;
    }
    app_runtime_poll();
    if ((app_runtime_get_state(2U) != APP_STATE_IDLE) ||
        (second.recover_calls != 1U))
    {
        printf("FAIL recover_transition\n");
        failures++;
    }

    if ((app_runtime_request_start(99U) != APP_RUNTIME_NOT_FOUND) ||
        (app_runtime_request_stop(99U) != APP_RUNTIME_NOT_FOUND))
    {
        printf("FAIL unknown_app_result\n");
        failures++;
    }

    if (failures != 0)
    {
        printf("FAIL app_runtime failures=%d\n", failures);
        return 1;
    }

    printf("PASS app_runtime\n");
    return 0;
}
