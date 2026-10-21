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

static int function_body_contains(const char *source,
                                  const char *function_name,
                                  const char *needle)
{
    const char *name;
    const char *body;
    const char *cursor;
    unsigned int depth = 0U;
    size_t body_length;
    char *copy;
    int found;

    name = strstr(source, function_name);
    if (name == NULL)
    {
        return 0;
    }
    body = strchr(name, '{');
    if (body == NULL)
    {
        return 0;
    }

    for (cursor = body; *cursor != '\0'; cursor++)
    {
        if (*cursor == '{')
        {
            depth++;
        }
        else if (*cursor == '}')
        {
            if (depth == 0U)
            {
                return 0;
            }
            depth--;
            if (depth == 0U)
            {
                body_length = (size_t)(cursor - body) + 1U;
                copy = (char *)malloc(body_length + 1U);
                if (copy == NULL)
                {
                    return 0;
                }
                memcpy(copy, body, body_length);
                copy[body_length] = '\0';
                found = (strstr(copy, needle) != NULL) ? 1 : 0;
                free(copy);
                return found;
            }
        }
    }
    return 0;
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

static int text_between_contains(const char *text,
                                 const char *start_marker,
                                 const char *end_marker,
                                 const char *needle)
{
    const char *start = strstr(text, start_marker);
    const char *end;
    const char *found;

    if (start == NULL)
    {
        return 0;
    }
    end = strstr(start, end_marker);
    if (end == NULL)
    {
        return 0;
    }
    found = strstr(start, needle);
    return ((found != NULL) && (found < end)) ? 1 : 0;
}

int main(void)
{
    char *gt_header = read_source("Core/Inc/gt9xxx.h");
    char *gt_source = read_source("Core/Src/gt9xxx.c");
    char *pcf_header = read_source("Core/Peripheral/Inc/pcf8574.h");
    char *pcf_source = read_source("Core/Peripheral/Src/pcf8574.c");
    char *qspi_header = read_source("Core/Peripheral/Inc/qspi_drv.h");
    char *qspi_source = read_source("Core/Peripheral/Src/qspi_drv.c");
    char *nor_header = read_source("Core/Peripheral/Inc/norflash.h");
    char *nor_source = read_source("Core/Peripheral/Src/norflash.c");
    char *target_source =
        read_source("Core/Application/Src/boot_probe_targets.c");
    char *project_source =
        read_source("MDK-ARM/H743_LCD_Keil5.uvprojx");
    char *main_source = read_source("Core/Src/main.c");
    char *command_source =
        read_source("Core/Peripheral/Src/command_parser.c");
    char *tpad_header = read_source("Core/Peripheral/Inc/tpad_service.h");
    char *tpad_source = read_source("Core/Peripheral/Src/tpad_service.c");
    char *ir_source = read_source("Core/Peripheral/Src/ir_remote_service.c");
    char *es8388_header = read_source("Core/Peripheral/Inc/es8388_service.h");
    char *es8388_source = read_source("Core/Peripheral/Src/es8388_service.c");
    char *mp3_decoder_source = read_source("Core/Peripheral/Src/mp3_decoder_service.c");
    char *mp3_buffers_source = read_source("Core/Peripheral/MP3/Helix/buffers.c");
    char *audio_source = read_source("Core/Peripheral/Src/audio_playback_service.c");
    char *custom_source = read_source("lvgl_GUI/custom/custom.c");
    char *events_source = read_source("lvgl_GUI/generated/events_init.c");
    static const char *const target_ids[] =
    {
        "{\"uart1\"", "{\"i2c2\"", "{\"sdmmc1\"", "{\"qspi\"",
        "{\"fmc_sdram\"", "{\"ltdc\"", "{\"dma2d\"", "{\"jpeg\"",
        "{\"rtc\"", "{\"adc1\"", "{\"adc3\"", "{\"sai1\"",
        "{\"spdifrx1\"", "{\"tim1\"", "{\"tim2\"", "{\"tim3\"",
        "{\"tim6\"", "{\"gt9xxx\"", "{\"pcf8574\"", "{\"sd\"",
        "{\"qspi_flash\"", "{\"nand\"", "{\"lcd_panel\"",
        "{\"sdram\"", "{\"imu\"", "{\"pressure\"",
        "{\"es8388\"", "{\"ov5640\"", "{\"ap3216c\"",
        "{\"24c02\"", "{\"dht11\"", "{\"ds18b20\""
    };
    static const char *const unsupported_ids[] =
    {
        "{\"lcd_panel\", BOOT_PROBE_KIND_DISPLAY, 1U, NULL}",
        "{\"es8388\", BOOT_PROBE_KIND_AUDIO, 0U, NULL}",
        "{\"ov5640\", BOOT_PROBE_KIND_CAMERA, 0U, NULL}",
        "{\"dht11\", BOOT_PROBE_KIND_SENSOR, 0U, NULL}",
        "{\"ds18b20\", BOOT_PROBE_KIND_SENSOR, 0U, NULL}"
    };
    static const char *const forbidden_target_calls[] =
    {
        "f_mount(", "HAL_ADC_Start", "HAL_DMA_Start", "HAL_SAI_Transmit",
        "HAL_SAI_Receive", "norflash_init(", "nand_scan_start(",
        "gt9xxx_init(", "pcf8574_init(", "lv_"
    };
    static const char *const startup_order[] =
    {
        "boot_report_init(HAL_GetTick());",
        "diagnostics_init();",
        "sdcard_manager_init();",
        "app_event_init();",
        "app_runtime_init();",
        "command_parser_init();",
        "boot_probe_init();",
        "norflash_set_progress_callback(boot_progress_resource_callback, NULL);",
        "norflash_init();",
        "sdcard_manager_poll(HAL_GetTick());",
        "sdcard_manager_get_status(&sd_status);",
        "norflash_preload_wallpaper();",
        "boot_progress_ui_set_wallpaper(norflash_get_wallpaper_dsc());",
        "audio_service_init();",
        "sh3001_service_init();",
        "gt9xxx_init();",
        "boot_probe_start_quiet(HAL_GetTick());",
        "while (boot_probe_is_complete() == 0U)",
        "boot_probe_poll(HAL_GetTick());",
        "boot_probe_get_progress(&probe_completed, &probe_total);",
        "boot_progress_apply_probe_status(probe_completed - 1U);",
        "boot_report_capture_probe();",
        "norflash_preload_images();",
        "boot_progress_set_current(\"Font cache\",",
        "font_cache_service_try_load()",
        "boot_progress_load_font(\"Font 12\", norflash_init_font12,",
        "&lv_font_myyh_12, 66U) != 0U",
        "g_boot_chinese_ready = 1U;",
        "boot_progress_ui_set_chinese(true);",
        "norflash_init_font14,",
        "&lv_font_myyh_14, 72U);",
        "norflash_init_font16,",
        "&lv_font_myyh_16, 78U);",
        "norflash_init_font18,",
        "&lv_font_myyh_18, 84U);",
        "norflash_init_font20,",
        "&lv_font_myyh_20, 90U);",
        "norflash_init_font30, NULL, 96U);",
        "norflash_set_progress_callback(NULL, NULL);",
        "\\xE5\\x90\\xAF\\xE5\\x8A\\xA8\\xE5\\xAE\\x8C\\xE6\\x88\\x90",
        "HAL_Delay(300);",
        "app_lvgl_finish_init();",
        "boot_progress_ui_destroy();",
        "norflash_get_boot_status(&g_boot_asset_status);",
        "app_lvgl_get_boot_status(&g_boot_lvgl_status);",
        "boot_report_set_assets(",
        "boot_report_set_display(",
        "boot_report_print(HAL_GetTick());"
    };
    static const char *const early_progress_order[] =
    {
        "MX_DMA2D_Init();",
        "/* BOOT_PROGRESS USER PATCH BEGIN",
        "boot_progress_init();",
        "app_lvgl_bootstrap_init()",
        "HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2)",
        "__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 499);",
        "boot_progress_complete_internal(\"DMA2D\"",
        "MX_I2C2_Init();", "boot_progress_complete_internal(\"I2C2\"",
        "MX_QUADSPI_Init();", "boot_progress_complete_internal(\"QSPI\"",
        "MX_SAI1_Init();", "boot_progress_complete_internal(\"SAI1\"",
        "MX_SDMMC1_SD_Init();", "boot_progress_complete_internal(\"SDMMC1\"",
        "MX_RTC_Init();", "boot_progress_complete_internal(\"RTC\"",
        "MX_JPEG_Init();", "boot_progress_complete_internal(\"JPEG\"",
        "MX_ADC1_Init();", "boot_progress_complete_internal(\"ADC1\"",
        "MX_ADC3_Init();", "boot_progress_complete_internal(\"ADC3\"",
        "MX_SPDIFRX1_Init();", "boot_progress_complete_internal(\"SPDIFRX1\"",
        "MX_TIM1_Init();", "boot_progress_complete_internal(\"TIM1\"",
        "MX_TIM2_Init();", "boot_progress_complete_internal(\"TIM2\"",
        "/* BOOT_PROGRESS USER PATCH END */"
    };
    size_t index;
    int failures = 0;

    failures += expect_true("read_gt_header", gt_header != NULL);
    failures += expect_true("read_gt_source", gt_source != NULL);
    failures += expect_true("read_pcf_header", pcf_header != NULL);
    failures += expect_true("read_pcf_source", pcf_source != NULL);
    failures += expect_true("read_qspi_header", qspi_header != NULL);
    failures += expect_true("read_qspi_source", qspi_source != NULL);
    failures += expect_true("read_nor_header", nor_header != NULL);
    failures += expect_true("read_nor_source", nor_source != NULL);
    failures += expect_true("read_target_source", target_source != NULL);
    failures += expect_true("read_project_source", project_source != NULL);
    failures += expect_true("read_main_source", main_source != NULL);
    failures += expect_true("read_command_source", command_source != NULL);
    failures += expect_true("read_tpad_header", tpad_header != NULL);
    failures += expect_true("read_tpad_source", tpad_source != NULL);
    failures += expect_true("read_ir_source", ir_source != NULL);
    failures += expect_true("read_es8388_header", es8388_header != NULL);
    failures += expect_true("read_es8388_source", es8388_source != NULL);
    failures += expect_true("read_mp3_decoder_source", mp3_decoder_source != NULL);
    failures += expect_true("read_mp3_buffers_source", mp3_buffers_source != NULL);
    failures += expect_true("read_audio_source", audio_source != NULL);
    failures += expect_true("read_custom_source", custom_source != NULL);
    failures += expect_true("read_events_source", events_source != NULL);
    if (failures != 0)
    {
        free(gt_header);
        free(gt_source);
        free(pcf_header);
        free(pcf_source);
        free(qspi_header);
        free(qspi_source);
        free(nor_header);
        free(nor_source);
        free(target_source);
        free(project_source);
        free(main_source);
        free(command_source);
        free(tpad_header);
        free(tpad_source);
        free(ir_source);
        free(es8388_header);
        free(es8388_source);
        free(mp3_decoder_source);
        free(mp3_buffers_source);
        free(audio_source);
        free(custom_source);
        free(events_source);
        return 1;
    }

    failures += expect_true(
        "gt_readonly_declared",
        strstr(gt_header,
               "uint8_t gt9xxx_probe_readonly(char product_id[5]);") != NULL);
    failures += expect_true(
        "gt_reads_product_id",
        function_body_contains(gt_source, "gt9xxx_probe_readonly",
                               "gt9xxx_read_reg(GT9XXX_REG_ID") != 0);
    failures += expect_true(
        "gt_does_not_write_register",
        function_body_contains(gt_source, "gt9xxx_probe_readonly",
                               "gt9xxx_write_reg") == 0);
    failures += expect_true(
        "gt_does_not_reset",
        function_body_contains(gt_source, "gt9xxx_probe_readonly",
                               "TOUCH_RST_") == 0);
    failures += expect_true(
        "gt_does_not_delay",
        function_body_contains(gt_source, "gt9xxx_probe_readonly",
                               "HAL_Delay") == 0);
    failures += expect_true(
        "gt_does_not_print",
        function_body_contains(gt_source, "gt9xxx_probe_readonly",
                               "printf") == 0);

    failures += expect_true(
        "pcf_readonly_declared",
        strstr(pcf_header,
               "uint8_t pcf8574_probe_readonly(uint32_t timeout_ms);") != NULL);
    failures += expect_true(
        "pcf_checks_fixed_address",
        function_body_contains(pcf_source, "pcf8574_probe_readonly",
                               "HAL_I2C_IsDeviceReady(&hi2c2, PCF8574_ADDR") != 0);
    failures += expect_true(
        "pcf_single_attempt",
        function_body_contains(pcf_source, "pcf8574_probe_readonly",
                               "PCF8574_ADDR, 1U, timeout_ms") != 0);
    failures += expect_true(
        "pcf_does_not_transmit",
        function_body_contains(pcf_source, "pcf8574_probe_readonly",
                               "HAL_I2C_Master_Transmit") == 0);
    failures += expect_true(
        "pcf_does_not_receive",
        function_body_contains(pcf_source, "pcf8574_probe_readonly",
                               "HAL_I2C_Master_Receive") == 0);
    failures += expect_true(
        "pcf_does_not_call_writer",
        function_body_contains(pcf_source, "pcf8574_probe_readonly",
                               "pcf8574_write_byte") == 0);
    failures += expect_true(
        "pcf_does_not_print",
        function_body_contains(pcf_source, "pcf8574_probe_readonly",
                               "printf") == 0);

    failures += expect_true(
        "qspi_bounded_command_declared",
        strstr(qspi_header, "qspi_send_cmd_timeout") != NULL);
    failures += expect_true(
        "qspi_bounded_receive_declared",
        strstr(qspi_header, "qspi_receive_timeout") != NULL);
    failures += expect_true(
        "qspi_command_uses_caller_timeout",
        function_body_contains(qspi_source, "qspi_send_cmd_timeout",
                               "HAL_QSPI_Command(&hqspi, &sCmd, timeout_ms)") != 0);
    failures += expect_true(
        "qspi_receive_uses_caller_timeout",
        function_body_contains(qspi_source, "qspi_receive_timeout",
                               "HAL_QSPI_Receive(&hqspi, buf, timeout_ms)") != 0);
    failures += expect_true(
        "nor_readonly_declared",
        strstr(nor_header,
               "uint8_t  norflash_probe_readonly(uint16_t *id, uint32_t timeout_ms);") != NULL);
    failures += expect_true(
        "nor_uses_cached_id",
        function_body_contains(nor_source, "norflash_probe_readonly",
                               "g_norflash_type") != 0);
    failures += expect_true(
        "nor_uses_bounded_command",
        function_body_contains(nor_source, "norflash_probe_readonly",
                               "qspi_send_cmd_timeout") != 0);
    failures += expect_true(
        "nor_uses_bounded_receive",
        function_body_contains(nor_source, "norflash_probe_readonly",
                               "qspi_receive_timeout") != 0);
    failures += expect_true(
        "nor_does_not_initialize",
        function_body_contains(nor_source, "norflash_probe_readonly",
                               "norflash_init") == 0);
    failures += expect_true(
        "nor_does_not_enable_write",
        function_body_contains(nor_source, "norflash_probe_readonly",
                               "norflash_write_enable") == 0);
    failures += expect_true(
        "nor_does_not_change_memmap",
        function_body_contains(nor_source, "norflash_probe_readonly",
                               "norflash_exit_memmap") == 0);

    failures += expect_true(
        "target_ids_fixed_order",
        strings_appear_in_order(target_source, target_ids,
                                sizeof(target_ids) / sizeof(target_ids[0])) != 0);
    failures += expect_true(
        "target_count_exactly_32",
        count_text(target_source, "{\"") == 32U);
    for (index = 0U;
         index < (sizeof(unsupported_ids) / sizeof(unsupported_ids[0]));
         index++)
    {
        failures += expect_true("unsupported_has_null_callback",
                                strstr(target_source,
                                       unsupported_ids[index]) != NULL);
    }
    for (index = 0U;
         index < (sizeof(forbidden_target_calls) /
                  sizeof(forbidden_target_calls[0]));
         index++)
    {
        failures += expect_true("target_has_no_forbidden_call",
                                strstr(target_source,
                                       forbidden_target_calls[index]) == NULL);
    }
    failures += expect_true(
        "target_uses_bounded_nor_probe",
        strstr(target_source, "norflash_probe_readonly(&id, 5U)") != NULL);
    failures += expect_true(
        "target_nand_does_not_scan",
        strstr(target_source, "nand_readonly_probe(100U)") != NULL);
    failures += expect_true(
        "target_includes_imu_service",
        strstr(target_source, "#include \"imu_service.h\"") != NULL);
    failures += expect_true(
        "target_imu_callback_exists",
        strstr(target_source, "boot_probe_imu") != NULL);
    failures += expect_true(
        "target_imu_uses_cached_status",
        function_body_contains(target_source, "boot_probe_imu",
                               "imu_get_status") != 0);
    failures += expect_true(
        "target_imu_does_not_probe_directly",
        function_body_contains(target_source, "boot_probe_imu",
                               "imu_probe_readonly") == 0);
    failures += expect_true(
        "target_imu_has_callback",
        strstr(target_source,
               "{\"imu\", BOOT_PROBE_KIND_SENSOR, 0U, boot_probe_imu}") != NULL);
    failures += expect_true(
        "target_st480mc_not_auto_probed_on_v26",
        strstr(target_source, "boot_probe_st480mc") == NULL);
    failures += expect_true(
        "target_st480mc_not_registered_on_v26",
        strstr(target_source, "{\"st480mc\"") == NULL);
    failures += expect_true(
        "target_includes_pressure_service",
        strstr(target_source, "#include \"pressure_service.h\"") != NULL);
    failures += expect_true(
        "target_pressure_callback_exists",
        strstr(target_source, "boot_probe_pressure") != NULL);
    failures += expect_true(
        "target_pressure_uses_cached_status",
        function_body_contains(target_source, "boot_probe_pressure",
                               "pressure_get_status") != 0);
    failures += expect_true(
        "target_pressure_does_not_probe_directly",
        function_body_contains(target_source, "boot_probe_pressure",
                               "pressure_probe_readonly") == 0);
    failures += expect_true(
        "target_pressure_has_callback",
        strstr(target_source,
               "{\"pressure\", BOOT_PROBE_KIND_SENSOR, 0U, boot_probe_pressure}") != NULL);
    failures += expect_true(
        "target_includes_ap3216c_service",
        strstr(target_source, "#include \"ap3216c_service.h\"") != NULL);
    failures += expect_true(
        "target_ap3216c_callback_exists",
        strstr(target_source, "boot_probe_ap3216c") != NULL);
    failures += expect_true(
        "target_ap3216c_uses_cached_status",
        function_body_contains(target_source, "boot_probe_ap3216c",
                               "ap3216c_get_status") != 0);
    failures += expect_true(
        "target_ap3216c_does_not_probe_directly",
        function_body_contains(target_source, "boot_probe_ap3216c",
                               "ap3216c_probe_readonly") == 0);
    failures += expect_true(
        "target_ap3216c_does_not_measure",
        function_body_contains(target_source, "boot_probe_ap3216c",
                               "ap3216c_read_once") == 0);
    failures += expect_true(
        "target_ap3216c_has_callback",
        strstr(target_source,
               "{\"ap3216c\", BOOT_PROBE_KIND_SENSOR, 0U, boot_probe_ap3216c}") != NULL);
    failures += expect_true(
        "target_includes_at24c02_service",
        strstr(target_source, "#include \"at24c02_service.h\"") != NULL);
    failures += expect_true(
        "target_at24c02_callback_exists",
        strstr(target_source, "boot_probe_at24c02") != NULL);
    failures += expect_true(
        "target_at24c02_uses_cached_status",
        function_body_contains(target_source, "boot_probe_at24c02",
                               "at24c02_get_status") != 0);
    failures += expect_true(
        "target_at24c02_does_not_probe_directly",
        function_body_contains(target_source, "boot_probe_at24c02",
                               "at24c02_probe_readonly") == 0);
    failures += expect_true(
        "target_at24c02_does_not_read_data",
        function_body_contains(target_source, "boot_probe_at24c02",
                               "at24c02_read(") == 0);
    failures += expect_true(
        "target_at24c02_has_callback",
        strstr(target_source,
               "{\"24c02\", BOOT_PROBE_KIND_STORAGE, 0U, boot_probe_at24c02}") != NULL);
    failures += expect_true(
        "uart_uses_global_state",
        strstr(target_source, "huart1.gState") != NULL);
    failures += expect_true(
        "uart_allows_active_receive",
        strstr(target_source, "huart1.RxState") != NULL &&
        strstr(target_source, "HAL_UART_STATE_BUSY_RX") != NULL);
    failures += expect_true(
        "uart_does_not_use_missing_state_field",
        strstr(target_source, "huart1.State") == NULL);
    failures += expect_true(
        "tim_running_state_is_operational",
        strstr(target_source, "HAL_TIM_STATE_BUSY") != NULL);
    failures += expect_true(
        "tim6_uses_operational_state_helper",
        strstr(target_source,
               "boot_probe_tim_state(status, &htim6)") != NULL);
    failures += expect_true(
        "tim6_does_not_require_ready_only",
        strstr(target_source,
               "BOOT_PROBE_DEFINE_STATE_CALLBACK(boot_probe_tim6") == NULL);
    failures += expect_true(
        "project_has_probe_service_name_once",
        count_text(project_source,
                   "<FileName>boot_probe_service.c</FileName>") == 1U);
    failures += expect_true(
        "project_has_probe_service_path_once",
        count_text(project_source,
                   "<FilePath>..\\Core\\Application\\Src\\boot_probe_service.c</FilePath>") == 1U);
    failures += expect_true(
        "project_has_probe_targets_name_once",
        count_text(project_source,
                   "<FileName>boot_probe_targets.c</FileName>") == 1U);
    failures += expect_true(
        "project_has_probe_targets_path_once",
        count_text(project_source,
                   "<FilePath>..\\Core\\Application\\Src\\boot_probe_targets.c</FilePath>") == 1U);
    failures += expect_true(
        "project_has_boot_report_name_once",
        count_text(project_source,
                   "<FileName>boot_report_service.c</FileName>") == 1U);
    failures += expect_true(
        "project_has_boot_report_path_once",
        count_text(project_source,
                   "<FilePath>..\\Core\\Application\\Src\\boot_report_service.c</FilePath>") == 1U);
    failures += expect_true(
        "project_has_board_i2c_name_once",
        count_text(project_source,
                   "<FileName>board_i2c.c</FileName>") == 1U);
    failures += expect_true(
        "project_has_board_i2c_path_once",
        count_text(project_source,
                   "<FilePath>..\\Core\\Peripheral\\Src\\board_i2c.c</FilePath>") == 1U);
    failures += expect_true(
        "project_has_sh3001_service_name_once",
        count_text(project_source,
                   "<FileName>sh3001_service.c</FileName>") == 1U);
    failures += expect_true(
        "project_has_sh3001_service_path_once",
        count_text(project_source,
                   "<FilePath>..\\Core\\Peripheral\\Src\\sh3001_service.c</FilePath>") == 1U);
    failures += expect_true(
        "project_has_imu_service_name_once",
        count_text(project_source,
                   "<FileName>imu_service.c</FileName>") == 1U);
    failures += expect_true(
        "project_has_imu_service_path_once",
        count_text(project_source,
                   "<FilePath>..\\Core\\Peripheral\\Src\\imu_service.c</FilePath>") == 1U);
    failures += expect_true(
        "project_has_st480mc_service_name_once",
        count_text(project_source,
                   "<FileName>st480mc_service.c</FileName>") == 1U);
    failures += expect_true(
        "project_has_st480mc_service_path_once",
        count_text(project_source,
                   "<FilePath>..\\Core\\Peripheral\\Src\\st480mc_service.c</FilePath>") == 1U);
    failures += expect_true(
        "project_has_pressure_service_name_once",
        count_text(project_source,
                   "<FileName>pressure_service.c</FileName>") == 1U);
    failures += expect_true(
        "project_has_pressure_service_path_once",
        count_text(project_source,
                   "<FilePath>..\\Core\\Peripheral\\Src\\pressure_service.c</FilePath>") == 1U);
    failures += expect_true(
        "project_has_ap3216c_service_name_once",
        count_text(project_source,
                   "<FileName>ap3216c_service.c</FileName>") == 1U);
    failures += expect_true(
        "project_has_ap3216c_service_path_once",
        count_text(project_source,
                   "<FilePath>..\\Core\\Peripheral\\Src\\ap3216c_service.c</FilePath>") == 1U);
    failures += expect_true(
        "project_has_at24c02_service_name_once",
        count_text(project_source,
                   "<FileName>at24c02_service.c</FileName>") == 1U);
    failures += expect_true(
        "project_has_at24c02_service_path_once",
        count_text(project_source,
                   "<FilePath>..\\Core\\Peripheral\\Src\\at24c02_service.c</FilePath>") == 1U);
    failures += expect_true(
        "project_has_adc_measurement_service_name_once",
        count_text(project_source,
                   "<FileName>adc_measurement_service.c</FileName>") == 1U);
    failures += expect_true(
        "project_has_adc_measurement_service_path_once",
        count_text(project_source,
                   "<FilePath>..\\Core\\Peripheral\\Src\\adc_measurement_service.c</FilePath>") == 1U);

    failures += expect_true(
        "main_probe_include_in_user_block",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN Includes */",
                              "/* USER CODE END Includes */",
                              "#include \"boot_probe_service.h\"") != 0);
    failures += expect_true(
        "main_report_include_in_user_block",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN Includes */",
                              "/* USER CODE END Includes */",
                              "#include \"boot_report_service.h\"") != 0);
    failures += expect_true(
        "main_sh3001_include_in_user_block",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN Includes */",
                              "/* USER CODE END Includes */",
                              "#include \"sh3001_service.h\"") != 0);
    failures += expect_true(
        "main_imu_include_in_user_block",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN Includes */",
                              "/* USER CODE END Includes */",
                              "#include \"imu_service.h\"") != 0);
    failures += expect_true(
        "main_st480mc_include_in_user_block",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN Includes */",
                              "/* USER CODE END Includes */",
                              "#include \"st480mc_service.h\"") != 0);
    failures += expect_true(
        "main_pressure_include_in_user_block",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN Includes */",
                              "/* USER CODE END Includes */",
                              "#include \"pressure_service.h\"") != 0);
    failures += expect_true(
        "main_ap3216c_include_in_user_block",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN Includes */",
                              "/* USER CODE END Includes */",
                              "#include \"ap3216c_service.h\"") != 0);
    failures += expect_true(
        "main_at24c02_include_in_user_block",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN Includes */",
                              "/* USER CODE END Includes */",
                              "#include \"at24c02_service.h\"") != 0);
    failures += expect_true(
        "main_adc_measurement_include_in_user_block",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN Includes */",
                              "/* USER CODE END Includes */",
                              "#include \"adc_measurement_service.h\"") != 0);
    failures += expect_true(
        "main_boot_progress_include_in_user_block",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN Includes */",
                              "/* USER CODE END Includes */",
                              "#include \"boot_progress_service.h\"") != 0);
    failures += expect_true(
        "main_boot_progress_ui_include_in_user_block",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN Includes */",
                              "/* USER CODE END Includes */",
                              "#include \"boot_progress_ui.h\"") != 0);
    failures += expect_true(
        "main_startup_order",
        strings_appear_in_order(main_source, startup_order,
                                sizeof(startup_order) /
                                sizeof(startup_order[0])) != 0);
    failures += expect_true(
        "main_early_progress_order",
        strings_appear_in_order(main_source, early_progress_order,
                                sizeof(early_progress_order) /
                                sizeof(early_progress_order[0])) != 0);
    failures += expect_true(
        "main_boot_patch_marker_once",
        count_text(main_source, "/* BOOT_PROGRESS USER PATCH BEGIN") == 1U &&
        count_text(main_source, "/* BOOT_PROGRESS USER PATCH END */") == 1U);
    failures += expect_true(
        "main_backlight_start_once",
        count_text(main_source,
                   "HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2)") == 1U);
    failures += expect_true(
        "main_early_patch_has_no_assets_or_devices",
        text_between_contains(main_source,
                              "/* BOOT_PROGRESS USER PATCH BEGIN",
                              "/* BOOT_PROGRESS USER PATCH END */",
                              "norflash_") == 0 &&
        text_between_contains(main_source,
                              "/* BOOT_PROGRESS USER PATCH BEGIN",
                              "/* BOOT_PROGRESS USER PATCH END */",
                              "BOOT_DEVICE_") == 0);
    failures += expect_true(
        "main_resource_refresh_thresholds",
        strstr(main_source, "#define BOOT_PROGRESS_COPY_REFRESH_BYTES (256UL * 1024UL)") != NULL &&
        strstr(main_source, "#define BOOT_PROGRESS_COPY_REFRESH_PERCENT 5U") != NULL &&
        strstr(main_source, "#define BOOT_PROGRESS_COPY_REFRESH_MS 200U") != NULL);
    failures += expect_true(
        "main_qspi_load_refreshes_only_boundaries",
        strstr(main_source,
               "if (progress->action == NORFLASH_PROGRESS_LOAD) { return 0U; }") != NULL);
    failures += expect_true(
        "main_fixed_device_mapping",
        strstr(main_source, "BOOT_DEVICE_SD") != NULL &&
        strstr(main_source, "BOOT_DEVICE_QSPI") != NULL &&
        strstr(main_source, "BOOT_DEVICE_NAND") != NULL &&
        strstr(main_source, "BOOT_DEVICE_GT911") != NULL &&
        strstr(main_source, "BOOT_DEVICE_PCF8574") != NULL &&
        strstr(main_source, "BOOT_DEVICE_OV5640") != NULL &&
        strstr(main_source, "BOOT_DEVICE_ES8388") != NULL &&
        strstr(main_source, "BOOT_DEVICE_SH3001") != NULL &&
        strstr(main_source, "BOOT_DEVICE_AP3216C") != NULL &&
        strstr(main_source, "BOOT_DEVICE_AT24C02") != NULL);
    failures += expect_true(
        "main_switches_font_paths_through_20",
        count_text(main_source, "boot_progress_ui_set_font(") == 2U &&
        strstr(main_source, "(display_font != NULL)") != NULL &&
        strstr(main_source,
               "boot_progress_ui_set_font(&lv_font_myyh_20)") != NULL &&
        strstr(main_source,
               "boot_progress_ui_set_font(&lv_font_myyh_30)") == NULL);
    failures += expect_true(
        "main_no_legacy_lvgl_wrapper_call",
        strstr(main_source, "app_lvgl_init();") == NULL);
    failures += expect_true(
        "main_chinese_after_cache_or_font12",
        strstr(main_source, "static uint8_t g_boot_chinese_ready;") != NULL &&
        count_text(main_source, "boot_progress_ui_set_chinese(true);") == 2U);
    failures += expect_true(
        "main_english_before_fonts_chinese_after_fonts",
        strstr(main_source, "\"Image resources\"") != NULL &&
        strstr(main_source, "\"Peripheral probe\"") != NULL &&
        strstr(main_source,
               "\\xE5\\x90\\xAF\\xE5\\x8A\\xA8\\xE5\\xAE\\x8C\\xE6\\x88\\x90") != NULL);
    failures += expect_true(
        "main_probe_init_in_user_block",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN 2 */",
                              "/* USER CODE END 2 */",
                              "boot_probe_init();") != 0);
    failures += expect_true(
        "main_sh3001_init_in_user_block",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN 2 */",
                              "/* USER CODE END 2 */",
                              "sh3001_service_init();") != 0);
    failures += expect_true(
        "main_imu_probe_in_user_block",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN 2 */",
                              "/* USER CODE END 2 */",
                              "imu_probe_readonly(10U);") != 0);
    failures += expect_true(
        "main_imu_init_order",
        strings_appear_in_order(main_source,
                                (const char *const[]){
                                    "boot_probe_init();",
                                    "sh3001_service_init();",
                                    "imu_service_init();",
                                    "imu_probe_readonly(10U);",
                                    "boot_probe_start_quiet(HAL_GetTick());"
                                }, 5U) != 0);
    failures += expect_true(
        "main_st480mc_has_no_auto_probe",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN 2 */",
                              "/* USER CODE END 2 */",
                              "st480mc_probe_readonly(10U);") == 0);
    failures += expect_true(
        "main_st480mc_init_without_auto_probe",
        strings_appear_in_order(main_source,
                                (const char *const[]){
                                    "boot_probe_init();",
                                    "st480mc_service_init();",
                                    "boot_probe_start_quiet(HAL_GetTick());"
                                }, 3U) != 0);
    failures += expect_true(
        "main_pressure_init_and_probe_order",
        strings_appear_in_order(main_source,
                                (const char *const[]){
                                    "boot_probe_init();",
                                    "pressure_service_init();",
                                    "pressure_probe_readonly(10U);",
                                    "boot_probe_start_quiet(HAL_GetTick());"
                                }, 4U) != 0);
    failures += expect_true(
        "main_pressure_probe_in_user_block",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN 2 */",
                              "/* USER CODE END 2 */",
                              "pressure_probe_readonly(10U);") != 0);
    failures += expect_true(
        "main_pressure_does_not_measure_at_boot",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN 2 */",
                              "/* USER CODE END 2 */",
                              "pressure_read_once(") == 0);
    failures += expect_true(
        "main_ap3216c_init_and_probe_order",
        strings_appear_in_order(main_source,
                                (const char *const[]){
                                    "boot_probe_init();",
                                    "ap3216c_service_init();",
                                    "ap3216c_probe_readonly(10U);",
                                    "boot_probe_start_quiet(HAL_GetTick());"
                                }, 4U) != 0);
    failures += expect_true(
        "main_ap3216c_probe_in_user_block",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN 2 */",
                              "/* USER CODE END 2 */",
                              "ap3216c_probe_readonly(10U);") != 0);
    failures += expect_true(
        "main_ap3216c_does_not_measure_at_boot",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN 2 */",
                              "/* USER CODE END 2 */",
                              "ap3216c_read_once(") == 0);
    failures += expect_true(
        "main_at24c02_init_and_probe_order",
        strings_appear_in_order(main_source,
                                (const char *const[]){
                                    "boot_probe_init();",
                                    "at24c02_service_init();",
                                    "at24c02_probe_readonly(10U);",
                                    "boot_probe_start_quiet(HAL_GetTick());"
                                }, 4U) != 0);
    failures += expect_true(
        "main_at24c02_probe_in_user_block",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN 2 */",
                              "/* USER CODE END 2 */",
                              "at24c02_probe_readonly(10U);") != 0);
    failures += expect_true(
        "main_at24c02_does_not_read_data_at_boot",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN 2 */",
                              "/* USER CODE END 2 */",
                              "at24c02_read(") == 0);
    failures += expect_true(
        "main_adc_measurement_init_in_user_block",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN 2 */",
                              "/* USER CODE END 2 */",
                              "adc_measurement_service_init();") != 0);
    failures += expect_true(
        "main_adc_measurement_does_not_read_at_boot",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN 2 */",
                              "/* USER CODE END 2 */",
                              "adc_measurement_read_") == 0);
    failures += expect_true(
        "main_quiet_probe_start_in_user_block",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN 2 */",
                              "/* USER CODE END 2 */",
                              "boot_probe_start_quiet(HAL_GetTick());") != 0);
    failures += expect_true(
        "main_loop_poll_in_user_block",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN WHILE */",
                              "/* USER CODE END WHILE */",
                              "boot_probe_poll(HAL_GetTick());") != 0);
    failures += expect_true(
        "main_loop_print_in_user_block",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN WHILE */",
                              "/* USER CODE END WHILE */",
                              "boot_probe_print_next();") != 0);
    failures += expect_true(
        "main_has_one_init",
        count_text(main_source, "boot_probe_init();") == 1U);
    failures += expect_true(
        "main_has_one_quiet_start",
        count_text(main_source,
                   "boot_probe_start_quiet(HAL_GetTick());") == 1U);
    failures += expect_true(
        "main_has_no_detailed_start",
        count_text(main_source, "boot_probe_start(HAL_GetTick());") == 0U);
    failures += expect_true(
        "main_has_startup_and_loop_poll",
        count_text(main_source, "boot_probe_poll(HAL_GetTick());") == 2U);
    failures += expect_true(
        "main_has_one_deferred_print",
        count_text(main_source, "boot_probe_print_next();") == 1U);
    failures += expect_true(
        "main_has_one_sd_manager_init",
        count_text(main_source, "sdcard_manager_init();") == 1U);
    failures += expect_true(
        "main_has_startup_and_loop_sd_poll",
        count_text(main_source,
                   "sdcard_manager_poll(HAL_GetTick());") == 2U);
    failures += expect_true(
        "camera_poll_precedes_sd_poll",
        text_between_contains(main_source,
                              "command_parser_poll_at(HAL_GetTick());",
                              "camera_ui_bridge_poll();",
                              "camera_preview_poll();\n    sdcard_manager_poll(HAL_GetTick());") != 0);
    failures += expect_true(
        "main_reads_sd_manager_status",
        count_text(main_source,
                   "sdcard_manager_get_status(&sd_status);") == 1U);
    failures += expect_true(
        "main_does_not_own_fatfs",
        strstr(main_source, "static FATFS boot_fs") == NULL);
    failures += expect_true(
        "main_does_not_mount_directly",
        strstr(main_source, "f_mount(") == NULL);
    failures += expect_true(
        "startup_does_not_print_probe_records",
        text_between_contains(main_source,
                              "boot_probe_start_quiet(HAL_GetTick());",
                              "sdcard_manager_poll(HAL_GetTick());",
                              "boot_probe_print_next();") == 0);
    failures += expect_true(
        "report_init_in_user_init_block",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN Init */",
                              "/* USER CODE END Init */",
                              "boot_report_init(HAL_GetTick());") != 0);
    failures += expect_true(
        "report_printed_once",
        count_text(main_source, "boot_report_print(HAL_GetTick());") == 1U);
    failures += expect_true(
        "old_system_ready_removed",
        strstr(main_source, "System Ready.") == NULL);
    failures += expect_true(
        "old_help_prompt_removed",
        strstr(main_source, "Input <help / ?>") == NULL);

    failures += expect_true(
        "probe_command_handler_exists",
        strstr(command_source, "command_probe_handler") != NULL);
    failures += expect_true(
        "probe_status_prints_cache",
        function_body_contains(command_source, "command_probe_handler",
                               "boot_probe_print_status();") != 0);
    failures += expect_true(
        "probe_run_starts_at_current_tick",
        function_body_contains(command_source, "command_probe_handler",
                               "boot_probe_start(HAL_GetTick());") != 0);
    failures += expect_true(
        "probe_command_does_not_clear_fault",
        function_body_contains(command_source, "command_probe_handler",
                               "diagnostics_clear_first_fault") == 0);
    failures += expect_true(
        "probe_command_does_not_wait",
        function_body_contains(command_source, "command_probe_handler",
                               "boot_probe_is_complete") == 0);
    failures += expect_true(
        "probe_command_usage_is_ascii",
        strstr(command_source,
               "INVALID_ARG usage=probe status|run\\r\\n") != NULL);
    failures += expect_true(
        "probe_command_registered_once",
        count_text(command_source, "{\"probe\",") == 1U);
    failures += expect_true(
        "probe_help_has_chinese_utf8_escape",
        strstr(command_source,
               "{\"probe\", \"probe status|run - \\xE6") != NULL);
    failures += expect_true(
        "sensor_includes_imu_service",
        strstr(command_source, "#include \"imu_service.h\"") != NULL);
    failures += expect_true(
        "sensor_command_handler_exists",
        strstr(command_source, "command_sensor_handler") != NULL);
    failures += expect_true(
        "sensor_imu_status_prints_cache",
        function_body_contains(command_source, "command_sensor_handler",
                               "imu_print_status();") != 0);
    failures += expect_true(
        "sensor_imu_probe_uses_bounded_readonly_probe",
        function_body_contains(command_source, "command_sensor_handler",
                               "imu_probe_readonly(10U)") != 0);
    failures += expect_true(
        "sensor_includes_st480mc_service",
        strstr(command_source, "#include \"st480mc_service.h\"") != NULL);
    failures += expect_true(
        "sensor_st480mc_status_prints_cache",
        function_body_contains(command_source, "command_sensor_handler",
                               "st480mc_print_status();") != 0);
    failures += expect_true(
        "sensor_st480mc_probe_uses_bounded_readonly_probe",
        function_body_contains(command_source, "command_sensor_handler",
                               "st480mc_probe_readonly(10U)") != 0);
    failures += expect_true(
        "sensor_includes_pressure_service",
        strstr(command_source, "#include \"pressure_service.h\"") != NULL);
    failures += expect_true(
        "sensor_pressure_status_prints_cache",
        function_body_contains(command_source, "command_sensor_handler",
                               "pressure_print_status();") != 0);
    failures += expect_true(
        "sensor_pressure_probe_is_bounded",
        function_body_contains(command_source, "command_sensor_handler",
                               "pressure_probe_readonly(10U)") != 0);
    failures += expect_true(
        "sensor_pressure_read_is_bounded",
        function_body_contains(command_source, "command_sensor_handler",
                               "pressure_read_once(500U)") != 0);
    failures += expect_true(
        "sensor_includes_ap3216c_service",
        strstr(command_source, "#include \"ap3216c_service.h\"") != NULL);
    failures += expect_true(
        "sensor_ap3216c_status_prints_cache",
        function_body_contains(command_source, "command_sensor_handler",
                               "ap3216c_print_status();") != 0);
    failures += expect_true(
        "sensor_ap3216c_probe_is_bounded",
        function_body_contains(command_source, "command_sensor_handler",
                               "ap3216c_probe_readonly(10U)") != 0);
    failures += expect_true(
        "sensor_ap3216c_read_is_bounded",
        function_body_contains(command_source, "command_sensor_handler",
                               "ap3216c_read_once(300U)") != 0);
    failures += expect_true(
        "sensor_command_usage_is_ascii",
        strstr(command_source,
               "INVALID_ARG usage=sensor imu|sh3001|st480mc status|probe | sensor pressure|ap3216c status|probe|read\\r\\n") != NULL);
    failures += expect_true(
        "sensor_command_registered_once",
        count_text(command_source, "{\"sensor\",") == 1U);
    failures += expect_true(
        "sensor_help_has_chinese_utf8_escape",
        strstr(command_source,
               "{\"sensor\", \"sensor imu|sh3001|st480mc status|probe | sensor pressure|ap3216c status|probe|read - \\xE6") != NULL);
    failures += expect_true(
        "eeprom_includes_at24c02_service",
        strstr(command_source, "#include \"at24c02_service.h\"") != NULL);
    failures += expect_true(
        "eeprom_command_handler_exists",
        strstr(command_source, "command_eeprom_handler") != NULL);
    failures += expect_true(
        "eeprom_status_prints_cache",
        function_body_contains(command_source, "command_eeprom_handler",
                               "at24c02_print_status();") != 0);
    failures += expect_true(
        "eeprom_probe_is_bounded",
        function_body_contains(command_source, "command_eeprom_handler",
                               "at24c02_probe_readonly(10U)") != 0);
    failures += expect_true(
        "eeprom_read_uses_service",
        function_body_contains(command_source, "command_eeprom_handler",
                               "at24c02_read(") != 0);
    failures += expect_true(
        "eeprom_read_buffer_is_64_bytes",
        strstr(command_source, "#define EEPROM_COMMAND_MAX_READ       64U") != NULL);
    failures += expect_true(
        "eeprom_has_no_write_action",
        function_body_contains(command_source, "command_eeprom_handler",
                               "board_i2c_write_reg") == 0);
    failures += expect_true(
        "eeprom_has_no_erase_action",
        function_body_contains(command_source, "command_eeprom_handler",
                               "\"erase\"") == 0);
    failures += expect_true(
        "eeprom_has_no_test_action",
        function_body_contains(command_source, "command_eeprom_handler",
                               "\"test\"") == 0);
    failures += expect_true(
        "eeprom_usage_is_ascii",
        strstr(command_source,
               "INVALID_ARG usage=eeprom status|probe|read <offset_0_255> <length_1_64>\\r\\n") != NULL);
    failures += expect_true(
        "eeprom_command_registered_once",
        count_text(command_source, "{\"eeprom\",") == 1U);
    failures += expect_true(
        "eeprom_help_has_chinese_utf8_escape",
        strstr(command_source,
               "{\"eeprom\", \"eeprom status|probe|read <offset> <length> - \\xE6") != NULL);
    failures += expect_true(
        "adc_includes_measurement_service",
        strstr(command_source, "#include \"adc_measurement_service.h\"") != NULL);
    failures += expect_true(
        "adc_command_handler_exists",
        strstr(command_source, "command_adc_handler") != NULL);
    failures += expect_true(
        "adc_status_prints_cache",
        function_body_contains(command_source, "command_adc_handler",
                               "adc_measurement_print_status();") != 0);
    failures += expect_true(
        "adc_external_read_is_bounded",
        function_body_contains(command_source, "command_adc_handler",
                               "adc_measurement_read_external(50U)") != 0);
    failures += expect_true(
        "adc_temperature_read_is_bounded",
        function_body_contains(command_source, "command_adc_handler",
                               "adc_measurement_read_temperature(50U)") != 0);
    failures += expect_true(
        "adc_all_read_is_bounded",
        function_body_contains(command_source, "command_adc_handler",
                               "adc_measurement_read_all(50U)") != 0);
    failures += expect_true(
        "adc_usage_is_ascii",
        strstr(command_source,
               "INVALID_ARG usage=adc status|read external|temperature|all\\r\\n") != NULL);
    failures += expect_true(
        "adc_command_registered_once",
        count_text(command_source, "{\"adc\",") == 1U);
    failures += expect_true(
        "adc_help_has_chinese_utf8_escape",
        strstr(command_source,
               "{\"adc\", \"adc status|read external|temperature|all - \\xE8") != NULL);
    failures += expect_true(
        "tpad_includes_service",
        strstr(command_source, "#include \"tpad_service.h\"") != NULL);
    failures += expect_true(
        "tpad_command_handler_exists",
        strstr(command_source, "command_tpad_handler") != NULL);
    failures += expect_true(
        "tpad_status_prints_cache",
        function_body_contains(command_source, "command_tpad_handler",
                               "tpad_print_status();") != 0);
    failures += expect_true(
        "tpad_calibrate_is_bounded",
        function_body_contains(command_source, "command_tpad_handler",
                               "tpad_calibrate(300U)") != 0);
    failures += expect_true(
        "tpad_read_is_bounded",
        function_body_contains(command_source, "command_tpad_handler",
                               "tpad_read(100U)") != 0);
    failures += expect_true(
        "tpad_usage_is_ascii",
        strstr(command_source,
               "INVALID_ARG usage=tpad status|calibrate|read\\r\\n") != NULL);
    failures += expect_true(
        "tpad_command_registered_once",
        count_text(command_source, "{\"tpad\",") == 1U);
    failures += expect_true(
        "tpad_help_has_chinese_utf8_escape",
        strstr(command_source,
               "{\"tpad\", \"tpad status|calibrate|read - \\xE6") != NULL);
    failures += expect_true(
        "project_has_tpad_service_name_once",
        count_text(project_source,
                   "<FileName>tpad_service.c</FileName>") == 1U);
    failures += expect_true(
        "project_has_tpad_service_path_once",
        count_text(project_source,
                   "<FilePath>..\\Core\\Peripheral\\Src\\tpad_service.c</FilePath>") == 1U);
    failures += expect_true(
        "project_has_ir_remote_service_name_once",
        count_text(project_source,
                   "<FileName>ir_remote_service.c</FileName>") == 1U);
    failures += expect_true(
        "project_has_ir_remote_service_path_once",
        count_text(project_source,
                   "<FilePath>..\\Core\\Peripheral\\Src\\ir_remote_service.c</FilePath>") == 1U);
    failures += expect_true(
        "main_tpad_include_in_user_block",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN Includes */",
                              "/* USER CODE END Includes */",
                              "#include \"tpad_service.h\"") != 0);
    failures += expect_true(
        "main_tpad_init_in_user_block",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN 2 */",
                              "/* USER CODE END 2 */",
                              "tpad_service_init();") != 0);
    failures += expect_true(
        "main_tpad_does_not_measure_at_boot",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN 2 */",
                              "/* USER CODE END 2 */",
                              "tpad_calibrate(") == 0);
    failures += expect_true(
        "ir_includes_service",
        strstr(command_source, "#include \"ir_remote_service.h\"") != NULL);
    failures += expect_true(
        "ir_command_handler_exists",
        strstr(command_source, "command_ir_handler") != NULL);
    failures += expect_true(
        "ir_status_prints_cache",
        function_body_contains(command_source, "command_ir_handler",
                               "ir_remote_print_status();") != 0);
    failures += expect_true(
        "ir_read_gets_snapshot",
        function_body_contains(command_source, "command_ir_handler",
                               "ir_remote_get_status") != 0);
    failures += expect_true(
        "ir_clear_calls_service",
        function_body_contains(command_source, "command_ir_handler",
                               "ir_remote_clear();") != 0);
    failures += expect_true(
        "ir_usage_is_ascii",
        strstr(command_source,
               "INVALID_ARG usage=ir status|read|clear\\r\\n") != NULL);
    failures += expect_true(
        "ir_command_registered_once",
        count_text(command_source, "{\"ir\",") == 1U);
    failures += expect_true(
        "ir_help_has_chinese_utf8_escape",
        strstr(command_source,
               "{\"ir\", \"ir status|read|clear - \\xE6") != NULL);
    failures += expect_true(
        "ir_service_is_isr_silent",
        function_body_contains(ir_source, "ir_remote_capture_pulse",
                               "printf") == 0);
    failures += expect_true(
        "main_ir_include_in_user_block",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN Includes */",
                              "/* USER CODE END Includes */",
                              "#include \"ir_remote_service.h\"") != 0);
    failures += expect_true(
        "main_ir_init_in_user_block",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN 2 */",
                              "/* USER CODE END 2 */",
                              "ir_remote_service_init();") != 0);
    failures += expect_true(
        "main_tim1_dispatches_ir_timeout",
        function_body_contains(main_source, "HAL_TIM_PeriodElapsedCallback",
                               "ir_remote_timeout();") != 0);
    failures += expect_true(
        "ir_runtime_configures_pullup",
        strstr(ir_source, "gpio.Pull = GPIO_PULLUP") != NULL);
    failures += expect_true(
        "ir_runtime_configures_filter",
        strstr(ir_source, "input.ICFilter = 3U") != NULL);
    failures += expect_true(
        "ir_runtime_starts_capture_interrupt",
        strstr(ir_source,
               "HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_1)") != NULL);
    failures += expect_true(
        "ir_runtime_enables_update_interrupt",
        strstr(ir_source,
               "__HAL_TIM_ENABLE_IT(&htim1, TIM_IT_UPDATE)") != NULL);
    failures += expect_true(
        "ir_capture_switches_to_falling_edge",
        function_body_contains(ir_source, "HAL_TIM_IC_CaptureCallback",
                               "TIM_INPUTCHANNELPOLARITY_FALLING") != 0);
    failures += expect_true(
        "ir_capture_switches_to_rising_edge",
        function_body_contains(ir_source, "HAL_TIM_IC_CaptureCallback",
                               "TIM_INPUTCHANNELPOLARITY_RISING") != 0);
    failures += expect_true(
        "ir_capture_resets_counter_at_rising_edge",
        function_body_contains(ir_source, "HAL_TIM_IC_CaptureCallback",
                               "__HAL_TIM_SET_COUNTER(htim, 0U)") != 0);
    failures += expect_true(
        "es8388_address_is_0x10",
        strstr(es8388_source, "ES8388_I2C_ADDRESS 0x10U") != NULL);
    failures += expect_true(
        "es8388_probe_uses_device_ready",
        function_body_contains(es8388_source, "es8388_probe_readonly",
                               "board_i2c_device_ready") != 0);
    failures += expect_true(
        "es8388_volume_uses_write_service",
        function_body_contains(es8388_source, "es8388_set_volume_percent",
                               "board_i2c_write_reg") != 0);
    failures += expect_true(
        "es8388_has_no_boot_probe",
        strstr(main_source, "es8388_probe_readonly") == NULL);
    failures += expect_true(
        "es8388_service_has_no_delay",
        strstr(es8388_source, "HAL_Delay") == NULL &&
        strstr(es8388_source, "delay_ms") == NULL);
    failures += expect_true(
        "mp3_decoder_has_no_dynamic_alloc",
        strstr(mp3_decoder_source, "mymalloc") == NULL &&
        strstr(mp3_decoder_source, "malloc") == NULL);
    failures += expect_true(
        "mp3_decoder_has_no_delay",
        strstr(mp3_decoder_source, "delay_ms") == NULL &&
        strstr(mp3_decoder_source, "HAL_Delay") == NULL);
    failures += expect_true(
        "mp3_decoder_decode_has_no_print",
        function_body_contains(mp3_decoder_source,
                               "mp3_decoder_decode_next", "printf") == 0);
    failures += expect_true(
        "mp3_decoder_uses_static_helix_buffers",
        strstr(mp3_buffers_source, "#define static_buffers") != NULL);
    failures += expect_true(
        "audio_has_no_lvgl_dependency",
        strstr(audio_source, "lv_") == NULL);
    failures += expect_true(
        "audio_dma_callback_has_no_print",
        function_body_contains(audio_source, "HAL_SAI_TxHalfCpltCallback",
                               "printf") == 0);
    failures += expect_true(
        "audio_pcm_cache_clean",
        strstr(audio_source, "SCB_CleanDCache_by_Addr") != NULL);
    failures += expect_true(
        "audio_sai_clears_osr_for_48k",
        strstr(audio_source, "CLEAR_BIT(hsai_BlockA1.Instance->CR1, SAI_xCR1_OSR)") != NULL);
    failures += expect_true(
        "main_audio_include_in_user_block",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN Includes */",
                              "/* USER CODE END Includes */",
                              "#include \"audio_playback_service.h\"") != 0);
    failures += expect_true(
        "main_audio_init_in_user_block",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN 2 */",
                              "/* USER CODE END 2 */",
                              "audio_service_init();") != 0);
    failures += expect_true(
        "main_audio_poll_in_user_loop",
        text_between_contains(main_source,
                              "/* USER CODE BEGIN WHILE */",
                              "/* USER CODE END WHILE */",
                              "audio_poll();") != 0);
    failures += expect_true(
        "audio_command_registered_once",
        count_text(command_source, "{\"audio\",") == 1U);
    failures += expect_true(
        "audio_command_has_ascii_usage",
        strstr(command_source,
               "usage=audio status|probe|play <sd_path>|record <sd_path>|stop|volume <0-100>") != NULL);
    failures += expect_true(
        "audio_slider_uses_custom_bridge",
        count_text(events_source, "audio_volume_slider_changed(val);") == 1U);
    failures += expect_true(
        "audio_custom_bridge_queues_music_service",
        function_body_contains(custom_source,
                               "audio_volume_slider_changed",
                               "music_player_request_volume") != 0);

    free(gt_header);
    free(gt_source);
    free(pcf_header);
    free(pcf_source);
    free(qspi_header);
    free(qspi_source);
    free(nor_header);
    free(nor_source);
    free(target_source);
    free(project_source);
    free(main_source);
    free(command_source);
    free(tpad_header);
    free(tpad_source);
    free(ir_source);
    free(es8388_header);
    free(es8388_source);
    free(mp3_decoder_source);
    free(mp3_buffers_source);
    free(audio_source);
    free(custom_source);
    free(events_source);

    if (failures != 0)
    {
        printf("FAIL boot_probe_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS boot_probe_contract\n");
    return 0;
}
