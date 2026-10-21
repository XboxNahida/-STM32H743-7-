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
    if (fseek(file, 0L, SEEK_END) != 0)
    {
        fclose(file);
        return NULL;
    }
    size = ftell(file);
    if (size < 0L || fseek(file, 0L, SEEK_SET) != 0)
    {
        fclose(file);
        return NULL;
    }
    text = (char *)malloc((size_t)size + 1U);
    if (text != NULL)
    {
        (void)fread(text, 1U, (size_t)size, file);
        text[size] = '\0';
    }
    fclose(file);
    return text;
}

static int require_text(const char *source, const char *text)
{
    if (strstr(source, text) == NULL)
    {
        printf("FAIL missing=%s\n", text);
        return 1;
    }
    return 0;
}

static int require_range(const char *begin, const char *end, const char *text)
{
    const char *found;
    if ((begin == NULL) || (end == NULL) || (begin >= end))
    {
        printf("FAIL invalid_range\n");
        return 1;
    }
    found = strstr(begin, text);
    if ((found == NULL) || (found >= end))
    {
        printf("FAIL range_missing=%s\n", text);
        return 1;
    }
    return 0;
}

static int forbid_range(const char *begin, const char *end, const char *text)
{
    const char *found;
    if ((begin == NULL) || (end == NULL) || (begin >= end))
    {
        printf("FAIL invalid_range\n");
        return 1;
    }
    found = strstr(begin, text);
    if ((found != NULL) && (found < end))
    {
        printf("FAIL range_forbidden=%s\n", text);
        return 1;
    }
    return 0;
}

int main(void)
{
    char *source = read_source("Core/Peripheral/Src/command_parser.c");
    const char *camera_begin;
    const char *sd_begin;
    const char *register_begin;
    int failures = 0;
    if (source == NULL)
    {
        printf("FAIL source_open\n");
        return 1;
    }
    failures += require_text(source, "command_camera_handler");
    failures += require_text(source, "{\"camera\"");
    failures += require_text(source, "camera status|probe|start|stop|mirror <0|1>|flip <0|1>|flash <0|1>|test_pattern <0|1>");
    failures += require_text(source, "camera_preview_start()");
    failures += require_text(source, "ov5640_set_mirror");
    failures += require_text(source, "ov5640_set_flip");
    failures += require_text(source, "ov5640_set_flash");
    failures += require_text(source, "ov5640_set_test_pattern");
    failures += require_text(source, "dcmi_error=0x%08lX");
    failures += require_text(source, "dma_error=0x%08lX");
    failures += require_text(source, "risr=0x%08lX");
    failures += require_text(source, "dma_lisr=0x%08lX");
    failures += require_text(source, "extra_tc=%lu");
    failures += require_text(source, "business_error=%lu");
    failures += require_text(source, "camera signal lines=%lu");
    failures += require_text(source, "frame_seen=%lu");
    failures += require_text(source, "dma_ndtr=%lu");
    failures += require_text(source, "sd status|reprobe");
    failures += require_text(source,
        "\\xE6\\x91\\x84\\xE5\\x83\\x8F\\xE5\\xA4\\xB4\\xE9\\xA2\\x84\\xE8\\xA7\\x88\\xE5\\x92\\x8C\\xE8\\xAE\\xBE\\xE7\\xBD\\xAE");
    failures += require_text(source,
        "\\x53\\x44\\xE5\\x8D\\xA1\\xE7\\x8A\\xB6\\xE6\\x80\\x81\\xE5\\x92\\x8C\\xE7\\x83\\xAD\\xE6\\x8F\\x92\\xE6\\x8B\\x94");
    camera_begin = strstr(source, "static int command_camera_handler");
    sd_begin = strstr(source, "static int command_sd_handler");
    register_begin = (sd_begin != NULL) ?
        strstr(sd_begin, "static void command_register_target_commands") : NULL;
    failures += require_range(camera_begin, sd_begin, "(argc == 1)");
    failures += require_range(camera_begin, sd_begin, "argv[0], \"status\"");
    failures += require_range(camera_begin, sd_begin,
                              "command_parse_uint(argv[1]");
    failures += forbid_range(camera_begin, sd_begin, "argv[2]");
    failures += require_range(sd_begin, register_begin, "(argc == 1)");
    failures += require_range(sd_begin, register_begin, "argv[0], \"status\"");
    if (strstr(source, "usage=camera") == NULL ||
        strstr(source, "usage=sd status|reprobe") == NULL)
    {
        printf("FAIL usage_missing\n");
        failures++;
    }
    free(source);
    if (failures == 0)
    {
        printf("PASS camera_command_contract\n");
        return 0;
    }
    printf("FAIL camera_command_contract failures=%d\n", failures);
    return 1;
}
