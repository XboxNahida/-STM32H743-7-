#ifndef TEST_NOTES_EDITOR_FIXTURE_H
#define TEST_NOTES_EDITOR_FIXTURE_H

#include "notes_editor_selection_target.h"
#include "notes_editor_ui.h"
#include "notes_storage_service.h"
#include "selection_keyboard.h"

extern uint32_t g_notes_fake_ignore_set_count;
extern uint32_t g_notes_fake_value_events;
extern lv_async_cb_t g_notes_fake_async_cb;
extern void *g_notes_fake_async_data;
extern notes_document_t g_notes_test_document;
extern lv_obj_t *g_notes_test_registered;
extern uint32_t g_notes_test_close_calls;
extern uint32_t g_notes_test_selection_delete_events;
extern uint8_t g_notes_test_selection_saw_storage_closed;

void notes_test_init_area(lv_obj_t *textarea, lv_obj_t *label, bool disabled);
bool notes_test_prepare_fixed(lv_obj_t *textarea, lv_obj_t *label,
    text_selection_controller_t *controller, uint8_t *clipboard,
    bool disabled);
bool notes_test_replace_failed(lv_obj_t *textarea, lv_obj_t *label,
                               bool disabled, uint32_t ignored_sets);

#endif
