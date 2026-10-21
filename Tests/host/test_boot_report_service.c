#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "boot_probe_service.h"
#include "boot_report_service.h"

#define TEST_STATUS_COUNT 32U
#define OUTPUT_SIZE 4096U

static boot_probe_status_t g_probe_statuses[TEST_STATUS_COUNT];
static boot_probe_summary_t g_probe_summary;
static char g_output[OUTPUT_SIZE];
static size_t g_output_length;

int boot_report_test_printf(const char *format, ...)
{
    va_list args;
    int written;
    size_t remaining;

    if (g_output_length >= sizeof(g_output))
    {
        return 0;
    }
    remaining = sizeof(g_output) - g_output_length;
    va_start(args, format);
    written = vsnprintf(&g_output[g_output_length], remaining, format, args);
    va_end(args);
    if (written > 0)
    {
        if ((size_t)written >= remaining)
        {
            g_output_length = sizeof(g_output) - 1U;
        }
        else
        {
            g_output_length += (size_t)written;
        }
    }
    return written;
}

void boot_probe_get_summary(boot_probe_summary_t *summary)
{
    if (summary != NULL)
    {
        *summary = g_probe_summary;
    }
}

int boot_probe_get_status(uint32_t index, boot_probe_status_t *status)
{
    if ((status == NULL) || (index >= TEST_STATUS_COUNT))
    {
        return BOOT_PROBE_ERROR;
    }
    *status = g_probe_statuses[index];
    return BOOT_PROBE_OK;
}

static void set_probe(uint32_t index,
                      const char *id,
                      boot_probe_kind_t kind,
                      uint8_t required,
                      int32_t result,
                      uint32_t detail0)
{
    boot_probe_status_t *status = &g_probe_statuses[index];

    memset(status, 0, sizeof(*status));
    status->id = id;
    status->kind = kind;
    status->required = required;
    status->result = result;
    if (result == BOOT_PROBE_OK)
    {
        status->present = 1U;
        status->ready = 1U;
    }
    status->detail0 = detail0;
}

static void reset_fixture(void)
{
    static const char *const ready_ids[27] =
    {
        "uart1", "i2c2", "sdmmc1", "qspi", "fmc_sdram", "ltdc",
        "dma2d", "jpeg", "rtc", "adc1", "adc3", "sai1", "spdifrx1",
        "tim1", "tim2", "tim3", "tim6", "gt9xxx", "pcf8574", "sd",
        "qspi_flash", "nand", "sdram", "imu", "pressure", "ap3216c",
        "24c02"
    };
    static const char *const skipped_ids[5] =
    {
        "lcd_panel", "es8388", "ov5640", "dht11", "ds18b20"
    };
    uint32_t index;

    memset(g_probe_statuses, 0, sizeof(g_probe_statuses));
    memset(&g_probe_summary, 0, sizeof(g_probe_summary));
    memset(g_output, 0, sizeof(g_output));
    g_output_length = 0U;

    for (index = 0U; index < 27U; index++)
    {
        set_probe(index, ready_ids[index], BOOT_PROBE_KIND_INTERNAL,
                  0U, BOOT_PROBE_OK, 0U);
    }
    for (index = 0U; index < 5U; index++)
    {
        set_probe(27U + index, skipped_ids[index], BOOT_PROBE_KIND_SENSOR,
                  0U, BOOT_PROBE_UNSUPPORTED, 0U);
    }

    g_probe_statuses[3].required = 1U;
    g_probe_statuses[4].required = 1U;
    g_probe_statuses[5].required = 1U;
    g_probe_statuses[6].required = 1U;
    g_probe_statuses[7].detail0 = 1U;
    g_probe_statuses[11].kind = BOOT_PROBE_KIND_AUDIO;
    g_probe_statuses[12].kind = BOOT_PROBE_KIND_AUDIO;
    g_probe_statuses[17].kind = BOOT_PROBE_KIND_I2C;
    g_probe_statuses[17].required = 1U;
    g_probe_statuses[17].detail0 = 0x39313100UL;
    g_probe_statuses[18].kind = BOOT_PROBE_KIND_I2C;
    g_probe_statuses[18].detail0 = 0x40U;
    g_probe_statuses[20].kind = BOOT_PROBE_KIND_STORAGE;
    g_probe_statuses[20].required = 1U;
    g_probe_statuses[20].detail0 = 0xEF40U;
    g_probe_statuses[21].kind = BOOT_PROBE_KIND_STORAGE;
    g_probe_statuses[21].detail0 = 0xDC909556UL;
    g_probe_statuses[22].kind = BOOT_PROBE_KIND_STORAGE;
    g_probe_statuses[22].required = 1U;

    g_probe_summary.total = TEST_STATUS_COUNT;
    g_probe_summary.ready = 27U;
    g_probe_summary.unsupported = 5U;
}

