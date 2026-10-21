#include "system_settings_service.h"

#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "at24c02_service.h"

#define SETTINGS_SLOT_SIZE       32U
#define SETTINGS_SLOT_A_OFFSET   0U
#define SETTINGS_SLOT_B_OFFSET   32U
#define SETTINGS_MAGIC_0         0x53U
#define SETTINGS_MAGIC_1         0x46U
#define SETTINGS_VERSION         1U
#define SETTINGS_CRC_LENGTH      28U
#define SETTINGS_CRC_OFFSET      28U
#define SETTINGS_COMMIT_OFFSET   31U
#define SETTINGS_SAVE_INTERVAL_MS 3600000UL
#define SETTINGS_RETRY_INTERVAL_MS 60000UL
#define SETTINGS_CHANGE_DELAY_MS   2000UL

typedef struct
{
    uint8_t valid;
    uint32_t sequence;
    system_settings_t settings;
} decoded_slot_t;

static system_settings_status_t g_settings_status;
static uint8_t g_storage_scanned;
static system_settings_t g_runtime_settings;
static uint8_t g_runtime_ready;
static uint8_t g_save_requested;
static uint8_t g_retry_pending;
static uint32_t g_last_save_ms;
static uint32_t g_retry_at_ms;
static uint8_t g_settings_dirty;
static uint32_t g_dirty_deadline_ms;

static void increment_saturated(uint32_t *value)
{
    if (*value != UINT32_MAX)
    {
        (*value)++;
    }
}

static uint16_t settings_crc16(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFFU;
    uint16_t index;

    for (index = 0U; index < length; index++)
    {
        uint8_t bit;

        crc ^= (uint16_t)data[index] << 8;
        for (bit = 0U; bit < 8U; bit++)
        {
            crc = ((crc & 0x8000U) != 0U) ?
                  (uint16_t)((crc << 1) ^ 0x1021U) :
                  (uint16_t)(crc << 1);
        }
    }
    return crc;
}

static uint8_t days_in_month(uint8_t year, uint8_t month)
{
    static const uint8_t days[12] =
        {31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U};
    uint8_t result;

    if ((month == 0U) || (month > 12U))
    {
        return 0U;
    }
    result = days[month - 1U];
    if ((month == 2U) && ((year % 4U) == 0U))
    {
        result = 29U;
    }
    return result;
}

static int settings_are_valid(const system_settings_t *settings)
{
    uint8_t month_days;

    if ((settings == NULL) ||
        (settings->brightness_percent > 100U) ||
        (settings->volume_percent > 100U) ||
        (settings->camera_mirror > 1U) ||
        (settings->camera_flip > 1U) ||
        (settings->camera_flash_mode > 2U) ||
        (settings->rtc_hour > 23U) ||
        (settings->rtc_minute > 59U) ||
        (settings->rtc_second > 59U))
    {
        return 0;
    }
    month_days = days_in_month(settings->rtc_year, settings->rtc_month);
    if ((settings->rtc_day == 0U) || (settings->rtc_day > month_days))
    {
        return 0;
    }
    return 1;
}

