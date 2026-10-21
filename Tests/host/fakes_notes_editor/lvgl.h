#ifndef TEST_NOTES_EDITOR_FAKE_LVGL_H
#define TEST_NOTES_EDITOR_FAKE_LVGL_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define LV_EVENT_VALUE_CHANGED 1U
#define LV_EVENT_DELETE 2U
#define LV_PART_MAIN 0U
#define LV_LABEL_TEXT_SELECTION_OFF UINT32_MAX
#define LV_STATE_DISABLED 1U

typedef uint32_t lv_event_code_t;
typedef uint32_t lv_state_t;
typedef enum { LV_RESULT_OK = 0, LV_RESULT_INVALID = 1 } lv_result_t;
typedef void (*lv_async_cb_t)(void *user_data);
typedef struct lv_obj_t lv_obj_t;
typedef struct lv_event_t lv_event_t;
typedef struct lv_event_dsc_t lv_event_dsc_t;
typedef void (*lv_event_cb_t)(lv_event_t *event);
typedef struct { int32_t x; int32_t y; } lv_point_t;
typedef struct { uint16_t line_height; } lv_font_t;

struct lv_event_t {
    lv_obj_t *target;
    lv_event_code_t code;
};

struct lv_event_dsc_t {
    lv_event_cb_t callback;
    lv_event_code_t filter;
};

struct lv_obj_t {
    char text[40000];
    lv_obj_t *label;
    lv_event_dsc_t events[8];
    uint32_t event_count;
    uint32_t cursor;
    uint32_t selection_start;
    uint32_t selection_end;
    uint32_t max_length;
    const char *accepted_chars;
    lv_font_t font;
    bool disabled;
};

extern uint32_t g_notes_fake_ignore_set_count;
extern uint32_t g_notes_fake_value_events;
extern lv_async_cb_t g_notes_fake_async_cb;
extern void *g_notes_fake_async_data;

static inline uint32_t fake_utf8_width(const char *text, uint32_t offset)
{
    uint8_t lead = (uint8_t)text[offset];
    if (lead < 0x80U) return 1U;
    if ((lead & 0xE0U) == 0xC0U) return 2U;
    if ((lead & 0xF0U) == 0xE0U) return 3U;
    return 4U;
}

static inline uint32_t fake_character_count(const char *text)
{
    uint32_t byte = 0U;
    uint32_t count = 0U;
    while (text[byte] != '\0') {
        byte += fake_utf8_width(text, byte);
        count++;
    }
    return count;
}

static inline void fake_send_event(lv_obj_t *obj, lv_event_code_t code)
{
    lv_event_t event = {obj, code};
    uint32_t index;
    if (code == LV_EVENT_VALUE_CHANGED) g_notes_fake_value_events++;
    for (index = 0U; index < obj->event_count; index++)
        if (obj->events[index].filter == code)
            obj->events[index].callback(&event);
}

static inline void fake_run_async(void)
{
    lv_async_cb_t callback = g_notes_fake_async_cb;
    void *data = g_notes_fake_async_data;
    g_notes_fake_async_cb = NULL;
    g_notes_fake_async_data = NULL;
    if (callback != NULL) callback(data);
}

static inline lv_result_t lv_async_call(lv_async_cb_t callback, void *user_data)
{
    if (callback == NULL || g_notes_fake_async_cb != NULL)
        return LV_RESULT_INVALID;
    g_notes_fake_async_cb = callback;
    g_notes_fake_async_data = user_data;
    return LV_RESULT_OK;
}

static inline lv_result_t lv_async_call_cancel(lv_async_cb_t callback,
                                                void *user_data)
{
    if (g_notes_fake_async_cb != callback ||
        g_notes_fake_async_data != user_data) return LV_RESULT_INVALID;
    g_notes_fake_async_cb = NULL;
    g_notes_fake_async_data = NULL;
    return LV_RESULT_OK;
}

static inline bool lv_obj_has_state(const lv_obj_t *obj, lv_state_t state)
{
    return state == LV_STATE_DISABLED && obj->disabled;
}

static inline void lv_obj_add_state(lv_obj_t *obj, lv_state_t state)
{
    if (state == LV_STATE_DISABLED) obj->disabled = true;
}

