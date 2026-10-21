#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_text(const char *path)
{
    FILE *file = fopen(path, "rb");
    long length;
    char *text;

    if (file == NULL) return NULL;
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

static unsigned int count_text(const char *text, const char *needle)
{
    unsigned int count = 0U;
    size_t length = strlen(needle);
    const char *cursor = text;

    while ((cursor = strstr(cursor, needle)) != NULL)
    {
        count++;
        cursor += length;
    }
    return count;
}

static int expect_true(const char *name, int condition)
{
    if (condition != 0) return 0;
    printf("FAIL %s\n", name);
    return 1;
}

int main(void)
{
    char *command = read_text("Core/Peripheral/Src/command_parser.c");
    char *project = read_text("MDK-ARM/H743_LCD_Keil5.uvprojx");
    char *service = read_text("Core/Application/Src/system_settings_service.c");
    int failures = 0;

    failures += expect_true("read_command", command != NULL);
    failures += expect_true("read_project", project != NULL);
    failures += expect_true("read_service", service != NULL);
    if (failures == 0)
    {
        failures += expect_true("settings_include",
            strstr(command, "#include \"system_settings_service.h\"") != NULL);
        failures += expect_true("settings_handler",
            strstr(command, "static int command_settings_handler") != NULL);
        failures += expect_true("settings_status",
            strstr(command, "system_settings_print_status();") != NULL);
        failures += expect_true("settings_save_request",
            strstr(command, "system_settings_request_save();") != NULL);
        failures += expect_true("settings_registration",
            strstr(command, "{\"settings\", \"settings status|save") != NULL);
        failures += expect_true("brightness_persistence",
            strstr(command, "system_settings_update_brightness") != NULL);
        failures += expect_true("volume_persistence",
            strstr(command, "system_settings_update_volume") != NULL);
        failures += expect_true("settings_print_implementation",
            strstr(service, "void system_settings_print_status(void)") != NULL);
        failures += expect_true("project_filename_once",
            count_text(project, "<FileName>system_settings_service.c</FileName>") == 1U);
        failures += expect_true("project_filepath_once",
            count_text(project,
                "<FilePath>..\\Core\\Application\\Src\\system_settings_service.c</FilePath>") == 1U);
    }

    free(command);
    free(project);
    free(service);
    if (failures != 0)
    {
        printf("FAIL system_settings_command_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS system_settings_command_contract\n");
    return 0;
}
