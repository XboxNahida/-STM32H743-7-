#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "system_settings_service.h"

#define SLOT_SIZE 32U
#define SLOT_A    0U
#define SLOT_B    32U
#define COMMIT_OFFSET 31U

static uint8_t g_eeprom[256];
static uint32_t g_read_calls;
static uint32_t g_write_calls;
static uint32_t g_fail_write_call;
static uint32_t g_fail_read_call;

int at24c02_read(uint16_t offset, uint8_t *data, uint16_t length,
                 uint32_t timeout_ms)
{
    (void)timeout_ms;
    g_read_calls++;
    if ((g_fail_read_call != 0U) && (g_read_calls == g_fail_read_call))
    {
        return 4;
    }
    memcpy(data, &g_eeprom[offset], length);
    return 0;
}

int at24c02_write(uint16_t offset, const uint8_t *data, uint16_t length,
                  uint32_t timeout_ms)
{
    (void)timeout_ms;
    g_write_calls++;
    if ((g_fail_write_call != 0U) && (g_write_calls == g_fail_write_call))
    {
        return 4;
    }
    memcpy(&g_eeprom[offset], data, length);
    return 0;
}

static int expect_i32(const char *name, int32_t actual, int32_t expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=%ld expected=%ld\n",
               name, (long)actual, (long)expected);
        return 1;
    }
    return 0;
}

static int expect_u32(const char *name, uint32_t actual, uint32_t expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=%lu expected=%lu\n",
               name, (unsigned long)actual, (unsigned long)expected);
        return 1;
    }
    return 0;
}

static void reset_fixture(void)
{
    memset(g_eeprom, 0xFF, sizeof(g_eeprom));
    g_eeprom[255] = 0x55U;
    g_read_calls = 0U;
    g_write_calls = 0U;
    g_fail_write_call = 0U;
    g_fail_read_call = 0U;
    system_settings_service_init();
}

static int test_one_failed_slot_read_is_not_reported_as_empty(void)
{
    system_settings_t loaded;
    int failures = 0;

    reset_fixture();
    g_fail_read_call = 2U;
    failures += expect_i32("partial_read_error",
        system_settings_load(&loaded, 20U), SYSTEM_SETTINGS_STORAGE_ERROR);
    return failures;
}

static system_settings_t make_settings(uint8_t brightness, uint8_t second)
{
    system_settings_t settings;

    memset(&settings, 0, sizeof(settings));
    settings.brightness_percent = brightness;
    settings.volume_percent = 35U;
    settings.camera_mirror = 1U;
    settings.camera_flip = 0U;
    settings.camera_flash_mode = 2U;
    settings.rtc_year = 26U;
    settings.rtc_month = 7U;
    settings.rtc_day = 23U;
    settings.rtc_hour = 14U;
    settings.rtc_minute = 30U;
    settings.rtc_second = second;
    return settings;
}

static uint16_t test_crc16(const uint8_t *data, uint16_t length)
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

static void set_slot_sequence(uint16_t slot_offset, uint32_t sequence)
{
    uint16_t crc;

    g_eeprom[slot_offset + 4U] = (uint8_t)sequence;
    g_eeprom[slot_offset + 5U] = (uint8_t)(sequence >> 8);
    g_eeprom[slot_offset + 6U] = (uint8_t)(sequence >> 16);
    g_eeprom[slot_offset + 7U] = (uint8_t)(sequence >> 24);
    crc = test_crc16(&g_eeprom[slot_offset], 28U);
    g_eeprom[slot_offset + 28U] = (uint8_t)crc;
    g_eeprom[slot_offset + 29U] = (uint8_t)(crc >> 8);
}

static int test_empty_storage_returns_not_found(void)
{
    system_settings_t loaded;
    system_settings_status_t status;
    int failures = 0;

    reset_fixture();
    failures += expect_i32("empty_load",
        system_settings_load(&loaded, 20U), SYSTEM_SETTINGS_NOT_FOUND);
    system_settings_get_status(&status);
    failures += expect_u32("empty_valid", status.valid, 0U);
    failures += expect_u32("empty_slot", status.active_slot,
                           SYSTEM_SETTINGS_SLOT_NONE);
    failures += expect_u32("empty_reads", g_read_calls, 2U);
    failures += expect_u32("marker_preserved", g_eeprom[255], 0x55U);
    return failures;
}

