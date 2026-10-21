#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_source(const char *path)
{
    FILE *file;
    long length;
    char *buffer;

    file = fopen(path, "rb");
    if (file == NULL)
    {
        return NULL;
    }
    if ((fseek(file, 0L, SEEK_END) != 0) ||
        ((length = ftell(file)) < 0L) ||
        (fseek(file, 0L, SEEK_SET) != 0))
    {
        (void)fclose(file);
        return NULL;
    }

    buffer = (char *)malloc((size_t)length + 1U);
    if (buffer == NULL)
    {
        (void)fclose(file);
        return NULL;
    }
    if (fread(buffer, 1U, (size_t)length, file) != (size_t)length)
    {
        free(buffer);
        (void)fclose(file);
        return NULL;
    }
    buffer[length] = '\0';
    (void)fclose(file);
    return buffer;
}

static unsigned int count_text(const char *text, const char *needle)
{
    unsigned int count = 0U;
    size_t length = strlen(needle);

    while ((text = strstr(text, needle)) != NULL)
    {
        count++;
        text += length;
    }
    return count;
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

static int strings_appear_in_order(const char *text,
                                   const char *const strings[],
                                   size_t count)
{
    size_t index;
    const char *cursor = text;

    for (index = 0U; index < count; index++)
    {
        cursor = strstr(cursor, strings[index]);
        if (cursor == NULL)
        {
            return 0;
        }
        cursor += strlen(strings[index]);
    }
    return 1;
}

int main(void)
{
    char *header = read_source("Core/Peripheral/Inc/norflash.h");
    char *source = read_source("Core/Peripheral/Src/norflash.c");
    char *custom_header = read_source("lvgl_GUI/custom/custom.h");
    char *custom_source = read_source("lvgl_GUI/custom/custom.c");
    char *font_source = read_source("lvgl_GUI/custom/font_bridge.c");
    char *sd_source = read_source("Core/Src/sdmmc.c");
    char *main_source = read_source("Core/Src/main.c");
    char *sdcard_source = read_source("Core/Peripheral/Src/sdcard.c");
    char *gt_source = read_source("Core/Src/gt9xxx.c");
    static const char *const lvgl_status_order[] =
    {
        "memset(&g_lvgl_boot_status, 0, sizeof(g_lvgl_boot_status));",
        "g_lvgl_boot_status.sdram_ready = 1U;",
        "lv_init();",
        "g_lvgl_boot_status.lvgl_ready = 1U;",
        "lv_port_disp_init();",
        "g_lvgl_boot_status.display_ready = 1U;",
        "lv_port_indev_init();",
        "g_lvgl_boot_status.input_ready = 1U;",
        "setup_ui(&guider_ui);",
        "g_lvgl_boot_status.ui_ready = 1U;"
    };
    static const char *const lvgl_bootstrap_order[] =
    {
        "memset(&g_lvgl_boot_status, 0, sizeof(g_lvgl_boot_status));",
        "g_lvgl_boot_status.sdram_ready = 1U;",
        "lv_init();",
        "lv_tick_set_cb(HAL_GetTick);",
        "lv_port_disp_init();",
        "g_lvgl_boot_status.display_ready = 1U;",
        "boot_progress_ui_create()"
    };
    static const char *const lvgl_finish_order[] =
    {
        "lv_port_indev_init();",
        "g_lvgl_boot_status.input_ready = 1U;",
        "setup_ui(&guider_ui);",
        "events_init(&guider_ui);",
        "image_viewer_ui_init();",
        "command_parser_set_image_callbacks(",
        "lv_timer_create(rtc_clock_1s_timer_cb, 1000, NULL);",
        "lv_timer_create(uart_display_timer_cb, 100, NULL);",
        "HAL_UART_Receive_IT(&huart1, uart_rx_buf, 1);",
        "g_lvgl_boot_status.ui_ready = 1U;"
    };
    int failures = 0;

    failures += expect_true("read_header", header != NULL);
    failures += expect_true("read_source", source != NULL);
    failures += expect_true("read_custom_header", custom_header != NULL);
    failures += expect_true("read_custom_source", custom_source != NULL);
    failures += expect_true("read_font_source", font_source != NULL);
    failures += expect_true("read_sd_source", sd_source != NULL);
    failures += expect_true("read_main_source", main_source != NULL);
    failures += expect_true("read_sdcard_source", sdcard_source != NULL);
    failures += expect_true("read_gt_source", gt_source != NULL);
    if ((header == NULL) || (source == NULL) ||
        (custom_header == NULL) || (custom_source == NULL) ||
        (font_source == NULL) || (sd_source == NULL) ||
        (main_source == NULL) || (sdcard_source == NULL) ||
        (gt_source == NULL))
    {
        free(header);
        free(source);
        free(custom_header);
        free(custom_source);
        free(font_source);
        free(sd_source);
        free(main_source);
        free(sdcard_source);
        free(gt_source);
        return 1;
    }

    failures += expect_true("status_type",
                            strstr(header, "norflash_boot_status_t") != NULL);
    failures += expect_true("wallpaper_field",
                            strstr(header, "uint8_t wallpaper_ready;") != NULL);
    failures += expect_true("images_ready_field",
                            strstr(header, "uint8_t images_ready;") != NULL);
    failures += expect_true("images_copied_field",
                            strstr(header, "uint8_t images_copied;") != NULL);
    failures += expect_true("fonts_ready_field",
                            strstr(header, "uint8_t fonts_ready;") != NULL);
    failures += expect_true("first_error_item_field",
                            strstr(header, "uint8_t first_error_item;") != NULL);
    failures += expect_true("first_error_code_field",
                            strstr(header, "int32_t first_error_code;") != NULL);
    failures += expect_true("reset_api",
                            strstr(header, "norflash_boot_status_reset(void);") != NULL);
    failures += expect_true("get_api",
                            strstr(header, "norflash_get_boot_status(norflash_boot_status_t *status);") != NULL);
    failures += expect_true("w25q256jw_id",
                            strstr(header, "W25Q256JW") != NULL &&
                            strstr(header, "0xEF60") != NULL);

    failures += expect_true("static_snapshot",
                            strstr(source, "static norflash_boot_status_t g_boot_status;") != NULL);
    failures += expect_true("reset_snapshot",
                            strstr(source, "memset(&g_boot_status, 0, sizeof(g_boot_status));") != NULL);
    failures += expect_true("init_resets_snapshot",
                            strstr(source, "norflash_boot_status_reset();") != NULL);
    failures += expect_true("getter_copies_snapshot",
                            strstr(source, "*status = g_boot_status;") != NULL);
    failures += expect_true("first_error_only_once",
                            strstr(source, "if (g_boot_status.first_error_code == 0)") != NULL);
    failures += expect_true("w25q256_4byte_addressing_preserved",
                            strstr(source, "g_norflash_type == W25Q256") != NULL &&
                            strstr(source, "g_norflash_type == W25Q256JV") != NULL &&
                            strstr(source, "g_norflash_type == BY25Q256") != NULL);
    failures += expect_true("w25q256jw_4byte_addressing",
                            strstr(source, "g_norflash_type == W25Q256JW") != NULL);

    failures += expect_true("wallpaper_ready_recorded",
                            strstr(source, "g_boot_status.wallpaper_ready = 1U;") != NULL);
    failures += expect_true("wallpaper_copy_error_recorded",
                            strstr(source, "NORFLASH_BOOT_ITEM_WALLPAPER") != NULL);
    failures += expect_true("three_image_items_recorded",
                            count_text(source, "NORFLASH_BOOT_ITEM_IMG") >= 3U);
    failures += expect_true("image_success_counted",
                            strstr(source, "g_boot_status.images_ready++;") != NULL);
    failures += expect_true("image_copy_counted",
                            strstr(source, "g_boot_status.images_copied++;") != NULL);

    failures += expect_true("all_font_load_results_checked",
                            count_text(source, "if (font_bridge_load_one(") == 12U);
    failures += expect_true("font_success_counted",
                            count_text(source, "g_boot_status.fonts_ready++;") == 12U);

    failures += expect_true("lvgl_status_type",
                            strstr(custom_header, "app_lvgl_boot_status_t") != NULL);
    failures += expect_true("lvgl_sdram_field",
                            strstr(custom_header, "uint8_t sdram_ready;") != NULL);
    failures += expect_true("lvgl_ready_field",
                            strstr(custom_header, "uint8_t lvgl_ready;") != NULL);
    failures += expect_true("lvgl_display_field",
                            strstr(custom_header, "uint8_t display_ready;") != NULL);
    failures += expect_true("lvgl_input_field",
                            strstr(custom_header, "uint8_t input_ready;") != NULL);
    failures += expect_true("lvgl_ui_field",
                            strstr(custom_header, "uint8_t ui_ready;") != NULL);
    failures += expect_true("lvgl_getter_api",
                            strstr(custom_header,
                                   "app_lvgl_get_boot_status(app_lvgl_boot_status_t *status);") != NULL);
    failures += expect_true("lvgl_bootstrap_api",
                            strstr(custom_header,
                                   "bool app_lvgl_bootstrap_init(void);") != NULL);
    failures += expect_true("lvgl_finish_api",
                            strstr(custom_header,
                                   "void app_lvgl_finish_init(void);") != NULL);
    failures += expect_true("lvgl_static_snapshot",
                            strstr(custom_source,
                                   "static app_lvgl_boot_status_t g_lvgl_boot_status;") != NULL);
    failures += expect_true("lvgl_status_order",
                            strings_appear_in_order(custom_source,
                                                    lvgl_status_order,
                                                    sizeof(lvgl_status_order) /
                                                    sizeof(lvgl_status_order[0])) != 0);
    failures += expect_true("lvgl_getter_copies_snapshot",
                            strstr(custom_source,
                                   "*status = g_lvgl_boot_status;") != NULL);
    failures += expect_true("lvgl_bootstrap_order",
                            strings_appear_in_order(custom_source,
                                                    lvgl_bootstrap_order,
                                                    sizeof(lvgl_bootstrap_order) /
                                                    sizeof(lvgl_bootstrap_order[0])) != 0);
    failures += expect_true("lvgl_finish_order",
                            strings_appear_in_order(custom_source,
                                                    lvgl_finish_order,
                                                    sizeof(lvgl_finish_order) /
                                                    sizeof(lvgl_finish_order[0])) != 0);
    failures += expect_true("lvgl_legacy_wrapper_removed",
                            strstr(custom_header,
                                   "void app_lvgl_init(void);") == NULL &&
                            strstr(custom_source,
                                   "void app_lvgl_init(void)") == NULL);
    failures += expect_true("lvgl_init_once",
                            count_text(custom_source, "lv_init();") == 1U);
    failures += expect_true("lvgl_display_init_once",
                            count_text(custom_source,
                                       "lv_port_disp_init();") == 1U);
    failures += expect_true("lvgl_input_init_once",
                            count_text(custom_source,
                                       "lv_port_indev_init();") == 1U);

    failures += expect_true("no_nor_sr_success_log",
                            strstr(source, "[NORFLASH] SR") == NULL);
    failures += expect_true("no_qpi_success_log",
                            strstr(source, "Entered QPI") == NULL);
    failures += expect_true("no_progress_dots",
                            strstr(source, "printf(\".\")") == NULL);
    failures += expect_true("no_wallpaper_loading_log",
                            strstr(source, "[WALLPAPER] Loading") == NULL);
    failures += expect_true("no_image_loading_log",
                            strstr(source, "[IMG] Loading") == NULL);
    failures += expect_true("no_copy_success_log",
                            strstr(source, "[SD") == NULL);
    failures += expect_true("structured_open_failure",
                            strstr(source,
                                   "[BOOT] assets FAIL item=%s action=open code=%d") != NULL);
    failures += expect_true("structured_verify_failure",
                            strstr(source,
                                   "[BOOT] assets FAIL item=%s action=verify code=%lu") != NULL);
    failures += expect_true("structured_qspi_failure",
                            strstr(source,
                                   "[BOOT] storage FAIL item=qspi") != NULL);
    failures += expect_true("font_bridge_has_no_direct_logs",
                            strstr(font_source, "printf(") == NULL);
    failures += expect_true("six_structured_font_parse_failures",
                            count_text(source, "action=parse code=2") == 6U);
    failures += expect_true("no_sd_success_log",
                            strstr(sd_source, "[SD] Card OK") == NULL);
    failures += expect_true("structured_sd_absent_warning",
                            strstr(sd_source,
                                   "[BOOT] storage WARN sd=absent") != NULL);
    failures += expect_true("sd_ready_state_preserved",
                            count_text(sd_source, "sdcard_set_ready(") == 2U);
    failures += expect_true("no_lvgl_step_logs",
                            strstr(custom_source, "[LVGL]") == NULL);
    failures += expect_true("structured_sdram_failure",
                            strstr(custom_source,
                                   "[BOOT] display FAIL stage=sdram_verify code=1") != NULL);
    failures += expect_true("no_main_sd_mount_success_log",
                            strstr(main_source,
                                   "[SD] Mounted for QSPI fallback") == NULL);
    failures += expect_true("no_main_old_mount_failure_log",
                            strstr(main_source,
                                   "[SD] Mount failed") == NULL);
    failures += expect_true("structured_mount_failure",
                            strstr(main_source,
                                   "[BOOT] storage WARN sd=mount_failed code=%d") != NULL);
    failures += expect_true("mount_failure_not_duplicated_when_absent",
                            strstr(main_source,
                                   "if (sdcard_is_ready() != 0U)") != NULL);
    failures += expect_true("sd_init_detail_print_suppressed",
                            (count_text(sdcard_source,
                                        "if (operation != SDCARD_OP_INIT)") == 2U) &&
                            (strstr(sdcard_source,
                                    "sdcard_print_first_error();") != NULL));
    failures += expect_true("no_old_gt_unknown_log",
                            strstr(gt_source,
                                   "Unknown GT9xxx ID: %s") == NULL);
    failures += expect_true("no_old_gt_retry_log",
                            strstr(gt_source,
                                   "Read failed, retrying") == NULL);
    failures += expect_true("no_old_gt_failed_log",
                            strstr(gt_source,
                                   "GT9xxx Init Failed") == NULL);
    failures += expect_true("structured_gt_unknown_failure",
                            strstr(gt_source,
                                   "[BOOT] input FAIL item=gt9xxx action=identify id=%s code=1") != NULL);
    failures += expect_true("structured_gt_retry_warning",
                            strstr(gt_source,
                                   "[BOOT] input WARN item=gt9xxx action=identify_retry remaining=%u") != NULL);
    failures += expect_true("structured_gt_init_failure",
                            strstr(gt_source,
                                   "[BOOT] input FAIL item=gt9xxx action=init code=2") != NULL);

    free(header);
    free(source);
    free(custom_header);
    free(custom_source);
    free(font_source);
    free(sd_source);
    free(main_source);
    free(sdcard_source);
    free(gt_source);

    if (failures != 0)
    {
        printf("FAIL boot_asset_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS boot_asset_contract\n");
    return 0;
}
