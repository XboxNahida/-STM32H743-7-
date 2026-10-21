#ifndef BOOT_PROGRESS_SERVICE_H
#define BOOT_PROGRESS_SERVICE_H

#include <stdint.h>

#define BOOT_PROGRESS_STAGE_TEXT_SIZE       32U
#define BOOT_PROGRESS_DETAIL_TEXT_SIZE      80U
#define BOOT_PROGRESS_ITEM_TEXT_SIZE        32U
#define BOOT_PROGRESS_ERROR_TEXT_SIZE       96U
#define BOOT_PROGRESS_DEVICE_NAME_SIZE      16U
#define BOOT_PROGRESS_DEVICE_DETAIL_SIZE    48U

typedef enum
{
    BOOT_PROGRESS_SUCCESS = 0,
    BOOT_PROGRESS_WARNING,
    BOOT_PROGRESS_ERROR
} boot_progress_level_t;

typedef enum
{
    BOOT_DEVICE_SD = 0,
    BOOT_DEVICE_QSPI,
    BOOT_DEVICE_NAND,
    BOOT_DEVICE_GT911,
    BOOT_DEVICE_PCF8574,
    BOOT_DEVICE_OV5640,
    BOOT_DEVICE_ES8388,
    BOOT_DEVICE_SH3001,
    BOOT_DEVICE_AP3216C,
    BOOT_DEVICE_AT24C02,
    BOOT_DEVICE_COUNT
} boot_progress_device_id_t;

typedef struct
{
    char name[BOOT_PROGRESS_DEVICE_NAME_SIZE];
    char detail[BOOT_PROGRESS_DEVICE_DETAIL_SIZE];
    boot_progress_level_t level;
    uint8_t updated;
} boot_progress_device_t;

typedef struct
{
    char stage[BOOT_PROGRESS_STAGE_TEXT_SIZE];
    char detail[BOOT_PROGRESS_DETAIL_TEXT_SIZE];
    char last_item[BOOT_PROGRESS_ITEM_TEXT_SIZE];
    char first_error[BOOT_PROGRESS_ERROR_TEXT_SIZE];
    boot_progress_device_t devices[BOOT_DEVICE_COUNT];
    uint32_t elapsed_ms;
    uint32_t completed_items;
    uint8_t total_percent;
    uint8_t item_percent;
    uint8_t first_error_valid;
} boot_progress_snapshot_t;

void boot_progress_init(void);
void boot_progress_set_current(const char *stage,
                               const char *detail,
                               uint8_t total_percent,
                               uint8_t item_percent);
void boot_progress_complete_item(const char *name,
                                 boot_progress_level_t level,
                                 const char *detail,
                                 uint8_t total_percent);
void boot_progress_set_device(boot_progress_device_id_t id,
                              boot_progress_level_t level,
                              const char *detail);
void boot_progress_set_elapsed_ms(uint32_t elapsed_ms);
void boot_progress_get_snapshot(boot_progress_snapshot_t *snapshot);

#endif
