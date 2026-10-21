#include "diagnostics.h"

#include <stdio.h>
#include <string.h>

#if defined(DIAGNOSTICS_HOST_TEST)
static uint32_t diagnostics_enter_critical(void)
{
    return 0U;
}

static void diagnostics_exit_critical(uint32_t state)
{
    (void)state;
}

static uint32_t diagnostics_now(void)
{
    return 0U;
}
#else
#include "main.h"

static uint32_t diagnostics_enter_critical(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void diagnostics_exit_critical(uint32_t state)
{
    if (state == 0U)
    {
        __enable_irq();
    }
}

static uint32_t diagnostics_now(void)
{
    return HAL_GetTick();
}
#endif

static volatile log_level_t g_log_level = LOG_INFO;
static volatile fault_snapshot_t g_first_fault;
static fault_snapshot_t g_fault_readback;

void diagnostics_init(void)
{
    uint32_t critical_state = diagnostics_enter_critical();

    g_log_level = LOG_INFO;
    memset((void *)&g_first_fault, 0, sizeof(g_first_fault));
    memset(&g_fault_readback, 0, sizeof(g_fault_readback));

    diagnostics_exit_critical(critical_state);
}

void diagnostics_set_log_level(log_level_t level)
{
    uint32_t critical_state;
    uint32_t level_value = (uint32_t)level;

    if (level_value > (uint32_t)LOG_TRACE)
    {
        return;
    }

    critical_state = diagnostics_enter_critical();
    g_log_level = level;
    diagnostics_exit_critical(critical_state);
}

log_level_t diagnostics_get_log_level(void)
{
    uint32_t critical_state;
    log_level_t level;

    critical_state = diagnostics_enter_critical();
    level = g_log_level;
    diagnostics_exit_critical(critical_state);
    return level;
}

void diagnostics_capture_first(uint16_t module,
                               int32_t code,
                               uint32_t detail0,
                               uint32_t detail1)
{
    uint32_t critical_state;

    critical_state = diagnostics_enter_critical();
    if (g_first_fault.valid == 0U)
    {
        g_first_fault.module = module;
        g_first_fault.code = code;
        g_first_fault.tick = diagnostics_now();
        g_first_fault.detail0 = detail0;
        g_first_fault.detail1 = detail1;
        g_first_fault.valid = 1U;
    }
    diagnostics_exit_critical(critical_state);
}

const fault_snapshot_t *diagnostics_get_first_fault(void)
{
    uint32_t critical_state;

    critical_state = diagnostics_enter_critical();
    g_fault_readback.valid = g_first_fault.valid;
    g_fault_readback.module = g_first_fault.module;
    g_fault_readback.code = g_first_fault.code;
    g_fault_readback.tick = g_first_fault.tick;
    g_fault_readback.detail0 = g_first_fault.detail0;
    g_fault_readback.detail1 = g_first_fault.detail1;
    diagnostics_exit_critical(critical_state);

    return &g_fault_readback;
}

void diagnostics_clear_first_fault(void)
{
    uint32_t critical_state;

    critical_state = diagnostics_enter_critical();
    memset((void *)&g_first_fault, 0, sizeof(g_first_fault));
    diagnostics_exit_critical(critical_state);
}

void diagnostics_print_fault(void)
{
    const fault_snapshot_t *fault = diagnostics_get_first_fault();

    if (fault->valid == 0U)
    {
        printf("fault valid=0\r\n");
        return;
    }

    printf("fault valid=1 module=%u code=%ld tick=%lu detail0=0x%08lX detail1=0x%08lX\r\n",
           (unsigned int)fault->module,
           (long)fault->code,
           (unsigned long)fault->tick,
           (unsigned long)fault->detail0,
           (unsigned long)fault->detail1);
}
