#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "command_parser.h"

static int g_handler_calls;
static int g_last_argc;
static char g_last_arg0[32];
static char g_last_arg1[32];
static int g_audio_handler_calls;
static int g_audio_argc;
static char g_audio_path[128];

static int probe_handler(int argc, const char *argv[])
{
    g_handler_calls++;
    g_last_argc = argc;
    g_last_arg0[0] = '\0';
    g_last_arg1[0] = '\0';

    if (argc > 0)
    {
        strncpy(g_last_arg0, argv[0], sizeof(g_last_arg0) - 1U);
        g_last_arg0[sizeof(g_last_arg0) - 1U] = '\0';
    }
    if (argc > 1)
    {
        strncpy(g_last_arg1, argv[1], sizeof(g_last_arg1) - 1U);
        g_last_arg1[sizeof(g_last_arg1) - 1U] = '\0';
    }

    return 0;
}

static int audio_handler(int argc, const char *argv[])
{
    g_audio_handler_calls++;
    g_audio_argc = argc;
    g_audio_path[0] = '\0';
    if ((argc > 1) && (argv[1] != NULL))
    {
        strncpy(g_audio_path, argv[1], sizeof(g_audio_path) - 1U);
        g_audio_path[sizeof(g_audio_path) - 1U] = '\0';
    }
    return COMMAND_OK;
}

static void feed_text(const char *text)
{
    while (*text != '\0')
    {
        command_parser_uart_rx_byte_from_isr((uint8_t)*text);
        text++;
    }
}

int main(void)
{
    static const command_entry_t probe_command = {
        "probe",
        "probe <arg0> <arg1> - 测试参数派发",
        probe_handler
    };
    static const command_entry_t probe_alias = {
        "p",
        NULL,
        probe_handler
    };
    static const command_entry_t audio_command = {
        "audio",
        NULL,
        audio_handler
    };
    uint32_t index;
    int failures = 0;

    command_parser_init();

    if (command_register(&probe_command) != COMMAND_OK)
    {
        printf("FAIL register_probe\n");
        failures++;
    }
    if (command_register(&probe_command) != COMMAND_DUPLICATE)
    {
        printf("FAIL duplicate_not_rejected\n");
        failures++;
    }
    if (command_register(&probe_alias) != COMMAND_OK)
    {
        printf("FAIL register_probe_alias\n");
        failures++;
    }
    if (command_register(&audio_command) != COMMAND_OK)
    {
        printf("FAIL register_audio\n");
        failures++;
    }

    feed_text("PrObE alpha beta\r");
    if (g_handler_calls != 0)
    {
        printf("FAIL handler_ran_in_isr_path\n");
        failures++;
    }

    command_parser_poll();
    if ((g_handler_calls != 1) ||
        (g_last_argc != 2) ||
        (strcmp(g_last_arg0, "alpha") != 0) ||
        (strcmp(g_last_arg1, "beta") != 0))
    {
        printf("FAIL dispatch calls=%d argc=%d arg0=%s arg1=%s\n",
               g_handler_calls,
               g_last_argc,
               g_last_arg0,
               g_last_arg1);
        failures++;
    }

    feed_text("audio play 0:/MUSIC/My File.mp3\r");
    command_parser_poll();
    if ((g_audio_handler_calls != 1) ||
        (g_audio_argc != 2) ||
        (strcmp(g_audio_path, "0:/MUSIC/My File.mp3") != 0))
    {
        printf("FAIL audio_path argc=%d path=%s calls=%d\n",
               g_audio_argc, g_audio_path, g_audio_handler_calls);
        failures++;
    }

    for (index = 0U; index < (COMMAND_LINE_SIZE + 20U); index++)
    {
        command_parser_uart_rx_byte_from_isr((uint8_t)'x');
        command_parser_poll();
    }
    command_parser_uart_rx_byte_from_isr((uint8_t)'\n');
    command_parser_poll();

    if (command_parser_get_line_overflow_count() != 1U)
    {
        printf("FAIL line_overflow_count actual=%lu\n",
               (unsigned long)command_parser_get_line_overflow_count());
        failures++;
    }

    feed_text("probe after overflow\n");
    command_parser_poll();
    if ((g_handler_calls != 2) ||
        (g_last_argc != 2) ||
        (strcmp(g_last_arg0, "after") != 0) ||
        (strcmp(g_last_arg1, "overflow") != 0))
    {
        printf("FAIL recovery_after_overflow\n");
        failures++;
    }

    if (command_parser_execute_line("probe ui path") != COMMAND_OK)
    {
        printf("FAIL execute_line_result\n");
        failures++;
    }
    if ((g_handler_calls != 3) ||
        (g_last_argc != 2) ||
        (strcmp(g_last_arg0, "ui") != 0) ||
        (strcmp(g_last_arg1, "path") != 0))
    {
        printf("FAIL execute_line_dispatch\n");
        failures++;
    }

    feed_text("probe idle submit");
    command_parser_poll_at(100U);
    if (g_handler_calls != 3)
    {
        printf("FAIL no_newline_ran_immediately\n");
        failures++;
    }
    command_parser_poll_at(149U);
    if (g_handler_calls != 3)
    {
        printf("FAIL no_newline_ran_before_timeout\n");
        failures++;
    }
    command_parser_poll_at(150U);
    if ((g_handler_calls != 4) ||
        (g_last_argc != 2) ||
        (strcmp(g_last_arg0, "idle") != 0) ||
        (strcmp(g_last_arg1, "submit") != 0))
    {
        printf("FAIL no_newline_idle_dispatch\n");
        failures++;
    }
    command_parser_poll_at(250U);
    if (g_handler_calls != 4)
    {
        printf("FAIL no_newline_repeated_dispatch\n");
        failures++;
    }

    printf("HELP_BEGIN\n");
    command_print_help();
    printf("HELP_END\n");

    if (failures != 0)
    {
        printf("FAIL command_parser failures=%d\n", failures);
        return 1;
    }

    printf("PASS command_parser\n");
    return 0;
}
