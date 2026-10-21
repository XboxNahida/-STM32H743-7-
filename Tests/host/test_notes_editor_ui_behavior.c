#include <process.h>
#include <stdio.h>
#include <string.h>

#include "notes_editor_selection_target.h"
#include "notes_editor_ui.h"
#include "notes_storage_service.h"
#include "selection_keyboard.h"

uint32_t g_notes_fake_ignore_set_count;
uint32_t g_notes_fake_value_events;
lv_async_cb_t g_notes_fake_async_cb;
void *g_notes_fake_async_data;
static uint8_t g_storage[NOTES_DOCUMENT_MAX_BYTES + 1U];
static uint8_t g_work[NOTES_DOCUMENT_WORK_BYTES];
static notes_document_t g_document;
static const char *g_open_text = "abc";
static uint8_t g_opened;
static uint32_t g_close_calls;
static uint32_t g_register_calls;
static uint32_t g_unregister_calls;
static lv_obj_t *g_registered;
static int g_failures;

#define CHECK(name, condition) do { if (!(condition)) { \
    printf("FAIL %s\n", name); g_failures++; } } while (0)

int notes_storage_open(const char *path)
{
    (void)path;
    if (g_opened != 0U) return NOTES_STORAGE_BUSY;
    if (notes_document_init(&g_document, g_storage, sizeof(g_storage),
            g_work, sizeof(g_work)) != NOTES_DOCUMENT_OK ||
        notes_document_load_utf8(&g_document, (const uint8_t *)g_open_text,
            (uint32_t)strlen(g_open_text)) != NOTES_DOCUMENT_OK)
        return NOTES_STORAGE_CAPACITY;
    g_opened = 1U;
    return NOTES_STORAGE_OK;
}

int notes_storage_new(void)
{
    g_open_text = "";
    return notes_storage_open("new");
}

int notes_storage_save(const char *path)
{
    (void)path;
    return g_opened != 0U ? NOTES_STORAGE_OK : NOTES_STORAGE_NOT_OPEN;
}

void notes_storage_close(uint8_t discard_changes)
{
    (void)discard_changes;
    g_close_calls++;
    g_opened = 0U;
}

notes_document_t *notes_storage_get_document(void)
{
    return g_opened != 0U ? &g_document : NULL;
}

void notes_storage_get_status(notes_storage_status_t *status)
{
    if (status != NULL) memset(status, 0, sizeof(*status));
}

bool selection_keyboard_register_target(lv_obj_t *textarea, void *context,
                                        const text_selection_target_ops_t *ops)
{
    (void)context;
    (void)ops;
    g_register_calls++;
    g_registered = textarea;
    return true;
}

void selection_keyboard_unregister_target(lv_obj_t *textarea)
{
    g_unregister_calls++;
    if (g_registered == textarea) g_registered = NULL;
}

void selection_keyboard_show_message(const char *message) { (void)message; }
bool selection_keyboard_install(lv_obj_t *keyboard) { (void)keyboard; return true; }
void selection_keyboard_focus(lv_obj_t *textarea) { (void)textarea; }
void selection_keyboard_blur(lv_obj_t *textarea) { (void)textarea; }

static void init_area(lv_obj_t *textarea, lv_obj_t *label, const char *text)
{
    memset(textarea, 0, sizeof(*textarea));
    memset(label, 0, sizeof(*label));
    textarea->label = label;
    textarea->font.line_height = 16U;
    label->font.line_height = 16U;
    lv_textarea_set_text(textarea, text);
}

static int scenario_attach_empty(void)
{
    static lv_obj_t textarea, label;
    init_area(&textarea, &label, "stale");
    CHECK("attach", notes_editor_ui_attach(&textarea));
    CHECK("not published", g_register_calls == 0U && g_registered == NULL);
    CHECK("display cleared", textarea.text[0] == '\0');
    CHECK("delete handler only", textarea.event_count == 1U &&
          textarea.events[0].filter == LV_EVENT_DELETE);
    notes_editor_ui_detach();
    return g_failures != 0;
}

