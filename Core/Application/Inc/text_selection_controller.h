#ifndef TEXT_SELECTION_CONTROLLER_H
#define TEXT_SELECTION_CONTROLLER_H

#include <stdint.h>

#define TEXT_SELECTION_CLIPBOARD_BYTES 65536U

typedef enum {
    TEXT_SELECT_LEFT, TEXT_SELECT_RIGHT, TEXT_SELECT_UP, TEXT_SELECT_DOWN,
    TEXT_SELECT_HOME, TEXT_SELECT_END, TEXT_SELECT_OK,
    TEXT_SELECT_COPY, TEXT_SELECT_PASTE, TEXT_SELECT_CANCEL
} text_selection_action_t;

typedef enum {
    TEXT_SELECTION_VERTICAL_UP,
    TEXT_SELECTION_VERTICAL_DOWN
} text_selection_vertical_t;

typedef enum {
    TEXT_SELECTION_OK,
    TEXT_SELECTION_INVALID_ARG,
    TEXT_SELECTION_NOT_READY,
    TEXT_SELECTION_TARGET_ERROR,
    TEXT_SELECTION_EMPTY,
    TEXT_SELECTION_CAPACITY
} text_selection_result_t;

typedef enum {
    TEXT_SELECTION_PHASE_IDLE,
    TEXT_SELECTION_PHASE_ANCHORED,
    TEXT_SELECTION_PHASE_FIXED
} text_selection_phase_t;

typedef struct {
    uint32_t (*get_length)(void *context);
    uint32_t (*get_cursor)(void *context);
    int (*set_cursor)(void *context, uint32_t cursor);
    int (*move_vertical)(void *context, text_selection_vertical_t direction,
                         uint32_t *cursor);
    /* 同线程绘制通知，不参与数据事务。 */
    void (*show_selection)(void *context, uint8_t visible, uint32_t start,
                           uint32_t end);
    /* 返回0时精确写入[start,end)；失败时完全不写output。 */
    int (*read_range)(void *context, uint32_t start, uint32_t end,
                      uint8_t *output, uint32_t capacity);
    int (*replace_range)(void *context, uint32_t start, uint32_t end,
                         const uint8_t *replacement, uint32_t replacement_bytes,
                         uint32_t *cursor);
} text_selection_target_ops_t;

typedef struct {
    void *target;
    const text_selection_target_ops_t *ops;
    uint8_t *clipboard;
    uint32_t clipboard_capacity;
    uint32_t clipboard_bytes;
    uint32_t anchor;
    uint32_t selection_start;
    uint32_t selection_end;
    text_selection_phase_t phase;
} text_selection_controller_t;

typedef struct {
    uint32_t anchor;
    uint32_t clipboard_bytes;
    uint32_t selection_start;
    uint32_t selection_end;
    text_selection_phase_t phase;
} text_selection_status_t;

text_selection_result_t text_selection_init(text_selection_controller_t *controller,
                                             uint8_t *clipboard,
                                             uint32_t clipboard_capacity);
text_selection_result_t text_selection_set_target(text_selection_controller_t *controller,
                                                   void *target,
                                                   const text_selection_target_ops_t *ops);
text_selection_result_t text_selection_action(text_selection_controller_t *controller,
                                               text_selection_action_t action);
text_selection_result_t text_selection_replace_active(
    text_selection_controller_t *controller, const uint8_t *replacement,
    uint32_t replacement_bytes, uint8_t *handled);
void text_selection_get_status(const text_selection_controller_t *controller,
                               text_selection_status_t *status);

#endif
