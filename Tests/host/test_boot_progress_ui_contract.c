#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int expect_contains(const char *source, const char *name,
                           const char *needle)
{
    if (strstr(source, needle) == NULL)
    {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

static int expect_absent(const char *source, const char *name,
                         const char *needle)
{
    if (strstr(source, needle) != NULL)
    {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

static char *read_source(const char *path)
{
    FILE *file;
    long size;
    char *data;

    file = fopen(path, "rb");
    if (file == NULL)
    {
        return NULL;
    }
    if ((fseek(file, 0L, SEEK_END) != 0) ||
        ((size = ftell(file)) < 0L) ||
        (fseek(file, 0L, SEEK_SET) != 0))
    {
        (void)fclose(file);
        return NULL;
    }

    data = (char *)malloc((size_t)size + 1U);
    if (data == NULL)
    {
        (void)fclose(file);
        return NULL;
    }
    if (fread(data, 1U, (size_t)size, file) != (size_t)size)
    {
        free(data);
        (void)fclose(file);
        return NULL;
    }
    data[size] = '\0';
    (void)fclose(file);
    return data;
}

int main(void)
{
    static const char *const required_source[] =
    {
        "#define BOOT_UI_SCREEN_WIDTH 1024",
        "#define BOOT_UI_SCREEN_HEIGHT 600",
        "#define BOOT_UI_LEFT_X 40",
        "#define BOOT_UI_LEFT_Y 105",
        "#define BOOT_UI_LEFT_WIDTH 528",
        "#define BOOT_UI_LEFT_HEIGHT 390",
        "#define BOOT_UI_RIGHT_X 584",
        "#define BOOT_UI_RIGHT_Y 105",
        "#define BOOT_UI_RIGHT_WIDTH 400",
        "#define BOOT_UI_RIGHT_HEIGHT 390",
        "#define BOOT_UI_ERROR_X 40",
        "#define BOOT_UI_ERROR_Y 515",
        "#define BOOT_UI_ERROR_WIDTH 944",
        "#define BOOT_UI_ERROR_HEIGHT 40",
        "#define BOOT_UI_PANEL_OPA 166",
        "#define BOOT_UI_BAR_WIDTH 456",
        "#define BOOT_UI_BAR_HEIGHT 18",
        "#define BOOT_UI_DEVICE_COLUMNS 2",
        "#define BOOT_UI_DEVICE_ROWS 5",
        "#define BOOT_UI_COLOR_SUCCESS 0x35C46A",
        "#define BOOT_UI_COLOR_WARNING 0xF2C94C",
        "#define BOOT_UI_COLOR_ERROR 0xFF5A5F",
        "g_device_labels[BOOT_DEVICE_COUNT]",
        "boot_progress_ui_create(void)",
        "boot_progress_ui_set_wallpaper(const lv_image_dsc_t *wallpaper)",
        "boot_progress_ui_set_font(const lv_font_t *font)",
        "boot_progress_ui_set_chinese(bool enabled)",
        "boot_progress_ui_refresh(const boot_progress_snapshot_t *snapshot)",
        "boot_progress_ui_destroy(void)"
    };
    static const char *const forbidden_source[] =
    {
        "HAL_", "f_mount", "f_open", "QSPI_", "SDMMC", "IRQHandler"
    };
    char *header;
    char *source;
    unsigned int index;
    int failures = 0;

    source = read_source("lvgl_GUI/custom/boot_progress_ui.c");
    if (source == NULL)
    {
        printf("FAIL source_open\n");
        return 1;
    }
    header = read_source("lvgl_GUI/custom/boot_progress_ui.h");
    if (header == NULL)
    {
        free(source);
        printf("FAIL header_open\n");
        return 1;
    }

    for (index = 0U;
         index < (sizeof(required_source) / sizeof(required_source[0]));
         index++)
    {
        failures += expect_contains(source, "required_source",
                                    required_source[index]);
    }
    failures += expect_contains(header, "header_create",
                                "bool boot_progress_ui_create(void);");
    failures += expect_contains(header, "header_wallpaper",
                                "boot_progress_ui_set_wallpaper");
    failures += expect_contains(header, "header_font",
                                "boot_progress_ui_set_font");
    failures += expect_contains(header, "header_chinese",
                                "boot_progress_ui_set_chinese");
    failures += expect_contains(header, "header_refresh",
                                "boot_progress_ui_refresh");
    failures += expect_contains(header, "header_destroy",
                                "boot_progress_ui_destroy");
    failures += expect_contains(source, "chinese_mode_state",
                                "static bool g_use_chinese;");
    failures += expect_contains(source, "chinese_system_title",
                                "\\xE7\\xB3\\xBB\\xE7\\xBB\\x9F\\xE5\\x90\\xAF\\xE5\\x8A\\xA8");
    failures += expect_contains(source, "chinese_peripheral_title",
                                "\\xE5\\xA4\\x96\\xE8\\xAE\\xBE\\xE7\\x8A\\xB6\\xE6\\x80\\x81");
    failures += expect_contains(source, "chinese_pending",
                                "\\xE7\\xAD\\x89\\xE5\\xBE\\x85");

    for (index = 0U;
         index < (sizeof(forbidden_source) / sizeof(forbidden_source[0]));
         index++)
    {
        failures += expect_absent(source, "forbidden_dependency",
                                  forbidden_source[index]);
    }

    free(header);
    free(source);
    if (failures == 0)
    {
        printf("PASS boot_progress_ui_contract\n");
        return 0;
    }
    printf("FAIL boot_progress_ui_contract failures=%d\n", failures);
    return 1;
}
