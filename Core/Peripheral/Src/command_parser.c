#include "command_parser.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COMMAND_IDLE_TIMEOUT_MS 50U

#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
#include "app_event.h"
#include "app_runtime.h"
#include "adc_measurement_service.h"
#include "ap3216c_service.h"
#include "audio_playback_service.h"
#include "audio_recording_service.h"
#include "at24c02_service.h"
#include "boot_probe_service.h"
#include "camera_preview_service.h"
#include "camera_resource_service.h"
#include "camera_dcmi_driver.h"
#include "ov5640_service.h"
#include "diagnostics.h"
#include "es8388_service.h"
#include "font_cache_service.h"
#include "hander.h"
#include "image_viewer_app.h"
#include "imu_service.h"
#include "ir_remote_service.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "music_player_service.h"
#include "pressure_service.h"
#include "main.h"
#include "nand_readonly.h"
#include "nand_scan_service.h"
#include "norflash.h"
#include "resource_manager.h"
#include "rtc.h"
#include "shared_buffer.h"
#include "sdcard_manager.h"
#include "sh3001_service.h"
#include "st480mc_service.h"
#include "tpad_service.h"
#include "storage_service.h"
#include "system_settings_service.h"
#include "tim.h"
#include "usart.h"

uint8_t uart_rx_buf[UART_RX_BUF_SIZE];
char uart_display_buf[UART_DISPLAY_BUF_SIZE];
uint16_t uart_display_len;
uint16_t uart_last_display_len;

extern uint32_t g_screen_off_timeout_ms;
static uint8_t g_rx_display_line_start = 1U;
static uint8_t g_rx_display_saw_cr;
#endif

static const command_entry_t *g_entries[COMMAND_MAX_ENTRIES];
static uint8_t g_entry_count;

static uint8_t g_rx_queue[COMMAND_RX_QUEUE_SIZE];
static volatile uint16_t g_rx_head;
static volatile uint16_t g_rx_tail;
static volatile uint32_t g_rx_drop_count;

static char g_line[COMMAND_LINE_SIZE];
static uint16_t g_line_length;
static uint8_t g_discard_line;
static uint32_t g_line_overflow_count;
static uint32_t g_last_line_activity_ms;
static uint8_t g_line_activity_valid;
static command_clear_callback_t g_clear_callback;
static command_path_callback_t g_image_open_file_callback;
static command_path_callback_t g_image_open_album_callback;

#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
static void command_register_target_commands(void);
#endif

static char command_ascii_lower(char value)
{
    if ((value >= 'A') && (value <= 'Z'))
    {
        return (char)(value + ('a' - 'A'));
    }
    return value;
}

static int command_name_equal(const char *left, const char *right)
{
    while ((*left != '\0') && (*right != '\0'))
    {
        if (command_ascii_lower(*left) != command_ascii_lower(*right))
        {
            return 0;
        }
        left++;
        right++;
    }
    return ((*left == '\0') && (*right == '\0')) ? 1 : 0;
}

static void command_increment_saturated(uint32_t *value)
{
    if (*value != UINT32_MAX)
    {
        (*value)++;
    }
}

static int command_dispatch_line(char *line)
{
    const char *argv[COMMAND_MAX_ARGS];
    char *cursor = line;
    char *argument_start;
    char *command_name;
    int argc = 0;
    uint8_t index;

    while ((*cursor == ' ') || (*cursor == '\t'))
    {
        cursor++;
    }
    if (*cursor == '\0')
    {
        return COMMAND_INVALID_ARG;
    }

    command_name = cursor;
    while ((*cursor != '\0') && (*cursor != ' ') && (*cursor != '\t'))
    {
        cursor++;
    }
    if (*cursor != '\0')
    {
        *cursor++ = '\0';
    }

    argument_start = cursor;
    while ((*argument_start == ' ') || (*argument_start == '\t'))
    {
        argument_start++;
    }
    if ((command_name_equal(command_name, "audio") != 0) &&
        (*argument_start != '\0'))
    {
        char *audio_action = argument_start;
        char *audio_path;
        char *path_end;

        while ((*argument_start != '\0') &&
               (*argument_start != ' ') && (*argument_start != '\t'))
        {
            argument_start++;
        }
        if (*argument_start != '\0')
        {
            *argument_start++ = '\0';
        }
        while ((*argument_start == ' ') || (*argument_start == '\t'))
        {
            argument_start++;
        }
        audio_path = argument_start;
        path_end = audio_path + strlen(audio_path);
        while ((path_end > audio_path) &&
               ((path_end[-1] == ' ') || (path_end[-1] == '\t')))
        {
            path_end--;
            *path_end = '\0';
        }
        if (((command_name_equal(audio_action, "play") != 0) ||
             (command_name_equal(audio_action, "record") != 0)) &&
            (*audio_path != '\0'))
        {
            argv[0] = audio_action;
            argv[1] = audio_path;
            argc = 2;
            cursor = audio_path + strlen(audio_path);
        }
    }

    while ((*cursor != '\0') && (argc < (int)COMMAND_MAX_ARGS))
    {
        while ((*cursor == ' ') || (*cursor == '\t'))
        {
            cursor++;
        }
        if (*cursor == '\0')
        {
            break;
        }
        argv[argc++] = cursor;
        while ((*cursor != '\0') && (*cursor != ' ') && (*cursor != '\t'))
        {
            cursor++;
        }
        if (*cursor != '\0')
        {
            *cursor++ = '\0';
        }
    }

    for (index = 0U; index < g_entry_count; index++)
    {
        if (command_name_equal(command_name, g_entries[index]->name) != 0)
        {
            return g_entries[index]->handler(argc, argv);
        }
    }

    printf("ERROR command_not_found name=%s\r\n", command_name);
    return COMMAND_NOT_FOUND;
}

static void command_consume_byte(uint8_t byte)
{
    if ((byte == (uint8_t)'\r') || (byte == (uint8_t)'\n'))
    {
        if (g_discard_line != 0U)
        {
            g_discard_line = 0U;
            g_line_length = 0U;
            return;
        }
        if (g_line_length != 0U)
        {
            g_line[g_line_length] = '\0';
            (void)command_dispatch_line(g_line);
            g_line_length = 0U;
        }
        return;
    }

    if (g_discard_line != 0U)
    {
        return;
    }
    if (g_line_length >= (COMMAND_LINE_SIZE - 1U))
    {
        g_discard_line = 1U;
        g_line_length = 0U;
        command_increment_saturated(&g_line_overflow_count);
        return;
    }

    g_line[g_line_length++] = (char)byte;
}

#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
static void command_display_rx_byte(uint8_t byte)
{
    static const char prefix[] = "[RX] ";
    uint32_t prefix_index;

    if ((byte == (uint8_t)'\n') && (g_rx_display_saw_cr != 0U))
    {
        g_rx_display_saw_cr = 0U;
        return;
    }
    if (uart_display_len >= (UART_DISPLAY_BUF_SIZE - 8U))
    {
        uart_display_len = 0U;
        uart_last_display_len = 0U;
        uart_display_buf[0] = '\0';
        g_rx_display_line_start = 1U;
    }
    if (g_rx_display_line_start != 0U)
    {
        for (prefix_index = 0U;
             prefix_index < (sizeof(prefix) - 1U);
             prefix_index++)
        {
            uart_display_buf[uart_display_len++] = prefix[prefix_index];
        }
        g_rx_display_line_start = 0U;
    }

    if (byte == (uint8_t)'\r')
    {
        uart_display_buf[uart_display_len++] = '\n';
        g_rx_display_line_start = 1U;
        g_rx_display_saw_cr = 1U;
    }
    else
    {
        uart_display_buf[uart_display_len++] = (char)byte;
        if (byte == (uint8_t)'\n')
        {
            g_rx_display_line_start = 1U;
        }
        g_rx_display_saw_cr = 0U;
    }
    uart_display_buf[uart_display_len] = '\0';
}
#endif

void command_parser_init(void)
{
    g_entry_count = 0U;
    g_rx_head = 0U;
    g_rx_tail = 0U;
    g_rx_drop_count = 0U;
    g_line_length = 0U;
    g_discard_line = 0U;
    g_line_overflow_count = 0U;
    g_last_line_activity_ms = 0U;
    g_line_activity_valid = 0U;
    g_clear_callback = NULL;
    g_image_open_file_callback = NULL;
    g_image_open_album_callback = NULL;
#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
    g_rx_display_line_start = 1U;
    g_rx_display_saw_cr = 0U;
    command_register_target_commands();
#endif
}

