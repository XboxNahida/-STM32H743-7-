#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    long length;
    char *text;

    if ((file == NULL) || (fseek(file, 0L, SEEK_END) != 0) ||
        ((length = ftell(file)) < 0L) || (fseek(file, 0L, SEEK_SET) != 0))
    {
        if (file != NULL) fclose(file);
        return NULL;
    }
    text = (char *)malloc((size_t)length + 1U);
    if ((text == NULL) ||
        (fread(text, 1U, (size_t)length, file) != (size_t)length))
    {
        free(text);
        fclose(file);
        return NULL;
    }
    text[length] = '\0';
    fclose(file);
    return text;
}

static int require_text(const char *text, const char *needle,
                        const char *label)
{
    if (strstr(text, needle) == NULL)
    {
        printf("FAIL %s\n", label);
        return 1;
    }
    return 0;
}

int main(void)
{
    char *source = read_file("lvgl/porting/lv_port_disp.c");
    char *header = read_file("lvgl/porting/lv_port_disp.h");
    char *command = read_file("Core/Peripheral/Src/command_parser.c");
    const char *flush;
    const char *last;
    const char *clean;
    const char *disabled;
    const char *reload;
    const char *barrier;
    const char *pending_clear;
    int failures = 0;

    if ((source == NULL) || (header == NULL) || (command == NULL))
    {
        printf("FAIL read display port\n");
        free(source);
        free(header);
        free(command);
        return 1;
    }

    failures += require_text(source, "#include \"sdram_layout.h\"",
                             "shared SDRAM layout used");
    failures += require_text(source, "#if LV_USE_LTDC_FULL_FRAME_BUFFER",
                             "full frame compile branch");
    failures += require_text(source, "(void *)SDRAM_LVGL_BACK_FRAME_ADDR",
                             "off-screen buffer is first");
    failures += require_text(source, "(void *)SDRAM_LVGL_FRAME_ADDR",
                             "front buffer is second");
    failures += require_text(source, "LV_DISPLAY_RENDER_MODE_DIRECT",
                             "direct render mode");
    failures += require_text(source, "lv_display_set_flush_wait_cb",
                             "reload wait callback installed");
    failures += require_text(source, "lv_display_flush_is_last(disp)",
                             "swap only after last dirty area");
    failures += require_text(source, "HAL_LTDC_SetAddress_NoReload",
                             "address staged without immediate reload");
    failures += require_text(source, "LTDC_RELOAD_VERTICAL_BLANKING",
                             "vertical blank reload used");
    failures += require_text(source, "DISP_RELOAD_TIMEOUT_MS",
                             "reload wait is bounded");
    failures += require_text(source, "LV_DISPLAY_RENDER_MODE_PARTIAL",
                             "legacy partial mode retained");
    failures += require_text(header, "uint32_t disp_get_frame_address(void);",
                             "active frame query exported");
    failures += require_text(header, "void disp_on_ltdc_reload(void);",
                             "reload notification exported");
    failures += require_text(header, "} disp_diagnostics_t;",
                             "display diagnostics type exported");
    failures += require_text(
        header,
        "void disp_get_diagnostics(disp_diagnostics_t *diagnostics);",
        "display diagnostics query exported");
    failures += require_text(source, "g_reload_request_count++;",
                             "reload requests counted");
    failures += require_text(source, "g_reload_callback_count++;",
                             "reload callbacks counted");
    failures += require_text(source, "g_reload_timeout_count++;",
                             "reload wait timeouts counted");
    failures += require_text(source, "g_reload_failure_count++;",
                             "reload HAL failures counted");
    failures += require_text(
        source,
        "LTDC_LAYER(&hltdc, LTDC_LAYER_1)->CFBAR",
        "programmed frame address sampled");
    failures += require_text(source, "hltdc.Instance->SRCR",
                             "reload request register sampled");
    failures += require_text(source, "hltdc.Instance->ISR",
                             "reload interrupt register sampled");
    failures += require_text(header, "fifo_underrun_count",
                             "FIFO underruns exported");
    failures += require_text(header, "transfer_error_count",
                             "transfer errors exported");
    failures += require_text(header, "last_error_cpsr",
                             "error scan position exported");
    failures += require_text(header, "last_error_frame_addr",
                             "error frame address exported");
    failures += require_text(source, "void HAL_LTDC_ErrorCallback",
                             "LTDC errors captured in ISR callback");
    failures += require_text(source, "HAL_LTDC_ERROR_FU",
                             "FIFO underruns counted independently");
    failures += require_text(source, "HAL_LTDC_ERROR_TE",
                             "transfer errors counted independently");
    failures += require_text(source, "LTDC_IT_TE | LTDC_IT_FU",
                             "HAL-disabled error interrupts re-enabled");
    failures += require_text(command, "fifo_underruns=%lu transfer_errors=%lu",
                             "LTDC errors visible in lvgl status");
    flush = strstr(source, "static void disp_flush(lv_display_t * disp");
    last = (flush != NULL) ?
        strstr(flush, "if (!lv_display_flush_is_last(disp))") : NULL;
    clean = (flush != NULL) ? strstr(flush, "SCB_CleanDCache();") : NULL;
    disabled = (flush != NULL) ?
        strstr(flush, "if (!disp_flush_enabled)") : NULL;
    if ((last == NULL) || (clean == NULL) || (disabled == NULL) ||
        (last > clean) || (clean > disabled))
    {
        printf("FAIL last direct flush cleans all cache before disabled check\n");
        failures++;
    }
    if (strstr(source, "disp_clean_direct_area") != NULL)
    {
        printf("FAIL full-frame mode still uses partial cache clean\n");
        failures++;
    }
    reload = strstr(source, "void disp_on_ltdc_reload(void)");
    barrier = (reload != NULL) ? strstr(reload, "__DMB();") : NULL;
    pending_clear = (reload != NULL) ?
        strstr(reload, "g_reload_pending = 0U;") : NULL;
    if ((barrier == NULL) || (pending_clear == NULL) ||
        (barrier > pending_clear))
    {
        printf("FAIL reload publishes address before pending clear\n");
        failures++;
    }

    free(source);
    free(header);
    free(command);
    if (failures == 0)
    {
        printf("PASS lvgl_display_double_buffer_contract\n");
    }
    return failures;
}
