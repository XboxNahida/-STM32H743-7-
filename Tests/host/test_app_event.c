#include <stdint.h>
#include <stdio.h>

#include "app_event.h"

int main(void)
{
    app_event_t event;
    uint32_t index;
    int failures = 0;

    app_event_init();

    for (index = 0U; index < (APP_EVENT_QUEUE_CAPACITY - 1U); index++)
    {
        if (app_event_publish(1U,
                              (uint16_t)(100U + index),
                              1000U + index) != APP_EVENT_OK)
        {
            printf("FAIL publish index=%lu\n", (unsigned long)index);
            failures++;
            break;
        }
    }

    if (app_event_publish(2U, 999U, 9999U) != APP_EVENT_FULL)
    {
        printf("FAIL full_not_reported\n");
        failures++;
    }

    if (app_event_get_drop_count() != 1U)
    {
        printf("FAIL drop_count actual=%lu\n",
               (unsigned long)app_event_get_drop_count());
        failures++;
    }

    for (index = 0U; index < (APP_EVENT_QUEUE_CAPACITY - 1U); index++)
    {
        if (app_event_take(&event) != APP_EVENT_OK)
        {
            printf("FAIL take index=%lu\n", (unsigned long)index);
            failures++;
            break;
        }

        if ((event.source != 1U) ||
            (event.type != (uint16_t)(100U + index)) ||
            (event.value != (1000U + index)))
        {
            printf("FAIL order index=%lu source=%u type=%u value=%lu\n",
                   (unsigned long)index,
                   (unsigned int)event.source,
                   (unsigned int)event.type,
                   (unsigned long)event.value);
            failures++;
        }
    }

    if (app_event_take(&event) != APP_EVENT_EMPTY)
    {
        printf("FAIL empty_not_reported\n");
        failures++;
    }

    if (app_event_take(NULL) != APP_EVENT_INVALID_ARG)
    {
        printf("FAIL null_event_not_rejected\n");
        failures++;
    }

    app_event_init();
    if (app_event_get_drop_count() != 0U)
    {
        printf("FAIL init_did_not_clear_drop_count\n");
        failures++;
    }

    if (app_event_publish_from_isr(3U, 4U, 5U) != APP_EVENT_OK)
    {
        printf("FAIL publish_from_isr\n");
        failures++;
    }

    if ((app_event_take(&event) != APP_EVENT_OK) ||
        (event.source != 3U) ||
        (event.type != 4U) ||
        (event.value != 5U))
    {
        printf("FAIL publish_from_isr_data\n");
        failures++;
    }

    if (app_event_run_selftest() != 0)
    {
        printf("FAIL app_event_selftest\n");
        failures++;
    }

    if (failures != 0)
    {
        printf("FAIL app_event failures=%d\n", failures);
        return 1;
    }

    printf("PASS app_event\n");
    return 0;
}