static int expect_true(const char *name, int condition)
{
    if (condition == 0)
    {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

static uint32_t count_lines(const char *text)
{
    uint32_t count = 0U;

    while (*text != '\0')
    {
        if (*text == '\n')
        {
            count++;
        }
        text++;
    }
    return count;
}

static int test_ready_report(void)
{
    boot_report_snapshot_t before;
    boot_report_snapshot_t after;
    int failures = 0;

    reset_fixture();
    boot_report_init(100U);
    boot_report_capture_probe();
    boot_report_set_sd_ready(1U);
    boot_report_set_assets(1U, 3U, 6U, 1U, 0U);
    boot_report_set_display(1U, 1U, 1U);
    before = *boot_report_get_snapshot();
    boot_report_print(1334U);
    after = *boot_report_get_snapshot();

    failures += expect_true(
        "core_line",
        strstr(g_output, "[BOOT] core      OK  probe=27/27\r\n") != NULL);
    failures += expect_true(
        "storage_line",
        strstr(g_output,
               "[BOOT] storage   OK  sd=ready nand=DC909556 qspi=EF40\r\n") != NULL);
    failures += expect_true(
        "assets_line",
        strstr(g_output,
               "[BOOT] assets    OK  wallpaper=ready images=3/3 fonts=6/6 copied=1\r\n") != NULL);
    failures += expect_true(
        "display_line",
        strstr(g_output,
               "[BOOT] display   OK  ltdc=ready sdram=ready lvgl=ready\r\n") != NULL);
    failures += expect_true(
        "input_line",
        strstr(g_output,
               "[BOOT] input     OK  gt911=ready pcf8574=ready\r\n") != NULL);
    failures += expect_true(
        "media_line",
        strstr(g_output,
               "[BOOT] media     OK  jpeg=ready audio=ready\r\n") != NULL);
    failures += expect_true(
        "optional_line",
        strstr(g_output, "[BOOT] optional  SKIP count=5\r\n") != NULL);
    failures += expect_true(
        "system_line",
        strstr(g_output,
               "[BOOT] system    READY elapsed_ms=1234\r\n") != NULL);
    failures += expect_true("eight_lines", count_lines(g_output) == 8U);
    failures += expect_true("print_no_side_effect",
                            memcmp(&before, &after, sizeof(before)) == 0);
    return failures;
}

static int test_degraded_report(void)
{
    const boot_report_snapshot_t *snapshot;
    int failures = 0;

    reset_fixture();
    boot_report_init(10U);
    boot_report_capture_probe();
    boot_report_set_sd_ready(0U);
    boot_report_set_assets(1U, 2U, 5U, 0U, 2U);
    boot_report_set_display(1U, 1U, 1U);
    snapshot = boot_report_get_snapshot();
    boot_report_print(20U);

    failures += expect_true(
        "storage_warn",
        strstr(g_output, "[BOOT] storage   WARN  sd=absent") != NULL);
    failures += expect_true(
        "assets_fail",
        strstr(g_output, "[BOOT] assets    FAIL  wallpaper=ready images=2/3 fonts=5/6") != NULL);
    failures += expect_true(
        "system_degraded",
        strstr(g_output, "[BOOT] system    DEGRADED elapsed_ms=10") != NULL);
    failures += expect_true("warning_count", snapshot->warning_count == 1U);
    failures += expect_true("failure_count", snapshot->failure_count == 1U);
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_ready_report();
    failures += test_degraded_report();
    if (failures != 0)
    {
        printf("FAIL boot_report_service failures=%d\n", failures);
        return 1;
    }
    printf("PASS boot_report_service\n");
    return 0;
}
