#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <stdint.h>

typedef enum
{
    RESOURCE_SDMMC1_DCMI_PINS = 0,
    RESOURCE_TIM1_PA8,
    RESOURCE_TIM2_PA5,
    RESOURCE_PB12,
    RESOURCE_DMA1_STREAM1,
    RESOURCE_DMA1_STREAM2,
    RESOURCE_SAI1,
    RESOURCE_SPDIFRX1,
    RESOURCE_COUNT
} resource_id_t;

typedef uint16_t resource_owner_t;
typedef int (*resource_restore_fn_t)(void *context);

#define RESOURCE_OWNER_NONE 0U

typedef enum
{
    RESOURCE_OK = 0,
    RESOURCE_INVALID_ARG,
    RESOURCE_BUSY,
    RESOURCE_NOT_OWNER,
    RESOURCE_RESTORE_FAILED
} resource_result_t;

int resource_manager_init(void);
int resource_configure(resource_id_t id,
                       resource_owner_t default_owner,
                       resource_restore_fn_t restore,
                       void *context);
int resource_acquire(resource_id_t id, resource_owner_t owner);
int resource_release(resource_id_t id, resource_owner_t owner);
int resource_restore_default(resource_id_t id, resource_owner_t owner);
resource_owner_t resource_get_owner(resource_id_t id);
void resource_print_status(void);
int resource_run_selftest(void);

#endif
