#include <stdio.h>
#include <string.h>

#include "boot_progress_service.h"

static int expect_true(const char *name, int condition)
{
    if (condition == 0)
    {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

static void fill_text(char *text, size_t size, char value)
{
    size_t index;

    for (index = 0U; index + 1U < size; index++)
    {
        text[index] = value;
    }
    text[size - 1U] = '\0';
}

int main(void)
{
    static const char *const expected_names[BOOT_DEVICE_COUNT] =
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
    boot_progress_snapshot_t snapshot;
    char long_text[192];
    unsigned int index;
    int failures = 0;

    boot_progress_init();
    boot_progress_get_snapshot(&snapshot);
    failures += expect_true("initial_total_zero",
                            snapshot.total_percent == 0U);
    failures += expect_true("initial_item_zero",
                            snapshot.item_percent == 0U);
    failures += expect_true("initial_completed_zero",
                            snapshot.completed_items == 0U);
    failures += expect_true("initial_first_error_clear",
                            snapshot.first_error_valid == 0U);

    for (index = 0U; index < (unsigned int)BOOT_DEVICE_COUNT; index++)
    {
        failures += expect_true("fixed_device_name",
                                strcmp(snapshot.devices[index].name,
                                       expected_names[index]) == 0);
        failures += expect_true("device_initially_pending",
                                snapshot.devices[index].updated == 0U);
    }

    boot_progress_set_current("Hardware", "Initializing QSPI", 140U, 180U);
    boot_progress_set_elapsed_ms(1234U);
    boot_progress_get_snapshot(&snapshot);
    failures += expect_true("total_clamped",
                            snapshot.total_percent == 100U);
    failures += expect_true("item_clamped",
                            snapshot.item_percent == 100U);
    failures += expect_true("stage_copied",
                            strcmp(snapshot.stage, "Hardware") == 0);
    failures += expect_true("detail_copied",
                            strcmp(snapshot.detail, "Initializing QSPI") == 0);
    failures += expect_true("elapsed_recorded",
                            snapshot.elapsed_ms == 1234U);

    boot_progress_set_device(BOOT_DEVICE_QSPI,
                             BOOT_PROGRESS_SUCCESS,
                             "Ready");
    boot_progress_get_snapshot(&snapshot);
    failures += expect_true("qspi_updated",
                            snapshot.devices[BOOT_DEVICE_QSPI].updated == 1U);
    failures += expect_true("qspi_success",
                            snapshot.devices[BOOT_DEVICE_QSPI].level ==
                            BOOT_PROGRESS_SUCCESS);
    failures += expect_true("qspi_ready_detail",
                            strcmp(snapshot.devices[BOOT_DEVICE_QSPI].detail,
                                   "Ready") == 0);

    boot_progress_set_device(BOOT_DEVICE_QSPI,
                             BOOT_PROGRESS_WARNING,
                             "Busy");
    boot_progress_complete_item("I2C2",
                                BOOT_PROGRESS_SUCCESS,
                                "Ready",
                                15U);
    boot_progress_get_snapshot(&snapshot);
    failures += expect_true("device_updated_in_place",
                            strcmp(snapshot.devices[BOOT_DEVICE_QSPI].detail,
                                   "Busy") == 0);
    failures += expect_true("internal_item_not_device",
                            snapshot.devices[BOOT_DEVICE_SD].updated == 0U);
    failures += expect_true("completed_item_counted",
                            snapshot.completed_items == 1U);
    failures += expect_true("complete_sets_item_100",
                            snapshot.item_percent == 100U);

    boot_progress_complete_item("Font 20",
                                BOOT_PROGRESS_ERROR,
                                "Parse failed",
                                90U);
    boot_progress_complete_item("SD Card",
                                BOOT_PROGRESS_ERROR,
                                "Mount failed",
                                91U);
    boot_progress_get_snapshot(&snapshot);
    failures += expect_true("first_error_latched",
                            snapshot.first_error_valid == 1U);
    failures += expect_true("first_error_keeps_first_name",
                            strstr(snapshot.first_error, "Font 20") != NULL);
    failures += expect_true("first_error_keeps_first_detail",
                            strstr(snapshot.first_error, "Parse failed") != NULL);
    failures += expect_true("later_error_not_overwrite",
                            strstr(snapshot.first_error, "SD Card") == NULL);

    fill_text(long_text, sizeof(long_text), 'X');
    boot_progress_init();
    boot_progress_set_current(long_text, long_text, 1U, 2U);
    boot_progress_complete_item(long_text,
                                BOOT_PROGRESS_ERROR,
                                long_text,
                                3U);
    boot_progress_set_device(BOOT_DEVICE_AT24C02,
                             BOOT_PROGRESS_WARNING,
                             long_text);
    boot_progress_get_snapshot(&snapshot);
    failures += expect_true("stage_nul_terminated",
                            snapshot.stage[BOOT_PROGRESS_STAGE_TEXT_SIZE - 1U] == '\0');
    failures += expect_true("detail_nul_terminated",
                            snapshot.detail[BOOT_PROGRESS_DETAIL_TEXT_SIZE - 1U] == '\0');
    failures += expect_true("item_nul_terminated",
                            snapshot.last_item[BOOT_PROGRESS_ITEM_TEXT_SIZE - 1U] == '\0');
    failures += expect_true("error_nul_terminated",
                            snapshot.first_error[BOOT_PROGRESS_ERROR_TEXT_SIZE - 1U] == '\0');
    failures += expect_true("device_detail_nul_terminated",
                            snapshot.devices[BOOT_DEVICE_AT24C02]
                                .detail[BOOT_PROGRESS_DEVICE_DETAIL_SIZE - 1U] == '\0');

    boot_progress_set_device((boot_progress_device_id_t)BOOT_DEVICE_COUNT,
                             BOOT_PROGRESS_ERROR,
                             "invalid");
    boot_progress_get_snapshot(NULL);

    if (failures == 0)
    {
        printf("PASS boot_progress_service\n");
        return 0;
    }
    printf("FAIL boot_progress_service failures=%d\n", failures);
    return 1;
}
