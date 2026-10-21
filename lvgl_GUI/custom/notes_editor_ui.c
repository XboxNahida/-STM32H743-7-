#include "notes_editor_ui.h"

#include <stddef.h>
#include <string.h>

#include "notes_editor_selection_target.h"
#include "notes_storage_service.h"
#include "selection_keyboard.h"

typedef struct {
    lv_obj_t *textarea;
    lv_event_dsc_t *delete_event;
    lv_event_dsc_t *changed_event;
    notes_editor_selection_target_t target;
    uint32_t window_focus;
    bool programmatic_update;
    bool target_registered;
} notes_editor_state_t;

static notes_editor_state_t g_editor;

static uint32_t bounded_text_length(const char *text)
{
    uint32_t length;
    if (text == NULL) return NOTES_WINDOW_MAX_BYTES + 1U;
    for (length = 0U; length <= NOTES_WINDOW_MAX_BYTES; length++)
        if (text[length] == '\0') return length;
    return NOTES_WINDOW_MAX_BYTES + 1U;
}

static void set_text_guarded(const char *text)
{
    uint32_t max_length;
    if (g_editor.textarea == NULL) return;
    max_length = lv_textarea_get_max_length(g_editor.textarea);
    g_editor.programmatic_update = true;
    lv_textarea_set_max_length(g_editor.textarea, 0U);
    lv_textarea_set_text(g_editor.textarea, text);
    lv_textarea_set_max_length(g_editor.textarea, max_length);
    g_editor.programmatic_update = false;
}

static bool focus_is_valid(notes_document_t *document, uint32_t focus)
{
    const uint8_t *data;
    uint32_t length;
    if (document == NULL) return false;
    data = notes_document_get_data(document);
    length = notes_document_get_length(document);
    return focus <= length && (focus == length ||
           ((data[focus] & 0xC0U) != 0x80U));
}

static void restore_window(notes_document_window_t old_window,
                           uint32_t old_cursor, uint32_t old_focus,
                           uint8_t old_active)
{
    notes_document_t *document = g_editor.target.document;
    notes_document_window_t restored;
    if (document != NULL && old_active != 0U &&
        notes_document_build_window(document, old_focus,
                                    &restored) == NOTES_DOCUMENT_OK) {
        g_editor.target.window = restored;
        set_text_guarded(restored.text);
        (void)notes_editor_selection_apply_cursor(&g_editor.target, old_cursor);
    }
    else {
        if (document != NULL) {
            document->window_start = old_window.start_byte;
            document->window_length = old_window.length_bytes;
            document->window_active = old_active;
        }
        g_editor.target.window = old_window;
        g_editor.target.cursor_byte = old_cursor;
    }
    g_editor.window_focus = old_focus;
}

static bool refresh_window(uint32_t focus_byte)
{
    notes_document_window_t old_window = g_editor.target.window;
    notes_document_window_t candidate;
    notes_document_t *document = g_editor.target.document;
    uint32_t old_cursor = g_editor.target.cursor_byte;
    uint32_t old_focus = g_editor.window_focus;
    uint8_t old_active;
    const char *shown;
    uint32_t shown_length;
    if (document == NULL || g_editor.textarea == NULL ||
        !focus_is_valid(document, focus_byte)) return false;
    old_active = document->window_active;
    if (notes_document_build_window(document, focus_byte,
                                    &candidate) != NOTES_DOCUMENT_OK)
        return false;
    g_editor.target.window = candidate;
    set_text_guarded(candidate.text);
    shown = lv_textarea_get_text(g_editor.textarea);
    shown_length = bounded_text_length(shown);
    if (shown_length != candidate.length_bytes ||
        memcmp(shown, candidate.text, shown_length) != 0 ||
        !notes_editor_selection_apply_cursor(&g_editor.target, focus_byte)) {
        restore_window(old_window, old_cursor, old_focus, old_active);
        return false;
    }
    g_editor.window_focus = focus_byte;
    return true;
}

static bool refresh_target(void *user_data, uint32_t focus_byte)
{
    (void)user_data;
    return refresh_window(focus_byte);
}

static int commit_window(void)
{
    const char *text;
    uint32_t length;
    uint32_t cursor;
    if (g_editor.target.document == NULL || g_editor.textarea == NULL) return 1;
    if (g_editor.programmatic_update) return 0;
    if (!notes_editor_refresh_guard_retry(&g_editor.target.refresh_guard))
        return 1;
    text = lv_textarea_get_text(g_editor.textarea);
    length = bounded_text_length(text);
    if (length <= NOTES_WINDOW_MAX_BYTES &&
        length == g_editor.target.window.length_bytes &&
        g_editor.target.window.text != NULL &&
        memcmp(text, g_editor.target.window.text, length) == 0) {
        g_editor.target.cursor_byte =
            notes_editor_selection_target_ops()->get_cursor(&g_editor.target);
        notes_editor_selection_recenter(&g_editor.target);
        return 0;
    }
    if (length > NOTES_WINDOW_MAX_BYTES ||
        notes_document_commit_window(g_editor.target.document, text, length,
            lv_textarea_get_cursor_pos(g_editor.textarea),
            &cursor) != NOTES_DOCUMENT_OK) {
        selection_keyboard_show_message("\xE6\x96\x87\xE6\x9C\xAC\xE5\xAE\xB9\xE9\x87\x8F\xE5\xB7\xB2\xE6\xBB\xA1");
        (void)refresh_window(g_editor.target.cursor_byte);
        return 1;
    }
    g_editor.target.cursor_byte = cursor;
    g_editor.target.window.length_bytes = length;
    notes_editor_selection_recenter(&g_editor.target);
    return 0;
}

