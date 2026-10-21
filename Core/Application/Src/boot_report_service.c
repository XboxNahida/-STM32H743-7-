#include "boot_report_service.h"

#include <stdio.h>
#include <string.h>

#include "boot_probe_service.h"

#if defined(BOOT_REPORT_HOST_TEST)
extern int boot_report_test_printf(const char *format, ...);
#define BOOT_REPORT_PRINTF boot_report_test_printf
#else
#define BOOT_REPORT_PRINTF printf
#endif

static boot_report_snapshot_t g_report;
static uint8_t g_probe_captured;
static uint8_t g_sd_reported;
static uint8_t g_assets_reported;
static uint8_t g_display_reported;
static uint8_t g_required_probe_failed;
static uint32_t g_asset_errors;

static uint8_t boot_report_is_ready(const boot_probe_status_t *status)
{
    return ((status != NULL) &&
            (status->result == BOOT_PROBE_OK) &&
            (status->ready != 0U)) ? 1U : 0U;
}

static void boot_report_recompute_health(void)
{
    uint32_t warnings = 0U;
    uint32_t failures = 0U;

    if ((g_probe_captured != 0U) && (g_required_probe_failed != 0U))
    {
        failures++;
    }
    if ((g_probe_captured != 0U) &&
        ((g_report.nand_id == 0U) || (g_report.pcf8574_ready == 0U) ||
         (g_report.jpeg_ready == 0U) || (g_report.audio_ready == 0U)))
    {
        warnings++;
    }
    if ((g_sd_reported != 0U) && (g_report.sd_ready == 0U))
    {
        warnings++;
    }
    if (g_assets_reported != 0U)
    {
        if ((g_asset_errors != 0U) || (g_report.wallpaper_ready == 0U) ||
            (g_report.images_ready != 3U) || (g_report.fonts_ready != 6U))
        {
            failures++;
        }
    }
    if (g_display_reported != 0U)
    {
        if ((g_report.ltdc_ready == 0U) ||
            (g_report.sdram_ready == 0U) ||
            (g_report.lvgl_ready == 0U))
        {
            failures++;
        }
    }

    g_report.warning_count = warnings;
    g_report.failure_count = failures;
}

static const char *boot_report_touch_name(uint32_t id)
{
    switch (id)
    {
        case 0x39313100UL:
            return "gt911";
        case 0x39313437UL:
            return "gt9147";
        case 0x31313538UL:
            return "gt1158";
        case 0x39323731UL:
            return "gt9271";
        default:
            return "gt9xxx";
    }
}

static uint8_t boot_report_id_equal(const char *left, const char *right)
{
    if ((left == NULL) || (right == NULL))
    {
        return 0U;
    }
    return (strcmp(left, right) == 0) ? 1U : 0U;
}

void boot_report_init(uint32_t start_ms)
{
    memset(&g_report, 0, sizeof(g_report));
    g_report.start_ms = start_ms;
    g_probe_captured = 0U;
    g_sd_reported = 0U;
    g_assets_reported = 0U;
    g_display_reported = 0U;
    g_required_probe_failed = 0U;
    g_asset_errors = 0U;
}

void boot_report_capture_probe(void)
{
    boot_probe_summary_t summary;
    boot_probe_status_t status;
    uint32_t index;
    uint8_t required_internal_ready = 1U;
    uint8_t sai_ready = 0U;
    uint8_t spdif_ready = 0U;

    memset(&summary, 0, sizeof(summary));
    boot_probe_get_summary(&summary);
    g_report.probe_ready = summary.ready;
    g_report.probe_expected = summary.total - summary.unsupported;
    g_report.optional_skipped = summary.unsupported;
    g_required_probe_failed = 0U;

    for (index = 0U;
         (index < summary.total) && (index < BOOT_PROBE_MAX_ENTRIES);
         index++)
    {
        if (boot_probe_get_status(index, &status) != BOOT_PROBE_OK)
        {
            continue;
        }
        if ((status.required != 0U) &&
            (status.result != BOOT_PROBE_OK) &&
            (status.result != BOOT_PROBE_UNSUPPORTED))
        {
            g_required_probe_failed = 1U;
        }
        if ((status.required != 0U) &&
            (status.kind == BOOT_PROBE_KIND_INTERNAL) &&
            (boot_report_is_ready(&status) == 0U))
        {
            required_internal_ready = 0U;
        }
        if (boot_report_id_equal(status.id, "qspi_flash") != 0U)
        {
            g_report.qspi_id = (uint16_t)status.detail0;
        }
        else if (boot_report_id_equal(status.id, "nand") != 0U)
        {
            g_report.nand_id = status.detail0;
        }
        else if (boot_report_id_equal(status.id, "gt9xxx") != 0U)
        {
            g_report.touch_id = status.detail0;
            g_report.touch_ready = boot_report_is_ready(&status);
        }
        else if (boot_report_id_equal(status.id, "pcf8574") != 0U)
        {
            g_report.pcf8574_ready = boot_report_is_ready(&status);
        }
        else if (boot_report_id_equal(status.id, "jpeg") != 0U)
        {
            g_report.jpeg_ready = boot_report_is_ready(&status);
        }
        else if (boot_report_id_equal(status.id, "sai1") != 0U)
        {
            sai_ready = boot_report_is_ready(&status);
        }
        else if (boot_report_id_equal(status.id, "spdifrx1") != 0U)
        {
            spdif_ready = boot_report_is_ready(&status);
        }
        else if (boot_report_id_equal(status.id, "ltdc") != 0U)
        {
            g_report.ltdc_ready = boot_report_is_ready(&status);
        }
        else if (boot_report_id_equal(status.id, "sdram") != 0U)
        {
            g_report.sdram_ready = boot_report_is_ready(&status);
        }
    }

    g_report.core_ready = required_internal_ready;
    g_report.audio_ready = ((sai_ready != 0U) && (spdif_ready != 0U)) ? 1U : 0U;
    g_probe_captured = 1U;
    boot_report_recompute_health();
}

