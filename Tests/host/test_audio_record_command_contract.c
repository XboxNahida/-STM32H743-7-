#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_source(const char *path)
{
    FILE *file = fopen(path, "rb");
    long size;
    char *text;

    if (file == NULL)
    {
        return NULL;
    }
    if ((fseek(file, 0L, SEEK_END) != 0) ||
        ((size = ftell(file)) < 0L) ||
        (fseek(file, 0L, SEEK_SET) != 0))
    {
        fclose(file);
        return NULL;
    }
    text = (char *)malloc((size_t)size + 1U);
    if (text == NULL)
    {
        fclose(file);
        return NULL;
    }
    if (fread(text, 1U, (size_t)size, file) != (size_t)size)
    {
        free(text);
        fclose(file);
        return NULL;
    }
    text[size] = '\0';
    fclose(file);
    return text;
}

static int expect_true(const char *name, int condition)
{
    if (condition != 0)
    {
        return 0;
    }
    printf("FAIL %s\n", name);
    return 1;
}

int main(void)
{
    char *command = read_source("Core/Peripheral/Src/command_parser.c");
    char *main_source = read_source("Core/Src/main.c");
    int failures = 0;

    failures += expect_true("read_command", command != NULL);
    failures += expect_true("read_main", main_source != NULL);
    if (failures == 0)
    {
        failures += expect_true(
            "record_header",
            strstr(command, "#include \"audio_recording_service.h\"") != NULL);
        failures += expect_true(
            "raw_record_path",
            strstr(command, "command_name_equal(audio_action, \"record\")") != NULL);
        failures += expect_true(
            "record_start",
            strstr(command, "audio_record_start(path)") != NULL);
        failures += expect_true(
            "record_status",
            strstr(command, "audio_record_print_status();") != NULL);
        failures += expect_true(
            "record_stop",
            strstr(command, "audio_record_stop()") != NULL);
        failures += expect_true(
            "record_usage",
            strstr(command, "record <sd_path>") != NULL);
        failures += expect_true(
            "main_header",
            strstr(main_source,
                   "#include \"audio_recording_service.h\"") != NULL);
        failures += expect_true(
            "main_init",
            strstr(main_source, "audio_recording_service_init();") != NULL);
        failures += expect_true(
            "main_poll",
            strstr(main_source, "audio_record_poll();") != NULL);
    }
    free(command);
    free(main_source);
    if (failures != 0)
    {
        printf("FAIL audio_record_command_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS audio_record_command_contract\n");
    return 0;
}
