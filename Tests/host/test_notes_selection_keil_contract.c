#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    const char *name;
    const char *path;
    const char *group;
} source_expectation_t;

static char *read_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    char *data;
    long size;

    if (file == NULL) return NULL;
    if (fseek(file, 0L, SEEK_END) != 0 || (size = ftell(file)) < 0L ||
        fseek(file, 0L, SEEK_SET) != 0) {
        (void)fclose(file);
        return NULL;
    }
    data = (char *)malloc((size_t)size + 1U);
    if (data == NULL || fread(data, 1U, (size_t)size, file) != (size_t)size) {
        free(data);
        (void)fclose(file);
        return NULL;
    }
    data[size] = '\0';
    (void)fclose(file);
    return data;
}

static size_t count_text(const char *text, const char *needle)
{
    const char *cursor = text;
    size_t count = 0U;
    size_t length = strlen(needle);

    while ((cursor = strstr(cursor, needle)) != NULL) {
        count++;
        cursor += length;
    }
    return count;
}

static int require_group_entry(const char *xml, const source_expectation_t *item)
{
    char group_tag[96];
    char name_tag[128];
    char path_tag[256];
    const char *group_start;
    const char *group_end;
    const char *name_pos;
    const char *path_pos;

    (void)snprintf(group_tag, sizeof(group_tag),
                   "<GroupName>%s</GroupName>", item->group);
    (void)snprintf(name_tag, sizeof(name_tag),
                   "<FileName>%s</FileName>", item->name);
    (void)snprintf(path_tag, sizeof(path_tag),
                   "<FilePath>%s</FilePath>", item->path);
    if (count_text(xml, name_tag) != 1U || count_text(xml, path_tag) != 1U) {
        printf("FAIL unique %s\n", item->name);
        return 1;
    }
    group_start = strstr(xml, group_tag);
    group_end = group_start == NULL ? NULL : strstr(group_start, "</Group>");
    name_pos = group_start == NULL ? NULL : strstr(group_start, name_tag);
    path_pos = group_start == NULL ? NULL : strstr(group_start, path_tag);
    if (group_start == NULL || group_end == NULL || name_pos == NULL ||
        path_pos == NULL || name_pos >= group_end || path_pos >= group_end) {
        printf("FAIL group %s\n", item->name);
        return 1;
    }
    return 0;
}

int main(void)
{
    static const source_expectation_t expected[] = {
        {"notes_utf8_internal.c", "..\\Core\\Application\\Src\\notes_utf8_internal.c", "Application_Services"},
        {"notes_document.c", "..\\Core\\Application\\Src\\notes_document.c", "Application_Services"},
        {"notes_storage_codec.c", "..\\Core\\Application\\Src\\notes_storage_codec.c", "Application_Services"},
        {"notes_storage_platform.c", "..\\Core\\Application\\Src\\notes_storage_platform.c", "Application_Services"},
        {"notes_storage_service.c", "..\\Core\\Application\\Src\\notes_storage_service.c", "Application_Services"},
        {"text_selection_controller.c", "..\\Core\\Application\\Src\\text_selection_controller.c", "Application_Services"},
        {"selection_textarea_adapter.c", "..\\lvgl_GUI\\custom\\selection_textarea_adapter.c", "LVGL_GUI"},
        {"selection_keyboard_maps.c", "..\\lvgl_GUI\\custom\\selection_keyboard_maps.c", "LVGL_GUI"},
        {"selection_keyboard.c", "..\\lvgl_GUI\\custom\\selection_keyboard.c", "LVGL_GUI"},
        {"notes_editor_refresh_guard.c", "..\\lvgl_GUI\\custom\\notes_editor_refresh_guard.c", "LVGL_GUI"},
        {"notes_editor_selection_target.c", "..\\lvgl_GUI\\custom\\notes_editor_selection_target.c", "LVGL_GUI"},
        {"notes_editor_ui.c", "..\\lvgl_GUI\\custom\\notes_editor_ui.c", "LVGL_GUI"}
    };
    char *xml = read_file("MDK-ARM/H743_LCD_Keil5.uvprojx");
    size_t i;
    int failures = 0;

    if (xml == NULL) {
        printf("FAIL project_open\n");
        return 1;
    }
    for (i = 0U; i < sizeof(expected) / sizeof(expected[0]); i++) {
        failures += require_group_entry(xml, &expected[i]);
    }
    free(xml);
    if (failures != 0) return 1;
    printf("PASS notes_selection_keil_contract\n");
    return 0;
}