void boot_report_set_sd_ready(uint8_t ready)
{
    g_report.sd_ready = (ready != 0U) ? 1U : 0U;
    g_sd_reported = 1U;
    boot_report_recompute_health();
}

void boot_report_set_assets(uint8_t wallpaper_ready,
                            uint8_t images_ready,
                            uint8_t fonts_ready,
                            uint8_t copied,
                            uint32_t errors)
{
    g_report.wallpaper_ready = (wallpaper_ready != 0U) ? 1U : 0U;
    g_report.images_ready = images_ready;
    g_report.fonts_ready = fonts_ready;
    g_report.assets_copied = copied;
    g_asset_errors = errors;
    g_assets_reported = 1U;
    boot_report_recompute_health();
}

void boot_report_set_display(uint8_t ltdc_ready,
                             uint8_t sdram_ready,
                             uint8_t lvgl_ready)
{
    g_report.ltdc_ready = (ltdc_ready != 0U) ? 1U : 0U;
    g_report.sdram_ready = (sdram_ready != 0U) ? 1U : 0U;
    g_report.lvgl_ready = (lvgl_ready != 0U) ? 1U : 0U;
    g_display_reported = 1U;
    boot_report_recompute_health();
}

const boot_report_snapshot_t *boot_report_get_snapshot(void)
{
    return &g_report;
}

void boot_report_print(uint32_t now_ms)
{
    const char *core_state = (g_report.core_ready != 0U) ? "OK" : "FAIL";
    const char *storage_state;
    const char *assets_state;
    const char *display_state;
    const char *input_state;
    const char *media_state;
    const char *system_state;

    storage_state = (g_report.qspi_id == 0U) ? "FAIL" :
                    (((g_report.sd_ready == 0U) ||
                      (g_report.nand_id == 0U)) ? "WARN" : "OK");
    assets_state = ((g_asset_errors != 0U) ||
                    (g_report.wallpaper_ready == 0U) ||
                    (g_report.images_ready != 3U) ||
                    (g_report.fonts_ready != 6U)) ? "FAIL" : "OK";
    display_state = ((g_report.ltdc_ready != 0U) &&
                     (g_report.sdram_ready != 0U) &&
                     (g_report.lvgl_ready != 0U)) ? "OK" : "FAIL";
    input_state = (g_report.touch_ready == 0U) ? "FAIL" :
                  ((g_report.pcf8574_ready == 0U) ? "WARN" : "OK");
    media_state = ((g_report.jpeg_ready != 0U) &&
                   (g_report.audio_ready != 0U)) ? "OK" : "WARN";
    system_state = ((g_report.warning_count == 0U) &&
                    (g_report.failure_count == 0U)) ? "READY" : "DEGRADED";

    BOOT_REPORT_PRINTF("[BOOT] core      %s  probe=%lu/%lu\r\n",
                       core_state,
                       (unsigned long)g_report.probe_ready,
                       (unsigned long)g_report.probe_expected);
    BOOT_REPORT_PRINTF("[BOOT] storage   %s  sd=%s nand=%08lX qspi=%04X\r\n",
                       storage_state,
                       (g_report.sd_ready != 0U) ? "ready" : "absent",
                       (unsigned long)g_report.nand_id,
                       (unsigned int)g_report.qspi_id);
    BOOT_REPORT_PRINTF("[BOOT] assets    %s  wallpaper=%s images=%u/3 fonts=%u/6 copied=%u\r\n",
                       assets_state,
                       (g_report.wallpaper_ready != 0U) ? "ready" : "failed",
                       (unsigned int)g_report.images_ready,
                       (unsigned int)g_report.fonts_ready,
                       (unsigned int)g_report.assets_copied);
    BOOT_REPORT_PRINTF("[BOOT] display   %s  ltdc=%s sdram=%s lvgl=%s\r\n",
                       display_state,
                       (g_report.ltdc_ready != 0U) ? "ready" : "failed",
                       (g_report.sdram_ready != 0U) ? "ready" : "failed",
                       (g_report.lvgl_ready != 0U) ? "ready" : "failed");
    BOOT_REPORT_PRINTF("[BOOT] input     %s  %s=%s pcf8574=%s\r\n",
                       input_state,
                       boot_report_touch_name(g_report.touch_id),
                       (g_report.touch_ready != 0U) ? "ready" : "failed",
                       (g_report.pcf8574_ready != 0U) ? "ready" : "absent");
    BOOT_REPORT_PRINTF("[BOOT] media     %s  jpeg=%s audio=%s\r\n",
                       media_state,
                       (g_report.jpeg_ready != 0U) ? "ready" : "failed",
                       (g_report.audio_ready != 0U) ? "ready" : "failed");
    BOOT_REPORT_PRINTF("[BOOT] optional  SKIP count=%lu\r\n",
                       (unsigned long)g_report.optional_skipped);
    BOOT_REPORT_PRINTF("[BOOT] system    %s elapsed_ms=%lu\r\n",
                       system_state,
                       (unsigned long)(now_ms - g_report.start_ms));
}
