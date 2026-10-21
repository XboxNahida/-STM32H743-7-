#ifndef SYSTEM_SETTINGS_SERVICE_H
#define SYSTEM_SETTINGS_SERVICE_H

#include <stdint.h>

#define SYSTEM_SETTINGS_SLOT_NONE      0xFFU
#define SYSTEM_SETTINGS_COMMIT_MARKER  0xA5U

typedef enum
{
    SYSTEM_SETTINGS_OK = 0,
    SYSTEM_SETTINGS_NOT_FOUND,
    SYSTEM_SETTINGS_STORAGE_ERROR,
    SYSTEM_SETTINGS_INVALID_ARG
} system_settings_result_t;

typedef struct
{
    uint8_t brightness_percent;
    uint8_t volume_percent;
    uint8_t camera_mirror;
    uint8_t camera_flip;
    uint8_t camera_flash_mode;
    uint8_t rtc_year;
    uint8_t rtc_month;
    uint8_t rtc_day;
    uint8_t rtc_hour;
    uint8_t rtc_minute;
    uint8_t rtc_second;
} system_settings_t;

typedef struct
{
    uint8_t valid;
    uint8_t active_slot;
    int32_t result;
    uint32_t sequence;
    uint32_t load_count;
    uint32_t save_count;
} system_settings_status_t;

void system_settings_service_init(void);
int system_settings_load(system_settings_t *settings, uint32_t timeout_ms);
int system_settings_save(const system_settings_t *settings,
                         uint32_t timeout_ms);
int system_settings_runtime_start(uint8_t rtc_backup_valid,
                                  const system_settings_t *live_settings,
                                  system_settings_t *effective_settings,
                                  uint32_t now_ms,
                                  uint32_t timeout_ms);
int system_settings_runtime_poll(const system_settings_t *live_settings,
                                 uint32_t now_ms,
                                 uint32_t timeout_ms);
void system_settings_request_save(void);
void system_settings_update_brightness(uint8_t percent, uint32_t now_ms);
void system_settings_update_volume(uint8_t percent, uint32_t now_ms);
void system_settings_update_camera(uint8_t mirror, uint8_t flip,
                                   uint8_t flash_mode, uint32_t now_ms);
void system_settings_get(system_settings_t *settings);
void system_settings_get_status(system_settings_status_t *status);
void system_settings_print_status(void);

#endif