void command_parser_uart_rx_byte_from_isr(uint8_t byte)
{
    uint16_t next = (uint16_t)((g_rx_head + 1U) % COMMAND_RX_QUEUE_SIZE);

    if (next == g_rx_tail)
    {
        if (g_rx_drop_count != UINT32_MAX)
        {
            g_rx_drop_count++;
        }
        return;
    }

    g_rx_queue[g_rx_head] = byte;
    g_rx_head = next;
}

static void command_parser_poll_internal(uint32_t now_ms, uint8_t check_idle)
{
    while (g_rx_tail != g_rx_head)
    {
        uint8_t byte = g_rx_queue[g_rx_tail];
        g_rx_tail = (uint16_t)((g_rx_tail + 1U) % COMMAND_RX_QUEUE_SIZE);
#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
        command_display_rx_byte(byte);
#endif
        command_consume_byte(byte);
        if ((byte == (uint8_t)'\r') || (byte == (uint8_t)'\n'))
        {
            g_line_activity_valid = 0U;
        }
        else
        {
            g_last_line_activity_ms = now_ms;
            g_line_activity_valid = 1U;
        }
    }

    if ((check_idle != 0U) &&
        (g_line_activity_valid != 0U) &&
        ((uint32_t)(now_ms - g_last_line_activity_ms) >=
         COMMAND_IDLE_TIMEOUT_MS))
    {
#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
        command_display_rx_byte((uint8_t)'\r');
#endif
        if (g_discard_line != 0U)
        {
            g_discard_line = 0U;
            g_line_length = 0U;
        }
        else if (g_line_length != 0U)
        {
            g_line[g_line_length] = '\0';
            (void)command_dispatch_line(g_line);
            g_line_length = 0U;
        }
        g_line_activity_valid = 0U;
    }
}

void command_parser_poll(void)
{
    command_parser_poll_internal(0U, 0U);
}

void command_parser_poll_at(uint32_t now_ms)
{
    command_parser_poll_internal(now_ms, 1U);
}

int command_parser_execute_line(const char *line)
{
    char line_copy[COMMAND_LINE_SIZE];
    size_t length;

    if (line == NULL)
    {
        return COMMAND_INVALID_ARG;
    }
    length = strlen(line);
    if ((length == 0U) || (length >= COMMAND_LINE_SIZE))
    {
        return COMMAND_INVALID_ARG;
    }

    memcpy(line_copy, line, length + 1U);
    return command_dispatch_line(line_copy);
}

int command_register(const command_entry_t *entry)
{
    uint8_t index;

    if ((entry == NULL) || (entry->name == NULL) ||
        (entry->name[0] == '\0') || (entry->handler == NULL))
    {
        return COMMAND_INVALID_ARG;
    }

    for (index = 0U; index < g_entry_count; index++)
    {
        if (command_name_equal(entry->name, g_entries[index]->name) != 0)
        {
            return COMMAND_DUPLICATE;
        }
    }
    if (g_entry_count >= COMMAND_MAX_ENTRIES)
    {
        return COMMAND_FULL;
    }

    g_entries[g_entry_count++] = entry;
    return COMMAND_OK;
}

void command_parser_set_clear_callback(command_clear_callback_t callback)
{
    g_clear_callback = callback;
}

void command_parser_set_image_callbacks(command_path_callback_t open_file,
                                        command_path_callback_t open_album)
{
    g_image_open_file_callback = open_file;
    g_image_open_album_callback = open_album;
}

void command_print_help(void)
{
    uint8_t index;

    for (index = 0U; index < g_entry_count; index++)
    {
        if (g_entries[index]->help != NULL)
        {
            printf("%s\r\n", g_entries[index]->help);
        }
    }
}

uint32_t command_parser_get_line_overflow_count(void)
{
    return g_line_overflow_count;
}

uint32_t command_parser_get_rx_drop_count(void)
{
    return g_rx_drop_count;
}

#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
#define NAND_COMMAND_TIMEOUT_MS       100U
#define NAND_COMMAND_MAX_READ          64U
#define NAND_COMMAND_BYTES_PER_LINE    16U
#define EEPROM_COMMAND_TIMEOUT_MS     100U
#define EEPROM_COMMAND_MAX_READ       64U
#define EEPROM_COMMAND_BYTES_PER_LINE  16U

static int command_require_argc(int argc, int expected, const char *usage)
{
    if (argc != expected)
    {
        printf("INVALID_ARG usage=%s\r\n", usage);
        return 0;
    }
    return 1;
}

static int command_parse_uint(const char *text,
                              uint32_t minimum,
                              uint32_t maximum,
                              uint32_t *value)
{
    char *end;
    unsigned long parsed;

    if ((text == NULL) || (value == NULL) || (text[0] == '\0'))
    {
        return 0;
    }
    parsed = strtoul(text, &end, 10);
    if ((*end != '\0') || (parsed < minimum) || (parsed > maximum))
    {
        return 0;
    }
    *value = (uint32_t)parsed;
    return 1;
}

static int command_parse_time(const char *text,
                              uint32_t *hour,
                              uint32_t *minute,
                              uint32_t *second,
                              int with_seconds)
{
    unsigned int hh;
    unsigned int mm;
    unsigned int ss = 0U;
    char tail;
    int fields;

    if ((text == NULL) || (hour == NULL) ||
        (minute == NULL) || (second == NULL))
    {
        return 0;
    }

    if (with_seconds != 0)
    {
        fields = sscanf(text, "%u:%u:%u%c", &hh, &mm, &ss, &tail);
        if (fields != 3)
        {
            return 0;
        }
    }
    else
    {
        fields = sscanf(text, "%u:%u%c", &hh, &mm, &tail);
        if (fields != 2)
        {
            return 0;
        }
    }

    if ((hh > 23U) || (mm > 59U) || (ss > 59U))
    {
        return 0;
    }
    *hour = hh;
    *minute = mm;
    *second = ss;
    return 1;
}

static int command_help_handler(int argc, const char *argv[])
{
    (void)argc;
    (void)argv;
    command_print_help();
    return COMMAND_OK;
}

static int command_clear_handler(int argc, const char *argv[])
{
    (void)argv;
    if (command_require_argc(argc, 0, "clear") == 0)
    {
        return COMMAND_INVALID_ARG;
    }
    if (g_clear_callback == NULL)
    {
        printf("NOT_READY component=uart_ui\r\n");
        return COMMAND_INVALID_ARG;
    }
    g_clear_callback();
    printf("OK command=clear\r\n");
    return COMMAND_OK;
}

static int command_brightness_handler(int argc, const char *argv[])
{
    uint32_t brightness;

    if ((command_require_argc(argc, 1, "bright <0-100>") == 0) ||
        (command_parse_uint(argv[0], 0U, 100U, &brightness) == 0))
    {
        printf("INVALID_ARG field=brightness\r\n");
        return COMMAND_INVALID_ARG;
    }
    if (brightness < 5U)
    {
        brightness = 5U;
    }
    __HAL_TIM_SET_COMPARE(&htim3,
                          TIM_CHANNEL_2,
                          (uint16_t)((brightness * 999U) / 100U));
    system_settings_update_brightness((uint8_t)brightness, HAL_GetTick());
    printf("OK brightness=%lu\r\n", (unsigned long)brightness);
    return COMMAND_OK;
}

static int command_screenoff_handler(int argc, const char *argv[])
{
    uint32_t seconds;

    if ((command_require_argc(argc, 1, "screenoff <5-3600>") == 0) ||
        (command_parse_uint(argv[0], 5U, 3600U, &seconds) == 0))
    {
        printf("INVALID_ARG field=seconds\r\n");
        return COMMAND_INVALID_ARG;
    }
    g_screen_off_timeout_ms = seconds * 1000U;
    printf("OK screenoff_seconds=%lu\r\n", (unsigned long)seconds);
    return COMMAND_OK;
}

static int command_time_handler(int argc, const char *argv[])
{
    uint32_t hour;
    uint32_t minute;
    uint32_t second;

    if ((command_require_argc(argc, 1, "time <HH:MM:SS>") == 0) ||
        (command_parse_time(argv[0], &hour, &minute, &second, 1) == 0))
    {
        printf("INVALID_ARG field=time\r\n");
        return COMMAND_INVALID_ARG;
    }
    if (rtc_set_time((uint8_t)hour, (uint8_t)minute, (uint8_t)second) != 0U)
    {
        printf("ERROR command=time\r\n");
        return -1;
    }
    printf("OK time=%02lu:%02lu:%02lu\r\n",
           (unsigned long)hour,
           (unsigned long)minute,
           (unsigned long)second);
    return COMMAND_OK;
}

