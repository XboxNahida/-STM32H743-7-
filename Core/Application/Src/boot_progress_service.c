#include "boot_progress_service.h"

#include <stddef.h>
#include <string.h>

static const char *const g_device_names[BOOT_DEVICE_COUNT] =
{
    "SD Card",
    "QSPI Flash",
    "NAND Flash",
    "GT911",
    "PCF8574",
    "OV5640",
    "ES8388",
    "SH3001",
    "AP3216C",
    "AT24C02"
};

static boot_progress_snapshot_t g_snapshot;

static uint8_t clamp_percent(uint8_t percent)
{
    return (percent > 100U) ? 100U : percent;
}

static void copy_text(char *destination, size_t destination_size,
                      const char *source)
{
    size_t length;

    if ((destination == NULL) || (destination_size == 0U))
    {
        return;
    }

    destination[0] = '\0';
    if (source == NULL)
    {
        return;
    }

    length = strlen(source);
    if (length >= destination_size)
    {
        length = destination_size - 1U;
    }
    if (length != 0U)
    {
        (void)memcpy(destination, source, length);
    }
    destination[length] = '\0';
}

static void append_text(char *destination, size_t destination_size,
                        const char *source)
{
    size_t used;
    size_t remaining;
    size_t length;

    if ((destination == NULL) || (destination_size == 0U) ||
        (source == NULL))
    {
        return;
    }

    used = strlen(destination);
    if (used >= (destination_size - 1U))
    {
        return;
    }

    remaining = destination_size - used - 1U;
    length = strlen(source);
    if (length > remaining)
    {
        length = remaining;
    }
    if (length != 0U)
    {
        (void)memcpy(&destination[used], source, length);
    }
    destination[used + length] = '\0';
}

static void latch_first_error(const char *name, const char *detail)
{
    if (g_snapshot.first_error_valid != 0U)
    {
        return;
    }

    copy_text(g_snapshot.first_error,
              sizeof(g_snapshot.first_error),
              name);
    if ((detail != NULL) && (detail[0] != '\0'))
    {
        append_text(g_snapshot.first_error,
                    sizeof(g_snapshot.first_error),
                    ": ");
        append_text(g_snapshot.first_error,
                    sizeof(g_snapshot.first_error),
                    detail);
    }
    g_snapshot.first_error_valid = 1U;
}

void boot_progress_init(void)
{
    uint32_t index;

    (void)memset(&g_snapshot, 0, sizeof(g_snapshot));
    for (index = 0U; index < (uint32_t)BOOT_DEVICE_COUNT; index++)
    {
        copy_text(g_snapshot.devices[index].name,
                  sizeof(g_snapshot.devices[index].name),
                  g_device_names[index]);
    }
}

void boot_progress_set_current(const char *stage,
                               const char *detail,
                               uint8_t total_percent,
                               uint8_t item_percent)
{
    copy_text(g_snapshot.stage, sizeof(g_snapshot.stage), stage);
    copy_text(g_snapshot.detail, sizeof(g_snapshot.detail), detail);
    g_snapshot.total_percent = clamp_percent(total_percent);
    g_snapshot.item_percent = clamp_percent(item_percent);
}

void boot_progress_complete_item(const char *name,
                                 boot_progress_level_t level,
                                 const char *detail,
                                 uint8_t total_percent)
{
    copy_text(g_snapshot.last_item, sizeof(g_snapshot.last_item), name);
    copy_text(g_snapshot.detail, sizeof(g_snapshot.detail), detail);
    g_snapshot.total_percent = clamp_percent(total_percent);
    g_snapshot.item_percent = 100U;
    g_snapshot.completed_items++;

    if (level == BOOT_PROGRESS_ERROR)
    {
        latch_first_error(name, detail);
    }
}

void boot_progress_set_device(boot_progress_device_id_t id,
                              boot_progress_level_t level,
                              const char *detail)
{
    uint32_t index = (uint32_t)id;

    if (index >= (uint32_t)BOOT_DEVICE_COUNT)
    {
        return;
    }

    g_snapshot.devices[index].level = level;
    g_snapshot.devices[index].updated = 1U;
    copy_text(g_snapshot.devices[index].detail,
              sizeof(g_snapshot.devices[index].detail),
              detail);
}

void boot_progress_set_elapsed_ms(uint32_t elapsed_ms)
{
    g_snapshot.elapsed_ms = elapsed_ms;
}

void boot_progress_get_snapshot(boot_progress_snapshot_t *snapshot)
{
    if (snapshot != NULL)
    {
        *snapshot = g_snapshot;
    }
}
