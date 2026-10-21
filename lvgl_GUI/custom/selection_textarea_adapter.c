#include "selection_textarea_adapter.h"

#include <string.h>

static lv_obj_t *g_active_textarea;
static text_selection_controller_t *g_active_controller;
static selection_textarea_delete_cb_t g_delete_cb;
static selection_textarea_result_cb_t g_result_cb;
static void *g_callback_user_data;
static bool g_internal_insert;

static uint32_t utf8_width(const char *text, uint32_t byte, uint32_t length)
{
    uint8_t lead = (uint8_t)text[byte];
    uint32_t width = lead < 0x80U ? 1U : (lead & 0xE0U) == 0xC0U ? 2U :
                     (lead & 0xF0U) == 0xE0U ? 3U : (lead & 0xF8U) == 0xF0U ? 4U : 0U;
    uint32_t index;
    if (width == 0U || byte + width > length) return 0U;
    for (index = 1U; index < width; index++)
        if (((uint8_t)text[byte + index] & 0xC0U) != 0x80U) return 0U;
    return width;
}

static bool utf8_char_to_byte(const char *text, uint32_t character, uint32_t *byte)
{
    uint32_t length = (uint32_t)strlen(text), offset = 0U, index = 0U, width;
    while (offset < length && index < character) {
        width = utf8_width(text, offset, length);
        if (width == 0U) return false;
        offset += width;
        index++;
    }
    if (index != character) return false;
    *byte = offset;
    return true;
}

static bool utf8_byte_to_char(const char *text, uint32_t byte, uint32_t *character)
{
    uint32_t length = (uint32_t)strlen(text), offset = 0U, index = 0U, width;
    if (byte > length) return false;
    while (offset < byte) {
        width = utf8_width(text, offset, length);
        if (width == 0U || offset + width > byte) return false;
        offset += width;
        index++;
    }
    *character = index;
    return true;
}

static bool utf8_character_allowed(const char *accepted, const uint8_t *character,
                                   uint32_t character_bytes)
{
    uint32_t length = (uint32_t)strlen(accepted);
    uint32_t offset = 0U;
    uint32_t width;
    while (offset < length) {
        width = utf8_width(accepted, offset, length);
        if (width == 0U) return false;
        if (width == character_bytes &&
            memcmp(accepted + offset, character, width) == 0) return true;
        offset += width;
    }
    return false;
}

static bool replacement_is_valid(lv_obj_t *textarea, const uint8_t *replacement,
                                 uint32_t replacement_bytes, uint32_t *characters)
{
    const char *accepted = lv_textarea_get_accepted_chars(textarea);
    uint32_t offset = 0U;
    uint32_t width;
    *characters = 0U;
    if (replacement_bytes == 0U) return replacement == NULL || replacement[0] == 0U;
    if (replacement == NULL || strlen((const char *)replacement) != replacement_bytes)
        return false;
    while (offset < replacement_bytes) {
        width = utf8_width((const char *)replacement, offset, replacement_bytes);
        if (width == 0U) return false;
        if (lv_textarea_get_one_line(textarea) && width == 1U &&
            (replacement[offset] == '\n' || replacement[offset] == '\r')) return false;
        if (accepted != NULL && accepted[0] != '\0' &&
            !utf8_character_allowed(accepted, replacement + offset, width)) return false;
        offset += width;
        (*characters)++;
    }
    return true;
}

static uint32_t target_length(void *context)
{
    return (uint32_t)strlen(lv_textarea_get_text((lv_obj_t *)context));
}

static uint32_t target_cursor(void *context)
{
    lv_obj_t *textarea = context;
    uint32_t byte = 0U;
    (void)utf8_char_to_byte(lv_textarea_get_text(textarea),
                            lv_textarea_get_cursor_pos(textarea), &byte);
    return byte;
}

static int target_set_cursor(void *context, uint32_t byte)
{
    lv_obj_t *textarea = context;
    const char *text = lv_textarea_get_text(textarea);
    uint32_t character;
    uint32_t current_byte = target_cursor(textarea);
    uint32_t current = lv_textarea_get_cursor_pos(textarea);
    if (!utf8_byte_to_char(text, byte, &character)) {
        if (byte > current_byte) character = current + 1U;
        else if (byte < current_byte && current > 0U) character = current - 1U;
        else return 1;
    }
    lv_textarea_set_cursor_pos(textarea, (int32_t)character);
    return 0;
}

static int target_move_vertical(void *context, text_selection_vertical_t direction,
                                uint32_t *cursor)
{
    lv_obj_t *textarea = context;
    lv_obj_t *label = lv_textarea_get_label(textarea);
    const lv_font_t *font;
    lv_point_t point;
    if (label == NULL) return 1;
    lv_label_get_letter_pos(label, lv_textarea_get_cursor_pos(textarea), &point);
    font = lv_obj_get_style_text_font(label, LV_PART_MAIN);
    if (font == NULL) return 1;
    point.y += (direction == TEXT_SELECTION_VERTICAL_UP ? -1 : 1) *
               ((int32_t)font->line_height +
                lv_obj_get_style_text_line_space(label, LV_PART_MAIN));
    lv_textarea_set_cursor_pos(textarea,
                               (int32_t)lv_label_get_letter_on(label, &point, false));
    *cursor = target_cursor(textarea);
    return 0;
}

static void target_show(void *context, uint8_t visible, uint32_t start, uint32_t end)
{
    lv_obj_t *textarea = context;
    lv_obj_t *label = lv_textarea_get_label(textarea);
    uint32_t first;
    uint32_t last;
    if (label == NULL) return;
    if (visible == 0U) {
        lv_label_set_text_selection_start(label, LV_LABEL_TEXT_SELECTION_OFF);
        lv_label_set_text_selection_end(label, LV_LABEL_TEXT_SELECTION_OFF);
        return;
    }
    if (!utf8_byte_to_char(lv_textarea_get_text(textarea), start, &first) ||
        !utf8_byte_to_char(lv_textarea_get_text(textarea), end, &last)) return;
    lv_label_set_text_selection_start(label, first);
    lv_label_set_text_selection_end(label, last);
}