static int command_date_handler(int argc, const char *argv[])
{
    unsigned int year;
    unsigned int month;
    unsigned int day;
    char tail;

    if ((command_require_argc(argc, 1, "date <YY-MM-DD>") == 0) ||
        (sscanf(argv[0], "%u-%u-%u%c", &year, &month, &day, &tail) != 3) ||
        (year > 99U) || (month < 1U) || (month > 12U) ||
        (day < 1U) || (day > 31U))
    {
        printf("INVALID_ARG field=date\r\n");
        return COMMAND_INVALID_ARG;
    }
    if (rtc_set_date((uint8_t)year, (uint8_t)month, (uint8_t)day) != 0U)
    {
        printf("ERROR command=date\r\n");
        return -1;
    }
    printf("OK date=20%02u-%02u-%02u\r\n", year, month, day);
    return COMMAND_OK;
}

static int command_alarm_handler(int argc, const char *argv[])
{
    uint32_t hour;
    uint32_t minute;
    uint32_t second;

    if ((command_require_argc(argc, 1, "alarm <HH:MM>") == 0) ||
        (command_parse_time(argv[0], &hour, &minute, &second, 0) == 0))
    {
        printf("INVALID_ARG field=alarm\r\n");
        return COMMAND_INVALID_ARG;
    }
    if (alarm_add((uint8_t)hour, (uint8_t)minute) != 0U)
    {
        printf("ERROR command=alarm reason=full\r\n");
        return -1;
    }
    printf("OK alarm=%02lu:%02lu\r\n",
           (unsigned long)hour,
           (unsigned long)minute);
    return COMMAND_OK;
}

static int command_alarmoff_handler(int argc, const char *argv[])
{
    (void)argv;
    if (command_require_argc(argc, 0, "alarmoff") == 0)
    {
        return COMMAND_INVALID_ARG;
    }
    alarm_remove_nearest();
    printf("OK command=alarmoff\r\n");
    return COMMAND_OK;
}

static int command_sys_handler(int argc, const char *argv[])
{
    const fault_snapshot_t *fault;

    if ((command_require_argc(argc, 1, "sys status") == 0) ||
        (command_name_equal(argv[0], "status") == 0))
    {
        printf("INVALID_ARG usage=sys status\r\n");
        return COMMAND_INVALID_ARG;
    }
    fault = diagnostics_get_first_fault();
    printf("sys log_level=%u fault_valid=%u command_line_overflow=%lu command_rx_drop=%lu event_drop=%lu\r\n",
           (unsigned int)diagnostics_get_log_level(),
           (unsigned int)fault->valid,
           (unsigned long)g_line_overflow_count,
           (unsigned long)g_rx_drop_count,
           (unsigned long)app_event_get_drop_count());
    return COMMAND_OK;
}

static int command_lvgl_handler(int argc, const char *argv[])
{
    lv_mem_monitor_t monitor;
    disp_diagnostics_t display;

    if ((command_require_argc(argc, 1, "lvgl status") == 0) ||
        (command_name_equal(argv[0], "status") == 0))
    {
        printf("INVALID_ARG usage=lvgl status\r\n");
        return COMMAND_INVALID_ARG;
    }
    lv_mem_monitor(&monitor);
    disp_get_diagnostics(&display);
    printf("lvgl memory total=%lu free=%lu biggest=%lu max_used=%lu used_pct=%u frag_pct=%u\r\n",
           (unsigned long)monitor.total_size,
           (unsigned long)monitor.free_size,
           (unsigned long)monitor.free_biggest_size,
           (unsigned long)monitor.max_used,
           (unsigned int)monitor.used_pct,
           (unsigned int)monitor.frag_pct);
    printf("lvgl display requests=%lu callbacks=%lu timeouts=%lu failures=%lu pending=%u enabled=%u active=0x%08lX target=0x%08lX cfbar=0x%08lX srcr=0x%08lX isr=0x%08lX\r\n",
           (unsigned long)display.reload_request_count,
           (unsigned long)display.reload_callback_count,
           (unsigned long)display.reload_timeout_count,
           (unsigned long)display.reload_failure_count,
           (unsigned int)display.reload_pending,
           (unsigned int)display.flush_enabled,
           (unsigned long)display.active_frame_addr,
           (unsigned long)display.pending_frame_addr,
           (unsigned long)display.programmed_frame_addr,
           (unsigned long)display.ltdc_srcr,
           (unsigned long)display.ltdc_isr);
    printf("lvgl ltdc fifo_underruns=%lu transfer_errors=%lu last_error=0x%08lX cpsr=0x%08lX line=%lu frame=0x%08lX\r\n",
           (unsigned long)display.fifo_underrun_count,
           (unsigned long)display.transfer_error_count,
           (unsigned long)display.last_error_code,
           (unsigned long)display.last_error_cpsr,
           (unsigned long)(display.last_error_cpsr & LTDC_CPSR_CYPOS),
           (unsigned long)display.last_error_frame_addr);
    return COMMAND_OK;
}

static int command_resource_handler(int argc, const char *argv[])
{
    if ((command_require_argc(argc, 1, "resource status") == 0) ||
        (command_name_equal(argv[0], "status") == 0))
    {
        printf("INVALID_ARG usage=resource status\r\n");
        return COMMAND_INVALID_ARG;
    }
    resource_print_status();
    return COMMAND_OK;
}

static int command_buffer_handler(int argc, const char *argv[])
{
    if ((command_require_argc(argc, 1, "buffer status") == 0) ||
        (command_name_equal(argv[0], "status") == 0))
    {
        printf("INVALID_ARG usage=buffer status\r\n");
        return COMMAND_INVALID_ARG;
    }
    shared_buffer_print_status();
    return COMMAND_OK;
}

static int command_infra_handler(int argc, const char *argv[])
{
    if ((command_require_argc(argc, 1, "infra selftest") == 0) ||
        (command_name_equal(argv[0], "selftest") == 0))
    {
        printf("INVALID_ARG usage=infra selftest\r\n");
        return COMMAND_INVALID_ARG;
    }
    if (shared_buffer_run_selftest() != 0)
    {
        printf("ERROR test=infra stopped=shared_buffer\r\n");
        return -1;
    }
    if (resource_run_selftest() != 0)
    {
        printf("ERROR test=infra stopped=resource_manager\r\n");
        return -1;
    }
    if (app_event_run_selftest() != 0)
    {
        printf("ERROR test=infra stopped=app_event\r\n");
        return -1;
    }
    printf("OK test=infra failures=0\r\n");
    return COMMAND_OK;
}

static int command_log_handler(int argc, const char *argv[])
{
    log_level_t level;

    if ((command_require_argc(argc, 2, "log level error|warn|info|trace") == 0) ||
        (command_name_equal(argv[0], "level") == 0))
    {
        printf("INVALID_ARG usage=log level error|warn|info|trace\r\n");
        return COMMAND_INVALID_ARG;
    }
    if (command_name_equal(argv[1], "error") != 0)
    {
        level = LOG_ERROR;
    }
    else if (command_name_equal(argv[1], "warn") != 0)
    {
        level = LOG_WARN;
    }
    else if (command_name_equal(argv[1], "info") != 0)
    {
        level = LOG_INFO;
    }
    else if (command_name_equal(argv[1], "trace") != 0)
    {
        level = LOG_TRACE;
    }
    else
    {
        printf("INVALID_ARG field=log_level\r\n");
        return COMMAND_INVALID_ARG;
    }
    diagnostics_set_log_level(level);
    printf("OK log_level=%u\r\n", (unsigned int)level);
    return COMMAND_OK;
}

static int command_fault_handler(int argc, const char *argv[])
{
    if (command_require_argc(argc, 1, "fault show|clear") == 0)
    {
        return COMMAND_INVALID_ARG;
    }
    if (command_name_equal(argv[0], "show") != 0)
    {
        diagnostics_print_fault();
        return COMMAND_OK;
    }
    if (command_name_equal(argv[0], "clear") != 0)
    {
        diagnostics_clear_first_fault();
        printf("OK fault=cleared\r\n");
        return COMMAND_OK;
    }
    printf("INVALID_ARG usage=fault show|clear\r\n");
    return COMMAND_INVALID_ARG;
}

static int command_app_handler(int argc, const char *argv[])
{
    if ((command_require_argc(argc, 1, "app status") == 0) ||
        (command_name_equal(argv[0], "status") == 0))
    {
        printf("INVALID_ARG usage=app status\r\n");
        return COMMAND_INVALID_ARG;
    }
    app_runtime_print_status();
    return COMMAND_OK;
}

