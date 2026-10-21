#ifndef BOOT_REPORT_SERVICE_H
#define BOOT_REPORT_SERVICE_H

#include <stdint.h>

typedef struct
{
    uint32_t start_ms;
    uint32_t probe_ready;
    uint32_t probe_expected;
    uint32_t optional_skipped;
    uint8_t core_ready;
    uint8_t sd_ready;
    uint32_t nand_id;
    uint16_t qspi_id;
    uint8_t wallpaper_ready;
    uint8_t images_ready;
    uint8_t fonts_ready;
    uint8_t assets_copied;
    uint8_t ltdc_ready;
    uint8_t sdram_ready;
    uint8_t lvgl_ready;
    uint32_t touch_id;
    uint8_t touch_ready;
    uint8_t pcf8574_ready;
    uint8_t jpeg_ready;
    uint8_t audio_ready;
    uint32_t warning_count;
    uint32_t failure_count;
} boot_report_snapshot_t;

void boot_report_init(uint32_t start_ms);
void boot_report_capture_probe(void);
void boot_report_set_sd_ready(uint8_t ready);
void boot_report_set_assets(uint8_t wallpaper_ready,
                            uint8_t images_ready,
                            uint8_t fonts_ready,
                            uint8_t copied,
                            uint32_t errors);
void boot_report_set_display(uint8_t ltdc_ready,
                             uint8_t sdram_ready,
                             uint8_t lvgl_ready);
const boot_report_snapshot_t *boot_report_get_snapshot(void);
void boot_report_print(uint32_t now_ms);

#endif
