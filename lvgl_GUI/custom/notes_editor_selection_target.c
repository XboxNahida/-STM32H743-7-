#include "notes_editor_selection_target.h"

#include <stddef.h>

#define NOTES_WINDOW_EDGE_BYTES 4096U

static uint32_t utf8_width(const char *text, uint32_t byte, uint32_t length)
{
    uint8_t lead = (uint8_t)text[byte];
    uint32_t width = lead < 0x80U ? 1U : (lead & 0xE0U) == 0xC0U ? 2U :
                     (lead & 0xF0U) == 0xE0U ? 3U :
                     (lead & 0xF8U) == 0xF0U ? 4U : 0U;
    uint32_t index;
    if (width == 0U || byte + width > length) return 0U;
    for (index = 1U; index < width; index++)
        if (((uint8_t)text[byte + index] & 0xC0U) != 0x80U) return 0U;
    return width;
}

static bool character_to_byte(const char *text, uint32_t length,
                              uint32_t character, uint32_t *byte)
{
    uint32_t offset = 0U;
    uint32_t index = 0U;
    while (offset < length && index < character) {
        uint32_t width = utf8_width(text, offset, length);
        if (width == 0U) return false;
        offset += width;
        index++;
    }
    if (index != character) return false;
    *byte = offset;
    return true;
}

static bool byte_to_character(const char *text, uint32_t length,
                              uint32_t byte, uint32_t *character)
{
    uint32_t offset = 0U;
    uint32_t count = 0U;
    if (byte > length) return false;
    while (offset < byte) {
        uint32_t width = utf8_width(text, offset, length);
        if (width == 0U || offset + width > byte) return false;
        offset += width;
        count++;
    }
    *character = count;
    return true;
}

void notes_editor_selection_target_init(notes_editor_selection_target_t *target,
    lv_obj_t *textarea, notes_editor_commit_cb_t commit,
    notes_editor_refresh_cb_t refresh, void *user_data)
{
    if (target == NULL) return;
    target->textarea = textarea;
    target->document = NULL;
    target->window.text = NULL;
    target->window.start_byte = 0U;
    target->window.length_bytes = 0U;
    target->cursor_byte = 0U;
    target->commit = commit;
    target->refresh = refresh;
    target->user_data = user_data;
    notes_editor_refresh_guard_init(&target->refresh_guard, textarea, refresh,
                                    user_data);
}

void notes_editor_selection_target_set_document(
    notes_editor_selection_target_t *target, notes_document_t *document)
{
    if (target == NULL) return;
    target->document = document;
    target->cursor_byte = 0U;
    target->window.text = NULL;
    target->window.start_byte = 0U;
    target->window.length_bytes = 0U;
    notes_editor_refresh_guard_reset(&target->refresh_guard);
}

bool notes_editor_selection_apply_cursor(notes_editor_selection_target_t *target,
                                         uint32_t full_byte)
{
    uint32_t local_character;
    if (target == NULL || target->textarea == NULL || target->window.text == NULL ||
        full_byte < target->window.start_byte ||
        full_byte > target->window.start_byte + target->window.length_bytes ||
        !byte_to_character(lv_textarea_get_text(target->textarea),
            target->window.length_bytes, full_byte - target->window.start_byte,
            &local_character)) return false;
    lv_textarea_set_cursor_pos(target->textarea, (int32_t)local_character);
    target->cursor_byte = full_byte;
    return true;
}

static bool target_ready(notes_editor_selection_target_t *target)
{
    if (target == NULL || target->document == NULL || target->textarea == NULL ||
        target->commit == NULL || target->refresh == NULL) return false;
    return notes_editor_refresh_guard_retry(&target->refresh_guard);
}

void notes_editor_selection_recenter(notes_editor_selection_target_t *target)
{
    uint32_t local;
    uint32_t length;
    if (!target_ready(target))
        return;
    if (target->cursor_byte < target->window.start_byte ||
        target->cursor_byte > target->window.start_byte +
                              target->window.length_bytes) return;
    local = target->cursor_byte - target->window.start_byte;
    length = notes_document_get_length(target->document);
    if ((target->window.start_byte != 0U && local < NOTES_WINDOW_EDGE_BYTES) ||
        (target->window.start_byte + target->window.length_bytes < length &&
         target->window.length_bytes - local < NOTES_WINDOW_EDGE_BYTES))
        (void)target->refresh(target->user_data, target->cursor_byte);
}

static uint32_t target_length(void *context)
{
    notes_editor_selection_target_t *target = context;
    if (target == NULL || target->document == NULL) return 0U;
    return notes_document_get_length(target->document);
}

static uint32_t target_cursor(void *context)
{
    notes_editor_selection_target_t *target = context;
    uint32_t local = 0U;
    if (!target_ready(target) ||
        !character_to_byte(lv_textarea_get_text(target->textarea),
        target->window.length_bytes, lv_textarea_get_cursor_pos(target->textarea),
        &local)) return target != NULL ? target->cursor_byte : 0U;
    target->cursor_byte = target->window.start_byte + local;
    return target->cursor_byte;
}

