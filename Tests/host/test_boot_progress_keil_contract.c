#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_source(const char *path)
{
    FILE *file = fopen(path, "rb");
    long size;
    char *data;

    if (file == NULL) { return NULL; }
    if ((fseek(file, 0L, SEEK_END) != 0) ||
        ((size = ftell(file)) < 0L) ||
        (fseek(file, 0L, SEEK_SET) != 0)) {
        (void)fclose(file);
        return NULL;
    }
    data = (char *)malloc((size_t)size + 1U);
    if (data == NULL) { (void)fclose(file); return NULL; }
    if (fread(data, 1U, (size_t)size, file) != (size_t)size) {
        free(data);
        (void)fclose(file);
        return NULL;
    }
    data[size] = '\0';
    (void)fclose(file);
    return data;
}

static unsigned int count_text(const char *text, const char *needle)
{
    unsigned int count = 0U;
    size_t length = strlen(needle);

    while ((text = strstr(text, needle)) != NULL) {
        count++;
        text += length;
    }
    return count;
}

static int group_contains(const char *project, const char *group_name,
                          const char *file_path)
{
    char marker[96];
    const char *group;
    const char *end;
    const char *file;

    (void)snprintf(marker, sizeof(marker), "<GroupName>%s</GroupName>",
                   group_name);
    group = strstr(project, marker);
    if (group == NULL) { return 0; }
    end = strstr(group, "</Group>");
    if (end == NULL) { return 0; }
    file = strstr(group, file_path);
    return ((file != NULL) && (file < end)) ? 1 : 0;
}

static int expect_true(const char *name, int condition)
{
    if (condition == 0) {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

int main(void)
{
    static const char *const service_path =
        "..\\Core\\Application\\Src\\boot_progress_service.c";
    static const char *const ui_path =
        "..\\lvgl_GUI\\custom\\boot_progress_ui.c";
    char *project = read_source("MDK-ARM/H743_LCD_Keil5.uvprojx");
    int failures = 0;

    if (project == NULL) {
        printf("FAIL project_open\n");
        return 1;
    }
    failures += expect_true("service_path_once",
                            count_text(project, service_path) == 1U);
    failures += expect_true("service_in_application_group",
                            group_contains(project, "Application_Services",
                                           service_path) != 0);
    failures += expect_true("ui_path_once",
                            count_text(project, ui_path) == 1U);
    failures += expect_true("ui_in_lvgl_group",
                            group_contains(project, "LVGL_GUI", ui_path) != 0);
    failures += expect_true("service_filename_once",
                            count_text(project,
                                       "<FileName>boot_progress_service.c</FileName>") == 1U);
    failures += expect_true("ui_filename_once",
                            count_text(project,
                                       "<FileName>boot_progress_ui.c</FileName>") == 1U);
    free(project);

    if (failures == 0) {
        printf("PASS boot_progress_keil_contract\n");
        return 0;
    }
    printf("FAIL boot_progress_keil_contract failures=%d\n", failures);
    return 1;
}