static int scenario_new_empty(void)
{
    static lv_obj_t textarea, label;
    init_area(&textarea, &label, "stale");
    CHECK("attach", notes_editor_ui_attach(&textarea));
    CHECK("new", notes_editor_ui_new() == NOTES_STORAGE_OK);
    CHECK("new published", g_registered == &textarea);
    CHECK("new empty", textarea.text[0] == '\0' &&
          notes_document_get_length(&g_document) == 0U);
    notes_editor_ui_discard();
    (void)notes_editor_ui_detach();
    return g_failures != 0;
}

static int scenario_discard(void)
{
    static lv_obj_t textarea, label;
    init_area(&textarea, &label, "stale");
    CHECK("open", notes_editor_ui_open("note") == NOTES_STORAGE_OK);
    CHECK("attach", notes_editor_ui_attach(&textarea));
    CHECK("published", g_registered == &textarea && strcmp(textarea.text, "abc") == 0);
    notes_editor_ui_discard();
    CHECK("discard state", g_registered == NULL && textarea.text[0] == '\0');
    CHECK("discard close", g_close_calls == 1U && notes_storage_get_document() == NULL);
    notes_editor_ui_detach();
    return g_failures != 0;
}

static int scenario_open_refresh_fail(void)
{
    static lv_obj_t textarea, label;
    init_area(&textarea, &label, "stale");
    CHECK("attach", notes_editor_ui_attach(&textarea));
    g_notes_fake_ignore_set_count = 1U;
    CHECK("open rejected", notes_editor_ui_open("note") != NOTES_STORAGE_OK);
    CHECK("open rollback", g_registered == NULL && textarea.text[0] == '\0');
    CHECK("opened doc closed", g_close_calls == 1U && notes_storage_get_document() == NULL);
    CHECK("open retry", notes_editor_ui_open("note") == NOTES_STORAGE_OK &&
          g_registered == &textarea && strcmp(textarea.text, "abc") == 0);
    notes_editor_ui_discard();
    (void)notes_editor_ui_detach();
    return g_failures != 0;
}

static uint32_t g_commit_calls;
static uint32_t g_refresh_calls;
static bool g_refresh_result;

static int target_commit(void *user_data)
{
    (void)user_data;
    g_commit_calls++;
    return 0;
}

static bool target_refresh(void *user_data, uint32_t focus)
{
    (void)user_data;
    (void)focus;
    g_refresh_calls++;
    return g_refresh_result;
}

static void init_target(notes_editor_selection_target_t *target,
                        lv_obj_t *textarea, lv_obj_t *label)
{
    init_area(textarea, label, "abc");
    (void)notes_storage_open("note");
    notes_editor_selection_target_init(target, textarea, target_commit,
                                       target_refresh, NULL);
    notes_editor_selection_target_set_document(target, &g_document);
    (void)notes_document_build_window(&g_document, 0U, &target->window);
    target->cursor_byte = 1U;
    textarea->cursor = 1U;
}

static int scenario_cursor_bounds(void)
{
    static lv_obj_t textarea, label;
    notes_editor_selection_target_t target;
    const text_selection_target_ops_t *ops = notes_editor_selection_target_ops();
    uint8_t output[2];
    uint32_t cursor = 0U;
    CHECK("null move rejected", ops->move_vertical(NULL,
          TEXT_SELECTION_VERTICAL_UP, &cursor) != 0);
    ops->show_selection(NULL, 0U, 0U, 0U);
    CHECK("null read rejected", ops->read_range(NULL, 0U, 0U,
          output, sizeof(output)) != 0);
    CHECK("null replace rejected", ops->replace_range(NULL, 0U, 0U,
          NULL, 0U, &cursor) != 0);
    init_target(&target, &textarea, &label);
    CHECK("range rejected", ops->set_cursor(&target, 4U) != 0);
    CHECK("range no commit", g_commit_calls == 0U);
    textarea.cursor = 99U;
    target.cursor_byte = 2U;
    CHECK("invalid cursor retained", ops->get_cursor(&target) == 2U);
    notes_storage_close(1U);
    return g_failures != 0;
}

