#ifndef BOOT_PROBE_SERVICE_H
#define BOOT_PROBE_SERVICE_H

#include <stdint.h>

#define BOOT_PROBE_MAX_ENTRIES       32U
#define BOOT_PROBE_TOTAL_TIMEOUT_MS 250U
#define BOOT_PROBE_DIAG_MODULE      0x4250U

typedef enum
{
    BOOT_PROBE_OK = 0,
    BOOT_PROBE_ABSENT,
    BOOT_PROBE_NOT_READY,
    BOOT_PROBE_TIMEOUT,
    BOOT_PROBE_UNSUPPORTED,
    BOOT_PROBE_ERROR
} boot_probe_result_t;

typedef enum
{
    BOOT_PROBE_KIND_INTERNAL = 0,
    BOOT_PROBE_KIND_I2C,
    BOOT_PROBE_KIND_STORAGE,
    BOOT_PROBE_KIND_DISPLAY,
    BOOT_PROBE_KIND_SENSOR,
    BOOT_PROBE_KIND_AUDIO,
    BOOT_PROBE_KIND_CAMERA
} boot_probe_kind_t;

typedef struct
{
    const char *id;
    boot_probe_kind_t kind;
    uint8_t required;
    uint8_t present;
    uint8_t ready;
    int32_t result;
    uint32_t elapsed_ms;
    uint32_t detail0;
    uint32_t detail1;
} boot_probe_status_t;

typedef int (*boot_probe_callback_t)(boot_probe_status_t *status);

typedef struct
{
    const char *id;
    boot_probe_kind_t kind;
    uint8_t required;
    boot_probe_callback_t callback;
} boot_probe_entry_t;

typedef struct
{
    uint32_t total;
    uint32_t ready;
    uint32_t absent;
    uint32_t not_ready;
    uint32_t timeout;
    uint32_t unsupported;
    uint32_t errors;
    uint32_t elapsed_ms;
    uint8_t truncated;
} boot_probe_summary_t;

void boot_probe_init(void);
void boot_probe_start(uint32_t now_ms);
void boot_probe_start_quiet(uint32_t now_ms);
void boot_probe_poll(uint32_t now_ms);
uint8_t boot_probe_is_complete(void);
int boot_probe_get_status(uint32_t index, boot_probe_status_t *status);
void boot_probe_get_summary(boot_probe_summary_t *summary);
void boot_probe_get_progress(uint32_t *completed, uint32_t *total);
void boot_probe_print_next(void);
void boot_probe_print_status(void);

const boot_probe_entry_t *boot_probe_target_entries(uint32_t *count);

#endif
