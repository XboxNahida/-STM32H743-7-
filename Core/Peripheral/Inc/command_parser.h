#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include <stdint.h>

#define COMMAND_LINE_SIZE       128U
#define COMMAND_MAX_ARGS        8U
#define COMMAND_MAX_ENTRIES     40U
#define COMMAND_RX_QUEUE_SIZE   256U

typedef int (*command_handler_t)(int argc, const char *argv[]);
typedef void (*command_clear_callback_t)(void);
typedef int (*command_path_callback_t)(const char *path);

typedef struct {
    const char *name;
    const char *help;
    command_handler_t handler;
} command_entry_t;

typedef enum {
    COMMAND_OK = 0,
    COMMAND_INVALID_ARG = -1,
    COMMAND_FULL = -2,
    COMMAND_DUPLICATE = -3,
    COMMAND_NOT_FOUND = -4
} command_result_t;

void command_parser_init(void);
void command_parser_uart_rx_byte_from_isr(uint8_t byte);
void command_parser_poll(void);
void command_parser_poll_at(uint32_t now_ms);
int command_parser_execute_line(const char *line);
int command_register(const command_entry_t *entry);
void command_parser_set_clear_callback(command_clear_callback_t callback);
void command_parser_set_image_callbacks(command_path_callback_t open_file,
                                        command_path_callback_t open_album);
void command_print_help(void);
uint32_t command_parser_get_line_overflow_count(void);
uint32_t command_parser_get_rx_drop_count(void);

#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
extern uint8_t uart_rx_buf[];
extern char uart_display_buf[];
extern uint16_t uart_display_len;
extern uint16_t uart_last_display_len;
#endif

#endif