static int command_storage_handler(int argc, const char *argv[])
{
    int result;

    if ((argc == 1) && (command_name_equal(argv[0], "status") != 0))
    {
        storage_print_status();
        return COMMAND_OK;
    }
    if ((argc == 2) &&
        (command_name_equal(argv[0], "test") != 0) &&
        (command_name_equal(argv[1], "sd") != 0))
    {
        result = storage_test_sd(1000U);
        printf("%s storage_test=sd result=%d timeout_ms=1000\r\n",
               (result == STORAGE_OK) ? "OK" : "ERROR",
               result);
        return (result == STORAGE_OK) ? COMMAND_OK : -1;
    }

    printf("INVALID_ARG usage=storage status|test sd\r\n");
    return COMMAND_INVALID_ARG;
}

static int command_probe_handler(int argc, const char *argv[])
{
    if ((argc == 1) && (command_name_equal(argv[0], "status") != 0))
    {
        boot_probe_print_status();
        return COMMAND_OK;
    }
    if ((argc == 1) && (command_name_equal(argv[0], "run") != 0))
    {
        boot_probe_start(HAL_GetTick());
        printf("OK probe action=run result=0\r\n");
        return COMMAND_OK;
    }

    printf("INVALID_ARG usage=probe status|run\r\n");
    return COMMAND_INVALID_ARG;
}

static int command_sensor_handler(int argc, const char *argv[])
{
    int result;

    if ((argc == 2) &&
        (command_name_equal(argv[0], "imu") != 0) &&
        (command_name_equal(argv[1], "status") != 0))
    {
        imu_print_status();
        return COMMAND_OK;
    }
    if ((argc == 2) &&
        (command_name_equal(argv[0], "imu") != 0) &&
        (command_name_equal(argv[1], "probe") != 0))
    {
        result = imu_probe_readonly(10U);
        imu_print_status();
        printf("%s sensor imu action=probe result=%d\r\n",
               (result == IMU_RESULT_OK) ? "OK" : "ERROR",
               result);
        return (result == IMU_RESULT_OK) ? COMMAND_OK : result;
    }
    if ((argc == 2) &&
        (command_name_equal(argv[0], "sh3001") != 0) &&
        (command_name_equal(argv[1], "status") != 0))
    {
        sh3001_print_status();
        return COMMAND_OK;
    }
    if ((argc == 2) &&
        (command_name_equal(argv[0], "sh3001") != 0) &&
        (command_name_equal(argv[1], "probe") != 0))
    {
        result = sh3001_probe_readonly(10U);
        sh3001_print_status();
        printf("%s sensor sh3001 action=probe result=%d\r\n",
               (result == SH3001_RESULT_OK) ? "OK" : "ERROR",
               result);
        return (result == SH3001_RESULT_OK) ? COMMAND_OK : result;
    }
    if ((argc == 2) &&
        (command_name_equal(argv[0], "st480mc") != 0) &&
        (command_name_equal(argv[1], "status") != 0))
    {
        st480mc_print_status();
        return COMMAND_OK;
    }
    if ((argc == 2) &&
        (command_name_equal(argv[0], "st480mc") != 0) &&
        (command_name_equal(argv[1], "probe") != 0))
    {
        result = st480mc_probe_readonly(10U);
        st480mc_print_status();
        printf("%s sensor st480mc action=probe result=%d\r\n",
               (result == ST480MC_RESULT_OK) ? "OK" : "ERROR",
               result);
        return (result == ST480MC_RESULT_OK) ? COMMAND_OK : result;
    }
    if ((argc == 2) &&
        (command_name_equal(argv[0], "pressure") != 0) &&
        (command_name_equal(argv[1], "status") != 0))
    {
        pressure_print_status();
        return COMMAND_OK;
    }
    if ((argc == 2) &&
        (command_name_equal(argv[0], "pressure") != 0) &&
        (command_name_equal(argv[1], "probe") != 0))
    {
        result = pressure_probe_readonly(10U);
        pressure_print_status();
        printf("%s sensor pressure action=probe result=%d\r\n",
               (result == PRESSURE_RESULT_OK) ? "OK" : "ERROR",
               result);
        return (result == PRESSURE_RESULT_OK) ? COMMAND_OK : result;
    }
    if ((argc == 2) &&
        (command_name_equal(argv[0], "pressure") != 0) &&
        (command_name_equal(argv[1], "read") != 0))
    {
        result = pressure_read_once(500U);
        pressure_print_status();
        printf("%s sensor pressure action=read result=%d\r\n",
               (result == PRESSURE_RESULT_OK) ? "OK" : "ERROR",
               result);
        return (result == PRESSURE_RESULT_OK) ? COMMAND_OK : result;
    }
    if ((argc == 2) &&
        (command_name_equal(argv[0], "ap3216c") != 0) &&
        (command_name_equal(argv[1], "status") != 0))
    {
        ap3216c_print_status();
        return COMMAND_OK;
    }
    if ((argc == 2) &&
        (command_name_equal(argv[0], "ap3216c") != 0) &&
        (command_name_equal(argv[1], "probe") != 0))
    {
        result = ap3216c_probe_readonly(10U);
        ap3216c_print_status();
        printf("%s sensor ap3216c action=probe result=%d\r\n",
               (result == AP3216C_RESULT_OK) ? "OK" : "ERROR",
               result);
        return (result == AP3216C_RESULT_OK) ? COMMAND_OK : result;
    }
    if ((argc == 2) &&
        (command_name_equal(argv[0], "ap3216c") != 0) &&
        (command_name_equal(argv[1], "read") != 0))
    {
        result = ap3216c_read_once(300U);
        ap3216c_print_status();
        printf("%s sensor ap3216c action=read result=%d\r\n",
               (result == AP3216C_RESULT_OK) ? "OK" : "ERROR",
               result);
        return (result == AP3216C_RESULT_OK) ? COMMAND_OK : result;
    }

    printf("INVALID_ARG usage=sensor imu|sh3001|st480mc status|probe | sensor pressure|ap3216c status|probe|read\r\n");
    return COMMAND_INVALID_ARG;
}

static int command_eeprom_handler(int argc, const char *argv[])
{
    uint8_t data[EEPROM_COMMAND_MAX_READ];
    uint32_t offset;
    uint32_t length;
    uint32_t index;
    int result;

    if ((argc == 1) && (command_name_equal(argv[0], "status") != 0))
    {
        at24c02_print_status();
        return COMMAND_OK;
    }
    if ((argc == 1) && (command_name_equal(argv[0], "probe") != 0))
    {
        result = at24c02_probe_readonly(10U);
        at24c02_print_status();
        printf("%s eeprom action=probe result=%d\r\n",
               (result == AT24C02_RESULT_OK) ? "OK" : "ERROR",
               result);
        return (result == AT24C02_RESULT_OK) ? COMMAND_OK : result;
    }
    if ((argc == 3) && (command_name_equal(argv[0], "read") != 0))
    {
        if ((command_parse_uint(argv[1], 0U, 255U, &offset) == 0) ||
            (command_parse_uint(argv[2], 1U, EEPROM_COMMAND_MAX_READ,
                                &length) == 0) ||
            ((offset + length) > 256U))
        {
            printf("INVALID_ARG usage=eeprom read <offset_0_255> <length_1_64>\r\n");
            return COMMAND_INVALID_ARG;
        }

        result = at24c02_read((uint16_t)offset, data, (uint16_t)length,
                              EEPROM_COMMAND_TIMEOUT_MS);
        if (result != AT24C02_RESULT_OK)
        {
            printf("ERROR eeprom action=read result=%d offset=%lu length=%lu\r\n",
                   result,
                   (unsigned long)offset,
                   (unsigned long)length);
            return result;
        }
        for (index = 0U; index < length; index++)
        {
            if ((index % EEPROM_COMMAND_BYTES_PER_LINE) == 0U)
            {
                printf("eeprom data offset=%lu:",
                       (unsigned long)(offset + index));
            }
            printf(" %02X", (unsigned int)data[index]);
            if (((index % EEPROM_COMMAND_BYTES_PER_LINE) ==
                 (EEPROM_COMMAND_BYTES_PER_LINE - 1U)) ||
                (index == (length - 1U)))
            {
                printf("\r\n");
            }
        }
        printf("OK eeprom action=read result=0 offset=%lu length=%lu\r\n",
               (unsigned long)offset,
               (unsigned long)length);
        return COMMAND_OK;
    }

    printf("INVALID_ARG usage=eeprom status|probe|read <offset_0_255> <length_1_64>\r\n");
    return COMMAND_INVALID_ARG;
}