static int test_save_alternates_slots_and_loads_newest(void)
{
    system_settings_t first;
    system_settings_t second;
    system_settings_t loaded;
    system_settings_status_t status;
    int failures = 0;

    reset_fixture();
    first = make_settings(40U, 10U);
    second = make_settings(75U, 20U);
    failures += expect_i32("first_save",
        system_settings_save(&first, 30U), SYSTEM_SETTINGS_OK);
    failures += expect_u32("first_commit", g_eeprom[SLOT_A + COMMIT_OFFSET],
                           SYSTEM_SETTINGS_COMMIT_MARKER);
    failures += expect_i32("second_save",
        system_settings_save(&second, 30U), SYSTEM_SETTINGS_OK);
    failures += expect_u32("second_commit", g_eeprom[SLOT_B + COMMIT_OFFSET],
                           SYSTEM_SETTINGS_COMMIT_MARKER);

    system_settings_service_init();
    failures += expect_i32("newest_load",
        system_settings_load(&loaded, 20U), SYSTEM_SETTINGS_OK);
    system_settings_get_status(&status);
    failures += expect_u32("newest_brightness", loaded.brightness_percent, 75U);
    failures += expect_u32("newest_second", loaded.rtc_second, 20U);
    failures += expect_u32("newest_slot", status.active_slot, 1U);
    failures += expect_u32("newest_sequence", status.sequence, 2U);
    failures += expect_u32("marker_after_saves", g_eeprom[255], 0x55U);
    return failures;
}

static int test_corrupt_or_uncommitted_new_slot_falls_back(void)
{
    system_settings_t first;
    system_settings_t second;
    system_settings_t loaded;
    int failures = 0;

    reset_fixture();
    first = make_settings(41U, 11U);
    second = make_settings(76U, 21U);
    failures += expect_i32("fallback_save_a",
        system_settings_save(&first, 30U), SYSTEM_SETTINGS_OK);
    failures += expect_i32("fallback_save_b",
        system_settings_save(&second, 30U), SYSTEM_SETTINGS_OK);

    g_eeprom[SLOT_B + 8U] ^= 0x01U;
    system_settings_service_init();
    failures += expect_i32("crc_fallback",
        system_settings_load(&loaded, 20U), SYSTEM_SETTINGS_OK);
    failures += expect_u32("crc_fallback_value", loaded.brightness_percent, 41U);

    g_eeprom[SLOT_B + 8U] ^= 0x01U;
    g_eeprom[SLOT_B + COMMIT_OFFSET] = 0U;
    system_settings_service_init();
    failures += expect_i32("commit_fallback",
        system_settings_load(&loaded, 20U), SYSTEM_SETTINGS_OK);
    failures += expect_u32("commit_fallback_value", loaded.rtc_second, 11U);
    return failures;
}

static int test_interrupted_save_keeps_previous_slot(void)
{
    system_settings_t first;
    system_settings_t second;
    system_settings_t loaded;
    int failures = 0;

    reset_fixture();
    first = make_settings(42U, 12U);
    second = make_settings(77U, 22U);
    failures += expect_i32("interrupt_first_save",
        system_settings_save(&first, 30U), SYSTEM_SETTINGS_OK);
    g_fail_write_call = g_write_calls + 2U;
    failures += expect_i32("interrupt_second_save",
        system_settings_save(&second, 30U), SYSTEM_SETTINGS_STORAGE_ERROR);

    system_settings_service_init();
    failures += expect_i32("interrupt_load",
        system_settings_load(&loaded, 20U), SYSTEM_SETTINGS_OK);
    failures += expect_u32("interrupt_old_value", loaded.brightness_percent, 42U);
    return failures;
}

static int test_invalid_fields_are_rejected_without_writes(void)
{
    system_settings_t settings;
    int failures = 0;

    reset_fixture();
    settings = make_settings(50U, 30U);
    settings.brightness_percent = 101U;
    failures += expect_i32("invalid_brightness",
        system_settings_save(&settings, 30U), SYSTEM_SETTINGS_INVALID_ARG);
    settings = make_settings(50U, 30U);
    settings.rtc_month = 13U;
    failures += expect_i32("invalid_month",
        system_settings_save(&settings, 30U), SYSTEM_SETTINGS_INVALID_ARG);
    settings = make_settings(50U, 30U);
    settings.rtc_day = 31U;
    settings.rtc_month = 2U;
    failures += expect_i32("invalid_day",
        system_settings_save(&settings, 30U), SYSTEM_SETTINGS_INVALID_ARG);
    failures += expect_u32("invalid_writes", g_write_calls, 0U);
    return failures;
}