static int target_read(void *context, uint32_t start, uint32_t end,
                       uint8_t *output, uint32_t capacity)
{
    const char *text = lv_textarea_get_text((lv_obj_t *)context);
    uint32_t character;
    if (start > end || end - start >= capacity ||
        !utf8_byte_to_char(text, start, &character) ||
        !utf8_byte_to_char(text, end, &character)) return 1;
    memcpy(output, text + start, end - start);
    return 0;
}

static int target_replace(void *context, uint32_t start, uint32_t end,
                          const uint8_t *replacement, uint32_t replacement_bytes,
                          uint32_t *cursor)
{
    lv_obj_t *textarea = context;
    const char *text = lv_textarea_get_text(textarea);
    uint32_t first, last, added, max, old_chars, index;
    if (start > end || !utf8_byte_to_char(text, start, &first) ||
        !utf8_byte_to_char(text, end, &last)) return 1;
    if (!replacement_is_valid(textarea, replacement, replacement_bytes, &added) ||
        !utf8_byte_to_char(text, (uint32_t)strlen(text), &old_chars)) return 1;
    max = lv_textarea_get_max_length(textarea);
    if (max != 0U && old_chars - (last - first) + added > max) return 1;
    g_internal_insert = true;
    lv_textarea_set_cursor_pos(textarea, (int32_t)last);
    for (index = first; index < last; index++) lv_textarea_delete_char(textarea);
    lv_textarea_set_cursor_pos(textarea, (int32_t)first);
    if (replacement_bytes != 0U)
        lv_textarea_add_text(textarea, (const char *)replacement);
    g_internal_insert = false;
    *cursor = target_cursor(textarea);
    return 0;
}

static const text_selection_target_ops_t g_target_ops = {
    target_length, target_cursor, target_set_cursor, target_move_vertical,
    target_show, target_read, target_replace
};

static bool has_fixed_selection(void)
{
    text_selection_status_t status;
    text_selection_get_status(g_active_controller, &status);
    return status.phase == TEXT_SELECTION_PHASE_FIXED &&
           status.selection_start != status.selection_end;
}

static void textarea_event_cb(lv_event_t *event)
{
    lv_obj_t *textarea = lv_event_get_current_target(event);
    lv_event_code_t code = lv_event_get_code(event);
    text_selection_result_t result;
    const char *text;
    uint8_t handled;
    bool fixed;
    if (code == LV_EVENT_DELETE) {
        if (g_active_textarea == textarea) {
            if (g_active_controller != NULL) {
                g_active_controller->target = NULL;
                g_active_controller->ops = NULL;
            }
            g_active_textarea = NULL;
            g_active_controller = NULL;
        }
        if (g_delete_cb != NULL) g_delete_cb(textarea, g_callback_user_data);
        return;
    }
    if (code != LV_EVENT_INSERT || g_internal_insert ||
        g_active_textarea != textarea || g_active_controller == NULL) return;
    text = lv_event_get_param(event);
    if (text == NULL) return;
    fixed = has_fixed_selection();
    result = text_selection_replace_active(g_active_controller, (const uint8_t *)text,
                                           (uint32_t)strlen(text), &handled);
    if (fixed) lv_textarea_set_insert_replace(textarea, "");
    if (result != TEXT_SELECTION_OK && g_result_cb != NULL)
        g_result_cb(result, g_callback_user_data);
}

const text_selection_target_ops_t *selection_textarea_adapter_ops(void)
{
    return &g_target_ops;
}

void selection_textarea_adapter_set_callbacks(selection_textarea_delete_cb_t delete_cb,
                                              selection_textarea_result_cb_t result_cb,
                                              void *user_data)
{
    g_delete_cb = delete_cb;
    g_result_cb = result_cb;
    g_callback_user_data = user_data;
}

bool selection_textarea_adapter_attach(lv_obj_t *textarea)
{
    lv_event_dsc_t *insert_event;
    lv_event_dsc_t *delete_event;
    if (textarea == NULL) return false;
    insert_event = lv_obj_add_event_cb(textarea, textarea_event_cb,
                                       LV_EVENT_INSERT, NULL);
    if (insert_event == NULL) return false;
    delete_event = lv_obj_add_event_cb(textarea, textarea_event_cb,
                                       LV_EVENT_DELETE, NULL);
    if (delete_event == NULL) {
        (void)lv_obj_remove_event_dsc(textarea, insert_event);
        return false;
    }
    return true;
}

void selection_textarea_adapter_detach(lv_obj_t *textarea)
{
    if (textarea == NULL) return;
    selection_textarea_adapter_blur(textarea);
    (void)lv_obj_remove_event_cb(textarea, textarea_event_cb);
}

void selection_textarea_adapter_focus(lv_obj_t *textarea,
                                      text_selection_controller_t *controller)
{
    if (textarea == NULL || controller == NULL) return;
    g_active_textarea = textarea;
    g_active_controller = controller;
}

void selection_textarea_adapter_blur(lv_obj_t *textarea)
{
    if (textarea == NULL || g_active_textarea != textarea) return;
    if (g_active_controller != NULL) {
        (void)text_selection_action(g_active_controller, TEXT_SELECT_CANCEL);
        g_active_controller->target = NULL;
        g_active_controller->ops = NULL;
    }
    g_active_textarea = NULL;
    g_active_controller = NULL;
}
