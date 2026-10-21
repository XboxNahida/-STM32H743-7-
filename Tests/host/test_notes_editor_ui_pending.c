#include <process.h>
#include <stdio.h>
#include <string.h>

#include "test_notes_editor_fixture.h"

static int g_failures;

#define CHECK(name, condition) do { if (!(condition)) { \
    printf("FAIL %s\n", name); g_failures++; } } while (0)

static int scenario_async_recovery(void)
{
    static lv_obj_t textarea, label;
    uint32_t events;
    CHECK("replace accepted", notes_test_replace_failed(&textarea, &label,
          false, 1U));
    CHECK("pending disabled", lv_obj_has_state(&textarea, LV_STATE_DISABLED));
    CHECK("async scheduled", g_notes_fake_async_cb != NULL);
    events = g_notes_fake_value_events;
    lv_textarea_add_text(&textarea, "Q");
    CHECK("input blocked", strcmp((const char *)notes_document_get_data(
          &g_notes_test_document), "aZc") == 0 &&
          g_notes_fake_value_events == events);
    fake_run_async();
    CHECK("authoritative display", strcmp(textarea.text, "aZc") == 0);
    CHECK("editing restored", !lv_obj_has_state(&textarea, LV_STATE_DISABLED));
    notes_editor_ui_discard();
    (void)notes_editor_ui_detach();
    return g_failures != 0;
}

static int scenario_originally_disabled(void)
{
    static lv_obj_t textarea, label;
    CHECK("replace accepted", notes_test_replace_failed(&textarea, &label,
          true, 1U));
    CHECK("async scheduled", g_notes_fake_async_cb != NULL);
    fake_run_async();
    CHECK("disabled retained", lv_obj_has_state(&textarea, LV_STATE_DISABLED));
    notes_editor_ui_discard();
    (void)notes_editor_ui_detach();
    return g_failures != 0;
}

static int scenario_permanent_failure(void)
{
    static lv_obj_t textarea, label;
    uint32_t events;
    CHECK("replace accepted", notes_test_replace_failed(&textarea, &label,
          false, 20U));
    events = g_notes_fake_value_events;
    lv_textarea_add_text(&textarea, "Q");
    fake_run_async();
    lv_textarea_add_text(&textarea, "R");
    CHECK("permanent disabled", lv_obj_has_state(&textarea,
          LV_STATE_DISABLED));
    CHECK("no input accepted", g_notes_fake_value_events == events &&
          strcmp((const char *)notes_document_get_data(
                 &g_notes_test_document), "aZc") == 0);
    notes_editor_ui_discard();
    (void)notes_editor_ui_detach();
    return g_failures != 0;
}

static int scenario_empty_length(void)
{
    notes_editor_selection_target_t target;
    memset(&target, 0, sizeof(target));
    CHECK("empty length", notes_editor_selection_target_ops()->get_length(
          &target) == 0U);
    CHECK("null length", notes_editor_selection_target_ops()->get_length(
          NULL) == 0U);
    return g_failures != 0;
}

static int run_named(const char *name)
{
    if (strcmp(name, "recovery") == 0) return scenario_async_recovery();
    if (strcmp(name, "disabled") == 0) return scenario_originally_disabled();
    if (strcmp(name, "permanent") == 0) return scenario_permanent_failure();
    return scenario_empty_length();
}

int main(int argc, char **argv)
{
    static const char *const cases[] = {
        "recovery", "disabled", "permanent", "length"
    };
    uint32_t index;
    if (argc == 2) return run_named(argv[1]);
    for (index = 0U; index < sizeof(cases) / sizeof(cases[0]); index++)
        if (_spawnl(_P_WAIT, argv[0], argv[0], cases[index], NULL) != 0) {
            printf("FAIL child_%s\n", cases[index]);
            return 1;
        }
    printf("PASS notes_editor_ui_pending\n");
    return 0;
}
