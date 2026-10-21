#include "test_notes_editor_fixture.h"

#include <string.h>

uint32_t g_notes_fake_ignore_set_count;
uint32_t g_notes_fake_value_events;
lv_async_cb_t g_notes_fake_async_cb;
void *g_notes_fake_async_data;
notes_document_t g_notes_test_document;
lv_obj_t *g_notes_test_registered;
uint32_t g_notes_test_close_calls;
uint32_t g_notes_test_selection_delete_events;
uint8_t g_notes_test_selection_saw_storage_closed;
static uint8_t g_storage[NOTES_DOCUMENT_MAX_BYTES + 1U];
static uint8_t g_work[NOTES_DOCUMENT_WORK_BYTES];
static void *g_target_context;
static const text_selection_target_ops_t *g_target_ops;
static lv_event_dsc_t *g_selection_delete_event;
static uint8_t g_opened;

int notes_storage_open(const char *path)
{
    static const uint8_t text[] = "abc";
    (void)path;
    if (notes_document_init(&g_notes_test_document, g_storage,
            sizeof(g_storage), g_work, sizeof(g_work)) != NOTES_DOCUMENT_OK ||
        notes_document_load_utf8(&g_notes_test_document, text, 3U) !=
            NOTES_DOCUMENT_OK) return NOTES_STORAGE_CAPACITY;
    g_opened = 1U;
    return NOTES_STORAGE_OK;
}

int notes_storage_new(void)
{
    static const uint8_t empty[] = "";
    if (notes_document_init(&g_notes_test_document, g_storage,
            sizeof(g_storage), g_work, sizeof(g_work)) != NOTES_DOCUMENT_OK ||
        notes_document_load_utf8(&g_notes_test_document, empty, 0U) !=
            NOTES_DOCUMENT_OK) return NOTES_STORAGE_CAPACITY;
    g_opened = 1U;
    return NOTES_STORAGE_OK;
}

int notes_storage_save(const char *path)
{
    (void)path;
    return g_opened != 0U ? NOTES_STORAGE_OK : NOTES_STORAGE_NOT_OPEN;
}

void notes_storage_close(uint8_t discard_changes)
{
    (void)discard_changes;
    g_notes_test_close_calls++;
    g_opened = 0U;
}

notes_document_t *notes_storage_get_document(void)
{
    return g_opened != 0U ? &g_notes_test_document : NULL;
}

void notes_storage_get_status(notes_storage_status_t *status)
{
    if (status != NULL) memset(status, 0, sizeof(*status));
}

static void selection_delete(lv_event_t *event)
{
    (void)event;
    g_notes_test_selection_delete_events++;
    g_notes_test_selection_saw_storage_closed =
        notes_storage_get_document() == NULL ? 1U : 0U;
    g_notes_test_registered = NULL;
}

bool selection_keyboard_register_target(lv_obj_t *textarea, void *context,
                                        const text_selection_target_ops_t *ops)
{
    g_notes_test_registered = textarea;
    g_target_context = context;
    g_target_ops = ops;
    g_selection_delete_event = lv_obj_add_event_cb(textarea, selection_delete,
                                                   LV_EVENT_DELETE, NULL);
    return g_selection_delete_event != NULL;
}

void selection_keyboard_unregister_target(lv_obj_t *textarea)
{
    if (g_selection_delete_event != NULL)
        (void)lv_obj_remove_event_dsc(textarea, g_selection_delete_event);
    g_selection_delete_event = NULL;
    if (g_notes_test_registered == textarea) g_notes_test_registered = NULL;
}

void selection_keyboard_show_message(const char *message) { (void)message; }
bool selection_keyboard_install(lv_obj_t *keyboard) { (void)keyboard; return true; }
void selection_keyboard_focus(lv_obj_t *textarea) { (void)textarea; }
void selection_keyboard_blur(lv_obj_t *textarea) { (void)textarea; }

void notes_test_init_area(lv_obj_t *textarea, lv_obj_t *label, bool disabled)
{
    memset(textarea, 0, sizeof(*textarea));
    memset(label, 0, sizeof(*label));
    textarea->label = label;
    textarea->font.line_height = 16U;
    label->font.line_height = 16U;
    textarea->disabled = disabled;
}

bool notes_test_prepare_fixed(lv_obj_t *textarea, lv_obj_t *label,
    text_selection_controller_t *controller, uint8_t *clipboard,
    bool disabled)
{
    notes_test_init_area(textarea, label, disabled);
    if (!notes_editor_ui_attach(textarea) ||
        notes_editor_ui_open("note") != NOTES_STORAGE_OK ||
        text_selection_init(controller, clipboard,
            TEXT_SELECTION_CLIPBOARD_BYTES + 1U) != TEXT_SELECTION_OK ||
        text_selection_set_target(controller, g_target_context,
                                  g_target_ops) != TEXT_SELECTION_OK ||
        g_target_ops->set_cursor(g_target_context, 1U) != 0 ||
        text_selection_action(controller, TEXT_SELECT_OK) != TEXT_SELECTION_OK ||
        text_selection_action(controller, TEXT_SELECT_RIGHT) != TEXT_SELECTION_OK ||
        text_selection_action(controller, TEXT_SELECT_OK) != TEXT_SELECTION_OK)
        return false;
    return true;
}

bool notes_test_replace_failed(lv_obj_t *textarea, lv_obj_t *label,
                               bool disabled, uint32_t ignored_sets)
{
    static uint8_t clipboard[TEXT_SELECTION_CLIPBOARD_BYTES + 1U];
    text_selection_controller_t controller;
    uint8_t handled = 0U;
    if (!notes_test_prepare_fixed(textarea, label, &controller,
                                  clipboard, disabled)) return false;
    g_notes_fake_ignore_set_count = ignored_sets;
    return text_selection_replace_active(&controller, (const uint8_t *)"Z",
               1U, &handled) == TEXT_SELECTION_OK && handled != 0U &&
           strcmp((const char *)notes_document_get_data(
                  &g_notes_test_document), "aZc") == 0;
}