static int scenario_replace_refresh_fail(void)
{
    static lv_obj_t textarea, label;
    static uint8_t clipboard[TEXT_SELECTION_CLIPBOARD_BYTES + 1U];
    notes_editor_selection_target_t target;
    text_selection_controller_t controller;
    text_selection_status_t status;
    uint8_t handled = 0U;
    init_target(&target, &textarea, &label);
    g_refresh_result = false;
    CHECK("controller init", text_selection_init(&controller, clipboard,
          sizeof(clipboard)) == TEXT_SELECTION_OK);
    CHECK("target set", text_selection_set_target(&controller, &target,
          notes_editor_selection_target_ops()) == TEXT_SELECTION_OK);
    CHECK("anchor", text_selection_action(&controller, TEXT_SELECT_OK) == TEXT_SELECTION_OK);
    CHECK("right", text_selection_action(&controller, TEXT_SELECT_RIGHT) == TEXT_SELECTION_OK);
    CHECK("fixed", text_selection_action(&controller, TEXT_SELECT_OK) == TEXT_SELECTION_OK);
    CHECK("replace succeeds", text_selection_replace_active(&controller,
          (const uint8_t *)"Z", 1U, &handled) == TEXT_SELECTION_OK && handled != 0U);
    text_selection_get_status(&controller, &status);
    CHECK("replace committed once", strcmp((const char *)notes_document_get_data(
          &g_document), "aZc") == 0 && g_refresh_calls == 1U);
    CHECK("selection cleared", status.phase == TEXT_SELECTION_PHASE_IDLE);
    notes_storage_close(1U);
    return g_failures != 0;
}

static int scenario_detach_failure(void)
{
    static lv_obj_t textarea, label;
    init_area(&textarea, &label, "");
    CHECK("open", notes_editor_ui_open("note") == NOTES_STORAGE_OK);
    CHECK("attach", notes_editor_ui_attach(&textarea));
    memset(textarea.text, 'x', NOTES_WINDOW_MAX_BYTES + 1U);
    textarea.text[NOTES_WINDOW_MAX_BYTES + 1U] = '\0';
    CHECK("detach rejected", notes_editor_ui_detach() != NOTES_STORAGE_OK);
    CHECK("detach retained binding", g_registered == &textarea && g_unregister_calls == 0U);
    CHECK("detach retained session", g_close_calls == 0U && notes_storage_get_document() != NULL);
    notes_editor_ui_discard();
    notes_editor_ui_detach();
    return g_failures != 0;
}

static int run_named(const char *name)
{
    if (strcmp(name, "attach") == 0) return scenario_attach_empty();
    if (strcmp(name, "new") == 0) return scenario_new_empty();
    if (strcmp(name, "discard") == 0) return scenario_discard();
    if (strcmp(name, "open_fail") == 0) return scenario_open_refresh_fail();
    if (strcmp(name, "cursor") == 0) return scenario_cursor_bounds();
    if (strcmp(name, "replace") == 0) return scenario_replace_refresh_fail();
    return scenario_detach_failure();
}

int main(int argc, char **argv)
{
    static const char *const cases[] = {
        "attach", "new", "discard", "open_fail", "cursor", "replace", "detach"
    };
    uint32_t index;
    if (argc == 2) return run_named(argv[1]);
    for (index = 0U; index < sizeof(cases) / sizeof(cases[0]); index++)
        if (_spawnl(_P_WAIT, argv[0], argv[0], cases[index], NULL) != 0) {
            printf("FAIL child_%s\n", cases[index]);
            return 1;
        }
    printf("PASS notes_editor_ui_behavior\n");
    return 0;
}