static int command_adc_handler(int argc, const char *argv[])
{
    int result;

    if ((argc == 1) && (command_name_equal(argv[0], "status") != 0))
    {
        adc_measurement_print_status();
        return COMMAND_OK;
    }
    if ((argc == 2) && (command_name_equal(argv[0], "read") != 0))
    {
        if (command_name_equal(argv[1], "external") != 0)
        {
            result = adc_measurement_read_external(50U);
        }
        else if (command_name_equal(argv[1], "temperature") != 0)
        {
            result = adc_measurement_read_temperature(50U);
        }
        else if (command_name_equal(argv[1], "all") != 0)
        {
            result = adc_measurement_read_all(50U);
        }
        else
        {
            printf("INVALID_ARG usage=adc status|read external|temperature|all\r\n");
            return COMMAND_INVALID_ARG;
        }

        adc_measurement_print_status();
        printf("%s adc action=read target=%s result=%d timeout_ms=50\r\n",
               (result == ADC_MEASUREMENT_OK) ? "OK" : "ERROR",
               argv[1], result);
        return (result == ADC_MEASUREMENT_OK) ? COMMAND_OK : result;
    }

    printf("INVALID_ARG usage=adc status|read external|temperature|all\r\n");
    return COMMAND_INVALID_ARG;
}

static int command_tpad_handler(int argc, const char *argv[])
{
    int result;

    if ((argc == 1) && (command_name_equal(argv[0], "status") != 0))
    {
        tpad_print_status();
        return COMMAND_OK;
    }
    if ((argc == 1) && (command_name_equal(argv[0], "calibrate") != 0))
    {
        result = tpad_calibrate(300U);
        tpad_print_status();
        printf("%s tpad action=calibrate result=%d timeout_ms=300\r\n",
               (result == TPAD_OK) ? "OK" : "ERROR", result);
        return (result == TPAD_OK) ? COMMAND_OK : result;
    }
    if ((argc == 1) && (command_name_equal(argv[0], "read") != 0))
    {
        result = tpad_read(100U);
        tpad_print_status();
        printf("%s tpad action=read result=%d timeout_ms=100\r\n",
               (result == TPAD_OK) ? "OK" : "ERROR", result);
        return (result == TPAD_OK) ? COMMAND_OK : result;
    }

    printf("INVALID_ARG usage=tpad status|calibrate|read\r\n");
    return COMMAND_INVALID_ARG;
}

static int command_ir_handler(int argc, const char *argv[])
{
    ir_remote_status_t status;

    if ((argc == 1) && (command_name_equal(argv[0], "status") != 0))
    {
        ir_remote_print_status();
        return COMMAND_OK;
    }
    if ((argc == 1) && (command_name_equal(argv[0], "read") != 0))
    {
        ir_remote_get_status(&status);
        ir_remote_print_status();
        if (status.frame_valid == 0U)
        {
            printf("ERROR ir action=read result=%d\r\n", IR_REMOTE_NOT_READY);
            return IR_REMOTE_NOT_READY;
        }
        printf("ir data address=0x%02X command=0x%02X\r\n",
               (unsigned int)status.address,
               (unsigned int)status.command);
        printf("OK ir action=read result=0\r\n");
        return COMMAND_OK;
    }
    if ((argc == 1) && (command_name_equal(argv[0], "clear") != 0))
    {
        ir_remote_clear();
        ir_remote_print_status();
        printf("OK ir action=clear result=0\r\n");
        return COMMAND_OK;
    }

    printf("INVALID_ARG usage=ir status|read|clear\r\n");
    return COMMAND_INVALID_ARG;
}

static int command_audio_handler(int argc, const char *argv[])
{
    char path[COMMAND_LINE_SIZE];
    size_t used;
    uint32_t volume;
    int index;
    int result;

    if ((argc == 1) && (command_name_equal(argv[0], "status") != 0))
    {
        audio_print_status();
        audio_record_print_status();
        return COMMAND_OK;
    }
    if ((argc == 1) && (command_name_equal(argv[0], "probe") != 0))
    {
        result = es8388_probe_readonly(10U);
        es8388_print_status();
        printf("%s audio action=probe result=%d timeout_ms=10\r\n",
               (result == ES8388_OK) ? "OK" : "ERROR", result);
        return (result == ES8388_OK) ? COMMAND_OK : result;
    }
    if ((argc == 1) && (command_name_equal(argv[0], "stop") != 0))
    {
        result = audio_record_stop();
        if (result == AUDIO_RECORD_OK)
        {
            result = music_player_stop();
        }
        printf("%s audio action=stop result=%d\r\n",
               (result == MUSIC_PLAYER_OK) ? "OK" : "ERROR", result);
        return (result == MUSIC_PLAYER_OK) ? COMMAND_OK : result;
    }
    if ((argc == 2) && (command_name_equal(argv[0], "volume") != 0))
    {
        if (command_parse_uint(argv[1], 0U, 100U, &volume) == 0)
        {
            printf("INVALID_ARG usage=audio volume <0-100>\r\n");
            return COMMAND_INVALID_ARG;
        }
        result = audio_set_volume_percent((uint8_t)volume);
        if (result == AUDIO_OK)
        {
            system_settings_update_volume((uint8_t)volume, HAL_GetTick());
        }
        printf("%s audio action=volume value=%lu result=%d\r\n",
               (result == AUDIO_OK) ? "OK" : "ERROR",
               (unsigned long)volume, result);
        return (result == AUDIO_OK) ? COMMAND_OK : result;
    }
    if ((argc >= 2) &&
        ((command_name_equal(argv[0], "play") != 0) ||
         (command_name_equal(argv[0], "record") != 0)))
    {
        path[0] = '\0';
        used = 0U;
        for (index = 1; index < argc; index++)
        {
            size_t length = strlen(argv[index]);
            size_t separator = (index > 1) ? 1U : 0U;

            if ((used + separator + length) >= sizeof(path))
            {
                printf("INVALID_ARG field=sd_path reason=too_long\r\n");
                return COMMAND_INVALID_ARG;
            }
            if (separator != 0U)
            {
                path[used++] = ' ';
            }
            memcpy(path + used, argv[index], length);
            used += length;
            path[used] = '\0';
        }
        if (command_name_equal(argv[0], "record") != 0)
        {
            result = audio_record_start(path);
            printf("%s audio action=record result=%d path=%s\r\n",
                   (result == AUDIO_RECORD_OK) ? "OK" : "ERROR",
                   result, path);
            return (result == AUDIO_RECORD_OK) ? COMMAND_OK : result;
        }
        result = audio_play(path);
        printf("%s audio action=play result=%d path=%s\r\n",
               (result == AUDIO_OK) ? "OK" : "ERROR", result, path);
        return (result == AUDIO_OK) ? COMMAND_OK : result;
    }

    printf("INVALID_ARG usage=audio status|probe|play <sd_path>|record <sd_path>|stop|volume <0-100>\r\n");
    return COMMAND_INVALID_ARG;
}

