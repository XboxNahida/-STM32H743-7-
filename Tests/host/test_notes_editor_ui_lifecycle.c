#include <process.h>
#include <stdio.h>
#include <string.h>

#include "test_notes_editor_fixture.h"

static int g_failures;

#define CHECK(name, condition) do { if (!(condition)) { \
    printf("FAIL %s\n", name); g_failures++; } } while (0)

static void capture_async(lv_async_cb_t *callback, void **data)
{
    *callback = g_notes_fake_async_cb;
    *data = g_notes_fake_async_data;
}

static int scenario_discard_cancels_old_async(void)
{
    static lv_obj_t textarea, label;
    lv_async_cb_t old_callback;
    void *old_data;
    CHECK("discard pending", notes_test_replace_failed(&textarea, &label,
          false, 1U));
    capture_async(&old_callback, &old_data);
    notes_editor_ui_discard();
    CHECK("discard canceled", old_callback != NULL &&
          g_notes_fake_async_cb == NULL);
    if (old_callback != NULL) old_callback(old_data);
    CHECK("discard text remains clear", textarea.text[0] == '\0');
    CHECK("discard storage remains closed",
          notes_storage_get_document() == NULL);
    CHECK("discard restores editing",
          !lv_obj_has_state(&textarea, LV_STATE_DISABLED));
    CHECK("discard remains attached", notes_editor_ui_detach() ==
          NOTES_STORAGE_OK);
    return g_failures != 0;
}

static int scenario_detach_cancels_old_async(void)
{
    static lv_obj_t textarea, label;
    lv_async_cb_t old_callback;
    void *old_data;
    CHECK("detach pending", notes_test_replace_failed(&textarea, &label,
          false, 1U));
    capture_async(&old_callback, &old_data);
    CHECK("detach succeeds", notes_editor_ui_detach() == NOTES_STORAGE_OK);
    CHECK("detach canceled", old_callback != NULL &&
          g_notes_fake_async_cb == NULL && g_notes_test_close_calls == 1U);
    if (old_callback != NULL) old_callback(old_data);
    CHECK("detach remains clear", textarea.text[0] == '\0' &&
          notes_storage_get_document() == NULL &&
          g_notes_test_registered == NULL);
    return g_failures != 0;
}

static int scenario_delete_abandons_old_async(void)
{
    static lv_obj_t textarea, label, replacement, replacement_label;
    lv_async_cb_t old_callback;
    void *old_data;
    CHECK("delete pending", notes_test_replace_failed(&textarea, &label,
          false, 1U));
    capture_async(&old_callback, &old_data);
    fake_send_event(&textarea, LV_EVENT_DELETE);
    CHECK("selection delete runs", g_notes_test_selection_delete_events == 1U &&
          g_notes_test_registered == NULL);
    CHECK("bridge delete runs first",
          g_notes_test_selection_saw_storage_closed != 0U);
    CHECK("delete releases storage", g_notes_test_close_calls == 1U &&
          notes_storage_get_document() == NULL &&
          g_notes_fake_async_cb == NULL);
    (void)strcpy(textarea.text, "deleted");
    textarea.disabled = true;
    if (old_callback != NULL) old_callback(old_data);
    CHECK("old async no-op", strcmp(textarea.text, "deleted") == 0 &&
          lv_obj_has_state(&textarea, LV_STATE_DISABLED));
    notes_test_init_area(&replacement, &replacement_label, false);
    CHECK("bridge reference cleared", notes_editor_ui_attach(&replacement));
    CHECK("new bridge detaches", notes_editor_ui_detach() == NOTES_STORAGE_OK);
    return g_failures != 0;
}

static int scenario_save_retry_success(void)
{
    static lv_obj_t textarea, label;
    CHECK("save pending", notes_test_replace_failed(&textarea, &label,
          false, 1U));
    CHECK("save retry succeeds", notes_editor_ui_save("note") ==
          NOTES_STORAGE_OK);
    CHECK("save canceled async", g_notes_fake_async_cb == NULL &&
          !lv_obj_has_state(&textarea, LV_STATE_DISABLED));
    notes_editor_ui_discard();
    (void)notes_editor_ui_detach();
    return g_failures != 0;
}

static int scenario_save_retry_failure(void)
{
    static lv_obj_t textarea, label;
    lv_async_cb_t old_callback;
    void *old_data;
    CHECK("save pending", notes_test_replace_failed(&textarea, &label,
          false, 20U));
    capture_async(&old_callback, &old_data);
    CHECK("save retry fails", notes_editor_ui_save("note") ==
          NOTES_STORAGE_CAPACITY);
    CHECK("save remains pending", g_notes_fake_async_cb == old_callback &&
          g_notes_fake_async_data == old_data &&
          lv_obj_has_state(&textarea, LV_STATE_DISABLED) &&
          notes_storage_get_document() != NULL);
    notes_editor_ui_discard();
    (void)notes_editor_ui_detach();
    return g_failures != 0;
}

static int scenario_detach_retry_failure(void)
{
    static lv_obj_t textarea, label;
    lv_async_cb_t old_callback;
    void *old_data;
    CHECK("detach pending", notes_test_replace_failed(&textarea, &label,
          false, 20U));
    capture_async(&old_callback, &old_data);
    CHECK("detach retry fails", notes_editor_ui_detach() ==
          NOTES_STORAGE_CAPACITY);
    CHECK("detach keeps owner", g_notes_test_close_calls == 0U &&
          g_notes_test_registered == &textarea &&
          notes_storage_get_document() != NULL &&
          g_notes_fake_async_cb == old_callback &&
          g_notes_fake_async_data == old_data);
    notes_editor_ui_discard();
    (void)notes_editor_ui_detach();
    return g_failures != 0;
}

static int run_named(const char *name)
{
    if (strcmp(name, "discard") == 0)
        return scenario_discard_cancels_old_async();
    if (strcmp(name, "detach_ok") == 0)
        return scenario_detach_cancels_old_async();
    if (strcmp(name, "delete") == 0)
        return scenario_delete_abandons_old_async();
    if (strcmp(name, "save_ok") == 0) return scenario_save_retry_success();
    if (strcmp(name, "save_fail") == 0) return scenario_save_retry_failure();
    return scenario_detach_retry_failure();
}

int main(int argc, char **argv)
{
    static const char *const cases[] = {
        "discard", "detach_ok", "delete", "save_ok", "save_fail",
        "detach_fail"
    };
    uint32_t index;
    if (argc == 2) return run_named(argv[1]);
    for (index = 0U; index < sizeof(cases) / sizeof(cases[0]); index++)
        if (_spawnl(_P_WAIT, argv[0], argv[0], cases[index], NULL) != 0) {
            printf("FAIL child_%s\n", cases[index]);
            return 1;
        }
    printf("PASS notes_editor_ui_lifecycle\n");
    return 0;
}