static int test_sequence_wrap_selects_zero_after_uint32_max(void)
{
    system_settings_t before_wrap;
    system_settings_t after_wrap;
    system_settings_t loaded;
    int failures = 0;

    reset_fixture();
    before_wrap = make_settings(43U, 13U);
    after_wrap = make_settings(78U, 23U);
    failures += expect_i32("wrap_seed",
        system_settings_save(&before_wrap, 30U), SYSTEM_SETTINGS_OK);
    set_slot_sequence(SLOT_A, UINT32_MAX - 1U);
    system_settings_service_init();
    failures += expect_i32("wrap_seed_load",
        system_settings_load(&loaded, 20U), SYSTEM_SETTINGS_OK);
    failures += expect_i32("wrap_max",
        system_settings_save(&before_wrap, 30U), SYSTEM_SETTINGS_OK);
    failures += expect_i32("wrap_zero",
        system_settings_save(&after_wrap, 30U), SYSTEM_SETTINGS_OK);

    system_settings_service_init();
    failures += expect_i32("wrap_load",
        system_settings_load(&loaded, 20U), SYSTEM_SETTINGS_OK);
    failures += expect_u32("wrap_new_value", loaded.brightness_percent, 78U);
    return failures;
}

static int test_valid_rtc_keeps_saved_preferences_and_checkpoints_live_time(void)
{
    system_settings_t stored;
    system_settings_t live;
    system_settings_t effective;
    system_settings_status_t status;
    int failures = 0;

    reset_fixture();
    stored = make_settings(44U, 14U);
    live = make_settings(50U, 45U);
    failures += expect_i32("valid_rtc_seed",
        system_settings_save(&stored, 30U), SYSTEM_SETTINGS_OK);
    system_settings_service_init();
    failures += expect_i32("valid_rtc_start",
        system_settings_runtime_start(1U, &live, &effective,
                                      1000U, 30U), SYSTEM_SETTINGS_OK);
    system_settings_get_status(&status);
    failures += expect_u32("valid_rtc_pref", effective.brightness_percent, 44U);
    failures += expect_u32("valid_rtc_time", effective.rtc_second, 45U);
    failures += expect_u32("valid_rtc_checkpoint", status.sequence, 2U);
    return failures;
}

static int test_invalid_rtc_restores_record_without_boot_rewrite(void)
{
    system_settings_t stored;
    system_settings_t live;
    system_settings_t effective;
    uint32_t writes_before_start;
    int failures = 0;

    reset_fixture();
    stored = make_settings(45U, 15U);
    live = make_settings(50U, 46U);
    failures += expect_i32("invalid_rtc_seed",
        system_settings_save(&stored, 30U), SYSTEM_SETTINGS_OK);
    writes_before_start = g_write_calls;
    system_settings_service_init();
    failures += expect_i32("invalid_rtc_start",
        system_settings_runtime_start(0U, &live, &effective,
                                      2000U, 30U), SYSTEM_SETTINGS_OK);
    failures += expect_u32("invalid_rtc_restored_time", effective.rtc_second, 15U);
    failures += expect_u32("invalid_rtc_no_rewrite", g_write_calls,
                           writes_before_start);
    return failures;
}

static int test_empty_storage_establishes_boot_record(void)
{
    system_settings_t live;
    system_settings_t effective;
    int failures = 0;

    reset_fixture();
    live = make_settings(50U, 47U);
    failures += expect_i32("empty_runtime_start",
        system_settings_runtime_start(0U, &live, &effective,
                                      3000U, 30U), SYSTEM_SETTINGS_OK);
    failures += expect_u32("empty_runtime_value", effective.rtc_second, 47U);
    failures += expect_u32("empty_runtime_writes", g_write_calls, 3U);
    return failures;
}