static int command_nand_handler(int argc, const char *argv[])
{
    nand_id_t id;
    nand_geometry_t geometry;
    storage_status_t storage_status;
    uint8_t data[NAND_COMMAND_MAX_READ];
    uint32_t page;
    uint32_t offset;
    uint32_t length;
    uint32_t index;
    int result;

    if ((argc == 1) && (command_name_equal(argv[0], "status") != 0))
    {
        result = storage_get_status(STORAGE_NAND, &storage_status);
        printf("nand present=%u ready=%u result=%ld errors=%lu compact=0x%08lX scan=0x%08lX\r\n",
               (unsigned int)storage_status.present,
               (unsigned int)storage_status.ready,
               (long)storage_status.last_result,
               (unsigned long)storage_status.error_count,
               (unsigned long)storage_status.detail0,
               (unsigned long)storage_status.detail1);
        nand_scan_print_status();
        return (result == STORAGE_OK) ? COMMAND_OK : result;
    }

    if ((argc == 1) && (command_name_equal(argv[0], "id") != 0))
    {
        result = nand_readonly_probe(NAND_COMMAND_TIMEOUT_MS);
        if ((result != NAND_READONLY_OK) &&
            (result != NAND_READONLY_UNKNOWN_ID))
        {
            printf("ERROR nand action=id result=%d\r\n", result);
            return result;
        }
        if (nand_readonly_get_id(&id) != NAND_READONLY_OK)
        {
            printf("ERROR nand action=id result=%d\r\n",
                   NAND_READONLY_IO_ERROR);
            return NAND_READONLY_IO_ERROR;
        }
        printf("%s nand id=%02X:%02X:%02X:%02X:%02X compact=0x%08lX result=%d\r\n",
               (result == NAND_READONLY_OK) ? "OK" : "ERROR",
               (unsigned int)id.bytes[0],
               (unsigned int)id.bytes[1],
               (unsigned int)id.bytes[2],
               (unsigned int)id.bytes[3],
               (unsigned int)id.bytes[4],
               (unsigned long)id.compact_id,
               result);
        if (nand_readonly_get_geometry(&geometry) == NAND_READONLY_OK)
        {
            printf("nand page_main=%lu page_spare=%lu pages_per_block=%lu blocks=%lu row_cycles=%u\r\n",
                   (unsigned long)geometry.page_main_size,
                   (unsigned long)geometry.page_spare_size,
                   (unsigned long)geometry.pages_per_block,
                   (unsigned long)geometry.block_count,
                   (unsigned int)geometry.row_address_cycles);
        }
        return (result == NAND_READONLY_OK) ? COMMAND_OK : result;
    }

    if ((argc == 1) && (command_name_equal(argv[0], "scan") != 0))
    {
        result = nand_scan_start();
        printf("%s nand action=scan result=%d\r\n",
               (result == NAND_READONLY_OK) ? "OK" : "ERROR", result);
        return (result == NAND_READONLY_OK) ? COMMAND_OK : result;
    }

    if ((argc == 1) && (command_name_equal(argv[0], "stop") != 0))
    {
        result = nand_scan_stop();
        printf("%s nand action=stop result=%d\r\n",
               (result == NAND_READONLY_OK) ? "OK" : "ERROR", result);
        return (result == NAND_READONLY_OK) ? COMMAND_OK : result;
    }

    if ((argc == 4) && (command_name_equal(argv[0], "read") != 0))
    {
        if ((command_parse_uint(argv[1], 0U, 0xFFFFFFFFUL, &page) == 0) ||
            (command_parse_uint(argv[2], 0U, 0xFFFFFFFFUL, &offset) == 0) ||
            (command_parse_uint(argv[3], 1U, NAND_COMMAND_MAX_READ,
                                &length) == 0))
        {
            printf("INVALID_ARG usage=nand read <page> <offset> <length_1_64>\r\n");
            return COMMAND_INVALID_ARG;
        }

        result = nand_readonly_read_page(page, offset, data, length,
                                         NAND_COMMAND_TIMEOUT_MS);
        if (result != NAND_READONLY_OK)
        {
            printf("ERROR nand action=read result=%d page=%lu offset=%lu length=%lu\r\n",
                   result,
                   (unsigned long)page,
                   (unsigned long)offset,
                   (unsigned long)length);
            return result;
        }
        for (index = 0U; index < length; index++)
        {
            if ((index % NAND_COMMAND_BYTES_PER_LINE) == 0U)
            {
                printf("nand data page=%lu offset=%lu:",
                       (unsigned long)page,
                       (unsigned long)(offset + index));
            }
            printf(" %02X", (unsigned int)data[index]);
            if (((index % NAND_COMMAND_BYTES_PER_LINE) ==
                 (NAND_COMMAND_BYTES_PER_LINE - 1U)) ||
                (index == (length - 1U)))
            {
                printf("\r\n");
            }
        }
        printf("OK nand action=read result=0 page=%lu offset=%lu length=%lu\r\n",
               (unsigned long)page,
               (unsigned long)offset,
               (unsigned long)length);
        return COMMAND_OK;
    }

    printf("INVALID_ARG usage=nand status|id|scan|stop|read <page> <offset> <length>\r\n");
    return COMMAND_INVALID_ARG;
}

static int command_image_handler(int argc, const char *argv[])
{
    char path[IMAGE_VIEWER_PATH_SIZE];
    size_t used;
    int index;
    int result;

    if ((argc == 1) && (command_name_equal(argv[0], "status") != 0))
    {
        image_viewer_app_print_status();
        return COMMAND_OK;
    }
    if ((argc == 1) && (command_name_equal(argv[0], "next") != 0))
        result = image_viewer_app_next();
    else if ((argc == 1) && (command_name_equal(argv[0], "prev") != 0))
        result = image_viewer_app_previous();
    else if ((argc == 1) && (command_name_equal(argv[0], "stop") != 0))
        result = image_viewer_app_stop();
    else if ((argc >= 2) &&
             ((command_name_equal(argv[0], "open") != 0) ||
              (command_name_equal(argv[0], "album") != 0)))
    {
        path[0] = '\0';
        used = 0U;
        for (index = 1; index < argc; index++)
        {
            size_t length = strlen(argv[index]);
            if ((used + length + ((index > 1) ? 1U : 0U)) >= sizeof(path))
            {
                printf("INVALID_ARG field=path reason=too_long\r\n");
                return COMMAND_INVALID_ARG;
            }
            if (index > 1) path[used++] = ' ';
            memcpy(path + used, argv[index], length);
            used += length;
            path[used] = '\0';
        }
        if (command_name_equal(argv[0], "open") != 0)
        {
            result = (g_image_open_file_callback != NULL) ?
                     g_image_open_file_callback(path) : IMAGE_VIEWER_NOT_READY;
        }
        else
        {
            result = (g_image_open_album_callback != NULL) ?
                     g_image_open_album_callback(path) : IMAGE_VIEWER_NOT_READY;
        }
    }
    else
    {
        printf("INVALID_ARG usage=image status|open <path>|album <path>|next|prev|stop\r\n");
        return COMMAND_INVALID_ARG;
    }

    printf("%s image action=%s result=%d\r\n",
           (result == IMAGE_VIEWER_OK) ? "OK" : "ERROR",
           argv[0], result);
    return (result == IMAGE_VIEWER_OK) ? COMMAND_OK : result;
}

