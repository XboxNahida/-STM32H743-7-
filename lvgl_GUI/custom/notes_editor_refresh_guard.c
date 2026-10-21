#include "notes_editor_refresh_guard.h"

#include <stddef.h>

static void refresh_async(void *user_data);

static void finish_refresh(notes_editor_refresh_guard_t *guard)
{
    guard->pending = 0U;
    if (guard->restore_enabled != 0U)
        lv_obj_remove_state(guard->textarea, LV_STATE_DISABLED);
    guard->restore_enabled = 0U;
}

void notes_editor_refresh_guard_init(notes_editor_refresh_guard_t *guard,
    lv_obj_t *textarea, notes_editor_refresh_cb_t refresh, void *user_data)
{
    if (guard == NULL) return;
    guard->textarea = textarea;
    guard->refresh = refresh;
    guard->user_data = user_data;
    guard->focus_byte = 0U;
    guard->pending = 0U;
    guard->restore_enabled = 0U;
    guard->async_scheduled = 0U;
}

void notes_editor_refresh_guard_reset(notes_editor_refresh_guard_t *guard)
{
    if (guard == NULL) return;
    if (guard->async_scheduled != 0U) {
        (void)lv_async_call_cancel(refresh_async, guard);
        guard->async_scheduled = 0U;
    }
    if (guard->restore_enabled != 0U && guard->textarea != NULL)
        lv_obj_remove_state(guard->textarea, LV_STATE_DISABLED);
    guard->focus_byte = 0U;
    guard->pending = 0U;
    guard->restore_enabled = 0U;
}

void notes_editor_refresh_guard_abandon(notes_editor_refresh_guard_t *guard)
{
    if (guard == NULL) return;
    if (guard->async_scheduled != 0U)
        (void)lv_async_call_cancel(refresh_async, guard);
    guard->textarea = NULL;
    guard->refresh = NULL;
    guard->user_data = NULL;
    guard->focus_byte = 0U;
    guard->pending = 0U;
    guard->restore_enabled = 0U;
    guard->async_scheduled = 0U;
}

void notes_editor_refresh_guard_begin(notes_editor_refresh_guard_t *guard,
                                      uint32_t focus_byte)
{
    if (guard == NULL || guard->textarea == NULL || guard->refresh == NULL ||
        guard->pending != 0U) return;
    guard->focus_byte = focus_byte;
    if (!lv_obj_has_state(guard->textarea, LV_STATE_DISABLED)) {
        lv_obj_add_state(guard->textarea, LV_STATE_DISABLED);
        guard->restore_enabled = 1U;
    }
    guard->pending = 1U;
    if (lv_async_call(refresh_async, guard) == LV_RESULT_OK)
        guard->async_scheduled = 1U;
}

bool notes_editor_refresh_guard_retry(notes_editor_refresh_guard_t *guard)
{
    if (guard == NULL) return false;
    if (guard->pending == 0U) return true;
    if (guard->refresh == NULL ||
        !guard->refresh(guard->user_data, guard->focus_byte)) return false;
    if (guard->async_scheduled != 0U) {
        (void)lv_async_call_cancel(refresh_async, guard);
        guard->async_scheduled = 0U;
    }
    finish_refresh(guard);
    return true;
}

bool notes_editor_refresh_guard_is_pending(
    const notes_editor_refresh_guard_t *guard)
{
    return guard != NULL && guard->pending != 0U;
}

static void refresh_async(void *user_data)
{
    notes_editor_refresh_guard_t *guard = user_data;
    if (guard == NULL || guard->async_scheduled == 0U) return;
    guard->async_scheduled = 0U;
    (void)notes_editor_refresh_guard_retry(guard);
}
