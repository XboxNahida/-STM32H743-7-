#include "text_selection_controller.h"

static uint8_t is_ready(const text_selection_controller_t *controller)
{
    return controller != 0 && controller->target != 0 && controller->ops != 0;
}

static text_selection_result_t get_cursor(const text_selection_controller_t *controller,
                                          uint32_t *cursor)
{
    uint32_t length;
    if (is_ready(controller) == 0U) return TEXT_SELECTION_NOT_READY;
    length = controller->ops->get_length(controller->target);
    *cursor = controller->ops->get_cursor(controller->target);
    return *cursor <= length ? TEXT_SELECTION_OK : TEXT_SELECTION_TARGET_ERROR;
}

static void show(text_selection_controller_t *controller, uint8_t visible,
                 uint32_t first, uint32_t second)
{
    uint32_t start = first < second ? first : second;
    uint32_t end = first < second ? second : first;
    controller->ops->show_selection(controller->target, visible, start, end);
}

static text_selection_result_t move_cursor(text_selection_controller_t *controller,
                                           text_selection_action_t action,
                                           uint32_t *cursor)
{
    uint32_t length;
    text_selection_vertical_t vertical;
    if (get_cursor(controller, cursor) != TEXT_SELECTION_OK) return TEXT_SELECTION_TARGET_ERROR;
    length = controller->ops->get_length(controller->target);
    if (action == TEXT_SELECT_UP || action == TEXT_SELECT_DOWN)
    {
        vertical = action == TEXT_SELECT_UP ? TEXT_SELECTION_VERTICAL_UP :
            TEXT_SELECTION_VERTICAL_DOWN;
        if (controller->ops->move_vertical(controller->target, vertical, cursor) != 0 ||
            *cursor > length) return TEXT_SELECTION_TARGET_ERROR;
        return TEXT_SELECTION_OK;
    }
    if (action == TEXT_SELECT_LEFT && *cursor > 0U) --*cursor;
    if (action == TEXT_SELECT_RIGHT && *cursor < length) ++*cursor;
    if (action == TEXT_SELECT_HOME) *cursor = 0U;
    if (action == TEXT_SELECT_END) *cursor = length;
    if (controller->ops->set_cursor(controller->target, *cursor) != 0)
        return TEXT_SELECTION_TARGET_ERROR;
    return get_cursor(controller, cursor);
}

static text_selection_result_t replace_range(text_selection_controller_t *controller,
                                             uint32_t start, uint32_t end,
                                             const uint8_t *replacement,
                                             uint32_t replacement_bytes)
{
    uint32_t cursor;
    if (controller->ops->replace_range(controller->target, start, end, replacement,
                                       replacement_bytes, &cursor) != 0)
        return TEXT_SELECTION_TARGET_ERROR;
    controller->phase = TEXT_SELECTION_PHASE_IDLE;
    controller->anchor = cursor;
    controller->selection_start = cursor;
    controller->selection_end = cursor;
    show(controller, 0U, cursor, cursor);
    return TEXT_SELECTION_OK;
}

text_selection_result_t text_selection_init(text_selection_controller_t *controller,
                                             uint8_t *clipboard,
                                             uint32_t clipboard_capacity)
{
    if (controller == 0 || clipboard == 0 ||
        clipboard_capacity < TEXT_SELECTION_CLIPBOARD_BYTES + 1U)
        return TEXT_SELECTION_INVALID_ARG;
    controller->target = 0;
    controller->ops = 0;
    controller->clipboard = clipboard;
    controller->clipboard_capacity = clipboard_capacity;
    controller->clipboard_bytes = 0U;
    controller->anchor = 0U;
    controller->selection_start = 0U;
    controller->selection_end = 0U;
    controller->phase = TEXT_SELECTION_PHASE_IDLE;
    clipboard[0] = 0U;
    return TEXT_SELECTION_OK;
}

text_selection_result_t text_selection_set_target(text_selection_controller_t *controller,
                                                   void *target,
                                                   const text_selection_target_ops_t *ops)
{
    uint32_t cursor;
    if (controller == 0 || target == 0 || ops == 0 || ops->get_length == 0 ||
        ops->get_cursor == 0 || ops->set_cursor == 0 || ops->move_vertical == 0 ||
        ops->show_selection == 0 || ops->read_range == 0 || ops->replace_range == 0)
        return TEXT_SELECTION_INVALID_ARG;
    cursor = ops->get_cursor(target);
    if (cursor > ops->get_length(target)) return TEXT_SELECTION_TARGET_ERROR;
    if (is_ready(controller) != 0U && controller->phase != TEXT_SELECTION_PHASE_IDLE)
        show(controller, 0U, controller->anchor, controller->anchor);
    controller->target = target;
    controller->ops = ops;
    controller->phase = TEXT_SELECTION_PHASE_IDLE;
    controller->anchor = cursor;
    controller->selection_start = cursor;
    controller->selection_end = cursor;
    return TEXT_SELECTION_OK;
}