static uint32_t read_u32_le(const uint8_t *data)
{
    return ((uint32_t)data[0]) |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

static void write_u32_le(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
    data[2] = (uint8_t)(value >> 16);
    data[3] = (uint8_t)(value >> 24);
}

static int sequence_is_newer(uint32_t candidate, uint32_t reference)
{
    uint32_t difference = candidate - reference;

    return (difference != 0U) && (difference < 0x80000000UL);
}

static int time_reached(uint32_t now_ms, uint32_t deadline_ms)
{
    return ((int32_t)(now_ms - deadline_ms) >= 0) ? 1 : 0;
}

static void copy_rtc_fields(system_settings_t *destination,
                            const system_settings_t *source)
{
    destination->rtc_year = source->rtc_year;
    destination->rtc_month = source->rtc_month;
    destination->rtc_day = source->rtc_day;
    destination->rtc_hour = source->rtc_hour;
    destination->rtc_minute = source->rtc_minute;
    destination->rtc_second = source->rtc_second;
}

static void decode_slot(const uint8_t record[SETTINGS_SLOT_SIZE],
                        decoded_slot_t *decoded)
{
    uint16_t stored_crc;
    uint16_t calculated_crc;

    memset(decoded, 0, sizeof(*decoded));
    if ((record[0] != SETTINGS_MAGIC_0) ||
        (record[1] != SETTINGS_MAGIC_1) ||
        (record[2] != SETTINGS_VERSION) ||
        (record[3] != SETTINGS_SLOT_SIZE) ||
        (record[SETTINGS_COMMIT_OFFSET] != SYSTEM_SETTINGS_COMMIT_MARKER))
    {
        return;
    }
    stored_crc = (uint16_t)record[SETTINGS_CRC_OFFSET] |
                 ((uint16_t)record[SETTINGS_CRC_OFFSET + 1U] << 8);
    calculated_crc = settings_crc16(record, SETTINGS_CRC_LENGTH);
    if (stored_crc != calculated_crc)
    {
        return;
    }

    decoded->sequence = read_u32_le(&record[4]);
    decoded->settings.brightness_percent = record[8];
    decoded->settings.volume_percent = record[9];
    decoded->settings.camera_mirror = record[10];
    decoded->settings.camera_flip = record[11];
    decoded->settings.camera_flash_mode = record[12];
    decoded->settings.rtc_year = record[13];
    decoded->settings.rtc_month = record[14];
    decoded->settings.rtc_day = record[15];
    decoded->settings.rtc_hour = record[16];
    decoded->settings.rtc_minute = record[17];
    decoded->settings.rtc_second = record[18];
    decoded->valid = (uint8_t)settings_are_valid(&decoded->settings);
}

static void encode_slot(uint8_t record[SETTINGS_SLOT_SIZE],
                        const system_settings_t *settings,
                        uint32_t sequence)
{
    uint16_t crc;

    memset(record, 0, SETTINGS_SLOT_SIZE);
    record[0] = SETTINGS_MAGIC_0;
    record[1] = SETTINGS_MAGIC_1;
    record[2] = SETTINGS_VERSION;
    record[3] = SETTINGS_SLOT_SIZE;
    write_u32_le(&record[4], sequence);
    record[8] = settings->brightness_percent;
    record[9] = settings->volume_percent;
    record[10] = settings->camera_mirror;
    record[11] = settings->camera_flip;
    record[12] = settings->camera_flash_mode;
    record[13] = settings->rtc_year;
    record[14] = settings->rtc_month;
    record[15] = settings->rtc_day;
    record[16] = settings->rtc_hour;
    record[17] = settings->rtc_minute;
    record[18] = settings->rtc_second;
    crc = settings_crc16(record, SETTINGS_CRC_LENGTH);
    record[SETTINGS_CRC_OFFSET] = (uint8_t)crc;
    record[SETTINGS_CRC_OFFSET + 1U] = (uint8_t)(crc >> 8);
    record[SETTINGS_COMMIT_OFFSET] = SYSTEM_SETTINGS_COMMIT_MARKER;
}

void system_settings_service_init(void)
{
    memset(&g_settings_status, 0, sizeof(g_settings_status));
    g_settings_status.active_slot = SYSTEM_SETTINGS_SLOT_NONE;
    g_settings_status.result = SYSTEM_SETTINGS_NOT_FOUND;
    g_storage_scanned = 0U;
    memset(&g_runtime_settings, 0, sizeof(g_runtime_settings));
    g_runtime_ready = 0U;
    g_save_requested = 0U;
    g_retry_pending = 0U;
    g_last_save_ms = 0U;
    g_retry_at_ms = 0U;
    g_settings_dirty = 0U;
    g_dirty_deadline_ms = 0U;
}

int system_settings_runtime_start(uint8_t rtc_backup_valid,
                                  const system_settings_t *live_settings,
                                  system_settings_t *effective_settings,
                                  uint32_t now_ms,
                                  uint32_t timeout_ms)
{
    system_settings_t stored;
    int load_result;
    int save_result = SYSTEM_SETTINGS_OK;

    if ((rtc_backup_valid > 1U) ||
        (settings_are_valid(live_settings) == 0) ||
        (effective_settings == NULL) || (timeout_ms == 0U))
    {
        return SYSTEM_SETTINGS_INVALID_ARG;
    }

    load_result = system_settings_load(&stored, timeout_ms);
    if (load_result == SYSTEM_SETTINGS_OK)
    {
        g_runtime_settings = stored;
        if (rtc_backup_valid != 0U)
        {
            copy_rtc_fields(&g_runtime_settings, live_settings);
            save_result = system_settings_save(&g_runtime_settings, timeout_ms);
        }
    }
    else if (load_result == SYSTEM_SETTINGS_NOT_FOUND)
    {
        g_runtime_settings = *live_settings;
        save_result = system_settings_save(&g_runtime_settings, timeout_ms);
    }
    else
    {
        *effective_settings = *live_settings;
        return load_result;
    }

    *effective_settings = g_runtime_settings;
    g_runtime_ready = 1U;
    g_save_requested = 0U;
    g_retry_pending = (uint8_t)(save_result != SYSTEM_SETTINGS_OK);
    g_last_save_ms = now_ms;
    g_retry_at_ms = now_ms + SETTINGS_RETRY_INTERVAL_MS;
    g_settings_dirty = 0U;
    g_dirty_deadline_ms = 0U;
    return save_result;
}

int system_settings_runtime_poll(const system_settings_t *live_settings,
                                 uint32_t now_ms,
                                 uint32_t timeout_ms)
{
    uint8_t save_due;
    int result;

    if ((g_runtime_ready == 0U) ||
        (settings_are_valid(live_settings) == 0) || (timeout_ms == 0U))
    {
        return SYSTEM_SETTINGS_INVALID_ARG;
    }
    g_runtime_settings = *live_settings;
    if ((g_retry_pending != 0U) &&
        (time_reached(now_ms, g_retry_at_ms) == 0) &&
        (g_save_requested == 0U))
    {
        return SYSTEM_SETTINGS_OK;
    }
    save_due = (uint8_t)((g_save_requested != 0U) ||
               (g_retry_pending != 0U) ||
               ((g_settings_dirty != 0U) &&
                (time_reached(now_ms, g_dirty_deadline_ms) != 0)) ||
               ((uint32_t)(now_ms - g_last_save_ms) >=
                SETTINGS_SAVE_INTERVAL_MS));
    if (save_due == 0U)
    {
        return SYSTEM_SETTINGS_OK;
    }

    g_save_requested = 0U;
    result = system_settings_save(&g_runtime_settings, timeout_ms);
    if (result == SYSTEM_SETTINGS_OK)
    {
        g_last_save_ms = now_ms;
        g_retry_pending = 0U;
        g_settings_dirty = 0U;
    }
    else
    {
        g_retry_pending = 1U;
        g_retry_at_ms = now_ms + SETTINGS_RETRY_INTERVAL_MS;
    }
    return result;
}

void system_settings_request_save(void)
{
    if (g_runtime_ready != 0U)
    {
        g_save_requested = 1U;
    }
}

static void mark_settings_dirty(uint32_t now_ms)
{
    g_settings_dirty = 1U;
    g_dirty_deadline_ms = now_ms + SETTINGS_CHANGE_DELAY_MS;
}

void system_settings_update_brightness(uint8_t percent, uint32_t now_ms)
{
    if ((g_runtime_ready != 0U) && (percent <= 100U) &&
        (g_runtime_settings.brightness_percent != percent))
    {
        g_runtime_settings.brightness_percent = percent;
        mark_settings_dirty(now_ms);
    }
}

void system_settings_update_volume(uint8_t percent, uint32_t now_ms)
{
    if ((g_runtime_ready != 0U) && (percent <= 100U) &&
        (g_runtime_settings.volume_percent != percent))
    {
        g_runtime_settings.volume_percent = percent;
        mark_settings_dirty(now_ms);
    }
}

void system_settings_update_camera(uint8_t mirror, uint8_t flip,
                                   uint8_t flash_mode, uint32_t now_ms)
{
    if ((g_runtime_ready == 0U) || (mirror > 1U) || (flip > 1U) ||
        (flash_mode > 2U))
    {
        return;
    }
    if ((g_runtime_settings.camera_mirror != mirror) ||
        (g_runtime_settings.camera_flip != flip) ||
        (g_runtime_settings.camera_flash_mode != flash_mode))
    {
        g_runtime_settings.camera_mirror = mirror;
        g_runtime_settings.camera_flip = flip;
        g_runtime_settings.camera_flash_mode = flash_mode;
        mark_settings_dirty(now_ms);
    }
}

void system_settings_get(system_settings_t *settings)
{
    if ((settings != NULL) && (g_runtime_ready != 0U))
    {
        *settings = g_runtime_settings;
    }
}

int system_settings_load(system_settings_t *settings, uint32_t timeout_ms)
{
    uint8_t raw_a[SETTINGS_SLOT_SIZE];
    uint8_t raw_b[SETTINGS_SLOT_SIZE];
    decoded_slot_t slot_a;
    decoded_slot_t slot_b;
    const decoded_slot_t *selected = NULL;
    uint8_t selected_slot = SYSTEM_SETTINGS_SLOT_NONE;
    int read_a;
    int read_b;
    int result;

    if ((settings == NULL) || (timeout_ms == 0U))
    {
        return SYSTEM_SETTINGS_INVALID_ARG;
    }
    read_a = at24c02_read(SETTINGS_SLOT_A_OFFSET, raw_a,
                          SETTINGS_SLOT_SIZE, timeout_ms);
    read_b = at24c02_read(SETTINGS_SLOT_B_OFFSET, raw_b,
                          SETTINGS_SLOT_SIZE, timeout_ms);
    memset(&slot_a, 0, sizeof(slot_a));
    memset(&slot_b, 0, sizeof(slot_b));
    if (read_a == AT24C02_RESULT_OK)
    {
        decode_slot(raw_a, &slot_a);
    }
    if (read_b == AT24C02_RESULT_OK)
    {
        decode_slot(raw_b, &slot_b);
    }

    if ((slot_a.valid != 0U) && (slot_b.valid != 0U))
    {
        if (sequence_is_newer(slot_b.sequence, slot_a.sequence) != 0)
        {
            selected = &slot_b;
            selected_slot = 1U;
        }
        else
        {
            selected = &slot_a;
            selected_slot = 0U;
        }
    }
    else if (slot_a.valid != 0U)
    {
        selected = &slot_a;
        selected_slot = 0U;
    }
    else if (slot_b.valid != 0U)
    {
        selected = &slot_b;
        selected_slot = 1U;
    }

    if (selected != NULL)
    {
        *settings = selected->settings;
        g_settings_status.valid = 1U;
        g_settings_status.active_slot = selected_slot;
        g_settings_status.sequence = selected->sequence;
        result = SYSTEM_SETTINGS_OK;
    }
    else
    {
        g_settings_status.valid = 0U;
        g_settings_status.active_slot = SYSTEM_SETTINGS_SLOT_NONE;
        g_settings_status.sequence = 0U;
        result = ((read_a != AT24C02_RESULT_OK) ||
                  (read_b != AT24C02_RESULT_OK)) ?
                 SYSTEM_SETTINGS_STORAGE_ERROR : SYSTEM_SETTINGS_NOT_FOUND;
    }
    g_storage_scanned = 1U;
    g_settings_status.result = result;
    increment_saturated(&g_settings_status.load_count);
    return result;
}

int system_settings_save(const system_settings_t *settings,
                         uint32_t timeout_ms)
{
    system_settings_t ignored;
    uint8_t record[SETTINGS_SLOT_SIZE];
    uint8_t invalid_commit = 0U;
    uint8_t commit = SYSTEM_SETTINGS_COMMIT_MARKER;
    uint8_t target_slot;
    uint16_t target_offset;
    uint32_t next_sequence;
    int scan_result;
    int result;

    if ((settings_are_valid(settings) == 0) || (timeout_ms == 0U))
    {
        return SYSTEM_SETTINGS_INVALID_ARG;
    }
    if (g_storage_scanned == 0U)
    {
        scan_result = system_settings_load(&ignored, timeout_ms);
        if ((scan_result != SYSTEM_SETTINGS_OK) &&
            (scan_result != SYSTEM_SETTINGS_NOT_FOUND))
        {
            return scan_result;
        }
    }

    target_slot = (g_settings_status.active_slot == 0U) ? 1U : 0U;
    target_offset = (target_slot == 0U) ?
                    SETTINGS_SLOT_A_OFFSET : SETTINGS_SLOT_B_OFFSET;
    next_sequence = (g_settings_status.valid != 0U) ?
                    (g_settings_status.sequence + 1U) : 1U;
    encode_slot(record, settings, next_sequence);

    result = at24c02_write((uint16_t)(target_offset + SETTINGS_COMMIT_OFFSET),
                           &invalid_commit, 1U, timeout_ms);
    if (result == AT24C02_RESULT_OK)
    {
        result = at24c02_write(target_offset, record,
                               SETTINGS_COMMIT_OFFSET, timeout_ms);
    }
    if (result == AT24C02_RESULT_OK)
    {
        result = at24c02_write((uint16_t)(target_offset +
                               SETTINGS_COMMIT_OFFSET),
                               &commit, 1U, timeout_ms);
    }

    increment_saturated(&g_settings_status.save_count);
    if (result != AT24C02_RESULT_OK)
    {
        g_settings_status.result = SYSTEM_SETTINGS_STORAGE_ERROR;
        return SYSTEM_SETTINGS_STORAGE_ERROR;
    }
    g_settings_status.valid = 1U;
    g_settings_status.active_slot = target_slot;
    g_settings_status.sequence = next_sequence;
    g_settings_status.result = SYSTEM_SETTINGS_OK;
    return SYSTEM_SETTINGS_OK;
}

void system_settings_get_status(system_settings_status_t *status)
{
    if (status != NULL)
    {
        *status = g_settings_status;
    }
}

void system_settings_print_status(void)
{
    printf("settings valid=%u active_slot=%u sequence=%lu result=%ld brightness=%u volume=%u camera_mirror=%u camera_flip=%u camera_flash=%u rtc=20%02u-%02u-%02uT%02u:%02u:%02u load_count=%lu save_count=%lu\r\n",
           (unsigned int)g_settings_status.valid,
           (unsigned int)g_settings_status.active_slot,
           (unsigned long)g_settings_status.sequence,
           (long)g_settings_status.result,
           (unsigned int)g_runtime_settings.brightness_percent,
           (unsigned int)g_runtime_settings.volume_percent,
           (unsigned int)g_runtime_settings.camera_mirror,
           (unsigned int)g_runtime_settings.camera_flip,
           (unsigned int)g_runtime_settings.camera_flash_mode,
           (unsigned int)g_runtime_settings.rtc_year,
           (unsigned int)g_runtime_settings.rtc_month,
           (unsigned int)g_runtime_settings.rtc_day,
           (unsigned int)g_runtime_settings.rtc_hour,
           (unsigned int)g_runtime_settings.rtc_minute,
           (unsigned int)g_runtime_settings.rtc_second,
           (unsigned long)g_settings_status.load_count,
           (unsigned long)g_settings_status.save_count);
}
