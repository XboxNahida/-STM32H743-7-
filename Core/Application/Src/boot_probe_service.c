#include "boot_probe_service.h"

#include <stdio.h>
#include <string.h>

#include "diagnostics.h"

#if defined(BOOT_PROBE_HOST_TEST)
extern uint32_t boot_probe_test_now_ms(void);
extern int boot_probe_test_printf(const char *format, ...);

#define BOOT_PROBE_PRINTF boot_probe_test_printf

static uint32_t boot_probe_now_ms(void)
{
    return boot_probe_test_now_ms();
}
#else
#include "main.h"

#define BOOT_PROBE_PRINTF printf

static uint32_t boot_probe_now_ms(void)
{
    return HAL_GetTick();
}
#endif

static const boot_probe_entry_t *g_entries;
static uint32_t g_entry_count;
static uint32_t g_current_index;
static uint32_t g_print_index;
static uint32_t g_probe_elapsed_ms;
static uint8_t g_started;
static uint8_t g_complete;
static uint8_t g_summary_printed;
static uint8_t g_print_enabled;
static boot_probe_status_t g_statuses[BOOT_PROBE_MAX_ENTRIES];
static boot_probe_summary_t g_summary;

static uint32_t boot_probe_elapsed(uint32_t now_ms, uint32_t start_ms)
{
    return now_ms - start_ms;
}

static uint32_t boot_probe_limit_elapsed(uint32_t elapsed_ms)
{
    if (elapsed_ms > BOOT_PROBE_TOTAL_TIMEOUT_MS)
    {
        return BOOT_PROBE_TOTAL_TIMEOUT_MS;
    }
    return elapsed_ms;
}

static const char *boot_probe_kind_name(boot_probe_kind_t kind)
{
    static const char *const names[] =
    {
        "internal", "i2c", "storage", "display",
        "sensor", "audio", "camera"
    };
    uint32_t value = (uint32_t)kind;

    if (value >= (sizeof(names) / sizeof(names[0])))
    {
        return "unknown";
    }
    return names[value];
}

static void boot_probe_reset_results(void)
{
    uint32_t index;

    memset(g_statuses, 0, sizeof(g_statuses));
    memset(&g_summary, 0, sizeof(g_summary));
    g_summary.total = g_entry_count;
    g_summary.not_ready = g_entry_count;

    for (index = 0U; index < g_entry_count; index++)
    {
        g_statuses[index].id = g_entries[index].id;
        g_statuses[index].kind = g_entries[index].kind;
        g_statuses[index].required = g_entries[index].required;
        g_statuses[index].result = BOOT_PROBE_NOT_READY;
    }

    g_current_index = 0U;
    g_print_index = 0U;
    g_summary_printed = 0U;
}

static void boot_probe_count_result(int32_t result)
{
    if (result != BOOT_PROBE_NOT_READY)
    {
        if (g_summary.not_ready != 0U)
        {
            g_summary.not_ready--;
        }
    }

    switch (result)
    {
        case BOOT_PROBE_OK:
            g_summary.ready++;
            break;
        case BOOT_PROBE_ABSENT:
            g_summary.absent++;
            break;
        case BOOT_PROBE_NOT_READY:
            break;
        case BOOT_PROBE_TIMEOUT:
            g_summary.timeout++;
            break;
        case BOOT_PROBE_UNSUPPORTED:
            g_summary.unsupported++;
            break;
        case BOOT_PROBE_ERROR:
        default:
            g_summary.errors++;
            break;
    }
}

static void boot_probe_print_one(const boot_probe_status_t *status)
{
    BOOT_PROBE_PRINTF("boot_probe id=%s kind=%s required=%u present=%u ready=%u result=%ld elapsed_ms=%lu detail0=0x%08lX detail1=0x%08lX\r\n",
                      (status->id != NULL) ? status->id : "unknown",
                      boot_probe_kind_name(status->kind),
                      (unsigned int)status->required,
                      (unsigned int)status->present,
                      (unsigned int)status->ready,
                      (long)status->result,
                      (unsigned long)status->elapsed_ms,
                      (unsigned long)status->detail0,
                      (unsigned long)status->detail1);
}

static void boot_probe_print_summary(void)
{
    BOOT_PROBE_PRINTF("boot_probe summary total=%lu ready=%lu absent=%lu not_ready=%lu timeout=%lu unsupported=%lu errors=%lu elapsed_ms=%lu truncated=%u\r\n",
                      (unsigned long)g_summary.total,
                      (unsigned long)g_summary.ready,
                      (unsigned long)g_summary.absent,
                      (unsigned long)g_summary.not_ready,
                      (unsigned long)g_summary.timeout,
                      (unsigned long)g_summary.unsupported,
                      (unsigned long)g_summary.errors,
                      (unsigned long)g_summary.elapsed_ms,
                      (unsigned int)g_summary.truncated);
}