text_selection_result_t text_selection_action(text_selection_controller_t *controller,
                                               text_selection_action_t action)
{
    uint32_t cursor, length, start, end;
    text_selection_result_t result;
    if (is_ready(controller) == 0U) return TEXT_SELECTION_NOT_READY;
    if (action <= TEXT_SELECT_END)
    {
        result = move_cursor(controller, action, &cursor);
        if (result != TEXT_SELECTION_OK) return result;
        if (controller->phase == TEXT_SELECTION_PHASE_ANCHORED)
            show(controller, 1U, controller->anchor, cursor);
        return TEXT_SELECTION_OK;
    }
    result = get_cursor(controller, &cursor);
    if (result != TEXT_SELECTION_OK) return result;
    if (action == TEXT_SELECT_OK)
    {
        if (controller->phase == TEXT_SELECTION_PHASE_ANCHORED)
        {
            show(controller, 1U, controller->anchor, cursor);
            controller->selection_start = controller->anchor < cursor ? controller->anchor : cursor;
            controller->selection_end = controller->anchor < cursor ? cursor : controller->anchor;
            controller->phase = TEXT_SELECTION_PHASE_FIXED;
            return TEXT_SELECTION_OK;
        }
        show(controller, 0U, cursor, cursor);
        controller->anchor = cursor;
        controller->selection_start = cursor;
        controller->selection_end = cursor;
        controller->phase = TEXT_SELECTION_PHASE_ANCHORED;
        return TEXT_SELECTION_OK;
    }
    if (action == TEXT_SELECT_CANCEL)
    {
        show(controller, 0U, cursor, cursor);
        controller->anchor = cursor;
        controller->selection_start = cursor;
        controller->selection_end = cursor;
        controller->phase = TEXT_SELECTION_PHASE_IDLE;
        return TEXT_SELECTION_OK;
    }
    if (action == TEXT_SELECT_COPY)
    {
        if (controller->phase != TEXT_SELECTION_PHASE_FIXED ||
            controller->selection_start == controller->selection_end)
            return TEXT_SELECTION_EMPTY;
        start = controller->selection_start;
        end = controller->selection_end;
        length = end - start;
        if (length > TEXT_SELECTION_CLIPBOARD_BYTES) return TEXT_SELECTION_CAPACITY;
        if (controller->ops->read_range(controller->target, start, end, controller->clipboard,
                                        controller->clipboard_capacity) != 0)
            return TEXT_SELECTION_TARGET_ERROR;
        controller->clipboard[length] = 0U;
        controller->clipboard_bytes = length;
        return TEXT_SELECTION_OK;
    }
    if (action == TEXT_SELECT_PASTE)
    {
        if (controller->clipboard_bytes == 0U) return TEXT_SELECTION_EMPTY;
        start = end = cursor;
        if (controller->phase == TEXT_SELECTION_PHASE_FIXED)
        {
            start = controller->selection_start;
            end = controller->selection_end;
        }
        return replace_range(controller, start, end, controller->clipboard,
                             controller->clipboard_bytes);
    }
    return TEXT_SELECTION_INVALID_ARG;
}

text_selection_result_t text_selection_replace_active(
    text_selection_controller_t *controller, const uint8_t *replacement,
    uint32_t replacement_bytes, uint8_t *handled)
{
    uint32_t cursor, start, end;
    text_selection_result_t result;
    if (handled == 0 || (replacement == 0 && replacement_bytes != 0U))
        return TEXT_SELECTION_INVALID_ARG;
    *handled = 0U;
    result = get_cursor(controller, &cursor);
    if (result != TEXT_SELECTION_OK) return result;
    if (controller->phase != TEXT_SELECTION_PHASE_FIXED ||
        controller->selection_start == controller->selection_end)
        return TEXT_SELECTION_OK;
    start = controller->selection_start;
    end = controller->selection_end;
    result = replace_range(controller, start, end, replacement, replacement_bytes);
    if (result == TEXT_SELECTION_OK) *handled = 1U;
    return result;
}

void text_selection_get_status(const text_selection_controller_t *controller,
                               text_selection_status_t *status)
{
    if (controller == 0 || status == 0) return;
    status->anchor = controller->anchor;
    status->clipboard_bytes = controller->clipboard_bytes;
    status->selection_start = controller->selection_start;
    status->selection_end = controller->selection_end;
    status->phase = controller->phase;
}
