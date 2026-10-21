#ifndef NOTES_EDITOR_REFRESH_GUARD_H
#define NOTES_EDITOR_REFRESH_GUARD_H

#include <stdbool.h>
#include <stdint.h>

#include "lvgl.h"

typedef bool (*notes_editor_refresh_cb_t)(void *user_data,
                                          uint32_t focus_byte);

typedef struct {
    lv_obj_t *textarea;
    notes_editor_refresh_cb_t refresh;
    void *user_data;
    uint32_t focus_byte;
    uint8_t pending;
    uint8_t restore_enabled;
    uint8_t async_scheduled;
} notes_editor_refresh_guard_t;

void notes_editor_refresh_guard_init(notes_editor_refresh_guard_t *guard,
    lv_obj_t *textarea, notes_editor_refresh_cb_t refresh, void *user_data);
void notes_editor_refresh_guard_reset(notes_editor_refresh_guard_t *guard);
void notes_editor_refresh_guard_abandon(notes_editor_refresh_guard_t *guard);
void notes_editor_refresh_guard_begin(notes_editor_refresh_guard_t *guard,
                                      uint32_t focus_byte);
bool notes_editor_refresh_guard_retry(notes_editor_refresh_guard_t *guard);
bool notes_editor_refresh_guard_is_pending(
    const notes_editor_refresh_guard_t *guard);

#endif
