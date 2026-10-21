#ifndef NOTES_EDITOR_SELECTION_TARGET_H
#define NOTES_EDITOR_SELECTION_TARGET_H

#include <stdbool.h>
#include <stdint.h>

#include "lvgl.h"
#include "notes_editor_refresh_guard.h"
#include "notes_document.h"
#include "text_selection_controller.h"

typedef int (*notes_editor_commit_cb_t)(void *user_data);

typedef struct {
    lv_obj_t *textarea;
    notes_document_t *document;
    notes_document_window_t window;
    uint32_t cursor_byte;
    notes_editor_commit_cb_t commit;
    notes_editor_refresh_cb_t refresh;
    void *user_data;
    notes_editor_refresh_guard_t refresh_guard;
} notes_editor_selection_target_t;

void notes_editor_selection_target_init(notes_editor_selection_target_t *target,
    lv_obj_t *textarea, notes_editor_commit_cb_t commit,
    notes_editor_refresh_cb_t refresh, void *user_data);
void notes_editor_selection_target_set_document(
    notes_editor_selection_target_t *target, notes_document_t *document);
bool notes_editor_selection_apply_cursor(notes_editor_selection_target_t *target,
                                         uint32_t full_byte);
void notes_editor_selection_recenter(notes_editor_selection_target_t *target);
const text_selection_target_ops_t *notes_editor_selection_target_ops(void);

#endif