void boot_probe_init(void)
{
    uint32_t count = 0U;

    g_entries = boot_probe_target_entries(&count);
    if ((g_entries == NULL) || (count > BOOT_PROBE_MAX_ENTRIES))
    {
        g_entries = NULL;
        g_entry_count = 0U;
    }
    else
    {
        g_entry_count = count;
    }

    boot_probe_reset_results();
    g_probe_elapsed_ms = 0U;
    g_started = 0U;
    g_complete = 0U;
    g_print_enabled = 1U;
}

static void boot_probe_start_mode(uint32_t now_ms, uint8_t print_enabled)
{
    (void)now_ms;
    boot_probe_reset_results();
    g_probe_elapsed_ms = 0U;
    g_print_enabled = print_enabled;
    g_started = 1U;
    g_complete = (g_entry_count == 0U) ? 1U : 0U;
}

void boot_probe_start(uint32_t now_ms)
{
    boot_probe_start_mode(now_ms, 1U);
}

void boot_probe_start_quiet(uint32_t now_ms)
{
    boot_probe_start_mode(now_ms, 0U);
}

void boot_probe_poll(uint32_t now_ms)
{
    const boot_probe_entry_t *entry;
    boot_probe_status_t *status;
    uint32_t callback_start_ms;
    uint32_t callback_end_ms;
    uint32_t callback_elapsed_ms;
    int result;

    (void)now_ms;

    if ((g_started == 0U) || (g_complete != 0U))
    {
        return;
    }

    if (g_probe_elapsed_ms >= BOOT_PROBE_TOTAL_TIMEOUT_MS)
    {
        g_summary.elapsed_ms = BOOT_PROBE_TOTAL_TIMEOUT_MS;
        g_summary.truncated = 1U;
        g_complete = 1U;
        return;
    }

    entry = &g_entries[g_current_index];
    status = &g_statuses[g_current_index];
    callback_start_ms = boot_probe_now_ms();

    if (entry->callback == NULL)
    {
        result = BOOT_PROBE_UNSUPPORTED;
    }
    else
    {
        result = entry->callback(status);
    }

    callback_end_ms = boot_probe_now_ms();
    callback_elapsed_ms = boot_probe_elapsed(callback_end_ms,
                                             callback_start_ms);
    status->elapsed_ms = callback_elapsed_ms;
    if (callback_elapsed_ms >=
        (BOOT_PROBE_TOTAL_TIMEOUT_MS - g_probe_elapsed_ms))
    {
        g_probe_elapsed_ms = BOOT_PROBE_TOTAL_TIMEOUT_MS;
    }
    else
    {
        g_probe_elapsed_ms += callback_elapsed_ms;
    }
    if (((uint32_t)result > (uint32_t)BOOT_PROBE_ERROR))
    {
        result = BOOT_PROBE_ERROR;
    }
    status->result = result;

    if ((result == BOOT_PROBE_UNSUPPORTED) && (entry->callback == NULL))
    {
        status->present = 0U;
        status->ready = 0U;
    }

    boot_probe_count_result(status->result);
    if ((status->required != 0U) &&
        ((status->result == BOOT_PROBE_TIMEOUT) ||
         (status->result == BOOT_PROBE_ERROR)))
    {
        diagnostics_capture_first(BOOT_PROBE_DIAG_MODULE,
                                  status->result,
                                  g_current_index,
                                  status->detail0);
    }

    g_current_index++;
    g_summary.elapsed_ms = boot_probe_limit_elapsed(g_probe_elapsed_ms);
    if (g_probe_elapsed_ms >= BOOT_PROBE_TOTAL_TIMEOUT_MS)
    {
        g_summary.truncated = 1U;
        g_complete = 1U;
        return;
    }
    if (g_current_index >= g_entry_count)
    {
        g_complete = 1U;
    }
}

uint8_t boot_probe_is_complete(void)
{
    return g_complete;
}

int boot_probe_get_status(uint32_t index, boot_probe_status_t *status)
{
    if ((status == NULL) || (index >= g_entry_count))
    {
        return BOOT_PROBE_ERROR;
    }
    *status = g_statuses[index];
    return BOOT_PROBE_OK;
}

void boot_probe_get_summary(boot_probe_summary_t *summary)
{
    if (summary != NULL)
    {
        *summary = g_summary;
    }
}

void boot_probe_get_progress(uint32_t *completed, uint32_t *total)
{
    if (completed != NULL)
    {
        *completed = g_current_index;
    }
    if (total != NULL)
    {
        *total = g_entry_count;
    }
}

void boot_probe_print_next(void)
{
    if ((g_print_enabled == 0U) ||
        (g_started == 0U) ||
        (g_complete == 0U))
    {
        return;
    }
    if (g_print_index < g_entry_count)
    {
        boot_probe_print_one(&g_statuses[g_print_index]);
        g_print_index++;
        return;
    }
    if (g_summary_printed == 0U)
    {
        boot_probe_print_summary();
        g_summary_printed = 1U;
    }
}

void boot_probe_print_status(void)
{
    uint32_t index;

    for (index = 0U; index < g_entry_count; index++)
    {
        boot_probe_print_one(&g_statuses[index]);
    }
    boot_probe_print_summary();
}