static int commit_target(void *user_data)
{
    (void)user_data;
    return commit_window();
}

static void textarea_changed(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_VALUE_CHANGED &&
        !g_editor.programmatic_update) (void)commit_window();
}

static void textarea_deleted(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_DELETE) return;
    notes_editor_refresh_guard_abandon(&g_editor.target.refresh_guard);
    if (notes_storage_get_document() != NULL) notes_storage_close(1U);
    memset(&g_editor, 0, sizeof(g_editor));
}

static void remove_delete_event(void)
{
    if (g_editor.textarea != NULL && g_editor.delete_event != NULL)
        (void)lv_obj_remove_event_dsc(g_editor.textarea,
                                      g_editor.delete_event);
    g_editor.delete_event = NULL;
}

static void unpublish_target(void)
{
    if (g_editor.textarea != NULL && g_editor.changed_event != NULL)
        (void)lv_obj_remove_event_dsc(g_editor.textarea,
                                      g_editor.changed_event);
    g_editor.changed_event = NULL;
    if (g_editor.textarea != NULL && g_editor.target_registered)
        selection_keyboard_unregister_target(g_editor.textarea);
    g_editor.target_registered = false;
}

static bool publish_target(void)
{
    if (g_editor.textarea == NULL || g_editor.target.document == NULL ||
        g_editor.target.window.text == NULL) return false;
    if (!selection_keyboard_register_target(g_editor.textarea, &g_editor.target,
            notes_editor_selection_target_ops())) return false;
    g_editor.target_registered = true;
    g_editor.changed_event = lv_obj_add_event_cb(g_editor.textarea,
        textarea_changed, LV_EVENT_VALUE_CHANGED, NULL);
    if (g_editor.changed_event != NULL) return true;
    selection_keyboard_unregister_target(g_editor.textarea);
    g_editor.target_registered = false;
    return false;
}

static void clear_document(uint8_t close_storage)
{
    unpublish_target();
    set_text_guarded("");
    if (close_storage != 0U && notes_storage_get_document() != NULL)
        notes_storage_close(1U);
    notes_editor_selection_target_set_document(&g_editor.target, NULL);
    g_editor.window_focus = 0U;
}

bool notes_editor_ui_attach(lv_obj_t *textarea)
{
    notes_document_t *document;
    if (textarea == NULL || g_editor.textarea != NULL) return false;
    g_editor.textarea = textarea;
    g_editor.delete_event = lv_obj_add_event_cb(textarea, textarea_deleted,
                                                LV_EVENT_DELETE, NULL);
    if (g_editor.delete_event == NULL) {
        memset(&g_editor, 0, sizeof(g_editor));
        return false;
    }
    lv_textarea_set_accepted_chars(textarea, NULL);
    notes_editor_selection_target_init(&g_editor.target, textarea, commit_target,
                                       refresh_target, &g_editor);
    document = notes_storage_get_document();
    notes_editor_selection_target_set_document(&g_editor.target, document);
    if (document == NULL) {
        set_text_guarded("");
        return true;
    }
    if (refresh_window(0U) && publish_target()) return true;
    clear_document(1U);
    remove_delete_event();
    memset(&g_editor, 0, sizeof(g_editor));
    return false;
}

static int activate_opened_document(void)
{
    notes_editor_selection_target_set_document(&g_editor.target,
                                               notes_storage_get_document());
    if (g_editor.target.document == NULL) return NOTES_STORAGE_NOT_OPEN;
    if (g_editor.textarea == NULL) return NOTES_STORAGE_OK;
    if (refresh_window(0U) && publish_target()) return NOTES_STORAGE_OK;
    clear_document(1U);
    return NOTES_STORAGE_CAPACITY;
}

int notes_editor_ui_open(const char *path)
{
    int result;
    if (g_editor.target.document != NULL ||
        notes_storage_get_document() != NULL) return NOTES_STORAGE_BUSY;
    result = notes_storage_open(path);
    if (result != NOTES_STORAGE_OK) {
        unpublish_target();
        set_text_guarded("");
        return result;
    }
    return activate_opened_document();
}

int notes_editor_ui_new(void)
{
    int result;
    if (g_editor.target.document != NULL ||
        notes_storage_get_document() != NULL) return NOTES_STORAGE_BUSY;
    result = notes_storage_new();
    if (result != NOTES_STORAGE_OK) return result;
    return activate_opened_document();
}

int notes_editor_ui_save(const char *path)
{
    if (g_editor.target.document == NULL) return NOTES_STORAGE_NOT_OPEN;
    if (commit_window() != 0) return NOTES_STORAGE_CAPACITY;
    return notes_storage_save(path);
}

void notes_editor_ui_discard(void)
{
    clear_document(1U);
}

int notes_editor_ui_detach(void)
{
    if (g_editor.target.document != NULL && commit_window() != 0)
        return NOTES_STORAGE_CAPACITY;
    notes_editor_refresh_guard_reset(&g_editor.target.refresh_guard);
    unpublish_target();
    if (g_editor.target.document != NULL) notes_storage_close(0U);
    set_text_guarded("");
    remove_delete_event();
    memset(&g_editor, 0, sizeof(g_editor));
    return NOTES_STORAGE_OK;
}