static int test_hourly_and_requested_runtime_saves(void)
{
    system_settings_t stored;
    system_settings_t live;
    system_settings_t effective;
    uint32_t writes_after_start;
    int failures = 0;

    reset_fixture();
    stored = make_settings(46U, 16U);
    live = stored;
    failures += expect_i32("poll_seed",
        system_settings_save(&stored, 30U), SYSTEM_SETTINGS_OK);
    system_settings_service_init();
    failures += expect_i32("poll_start",
        system_settings_runtime_start(0U, &live, &effective,
                                      4000U, 30U), SYSTEM_SETTINGS_OK);
    writes_after_start = g_write_calls;
    live.rtc_second = 17U;
    failures += expect_i32("poll_before_hour",
        system_settings_runtime_poll(&live, 3603999U, 30U),
        SYSTEM_SETTINGS_OK);
    failures += expect_u32("poll_before_hour_writes", g_write_calls,
                           writes_after_start);
    failures += expect_i32("poll_at_hour",
        system_settings_runtime_poll(&live, 3604000U, 30U),
        SYSTEM_SETTINGS_OK);
    failures += expect_u32("poll_hour_writes", g_write_calls,
                           writes_after_start + 3U);

    system_settings_request_save();
    failures += expect_i32("poll_requested",
        system_settings_runtime_poll(&live, 3604001U, 30U),
        SYSTEM_SETTINGS_OK);
    failures += expect_u32("poll_requested_writes", g_write_calls,
                           writes_after_start + 6U);
    return failures;
}

static int test_configuration_changes_are_coalesced_for_two_seconds(void)
{
    system_settings_t stored;
    system_settings_t live;
    system_settings_t effective;
    system_settings_t current;
    uint32_t writes_after_start;
    int failures = 0;

    reset_fixture();
    stored = make_settings(47U, 17U);
    live = stored;
    failures += expect_i32("coalesce_seed",
        system_settings_save(&stored, 30U), SYSTEM_SETTINGS_OK);
    system_settings_service_init();
    failures += expect_i32("coalesce_start",
        system_settings_runtime_start(0U, &live, &effective,
                                      5000U, 30U), SYSTEM_SETTINGS_OK);
    writes_after_start = g_write_calls;
    system_settings_update_brightness(60U, 5100U);
    system_settings_update_brightness(70U, 5200U);
    system_settings_update_volume(80U, 5200U);
    system_settings_update_camera(0U, 1U, 1U, 5200U);
    system_settings_get(&current);
    failures += expect_u32("coalesce_brightness", current.brightness_percent, 70U);
    failures += expect_u32("coalesce_volume", current.volume_percent, 80U);
    failures += expect_u32("coalesce_mirror", current.camera_mirror, 0U);
    failures += expect_u32("coalesce_flip", current.camera_flip, 1U);
    failures += expect_u32("coalesce_flash", current.camera_flash_mode, 1U);

    live = current;
    failures += expect_i32("coalesce_before_deadline",
        system_settings_runtime_poll(&live, 7199U, 30U), SYSTEM_SETTINGS_OK);
    failures += expect_u32("coalesce_no_early_write", g_write_calls,
                           writes_after_start);
    failures += expect_i32("coalesce_at_deadline",
        system_settings_runtime_poll(&live, 7200U, 30U), SYSTEM_SETTINGS_OK);
    failures += expect_u32("coalesce_one_record", g_write_calls,
                           writes_after_start + 3U);
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_empty_storage_returns_not_found();
    failures += test_one_failed_slot_read_is_not_reported_as_empty();
    failures += test_save_alternates_slots_and_loads_newest();
    failures += test_corrupt_or_uncommitted_new_slot_falls_back();
    failures += test_interrupted_save_keeps_previous_slot();
    failures += test_invalid_fields_are_rejected_without_writes();
    failures += test_sequence_wrap_selects_zero_after_uint32_max();
    failures += test_valid_rtc_keeps_saved_preferences_and_checkpoints_live_time();
    failures += test_invalid_rtc_restores_record_without_boot_rewrite();
    failures += test_empty_storage_establishes_boot_record();
    failures += test_hourly_and_requested_runtime_saves();
    failures += test_configuration_changes_are_coalesced_for_two_seconds();
    if (failures != 0)
    {
        printf("FAIL system_settings_service failures=%d\n", failures);
        return 1;
    }
    printf("PASS system_settings_service\n");
    return 0;
}