static uint32_t snap_cursor(notes_editor_selection_target_t *target,
                            uint32_t requested)
{
    const uint8_t *data = notes_document_get_data(target->document);
    uint32_t length = notes_document_get_length(target->document);
    if (requested >= length) return length;
    if ((data[requested] & 0xC0U) != 0x80U) return requested;
    if (requested > target->cursor_byte)
        while (requested < length && (data[requested] & 0xC0U) == 0x80U)
            requested++;
    else
        while (requested > 0U && (data[requested] & 0xC0U) == 0x80U)
            requested--;
    return requested;
}

static int target_set_cursor(void *context, uint32_t requested)
{
    notes_editor_selection_target_t *target = context;
    if (target == NULL || target->document == NULL ||
        requested > notes_document_get_length(target->document)) return 1;
    if (!target_ready(target) || target->commit(target->user_data) != 0) return 1;
    requested = snap_cursor(target, requested);
    if ((requested < target->window.start_byte ||
         requested > target->window.start_byte + target->window.length_bytes) &&
        !target->refresh(target->user_data, requested)) return 1;
    if (!notes_editor_selection_apply_cursor(target, requested)) return 1;
    notes_editor_selection_recenter(target);
    return 0;
}

static int target_move_vertical(void *context, text_selection_vertical_t direction,
                                uint32_t *cursor)
{
    notes_editor_selection_target_t *target = context;
    lv_obj_t *label;
    const lv_font_t *font;
    lv_point_t point;
    uint32_t local_byte;
    if (target == NULL || cursor == NULL) return 1;
    if (!target_ready(target) || target->commit(target->user_data) != 0)
        return 1;
    label = lv_textarea_get_label(target->textarea);
    if (label == NULL) return 1;
    lv_label_get_letter_pos(label, lv_textarea_get_cursor_pos(target->textarea),
                            &point);
    font = lv_obj_get_style_text_font(label, LV_PART_MAIN);
    if (font == NULL) return 1;
    point.y += (direction == TEXT_SELECTION_VERTICAL_UP ? -1 : 1) *
        ((int32_t)font->line_height +
         lv_obj_get_style_text_line_space(label, LV_PART_MAIN));
    lv_textarea_set_cursor_pos(target->textarea,
        (int32_t)lv_label_get_letter_on(label, &point, false));
    if (!character_to_byte(lv_textarea_get_text(target->textarea),
            target->window.length_bytes, lv_textarea_get_cursor_pos(target->textarea),
            &local_byte)) return 1;
    target->cursor_byte = target->window.start_byte + local_byte;
    *cursor = target->cursor_byte;
    notes_editor_selection_recenter(target);
    return 0;
}

static void hide_selection(lv_obj_t *label)
{
    lv_label_set_text_selection_start(label, LV_LABEL_TEXT_SELECTION_OFF);
    lv_label_set_text_selection_end(label, LV_LABEL_TEXT_SELECTION_OFF);
}

static void target_show_selection(void *context, uint8_t visible,
                                  uint32_t start, uint32_t end)
{
    notes_editor_selection_target_t *target = context;
    lv_obj_t *label;
    uint32_t window_start;
    uint32_t window_end;
    uint32_t first;
    uint32_t last;
    uint32_t temporary;
    if (target == NULL || target->textarea == NULL) return;
    label = lv_textarea_get_label(target->textarea);
    if (label == NULL) return;
    window_start = target->window.start_byte;
    window_end = window_start + target->window.length_bytes;
    if (start > end) { temporary = start; start = end; end = temporary; }
    if (visible == 0U || end <= window_start || start >= window_end) {
        hide_selection(label);
        return;
    }
    if (start < window_start) start = window_start;
    if (end > window_end) end = window_end;
    if (!byte_to_character(lv_textarea_get_text(target->textarea),
            target->window.length_bytes, start - window_start, &first) ||
        !byte_to_character(lv_textarea_get_text(target->textarea),
            target->window.length_bytes, end - window_start, &last)) return;
    lv_label_set_text_selection_start(label, first);
    lv_label_set_text_selection_end(label, last);
}

static int target_read_range(void *context, uint32_t start, uint32_t end,
                             uint8_t *output, uint32_t capacity)
{
    notes_editor_selection_target_t *target = context;
    uint32_t written;
    if (target == NULL) return 1;
    if (!target_ready(target) || target->commit(target->user_data) != 0) return 1;
    return notes_document_read_range(target->document, start, end, output,
                                     capacity, &written) == NOTES_DOCUMENT_OK ? 0 : 1;
}

static int target_replace_range(void *context, uint32_t start, uint32_t end,
                                const uint8_t *replacement, uint32_t bytes,
                                uint32_t *cursor)
{
    notes_editor_selection_target_t *target = context;
    if (target == NULL || cursor == NULL) return 1;
    if (!target_ready(target) || target->commit(target->user_data) != 0 ||
        notes_document_replace_range(target->document, start, end, replacement,
                                     bytes, cursor) != NOTES_DOCUMENT_OK)
        return 1;
    target->cursor_byte = *cursor;
    if (!target->refresh(target->user_data, *cursor))
        notes_editor_refresh_guard_begin(&target->refresh_guard, *cursor);
    return 0;
}

static const text_selection_target_ops_t g_target_ops = {
    target_length, target_cursor, target_set_cursor, target_move_vertical,
    target_show_selection, target_read_range, target_replace_range
};

const text_selection_target_ops_t *notes_editor_selection_target_ops(void)
{
    return &g_target_ops;
}
