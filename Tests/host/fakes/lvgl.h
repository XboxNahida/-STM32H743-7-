#ifndef TEST_FAKE_LVGL_H
#define TEST_FAKE_LVGL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define LV_USE_ARABIC_PERSIAN_CHARS 0
#define LV_EVENT_INSERT 1
#define LV_EVENT_DELETE 2
#define LV_EVENT_VALUE_CHANGED 3
#define LV_PART_MAIN 0
#define LV_STATE_DEFAULT 0
#define LV_STATE_DISABLED 1
#define LV_OPA_80 80
#define LV_ALIGN_TOP_MID 0
#define LV_LABEL_TEXT_SELECTION_OFF UINT32_MAX
#define LV_TEXTAREA_CURSOR_LAST (-1)
#define LV_BUTTONMATRIX_BUTTON_NONE UINT32_MAX
#define LV_BUTTONMATRIX_CTRL_POPOVER 0x100U
#define LV_BUTTONMATRIX_CTRL_CHECKED 0x200U
#define LV_BUTTONMATRIX_CTRL_HIDDEN 0x010U
#define LV_KEYBOARD_CTRL_BUTTON_FLAGS 0x400U
#define LV_SYMBOL_BACKSPACE "<BS>"
#define LV_SYMBOL_NEW_LINE "<NL>"
#define LV_SYMBOL_LEFT "<LEFT>"
#define LV_SYMBOL_RIGHT "<RIGHT>"
#define LV_SYMBOL_UP "<UP>"
#define LV_SYMBOL_DOWN "<DOWN>"
#define LV_SYMBOL_OK "<OK>"
#define LV_UNUSED(value) ((void)(value))
#define LV_FONT_DECLARE(name) extern lv_font_t name

typedef uint32_t lv_event_code_t;
typedef uint32_t lv_buttonmatrix_ctrl_t;
typedef struct lv_obj_t lv_obj_t;
typedef struct lv_event_t lv_event_t;
typedef struct lv_event_dsc_t lv_event_dsc_t;
typedef struct lv_timer_t lv_timer_t;
typedef void (*lv_event_cb_t)(lv_event_t *event);
typedef enum {
    LV_KEYBOARD_MODE_TEXT_LOWER,
    LV_KEYBOARD_MODE_TEXT_UPPER,
    LV_KEYBOARD_MODE_SPECIAL,
    LV_KEYBOARD_MODE_NUMBER,
    LV_KEYBOARD_MODE_USER_1,
    LV_KEYBOARD_MODE_USER_2
} lv_keyboard_mode_t;
typedef struct { int32_t x; int32_t y; } lv_point_t;
typedef struct { uint16_t line_height; } lv_font_t;
struct lv_timer_t { void (*callback)(lv_timer_t *); int repeats; };
struct lv_event_t {
    lv_obj_t *current_target;
    lv_event_code_t code;
    void *param;
    void *user_data;
    bool stopped;
};
struct lv_event_dsc_t {
    lv_event_cb_t callback;
    lv_event_code_t filter;
    void *user_data;
};
struct lv_obj_t {
    char text[256];
    const char *accepted;
    const char *insert_replace;
    lv_obj_t *label;
    lv_obj_t *textarea;
    lv_event_dsc_t *events[24];
    const char *const *maps[8];
    const lv_buttonmatrix_ctrl_t *ctrl_maps[8];
    const lv_font_t *font;
    uint32_t event_count;
    uint32_t cursor;
    uint32_t max_length;
    uint32_t selected_button;
    uint32_t selection_start;
    uint32_t selection_end;
    lv_keyboard_mode_t mode;
    bool one_line;
    bool disabled;
    bool deleted;
};
typedef struct {
    lv_event_dsc_t descriptors[96];
    lv_obj_t top_layer;
    lv_obj_t message;
    lv_timer_t timer;
    uint32_t descriptor_count;
    uint32_t add_calls;
    uint32_t fail_add_call;
    uint32_t malloc_calls;
    bool fail_malloc;
    bool fail_remove_event;
    uint32_t default_calls;
} fake_lvgl_state_t;

extern fake_lvgl_state_t g_fake_lvgl;
extern lv_font_t lv_font_myyh_18;
extern lv_font_t g_fake_default_font;
void lv_keyboard_def_event_cb(lv_event_t *event);