static int command_camera_handler(int argc, const char *argv[])
{
    static const char *usage =
        "usage=camera status|probe|start|stop|mirror <0|1>|flip <0|1>|flash <0|1>|test_pattern <0|1>";
    uint32_t value;
    int result;
    camera_preview_status_t preview;
    camera_preview_frame_t frame;
    ov5640_status_t sensor;
    camera_resource_status_t resource;
    camera_dcmi_status_t dcmi;

    if ((argc < 1) || (argv == NULL))
    {
        printf("INVALID_ARG %s\r\n", usage);
        return COMMAND_INVALID_ARG;
    }
    if ((argc == 1) && (command_name_equal(argv[0], "status") != 0))
    {
        camera_preview_get_status(&preview);
        ov5640_get_status(&sensor);
        camera_resource_get_status(&resource);
        camera_dcmi_get_status(&dcmi);
        printf("camera status state=%d frame_valid=%u published=%lu dropped=%lu generation=%lu address=0x%08lX width=%u height=%u bytes=%lu result=%d\r\n",
               preview.state, preview.frame_valid,
               (unsigned long)preview.published_frames,
               (unsigned long)preview.dropped_frames,
               (unsigned long)preview.generation,
               (unsigned long)preview.rgb_address, preview.width,
               preview.height, (unsigned long)preview.bytes,
               preview.last_result);
        printf("camera sensor present=%u ready=%u id=0x%04X mirror=%u flip=%u flash=%u test_pattern=%u result=%d\r\n",
               sensor.present, sensor.ready, sensor.chip_id, sensor.mirror,
               sensor.flip, sensor.flash, sensor.test_pattern,
               sensor.last_result);
        printf("camera resource stage=%d acquired=%u release_errors=%u result=%d\r\n",
               resource.stage, resource.acquired, resource.release_errors,
               resource.last_result);
        printf("camera dcmi initialized=%u running=%u frames=%lu errors=%lu dma_errors=%lu extra_tc=%lu business_error=%lu dcmi_error=0x%08lX dma_error=0x%08lX risr=0x%08lX dma_lisr=0x%08lX result=%d\r\n",
               dcmi.initialized, dcmi.running,
               (unsigned long)dcmi.frame_events,
               (unsigned long)dcmi.error_events,
               (unsigned long)dcmi.dma_errors,
               (unsigned long)dcmi.extra_tc_events,
               (unsigned long)dcmi.business_error_code,
               (unsigned long)dcmi.dcmi_error_code,
               (unsigned long)dcmi.dma_error_code,
               (unsigned long)dcmi.dcmi_risr,
               (unsigned long)dcmi.dma_lisr,
               dcmi.last_result);
        printf("camera signal lines=%lu frame_seen=%lu dcmi_cr=0x%08lX dcmi_sr=0x%08lX dma_ndtr=%lu dma_cr=0x%08lX\r\n",
               (unsigned long)dcmi.line_count,
               (unsigned long)dcmi.frame_seen,
               (unsigned long)dcmi.dcmi_cr,
               (unsigned long)dcmi.dcmi_sr,
               (unsigned long)dcmi.dma_ndtr,
               (unsigned long)dcmi.dma_cr);
        if (camera_preview_get_frame(&frame) == CAMERA_PREVIEW_OK)
        {
            printf("camera frame address=0x%08lX generation=%lu\r\n",
                   (unsigned long)frame.rgb_address,
                   (unsigned long)frame.generation);
        }
        return COMMAND_OK;
    }
    if (command_name_equal(argv[0], "probe") != 0)
    {
        if (argc != 1)
        {
            printf("INVALID_ARG %s\r\n", usage);
            return COMMAND_INVALID_ARG;
        }
        result = ov5640_probe();
        printf("%s camera action=probe result=%d\r\n",
               (result == OV5640_OK) ? "OK" : "ERROR", result);
        return (result == OV5640_OK) ? COMMAND_OK : result;
    }
    if (command_name_equal(argv[0], "start") != 0)
    {
        if (argc != 1)
        {
            printf("INVALID_ARG %s\r\n", usage);
            return COMMAND_INVALID_ARG;
        }
        result = camera_preview_start();
        printf("%s camera action=start result=%d\r\n",
               (result == CAMERA_PREVIEW_IDLE) ? "OK" : "ERROR", result);
        return (result == CAMERA_PREVIEW_IDLE) ? COMMAND_OK : result;
    }
    if (command_name_equal(argv[0], "stop") != 0)
    {
        if (argc != 1)
        {
            printf("INVALID_ARG %s\r\n", usage);
            return COMMAND_INVALID_ARG;
        }
        result = camera_preview_stop();
        printf("%s camera action=stop result=%d\r\n",
               (result == CAMERA_PREVIEW_IDLE) ? "OK" : "ERROR", result);
        return (result == CAMERA_PREVIEW_IDLE) ? COMMAND_OK : result;
    }
    if ((command_name_equal(argv[0], "mirror") != 0) ||
        (command_name_equal(argv[0], "flip") != 0) ||
        (command_name_equal(argv[0], "flash") != 0) ||
        (command_name_equal(argv[0], "test_pattern") != 0))
    {
        if ((argc != 2) || !command_parse_uint(argv[1], 0U, 1U, &value))
        {
            printf("INVALID_ARG %s\r\n", usage);
            return COMMAND_INVALID_ARG;
        }
        if (command_name_equal(argv[0], "mirror") != 0)
            result = ov5640_set_mirror((uint8_t)value);
        else if (command_name_equal(argv[0], "flip") != 0)
            result = ov5640_set_flip((uint8_t)value);
        else if (command_name_equal(argv[0], "flash") != 0)
            result = ov5640_set_flash((uint8_t)value);
        else
            result = ov5640_set_test_pattern((uint8_t)value);
        printf("%s camera action=%s value=%lu result=%d\r\n",
               (result == OV5640_OK) ? "OK" : "ERROR", argv[0],
               (unsigned long)value, result);
        return (result == OV5640_OK) ? COMMAND_OK : result;
    }
    printf("INVALID_ARG %s\r\n", usage);
    return COMMAND_INVALID_ARG;
}

static int command_sd_handler(int argc, const char *argv[])
{
    sdcard_manager_status_t status;
    if ((argc == 1) && (command_name_equal(argv[0], "status") != 0))
    {
        sdcard_manager_get_status(&status);
        printf("sd status state=%d mounted=%u sessions=%u failures=%u init_attempts=%lu mount_failures=%lu removals=%lu result=%d\r\n",
               status.state, status.mounted, status.session_count,
               status.consecutive_check_failures,
               (unsigned long)status.init_attempts,
               (unsigned long)status.mount_failures,
               (unsigned long)status.removal_count, status.last_result);
        return COMMAND_OK;
    }
    if ((argc == 1) && (command_name_equal(argv[0], "reprobe") != 0))
    {
        sdcard_manager_request_reprobe();
        printf("OK sd action=reprobe result=0\r\n");
        return COMMAND_OK;
    }
    printf("INVALID_ARG usage=sd status|reprobe\r\n");
    return COMMAND_INVALID_ARG;
}

static int command_settings_handler(int argc, const char *argv[])
{
    if ((argc == 1) && (command_name_equal(argv[0], "status") != 0))
    {
        system_settings_print_status();
        return COMMAND_OK;
    }
    if ((argc == 1) && (command_name_equal(argv[0], "save") != 0))
    {
        system_settings_request_save();
        printf("OK settings action=save result=0\r\n");
        return COMMAND_OK;
    }
    printf("INVALID_ARG usage=settings status|save\r\n");
    return COMMAND_INVALID_ARG;
}

static int command_qspi_handler(int argc, const char *argv[])
{
    uint8_t result;

    if ((argc == 3) &&
        (command_name_equal(argv[0], "erase") != 0) &&
        (command_name_equal(argv[1], "assets") != 0) &&
        (strcmp(argv[2], "CONFIRM") == 0))
    {
        printf("qspi erase start target=assets\r\n");
        result = norflash_erase_assets();
        if (result == 0U)
        {
            printf("OK qspi action=erase target=assets result=0 reset_required=1\r\n");
            return COMMAND_OK;
        }
        printf("ERROR qspi action=erase target=assets result=%u\r\n",
               (unsigned int)result);
        return (int)result;
    }

    printf("INVALID_ARG usage=qspi erase assets CONFIRM\r\n");
    return COMMAND_INVALID_ARG;
}

static int command_font_cache_handler(int argc, const char *argv[])
{
    font_cache_service_status_t status;
    int result;

    if ((argc == 1) && (command_name_equal(argv[0], "status") != 0))
    {
        font_cache_service_get_status(&status);
        printf("font_cache status state=%u slot=%u sequence=%lu package=%lu corrected=%lu load_ms=%lu rebuild_ms=%lu result=%ld stage=%u target=%lu io_result=%ld nand_status=0x%02X status_valid=%u ready=%u\r\n",
               (unsigned int)status.state,
               (unsigned int)status.active_slot,
               (unsigned long)status.sequence,
               (unsigned long)status.package_size,
               (unsigned long)status.corrected_pages,
               (unsigned long)status.load_elapsed_ms,
               (unsigned long)status.rebuild_elapsed_ms,
               (long)status.last_result,
               (unsigned int)status.rebuild.stage,
               (unsigned long)status.rebuild.target,
               (long)status.rebuild.detail_result,
               (unsigned int)status.rebuild.nand_status,
               (unsigned int)status.rebuild.nand_status_valid,
               (unsigned int)status.rebuild.nand_ready);
        return COMMAND_OK;
    }
    if ((argc == 1) && (command_name_equal(argv[0], "rebuild") != 0))
    {
        result = font_cache_service_rebuild();
        font_cache_service_get_status(&status);
        printf("%s font_cache action=rebuild result=%d slot=%u sequence=%lu package=%lu elapsed_ms=%lu\r\n",
               (result == 0) ? "OK" : "ERROR", result,
               (unsigned int)status.active_slot,
               (unsigned long)status.sequence,
               (unsigned long)status.package_size,
               (unsigned long)status.rebuild_elapsed_ms);
        return (result == 0) ? COMMAND_OK : result;
    }
    if ((argc == 1) && (command_name_equal(argv[0], "invalidate") != 0))
    {
        result = font_cache_service_invalidate();
        printf("%s font_cache action=invalidate result=%d\r\n",
               (result == 0) ? "OK" : "ERROR", result);
        return (result == 0) ? COMMAND_OK : result;
    }
    printf("INVALID_ARG usage=font_cache status|rebuild|invalidate\r\n");
    return COMMAND_INVALID_ARG;
}

