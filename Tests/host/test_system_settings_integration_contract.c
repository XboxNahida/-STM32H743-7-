#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_text(const char *path)
{
    FILE *file;
    long length;
    char *text;

    file = fopen(path, "rb");
    if (file == NULL)
    {
        return NULL;
    }
    if ((fseek(file, 0L, SEEK_END) != 0) ||
        ((length = ftell(file)) < 0L) ||
        (fseek(file, 0L, SEEK_SET) != 0))
    {
        fclose(file);
        return NULL;
    }
    text = (char *)malloc((size_t)length + 1U);
    if (text == NULL)
    {
        fclose(file);
        return NULL;
    }
    if (fread(text, 1U, (size_t)length, file) != (size_t)length)
    {
        free(text);
        fclose(file);
        return NULL;
    }
    text[length] = '\0';
    fclose(file);
    return text;
}

static int expect_contains(const char *name, const char *text,
                           const char *needle)
{
    if ((text == NULL) || (strstr(text, needle) == NULL))
    {
        printf("FAIL %s missing=%s\n", name, needle);
        return 1;
    }
    return 0;
}

static int expect_order(const char *name, const char *text,
                        const char *first, const char *second)
{
    const char *first_at = (text != NULL) ? strstr(text, first) : NULL;
    const char *second_at = (text != NULL) ? strstr(text, second) : NULL;

    if ((first_at == NULL) || (second_at == NULL) || (first_at >= second_at))
    {
        printf("FAIL %s order=%s before %s\n", name, first, second);
        return 1;
    }
    return 0;
}

int main(void)
{
    char *rtc_header = read_text("Core/Inc/rtc.h");
    char *rtc_source = read_text("Core/Src/rtc.c");
    char *main_source = read_text("Core/Src/main.c");
    int failures = 0;

    failures += expect_contains("rtc_callback_type", rtc_header,
        "typedef void (*rtc_change_callback_t)(void);");
    failures += expect_contains("rtc_backup_api", rtc_header,
        "uint8_t rtc_backup_was_valid(void);");
    failures += expect_contains("rtc_callback_api", rtc_header,
        "rtc_set_change_callback(rtc_change_callback_t callback);");
    failures += expect_contains("rtc_captures_original_backup", rtc_source,
        "g_rtc_backup_was_valid = (uint8_t)(HAL_RTCEx_BKUPRead");
    failures += expect_contains("rtc_time_notifies", rtc_source,
        "rtc_notify_change();");

    failures += expect_contains("main_settings_include", main_source,
        "#include \"system_settings_service.h\"");
    failures += expect_order("main_after_eeprom_probe", main_source,
        "at24c02_probe_readonly(10U);", "system_settings_bootstrap();");
    failures += expect_contains("main_registers_rtc_callback", main_source,
        "rtc_set_change_callback(system_settings_rtc_changed);");
    failures += expect_contains("main_polls_settings", main_source,
        "system_settings_checkpoint_poll(HAL_GetTick());");

    free(rtc_header);
    free(rtc_source);
    free(main_source);
    if (failures != 0)
    {
        printf("FAIL system_settings_integration_contract failures=%d\n",
               failures);
        return 1;
    }
    printf("PASS system_settings_integration_contract\n");
    return 0;
}