static inline uint32_t fake_utf8_width(const char *text, uint32_t offset)
{
    uint8_t lead = (uint8_t)text[offset];
    if (lead < 0x80U) return 1U;
    if ((lead & 0xE0U) == 0xC0U) return 2U;
    if ((lead & 0xF0U) == 0xE0U) return 3U;
    return 4U;
}
static inline uint32_t fake_char_to_byte(const char *text, uint32_t character)
{
    uint32_t byte = 0U;
    while (text[byte] != '\0' && character-- != 0U) byte += fake_utf8_width(text, byte);
    return byte;
}
static inline uint32_t fake_character_count(const char *text)
{
    uint32_t count = 0U;
    uint32_t byte = 0U;
    while (text[byte] != '\0') { byte += fake_utf8_width(text, byte); count++; }
    return count;
}
static inline void fake_send_event(lv_obj_t *obj, lv_event_code_t code, void *param)
{
    lv_event_t event = {obj, code, param, NULL, false};
    uint32_t index;
    for (index = 0U; index < obj->event_count && !event.stopped; index++) {
        lv_event_dsc_t *descriptor = obj->events[index];
        if (descriptor->filter != code) continue;
        event.user_data = descriptor->user_data;
        descriptor->callback(&event);
    }
}
static inline void fake_raw_insert(lv_obj_t *obj, const char *text)
{
    uint32_t byte = fake_char_to_byte(obj->text, obj->cursor);
    size_t added = strlen(text);
    size_t tail = strlen(obj->text + byte);
    memmove(obj->text + byte + added, obj->text + byte, tail + 1U);
    memcpy(obj->text + byte, text, added);
    obj->cursor += fake_character_count(text);
}
static inline void lv_textarea_add_text(lv_obj_t *obj, const char *text)
{
    const char *actual;
    obj->insert_replace = NULL;
    fake_send_event(obj, LV_EVENT_INSERT, (void *)text);
    actual = obj->insert_replace != NULL ? obj->insert_replace : text;
    if (actual[0] != '\0') fake_raw_insert(obj, actual);
}
static inline void lv_textarea_delete_char(lv_obj_t *obj)
{
    uint32_t start;
    uint32_t end;
    if (obj->cursor == 0U) return;
    start = fake_char_to_byte(obj->text, obj->cursor - 1U);
    end = fake_char_to_byte(obj->text, obj->cursor);
    memmove(obj->text + start, obj->text + end, strlen(obj->text + end) + 1U);
    obj->cursor--;
}
static inline lv_event_dsc_t *lv_obj_add_event_cb(lv_obj_t *obj, lv_event_cb_t callback,
                                                   lv_event_code_t filter, void *user_data)
{
    lv_event_dsc_t *descriptor;
    g_fake_lvgl.add_calls++;
    if (g_fake_lvgl.fail_add_call == g_fake_lvgl.add_calls) return NULL;
    descriptor = &g_fake_lvgl.descriptors[g_fake_lvgl.descriptor_count++];
    descriptor->callback = callback; descriptor->filter = filter; descriptor->user_data = user_data;
    obj->events[obj->event_count++] = descriptor;
    return descriptor;
}
static inline bool lv_obj_remove_event_dsc(lv_obj_t *obj, lv_event_dsc_t *descriptor)
{
    uint32_t index;
    for (index = 0U; index < obj->event_count; index++) if (obj->events[index] == descriptor) {
        memmove(&obj->events[index], &obj->events[index + 1U],
                (obj->event_count - index - 1U) * sizeof(obj->events[0]));
        obj->event_count--; return true;
    }
    return false;
}
static inline bool lv_obj_remove_event(lv_obj_t *obj, uint32_t index)
{
    if (g_fake_lvgl.fail_remove_event || index >= obj->event_count) return false;
    return lv_obj_remove_event_dsc(obj, obj->events[index]);
}
static inline uint32_t lv_obj_remove_event_cb(lv_obj_t *obj, lv_event_cb_t callback)
{
    uint32_t removed = 0U, index = 0U;
    while (index < obj->event_count) {
        if (obj->events[index]->callback == callback) {
            (void)lv_obj_remove_event_dsc(obj, obj->events[index]); removed++;
        }
        else index++;
    }
    return removed;
}
static inline uint32_t lv_obj_get_event_count(lv_obj_t *obj) { return obj->event_count; }
static inline lv_event_dsc_t *lv_obj_get_event_dsc(lv_obj_t *obj, uint32_t index) { return obj->events[index]; }
static inline lv_event_cb_t lv_event_dsc_get_cb(lv_event_dsc_t *descriptor) { return descriptor->callback; }
static inline void *lv_malloc(size_t size) { g_fake_lvgl.malloc_calls++; return g_fake_lvgl.fail_malloc ? NULL : malloc(size); }
static inline void lv_free(void *memory) { free(memory); }
static inline lv_obj_t *lv_layer_top(void) { return &g_fake_lvgl.top_layer; }
static inline lv_obj_t *lv_label_create(lv_obj_t *parent) { LV_UNUSED(parent); return &g_fake_lvgl.message; }
static inline void lv_obj_delete(lv_obj_t *obj) { obj->deleted = true; }
static inline void lv_label_set_text(lv_obj_t *obj, const char *text) { strncpy(obj->text, text, sizeof(obj->text) - 1U); }
static inline void lv_obj_set_style_text_font(lv_obj_t *obj, const lv_font_t *font, uint32_t selector) { LV_UNUSED(selector); obj->font = font; }
static inline const lv_font_t *lv_obj_get_style_text_font(lv_obj_t *obj, uint32_t part) { LV_UNUSED(part); return obj->font != NULL ? obj->font : &g_fake_default_font; }
static inline int32_t lv_obj_get_style_text_line_space(lv_obj_t *obj, uint32_t part) { LV_UNUSED(obj); LV_UNUSED(part); return 0; }
static inline void lv_obj_set_style_bg_opa(lv_obj_t *obj, uint32_t value, uint32_t selector) { LV_UNUSED(obj); LV_UNUSED(value); LV_UNUSED(selector); }
static inline void lv_obj_set_style_pad_all(lv_obj_t *obj, int32_t value, uint32_t selector) { LV_UNUSED(obj); LV_UNUSED(value); LV_UNUSED(selector); }
static inline void lv_obj_align(lv_obj_t *obj, int align, int32_t x, int32_t y) { LV_UNUSED(obj); LV_UNUSED(align); LV_UNUSED(x); LV_UNUSED(y); }
static inline lv_timer_t *lv_timer_create(void (*callback)(lv_timer_t *), uint32_t period, void *user_data) { LV_UNUSED(period); LV_UNUSED(user_data); g_fake_lvgl.timer.callback = callback; return &g_fake_lvgl.timer; }
static inline void lv_timer_set_repeat_count(lv_timer_t *timer, int count) { timer->repeats = count; }
static inline void lv_timer_delete(lv_timer_t *timer) { LV_UNUSED(timer); }
static inline lv_event_code_t lv_event_get_code(lv_event_t *event) { return event->code; }
static inline lv_obj_t *lv_event_get_current_target(lv_event_t *event) { return event->current_target; }
static inline void *lv_event_get_param(lv_event_t *event) { return event->param; }
static inline void lv_event_stop_processing(lv_event_t *event) { event->stopped = true; }
static inline uint32_t lv_buttonmatrix_get_selected_button(lv_obj_t *obj) { return obj->selected_button; }
static inline const char *fake_map_button(const char *const *map, uint32_t button)
{
    uint32_t index = 0U, current = 0U;
    if (map == NULL) return NULL;
    while (map[index][0] != '\0') {
        if (strcmp(map[index], "\n") != 0) {
            if (current == button) return map[index];
            current++;
        }
        index++;
    }
    return NULL;
}
static inline const char *lv_buttonmatrix_get_button_text(lv_obj_t *obj, uint32_t button)
{
    return fake_map_button(obj->maps[obj->mode], button);
}
static inline void lv_keyboard_set_map(lv_obj_t *obj, lv_keyboard_mode_t mode,
                                       const char *const *map,
                                       const lv_buttonmatrix_ctrl_t *ctrl)
{
    obj->maps[mode] = map; obj->ctrl_maps[mode] = ctrl;
}
static inline void lv_keyboard_set_mode(lv_obj_t *obj, lv_keyboard_mode_t mode) { obj->mode = mode; }
static inline lv_keyboard_mode_t lv_keyboard_get_mode(lv_obj_t *obj) { return obj->mode; }
static inline lv_obj_t *lv_keyboard_get_textarea(lv_obj_t *obj) { return obj->textarea; }
static inline void lv_keyboard_set_textarea(lv_obj_t *obj, lv_obj_t *textarea) { obj->textarea = textarea; }
static inline bool lv_obj_has_state(lv_obj_t *obj, uint32_t state) { return state == LV_STATE_DISABLED && obj->disabled; }
static inline const char *lv_textarea_get_text(lv_obj_t *obj) { return obj->text; }
static inline uint32_t lv_textarea_get_cursor_pos(lv_obj_t *obj) { return obj->cursor; }
static inline void lv_textarea_set_cursor_pos(lv_obj_t *obj, int32_t cursor)
{
    uint32_t length = fake_character_count(obj->text);
    obj->cursor = cursor < 0 || (uint32_t)cursor > length ? length : (uint32_t)cursor;
}
static inline uint32_t lv_textarea_get_max_length(lv_obj_t *obj) { return obj->max_length; }
static inline bool lv_textarea_get_one_line(lv_obj_t *obj) { return obj->one_line; }
static inline const char *lv_textarea_get_accepted_chars(lv_obj_t *obj) { return obj->accepted; }
static inline void lv_textarea_set_insert_replace(lv_obj_t *obj, const char *text) { obj->insert_replace = text; }
static inline lv_obj_t *lv_textarea_get_label(lv_obj_t *obj) { return obj->label; }
static inline void lv_label_set_text_selection_start(lv_obj_t *obj, uint32_t value) { obj->selection_start = value; }
static inline void lv_label_set_text_selection_end(lv_obj_t *obj, uint32_t value) { obj->selection_end = value; }
static inline void lv_label_get_letter_pos(lv_obj_t *obj, uint32_t character, lv_point_t *point)
{
    LV_UNUSED(obj); point->x = (int32_t)character; point->y = 0;
}
static inline uint32_t lv_label_get_letter_on(lv_obj_t *obj, lv_point_t *point, bool bidi)
{
    LV_UNUSED(bidi);
    return point->x < 0 ? 0U : (uint32_t)point->x > fake_character_count(obj->text) ?
           fake_character_count(obj->text) : (uint32_t)point->x;
}

#endif
