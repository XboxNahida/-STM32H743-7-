#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define SOURCE_BUFFER_SIZE 40000U

static char g_command_source[SOURCE_BUFFER_SIZE];
static char g_readonly_source[SOURCE_BUFFER_SIZE];
static char g_scan_source[SOURCE_BUFFER_SIZE];
static char g_main_source[SOURCE_BUFFER_SIZE];

static int read_source(const char *path, char *buffer, size_t capacity)
{
    FILE *file;
    size_t length;

    file = fopen(path, "rb");
    if (file == NULL)
    {
        return 0;
    }
    length = fread(buffer, 1U, capacity - 1U, file);
    if ((ferror(file) != 0) || (feof(file) == 0))
    {
        (void)fclose(file);
        return 0;
    }
    buffer[length] = '\0';
    (void)fclose(file);
    return 1;
}

static uint32_t count_text(const char *text, const char *needle)
{
    uint32_t count = 0U;
    size_t needle_length = strlen(needle);

    while ((text = strstr(text, needle)) != NULL)
    {
        count++;
        text += needle_length;
    }
    return count;
}

static int has_forbidden_text(const char *text)
{
    static const char *forbidden[] =
    {
        "nand write",
        "nand erase",
        "nand format",
        "NAND_WRITE0",
        "NAND_ERASE0"
    };
    uint32_t index;

    for (index = 0U; index < (sizeof(forbidden) / sizeof(forbidden[0])); index++)
    {
        if (strstr(text, forbidden[index]) != NULL)
        {
            return 1;
        }
    }
    return 0;
}

static int define_value_equals(const char *text,
                               const char *name,
                               unsigned long expected)
{
    const char *definition = strstr(text, name);
    unsigned long value;

    if (definition == NULL)
    {
        return 0;
    }
    definition += strlen(name);
    return (sscanf(definition, "%lu", &value) == 1) &&
           (value == expected);
}

int main(void)
{
    const char *handler;
    const char *handler_end;
    char handler_source[10000];
    size_t handler_length;
    int failures = 0;

    if (!read_source("Core/Peripheral/Src/command_parser.c",
                     g_command_source, sizeof(g_command_source)) ||
        !read_source("Core/Peripheral/Src/nand_readonly.c",
                     g_readonly_source, sizeof(g_readonly_source)) ||
        !read_source("Core/Application/Src/nand_scan_service.c",
                     g_scan_source, sizeof(g_scan_source)) ||
        !read_source("Core/Src/main.c",
                     g_main_source, sizeof(g_main_source)))
    {
        printf("FAIL source_read\n");
        return 1;
    }

    handler = strstr(g_command_source, "static int command_nand_handler");
    handler_end = (handler != NULL) ?
                  strstr(handler + 1, "\nstatic int ") : NULL;
    if ((handler == NULL) || (handler_end == NULL))
    {
        printf("FAIL nand_handler_missing\n");
        failures++;
        handler_source[0] = '\0';
    }
    else
    {
        handler_length = (size_t)(handler_end - handler);
        if (handler_length >= sizeof(handler_source))
        {
            printf("FAIL nand_handler_too_large\n");
            failures++;
            handler_source[0] = '\0';
        }
        else
        {
            memcpy(handler_source, handler, handler_length);
            handler_source[handler_length] = '\0';
        }
    }

    if ((count_text(g_command_source, "{\"nand\",") != 1U) ||
        (strstr(g_command_source,
                "{\"nand\", \"nand status|id|scan|stop|read <page> <offset> <length>") == NULL))
    {
        printf("FAIL nand_registration\n");
        failures++;
    }

    {
        const char *readonly_init = strstr(g_main_source,
                                           "nand_readonly_init();");
        const char *scan_init = strstr(g_main_source, "nand_scan_init();");
        const char *parser_init = strstr(g_main_source,
                                         "command_parser_init();");

        if ((readonly_init == NULL) || (scan_init == NULL) ||
            (parser_init == NULL) || (readonly_init > scan_init) ||
            (readonly_init > parser_init))
        {
            printf("FAIL nand_initialization_order\n");
            failures++;
        }
    }

    if ((count_text(handler_source,
                    "command_name_equal(argv[0], \"") != 5U) ||
        (strstr(handler_source, "command_name_equal(argv[0], \"status\")") == NULL) ||
        (strstr(handler_source, "command_name_equal(argv[0], \"id\")") == NULL) ||
        (strstr(handler_source, "command_name_equal(argv[0], \"scan\")") == NULL) ||
        (strstr(handler_source, "command_name_equal(argv[0], \"stop\")") == NULL) ||
        (strstr(handler_source, "command_name_equal(argv[0], \"read\")") == NULL))
    {
        printf("FAIL nand_subcommands\n");
        failures++;
    }

    if ((strstr(handler_source, "command_parse_uint(argv[1]") == NULL) ||
        (strstr(handler_source, "command_parse_uint(argv[2]") == NULL) ||
        (strstr(handler_source, "command_parse_uint(argv[3]") == NULL) ||
        !define_value_equals(g_command_source,
                             "#define NAND_COMMAND_MAX_READ", 64UL) ||
        !define_value_equals(g_command_source,
                             "#define NAND_COMMAND_BYTES_PER_LINE", 16UL) ||
        (strstr(handler_source, "nand_readonly_read_page") == NULL))
    {
        printf("FAIL nand_read_contract\n");
        failures++;
    }

    if (has_forbidden_text(g_command_source) ||
        has_forbidden_text(g_readonly_source) ||
        has_forbidden_text(g_scan_source) ||
        (strstr(g_readonly_source, "0x80U") != NULL) ||
        (strstr(g_readonly_source, "0x10U") != NULL) ||
        (strstr(g_readonly_source, "0x60U") != NULL) ||
        (strstr(g_readonly_source, "0xD0U") != NULL) ||
        (strstr(g_readonly_source, "0xEFU") != NULL))
    {
        printf("FAIL forbidden_write_erase_contract\n");
        failures++;
    }

    if (failures != 0)
    {
        printf("FAIL nand_command_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS nand_command_contract\n");
    return 0;
}