static void command_register_target_commands(void)
{
    static const command_entry_t entries[] = {
        {"help", "help / ? - \xE6\x98\xBE\xE7\xA4\xBA\xE5\x91\xBD\xE4\xBB\xA4\xE5\xB8\xAE\xE5\x8A\xA9", command_help_handler},
        {"?", NULL, command_help_handler},
        {"clear", "clear / cls - \xE6\xB8\x85\xE7\xA9\xBA\xE4\xB8\xB2\xE5\x8F\xA3\xE6\x98\xBE\xE7\xA4\xBA", command_clear_handler},
        {"cls", NULL, command_clear_handler},
        {"bright", "bright <0-100> - \xE8\xAE\xBE\xE7\xBD\xAE\xE5\xB1\x8F\xE5\xB9\x95\xE4\xBA\xAE\xE5\xBA\xA6", command_brightness_handler},
        {"brightness", NULL, command_brightness_handler},
        {"screenoff", "screenoff <5-3600> - \xE8\xAE\xBE\xE7\xBD\xAE\xE8\x87\xAA\xE5\x8A\xA8\xE6\x81\xAF\xE5\xB1\x8F\xE7\xA7\x92\xE6\x95\xB0", command_screenoff_handler},
        {"timeout", NULL, command_screenoff_handler},
        {"time", "time <HH:MM:SS> - \xE8\xAE\xBE\xE7\xBD\xAE\x52\x54\x43\xE6\x97\xB6\xE9\x97\xB4", command_time_handler},
        {"date", "date <YY-MM-DD> - \xE8\xAE\xBE\xE7\xBD\xAE\x52\x54\x43\xE6\x97\xA5\xE6\x9C\x9F", command_date_handler},
        {"alarm", "alarm <HH:MM> - \xE6\xB7\xBB\xE5\x8A\xA0\xE9\x97\xB9\xE9\x92\x9F", command_alarm_handler},
        {"alarmoff", "alarmoff - \xE5\x85\xB3\xE9\x97\xAD\xE6\x9C\x80\xE8\xBF\x91\xE7\x9A\x84\xE9\x97\xB9\xE9\x92\x9F", command_alarmoff_handler},
        {"sys", "sys status - \xE6\x9F\xA5\xE7\x9C\x8B\xE7\xB3\xBB\xE7\xBB\x9F\xE8\xAF\x8A\xE6\x96\xAD\xE7\x8A\xB6\xE6\x80\x81", command_sys_handler},
        {"lvgl", "lvgl status - LVGL memory diagnostics", command_lvgl_handler},
        {"settings", "settings status|save - \xE6\x9F\xA5\xE7\x9C\x8B\xE6\x88\x96\xE4\xBF\x9D\xE5\xAD\x98\xE7\xB3\xBB\xE7\xBB\x9F\xE9\x85\x8D\xE7\xBD\xAE", command_settings_handler},
        {"font_cache", "font_cache status|rebuild|invalidate - font cache control", command_font_cache_handler},
        {"qspi", "qspi erase assets CONFIRM - erase wallpaper and image assets", command_qspi_handler},
        {"resource", "resource status - \xE6\x9F\xA5\xE7\x9C\x8B\xE5\x85\xAC\xE5\x85\xB1\xE8\xB5\x84\xE6\xBA\x90\xE5\x8D\xA0\xE7\x94\xA8\xE7\x8A\xB6\xE6\x80\x81", command_resource_handler},
        {"buffer", "buffer status - \xE6\x9F\xA5\xE7\x9C\x8B\xE5\x85\xB1\xE4\xBA\xAB\xE7\xBC\x93\xE5\x86\xB2\xE5\x8C\xBA\xE7\x8A\xB6\xE6\x80\x81", command_buffer_handler},
        {"infra", "infra selftest - \xE8\xBF\x90\xE8\xA1\x8C\xE5\x85\xAC\xE5\x85\xB1\xE5\x9F\xBA\xE7\xA1\x80\xE8\xAE\xBE\xE6\x96\xBD\xE8\x87\xAA\xE6\xB5\x8B", command_infra_handler},
        {"log", "log level error|warn|info|trace - \xE8\xAE\xBE\xE7\xBD\xAE\xE6\x97\xA5\xE5\xBF\x97\xE7\xAD\x89\xE7\xBA\xA7", command_log_handler},
        {"fault", "fault show|clear - \xE6\x9F\xA5\xE7\x9C\x8B\xE6\x88\x96\xE6\xB8\x85\xE9\x99\xA4\xE9\xA6\x96\xE4\xB8\xAA\xE6\x95\x85\xE9\x9A\x9C\xE5\xBF\xAB\xE7\x85\xA7", command_fault_handler},
        {"app", "app status - \xE6\x9F\xA5\xE7\x9C\x8B\xE5\xBA\x94\xE7\x94\xA8\xE8\xBF\x90\xE8\xA1\x8C\xE7\x8A\xB6\xE6\x80\x81", command_app_handler},
        {"probe", "probe status|run - \xE6\x9F\xA5\xE7\x9C\x8B\xE6\x88\x96\xE9\x87\x8D\xE6\x96\xB0\xE8\xBF\x90\xE8\xA1\x8C\xE4\xB8\x8A\xE7\x94\xB5\xE5\xA4\x96\xE8\xAE\xBE\xE6\x8E\xA2\xE6\xB5\x8B", command_probe_handler},
        {"sensor", "sensor imu|sh3001|st480mc status|probe | sensor pressure|ap3216c status|probe|read - \xE6\x9F\xA5\xE7\x9C\x8B\xE3\x80\x81\xE8\xAF\x86\xE5\x88\xAB\xE6\x88\x96\xE8\xAF\xBB\xE5\x8F\x96\xE4\xBC\xA0\xE6\x84\x9F\xE5\x99\xA8", command_sensor_handler},
        {"storage", "storage status|test sd - \xE6\x9F\xA5\xE7\x9C\x8B\xE5\xAD\x98\xE5\x82\xA8\xE7\x8A\xB6\xE6\x80\x81\xE6\x88\x96\xE6\x89\xA7\xE8\xA1\x8C\xE5\x8F\xAA\xE8\xAF\xBB\x53\x44\xE6\xB5\x8B\xE8\xAF\x95", command_storage_handler},
        {"adc", "adc status|read external|temperature|all - \xE8\xAF\xBB\xE5\x8F\x96\xE5\xA4\x96\xE9\x83\xA8\xE7\x94\xB5\xE5\x8E\x8B\xE5\x92\x8C\xE8\x8A\xAF\xE7\x89\x87\xE6\xB8\xA9\xE5\xBA\xA6", command_adc_handler},
        {"eeprom", "eeprom status|probe|read <offset> <length> - \xE6\x9F\xA5\xE7\x9C\x8B\x45\x45\x50\x52\x4F\x4D\xE5\x8F\xAA\xE8\xAF\xBB\xE7\x8A\xB6\xE6\x80\x81\xE5\x92\x8C\xE5\x86\x85\xE5\xAE\xB9", command_eeprom_handler},
        {"nand", "nand status|id|scan|stop|read <page> <offset> <length> - \xE6\x9F\xA5\xE7\x9C\x8B\x4E\x41\x4E\x44\xE5\x8F\xAA\xE8\xAF\xBB\xE7\x8A\xB6\xE6\x80\x81\xE5\x92\x8C\xE6\x89\xAB\xE6\x8F\x8F", command_nand_handler},
        {"tpad", "tpad status|calibrate|read - \xE6\x9F\xA5\xE7\x9C\x8B\xE6\x88\x96\xE8\xAF\xBB\xE5\x8F\x96 TPAD \xE7\x94\xB5\xE5\xAE\xB9\xE8\xA7\xA6\xE6\x91\xB8", command_tpad_handler},
        {"ir", "ir status|read|clear - \xE6\x9F\xA5\xE7\x9C\x8B\xE6\x88\x96\xE8\xAF\xBB\xE5\x8F\x96\xE7\xBA\xA2\xE5\xA4\x96\xE5\xB8\xA7", command_ir_handler},
        {"audio", "audio status|probe|play <sd_path>|record <sd_path>|stop|volume <0-100> - \xE9\x9F\xB3\xE9\xA2\x91\xE6\x92\xAD\xE6\x94\xBE\xE5\x92\x8C\x5B\x55\xE9\x9F\xB3\xE5\xBD\x95\xE5\x88\xB6", command_audio_handler},
        {"image", "image status|open <path>|album <path>|next|prev|stop - image viewer control", command_image_handler},
        {"camera", "camera status|probe|start|stop|mirror <0|1>|flip <0|1>|flash <0|1>|test_pattern <0|1> - \xE6\x91\x84\xE5\x83\x8F\xE5\xA4\xB4\xE9\xA2\x84\xE8\xA7\x88\xE5\x92\x8C\xE8\xAE\xBE\xE7\xBD\xAE", command_camera_handler},
        {"sd", "sd status|reprobe - \x53\x44\xE5\x8D\xA1\xE7\x8A\xB6\xE6\x80\x81\xE5\x92\x8C\xE7\x83\xAD\xE6\x8F\x92\xE6\x8B\x94", command_sd_handler}
    };
    uint32_t index;

    for (index = 0U; index < (sizeof(entries) / sizeof(entries[0])); index++)
    {
        (void)command_register(&entries[index]);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if ((huart != NULL) && (huart->Instance == USART1))
    {
        command_parser_uart_rx_byte_from_isr(uart_rx_buf[0]);
        (void)HAL_UART_Receive_IT(&huart1, uart_rx_buf, 1U);
    }
}
#endif
