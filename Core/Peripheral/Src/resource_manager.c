#include "resource_manager.h"

#include <stdio.h>
#include <string.h>

typedef struct
{
    resource_owner_t owner;
    resource_owner_t default_owner;
    resource_restore_fn_t restore;
    void *context;
    uint8_t configured;
} resource_entry_t;

static resource_entry_t g_resources[RESOURCE_COUNT];

static int resource_id_is_valid(resource_id_t id)
{
    return ((uint32_t)id < (uint32_t)RESOURCE_COUNT) ? 1 : 0;
}

static const char *resource_name(resource_id_t id)
{
    switch (id)
    {
        case RESOURCE_SDMMC1_DCMI_PINS: return "sdmmc1_dcmi_pins";
        case RESOURCE_TIM1_PA8:         return "tim1_pa8";
        case RESOURCE_TIM2_PA5:         return "tim2_pa5";
        case RESOURCE_PB12:             return "pb12";
        case RESOURCE_DMA1_STREAM1:     return "dma1_stream1";
        case RESOURCE_DMA1_STREAM2:     return "dma1_stream2";
        case RESOURCE_SAI1:             return "sai1";
        case RESOURCE_SPDIFRX1:         return "spdifrx1";
        default:                        return "invalid";
    }
}

int resource_manager_init(void)
{
    memset(g_resources, 0, sizeof(g_resources));
    return RESOURCE_OK;
}

int resource_configure(resource_id_t id,
                       resource_owner_t default_owner,
                       resource_restore_fn_t restore,
                       void *context)
{
    resource_entry_t *entry;

    if (!resource_id_is_valid(id))
    {
        return RESOURCE_INVALID_ARG;
    }

    entry = &g_resources[id];
    if ((entry->owner != RESOURCE_OWNER_NONE) &&
        (entry->owner != entry->default_owner))
    {
        return RESOURCE_BUSY;
    }

    entry->owner = default_owner;
    entry->default_owner = default_owner;
    entry->restore = restore;
    entry->context = context;
    entry->configured = 1U;
    return RESOURCE_OK;
}

int resource_acquire(resource_id_t id, resource_owner_t owner)
{
    resource_entry_t *entry;

    if (!resource_id_is_valid(id) || (owner == RESOURCE_OWNER_NONE))
    {
        return RESOURCE_INVALID_ARG;
    }

    entry = &g_resources[id];
    if ((entry->owner != RESOURCE_OWNER_NONE) && (entry->owner != owner))
    {
        return RESOURCE_BUSY;
    }

    entry->owner = owner;
    return RESOURCE_OK;
}

int resource_release(resource_id_t id, resource_owner_t owner)
{
    resource_entry_t *entry;

    if (!resource_id_is_valid(id) || (owner == RESOURCE_OWNER_NONE))
    {
        return RESOURCE_INVALID_ARG;
    }

    entry = &g_resources[id];
    if (entry->owner != owner)
    {
        return RESOURCE_NOT_OWNER;
    }

    entry->owner = RESOURCE_OWNER_NONE;
    return RESOURCE_OK;
}

int resource_restore_default(resource_id_t id, resource_owner_t owner)
{
    resource_entry_t *entry;

    if (!resource_id_is_valid(id) || (owner == RESOURCE_OWNER_NONE))
    {
        return RESOURCE_INVALID_ARG;
    }

    entry = &g_resources[id];
    if (entry->owner != owner)
    {
        return RESOURCE_NOT_OWNER;
    }

    if ((entry->restore != NULL) && (entry->restore(entry->context) != 0))
    {
        return RESOURCE_RESTORE_FAILED;
    }

    entry->owner = entry->default_owner;
    return RESOURCE_OK;
}

resource_owner_t resource_get_owner(resource_id_t id)
{
    if (!resource_id_is_valid(id))
    {
        return RESOURCE_OWNER_NONE;
    }

    return g_resources[id].owner;
}

void resource_print_status(void)
{
    uint32_t index;

    for (index = 0U; index < (uint32_t)RESOURCE_COUNT; index++)
    {
        printf("resource name=%s configured=%u owner=%u default=%u\r\n",
               resource_name((resource_id_t)index),
               (unsigned int)g_resources[index].configured,
               (unsigned int)g_resources[index].owner,
               (unsigned int)g_resources[index].default_owner);
    }
}

static int resource_selftest_restore_ok(void *context)
{
    uint32_t *calls = (uint32_t *)context;
    (*calls)++;
    return 0;
}

static int resource_selftest_restore_fail(void *context)
{
    uint32_t *calls = (uint32_t *)context;
    (*calls)++;
    return -1;
}

int resource_run_selftest(void)
{
    resource_entry_t backup[RESOURCE_COUNT];
    uint32_t ok_calls = 0U;
    uint32_t fail_calls = 0U;
    const resource_owner_t default_owner = 100U;
    const resource_owner_t test_owner = 101U;
    int failures = 0;

    memcpy(backup, g_resources, sizeof(backup));
    (void)resource_manager_init();

    if (resource_configure(RESOURCE_SDMMC1_DCMI_PINS,
                           default_owner,
                           resource_selftest_restore_ok,
                           &ok_calls) != RESOURCE_OK)
    {
        failures++;
    }

    if (resource_release(RESOURCE_SDMMC1_DCMI_PINS,
                         default_owner) != RESOURCE_OK)
    {
        failures++;
    }

    if (resource_acquire(RESOURCE_SDMMC1_DCMI_PINS,
                         test_owner) != RESOURCE_OK)
    {
        failures++;
    }

    if (resource_acquire(RESOURCE_SDMMC1_DCMI_PINS,
                         default_owner) != RESOURCE_BUSY)
    {
        failures++;
    }

    if (resource_restore_default(RESOURCE_SDMMC1_DCMI_PINS,
                                 test_owner) != RESOURCE_OK)
    {
        failures++;
    }

    if ((ok_calls != 1U) ||
        (resource_get_owner(RESOURCE_SDMMC1_DCMI_PINS) != default_owner))
    {
        failures++;
    }

    if (resource_configure(RESOURCE_TIM1_PA8,
                           default_owner,
                           resource_selftest_restore_fail,
                           &fail_calls) != RESOURCE_OK)
    {
        failures++;
    }

    if (resource_release(RESOURCE_TIM1_PA8, default_owner) != RESOURCE_OK)
    {
        failures++;
    }

    if (resource_acquire(RESOURCE_TIM1_PA8, test_owner) != RESOURCE_OK)
    {
        failures++;
    }

    if (resource_restore_default(RESOURCE_TIM1_PA8,
                                 test_owner) != RESOURCE_RESTORE_FAILED)
    {
        failures++;
    }

    if ((fail_calls != 1U) ||
        (resource_get_owner(RESOURCE_TIM1_PA8) != test_owner))
    {
        failures++;
    }

    memcpy(g_resources, backup, sizeof(g_resources));

    if (failures == 0)
    {
        printf("OK test=resource_manager failures=0\r\n");
        return 0;
    }

    printf("ERROR test=resource_manager failures=%d\r\n", failures);
    return failures;
}