static inline void lv_obj_remove_state(lv_obj_t *obj, lv_state_t state)
{
    if (state == LV_STATE_DISABLED) obj->disabled = false;
}

static inline lv_event_dsc_t *lv_obj_add_event_cb(lv_obj_t *obj,
    lv_event_cb_t callback, lv_event_code_t filter, void *user_data)
{
    lv_event_dsc_t *event;
    (void)user_data;
    if (obj->event_count >= 8U) return NULL;
    event = &obj->events[obj->event_count++];
    event->callback = callback;
    event->filter = filter;
    return event;
}

static inline bool lv_obj_remove_event_dsc(lv_obj_t *obj,
                                           lv_event_dsc_t *event)
{
    uint32_t index;
    for (index = 0U; index < obj->event_count; index++) {
        if (&obj->events[index] != event) continue;
        memmove(&obj->events[index], &obj->events[index + 1U],
                (obj->event_count - index - 1U) * sizeof(obj->events[0]));
        obj->event_count--;
        return true;
    }
    return false;
}

static inline lv_event_code_t lv_event_get_code(lv_event_t *event)
{
    return event->code;
}

static inline const char *lv_textarea_get_text(lv_obj_t *obj)
{
    return obj->text;
}

static inline uint32_t lv_textarea_get_max_length(lv_obj_t *obj)
{
    return obj->max_length;
}

static inline void lv_textarea_set_max_length(lv_obj_t *obj, uint32_t length)
{
    obj->max_length = length;
}

static inline void lv_textarea_set_accepted_chars(lv_obj_t *obj,
                                                   const char *characters)
{
    obj->accepted_chars = characters;
}

static inline void lv_textarea_set_text(lv_obj_t *obj, const char *text)
{
    if (g_notes_fake_ignore_set_count != 0U) {
        g_notes_fake_ignore_set_count--;
        return;
    }
    (void)strncpy(obj->text, text, sizeof(obj->text) - 1U);
    obj->text[sizeof(obj->text) - 1U] = '\0';
    if (obj->label != NULL) {
        (void)strncpy(obj->label->text, obj->text,
                      sizeof(obj->label->text) - 1U);
        obj->label->text[sizeof(obj->label->text) - 1U] = '\0';
    }
}

static inline void lv_textarea_add_text(lv_obj_t *obj, const char *text)
{
    size_t current;
    size_t added;
    if (obj->disabled) return;
    current = strlen(obj->text);
    added = strlen(text);
    if (current + added >= sizeof(obj->text)) return;
    memcpy(obj->text + current, text, added + 1U);
    fake_send_event(obj, LV_EVENT_VALUE_CHANGED);
}

static inline uint32_t lv_textarea_get_cursor_pos(lv_obj_t *obj)
{
    return obj->cursor;
}

static inline void lv_textarea_set_cursor_pos(lv_obj_t *obj, int32_t cursor)
{
    uint32_t length = fake_character_count(obj->text);
    obj->cursor = cursor < 0 || (uint32_t)cursor > length ?
                  length : (uint32_t)cursor;
}

static inline lv_obj_t *lv_textarea_get_label(lv_obj_t *obj)
{
    return obj->label;
}

static inline const lv_font_t *lv_obj_get_style_text_font(lv_obj_t *obj,
                                                           uint32_t part)
{
    (void)part;
    return &obj->font;
}

static inline int32_t lv_obj_get_style_text_line_space(lv_obj_t *obj,
                                                       uint32_t part)
{
    (void)obj;
    (void)part;
    return 0;
}

static inline void lv_label_get_letter_pos(lv_obj_t *obj, uint32_t character,
                                            lv_point_t *point)
{
    (void)obj;
    point->x = (int32_t)character;
    point->y = 0;
}

static inline uint32_t lv_label_get_letter_on(lv_obj_t *obj,
                                               lv_point_t *point, bool bidi)
{
    uint32_t length = fake_character_count(obj->text);
    (void)bidi;
    if (point->x < 0) return 0U;
    return (uint32_t)point->x > length ? length : (uint32_t)point->x;
}

static inline void lv_label_set_text_selection_start(lv_obj_t *obj,
                                                      uint32_t value)
{
    obj->selection_start = value;
}

static inline void lv_label_set_text_selection_end(lv_obj_t *obj,
                                                    uint32_t value)
{
    obj->selection_end = value;
}

#endif
