#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "text_selection_controller.h"

#define TARGET_BYTES 70000U
#define CHECK(name, actual, expected) do { if ((actual) != (expected)) { \
    printf("FAIL %s actual=%lu expected=%lu\n", name, (unsigned long)(actual), \
           (unsigned long)(expected)); failures++; } } while (0)

typedef struct {
    uint8_t text[TARGET_BYTES + 1U]; uint32_t length; uint32_t cursor;
    uint8_t visible, fail_read, fail_replace, snap_cursor; uint32_t start, end;
    const text_selection_controller_t *observer;
    text_selection_phase_t observed_phase; uint32_t observed_start, observed_end;
} target_t;

static uint8_t g_clipboard[TEXT_SELECTION_CLIPBOARD_BYTES + 1U];

static uint32_t get_length(void *context) { return ((target_t *)context)->length; }
static uint32_t get_cursor(void *context) { return ((target_t *)context)->cursor; }
static int set_cursor(void *context, uint32_t cursor)
{
    target_t *target = (target_t *)context;
    if (cursor > target->length) return 1;
    if (target->snap_cursor != 0U && cursor == 1U) cursor = 3U;
    if (target->snap_cursor != 0U && cursor == 2U) cursor = 0U;
    target->cursor = cursor;
    return 0;
}

static int move_vertical(void *context, text_selection_vertical_t direction,
                         uint32_t *cursor)
{
    target_t *target = (target_t *)context;
    if (direction == TEXT_SELECTION_VERTICAL_UP)
        target->cursor = target->cursor >= 10U ? target->cursor - 10U : 0U;
    else if (direction == TEXT_SELECTION_VERTICAL_DOWN)
        target->cursor = target->length - target->cursor >= 10U ?
            target->cursor + 10U : target->length;
    else return 1;
    *cursor = target->cursor;
    return 0;
}

static void show_selection(void *context, uint8_t visible, uint32_t start,
                           uint32_t end)
{
    target_t *target = (target_t *)context;
    text_selection_status_t status;
    target->visible = visible; target->start = start; target->end = end;
    if (visible == 0U && target->observer != 0)
    {
        text_selection_get_status(target->observer, &status);
        target->observed_phase = status.phase;
        target->observed_start = status.selection_start;
        target->observed_end = status.selection_end;
    }
}

static int read_range(void *context, uint32_t start, uint32_t end,
                      uint8_t *output, uint32_t capacity)
{
    target_t *target = (target_t *)context;
    uint32_t count;
    if (target->fail_read != 0U || start > end || end > target->length) return 1;
    count = end - start;
    if (capacity <= count) return 1;
    memcpy(output, target->text + start, count); output[count] = 0U;
    return 0;
}

static int replace_range(void *context, uint32_t start, uint32_t end,
                         const uint8_t *input, uint32_t input_bytes,
                         uint32_t *cursor)
{
    target_t *target = (target_t *)context;
    uint32_t next;
    if (target->fail_replace != 0U || start > end || end > target->length) return 1;
    next = target->length - (end - start) + input_bytes;
    if (next > TARGET_BYTES) return 1;
    memmove(target->text + start + input_bytes, target->text + end,
            target->length - end + 1U);
    if (input_bytes != 0U) memcpy(target->text + start, input, input_bytes);
    target->length = next; target->cursor = start + input_bytes; *cursor = target->cursor;
    return 0;
}

static const text_selection_target_ops_t g_ops = {
    get_length, get_cursor, set_cursor, move_vertical, show_selection,
    read_range, replace_range
};

static void reset(target_t *target, const char *text)
{
    memset(target, 0, sizeof(*target)); target->length = (uint32_t)strlen(text);
    memcpy(target->text, text, target->length + 1U);
}

static void select_range(text_selection_controller_t *controller, target_t *target,
                         uint32_t start, uint32_t end)
{
    target->cursor = start; (void)text_selection_action(controller, TEXT_SELECT_OK);
    target->cursor = end; (void)text_selection_action(controller, TEXT_SELECT_OK);
}

static int test_state_machine(void)
{
    text_selection_controller_t controller; text_selection_status_t status; target_t target;
    int failures = 0;
    reset(&target, "0123456789abcdefghijklmnopqrst"); target.cursor = 5U;
    CHECK("init", text_selection_init(&controller, g_clipboard, sizeof(g_clipboard)), TEXT_SELECTION_OK);
    CHECK("target", text_selection_set_target(&controller, &target, &g_ops), TEXT_SELECTION_OK);
    CHECK("move idle", text_selection_action(&controller, TEXT_SELECT_RIGHT), TEXT_SELECTION_OK);
    CHECK("idle cursor", target.cursor, 6U); CHECK("idle hidden", target.visible, 0U);
    CHECK("first ok", text_selection_action(&controller, TEXT_SELECT_OK), TEXT_SELECTION_OK);
    CHECK("down", text_selection_action(&controller, TEXT_SELECT_DOWN), TEXT_SELECTION_OK);
    CHECK("selection shown", target.visible, 1U); CHECK("selection start", target.start, 6U);
    CHECK("selection end", target.end, 16U);
    CHECK("second ok", text_selection_action(&controller, TEXT_SELECT_OK), TEXT_SELECTION_OK);
    CHECK("fixed move", text_selection_action(&controller, TEXT_SELECT_RIGHT), TEXT_SELECTION_OK);
    CHECK("fixed selection", target.end, 16U); text_selection_get_status(&controller, &status);
    CHECK("fixed phase", status.phase, TEXT_SELECTION_PHASE_FIXED);
    CHECK("third ok", text_selection_action(&controller, TEXT_SELECT_OK), TEXT_SELECTION_OK);
    CHECK("third hidden", target.visible, 0U); CHECK("anchor", controller.anchor, 17U);
    CHECK("cancel", text_selection_action(&controller, TEXT_SELECT_CANCEL), TEXT_SELECTION_OK);
    text_selection_get_status(&controller, &status); CHECK("cancel idle", status.phase, TEXT_SELECTION_PHASE_IDLE);
    target.cursor = 6U; select_range(&controller, &target, 6U, 2U);
    CHECK("reverse start", target.start, 2U); CHECK("reverse end", target.end, 6U);
    return failures;
}

static int test_copy_boundaries_and_failures(void)
{
    text_selection_controller_t controller; text_selection_status_t status; target_t target;
    uint32_t index; int failures = 0;
    reset(&target, "seed");
    CHECK("init", text_selection_init(&controller, g_clipboard, sizeof(g_clipboard)), TEXT_SELECTION_OK);
    CHECK("target", text_selection_set_target(&controller, &target, &g_ops), TEXT_SELECTION_OK);
    select_range(&controller, &target, 1U, 1U);
    CHECK("copy empty", text_selection_action(&controller, TEXT_SELECT_COPY), TEXT_SELECTION_EMPTY);
    select_range(&controller, &target, 0U, 4U);
    CHECK("copy seed", text_selection_action(&controller, TEXT_SELECT_COPY), TEXT_SELECTION_OK);
    for (index = 0U; index < TARGET_BYTES; ++index) target.text[index] = 'x';
    target.text[TARGET_BYTES] = 0U; target.length = TEXT_SELECTION_CLIPBOARD_BYTES;
    (void)text_selection_action(&controller, TEXT_SELECT_CANCEL);
    select_range(&controller, &target, 0U, TEXT_SELECTION_CLIPBOARD_BYTES);
    CHECK("copy 65536", text_selection_action(&controller, TEXT_SELECT_COPY), TEXT_SELECTION_OK);
    text_selection_get_status(&controller, &status); CHECK("clipboard 65536", status.clipboard_bytes, TEXT_SELECTION_CLIPBOARD_BYTES);
    CHECK("terminator", g_clipboard[TEXT_SELECTION_CLIPBOARD_BYTES], 0U);
    target.length = TEXT_SELECTION_CLIPBOARD_BYTES + 1U;
    (void)text_selection_action(&controller, TEXT_SELECT_CANCEL);
    select_range(&controller, &target, 0U, target.length);
    CHECK("copy 65537", text_selection_action(&controller, TEXT_SELECT_COPY), TEXT_SELECTION_CAPACITY);
    text_selection_get_status(&controller, &status); CHECK("old size kept", status.clipboard_bytes, TEXT_SELECTION_CLIPBOARD_BYTES);
    CHECK("old data kept", g_clipboard[0], 'x');
    target.fail_read = 1U;
    (void)text_selection_action(&controller, TEXT_SELECT_CANCEL);
    select_range(&controller, &target, 0U, 1U);
    CHECK("read failure", text_selection_action(&controller, TEXT_SELECT_COPY), TEXT_SELECTION_TARGET_ERROR);
    text_selection_get_status(&controller, &status); CHECK("read selection kept", status.phase, TEXT_SELECTION_PHASE_FIXED);
    CHECK("read clipboard kept", status.clipboard_bytes, TEXT_SELECTION_CLIPBOARD_BYTES);
    CHECK("read clipboard data kept", g_clipboard[0], 'x');
    CHECK("read failure did not write", g_clipboard[1], 'x');
    return failures;
}

static int test_paste_and_active_replace(void)
{
    text_selection_controller_t controller; text_selection_status_t status; target_t source, target, invalid;
    uint8_t replacement = 'Z', handled = 0U; int failures = 0;
    reset(&source, "copy"); reset(&target, "ABCD");
    (void)text_selection_init(&controller, g_clipboard, sizeof(g_clipboard));
    (void)text_selection_set_target(&controller, &source, &g_ops);
    select_range(&controller, &source, 0U, 4U);
    CHECK("copy", text_selection_action(&controller, TEXT_SELECT_COPY), TEXT_SELECTION_OK);
    reset(&invalid, "x"); invalid.cursor = 2U;
    CHECK("invalid target", text_selection_set_target(&controller, &invalid, &g_ops), TEXT_SELECTION_TARGET_ERROR);
    text_selection_get_status(&controller, &status); CHECK("target selection kept", status.phase, TEXT_SELECTION_PHASE_FIXED);
    target.cursor = 2U; (void)text_selection_set_target(&controller, &target, &g_ops);
    target.observer = &controller;
    CHECK("insert", text_selection_action(&controller, TEXT_SELECT_PASTE), TEXT_SELECTION_OK);
    CHECK("insert text", memcmp(target.text, "ABcopyCD", 9U), 0U);
    CHECK("insert cursor", target.cursor, 6U);
    select_range(&controller, &target, 2U, 6U);
    CHECK("replace paste", text_selection_action(&controller, TEXT_SELECT_PASTE), TEXT_SELECTION_OK);
    CHECK("replace text", memcmp(target.text, "ABcopyCD", 9U), 0U);
    text_selection_get_status(&controller, &status);
    CHECK("replace paste idle", status.phase, TEXT_SELECTION_PHASE_IDLE);
    CHECK("replace paste hidden", target.visible, 0U);
    CHECK("replace paste endpoint", status.selection_start, 6U);
    CHECK("hide sees idle", target.observed_phase, TEXT_SELECTION_PHASE_IDLE);
    CHECK("hide sees endpoint", target.observed_end, 6U);
    select_range(&controller, &target, 2U, 6U); target.fail_replace = 1U;
    CHECK("replace failure", text_selection_action(&controller, TEXT_SELECT_PASTE), TEXT_SELECTION_TARGET_ERROR);
    text_selection_get_status(&controller, &status); CHECK("replace selection kept", status.phase, TEXT_SELECTION_PHASE_FIXED);
    CHECK("replace clipboard kept", status.clipboard_bytes, 4U); CHECK("replace text kept", memcmp(target.text, "ABcopyCD", 9U), 0U);
    target.fail_replace = 0U; select_range(&controller, &target, 2U, 6U);
    CHECK("active replace", text_selection_replace_active(&controller, &replacement, 1U, &handled), TEXT_SELECTION_OK);
    CHECK("active handled", handled, 1U); CHECK("active text", memcmp(target.text, "ABZCD", 6U), 0U);
    text_selection_get_status(&controller, &status);
    CHECK("active replace idle", status.phase, TEXT_SELECTION_PHASE_IDLE);
    CHECK("active replace endpoint", status.selection_end, 3U);
    return failures;
}

static int test_fixed_extent_survives_movement(void)
{
    text_selection_controller_t controller; target_t target; uint8_t replacement = 'Z', handled = 0U;
    int failures = 0;
    reset(&target, "0123456789abcdefghijklmnopqrst");
    (void)text_selection_init(&controller, g_clipboard, sizeof(g_clipboard));
    (void)text_selection_set_target(&controller, &target, &g_ops);
    select_range(&controller, &target, 6U, 16U);
    CHECK("fixed move", text_selection_action(&controller, TEXT_SELECT_RIGHT), TEXT_SELECTION_OK);
    CHECK("fixed moved cursor", target.cursor, 17U);
    CHECK("fixed copy", text_selection_action(&controller, TEXT_SELECT_COPY), TEXT_SELECTION_OK);
    CHECK("fixed copy first", g_clipboard[0], '6');
    CHECK("fixed copy last", g_clipboard[9], 'f');
    CHECK("fixed copy terminator", g_clipboard[10], 0U);
    CHECK("fixed paste", text_selection_action(&controller, TEXT_SELECT_PASTE), TEXT_SELECTION_OK);
    CHECK("fixed paste range", memcmp(target.text, "0123456789abcdefghijklmnopqrst", 31U), 0U);
    select_range(&controller, &target, 6U, 16U);
    (void)text_selection_action(&controller, TEXT_SELECT_RIGHT);
    CHECK("fixed replace", text_selection_replace_active(&controller, &replacement, 1U, &handled), TEXT_SELECTION_OK);
    CHECK("fixed handled", handled, 1U);
    CHECK("fixed range", memcmp(target.text, "012345Zghijklmnopqrst", 22U), 0U);
    return failures;
}

static int test_snapped_opaque_cursor(void)
{
    text_selection_controller_t controller; text_selection_status_t status; target_t target;
    int failures = 0;
    reset(&target, "中A"); target.snap_cursor = 1U;
    CHECK("snap init", text_selection_init(&controller, g_clipboard, sizeof(g_clipboard)), TEXT_SELECTION_OK);
    CHECK("snap target", text_selection_set_target(&controller, &target, &g_ops), TEXT_SELECTION_OK);
    CHECK("snap first ok", text_selection_action(&controller, TEXT_SELECT_OK), TEXT_SELECTION_OK);
    CHECK("snap right", text_selection_action(&controller, TEXT_SELECT_RIGHT), TEXT_SELECTION_OK);
    CHECK("snap right cursor", target.cursor, 3U); CHECK("snap right shown", target.end, 3U);
    CHECK("snap second ok", text_selection_action(&controller, TEXT_SELECT_OK), TEXT_SELECTION_OK);
    text_selection_get_status(&controller, &status);
    CHECK("snap fixed start", status.selection_start, 0U); CHECK("snap fixed end", status.selection_end, 3U);
    CHECK("snap copy", text_selection_action(&controller, TEXT_SELECT_COPY), TEXT_SELECTION_OK);
    text_selection_get_status(&controller, &status);
    CHECK("snap copy bytes", status.clipboard_bytes, 3U);
    CHECK("snap third ok", text_selection_action(&controller, TEXT_SELECT_OK), TEXT_SELECTION_OK);
    CHECK("snap left", text_selection_action(&controller, TEXT_SELECT_LEFT), TEXT_SELECTION_OK);
    CHECK("snap left cursor", target.cursor, 0U); CHECK("snap left shown", target.end, 3U);
    CHECK("snap left shown start", target.start, 0U);
    CHECK("snap left second ok", text_selection_action(&controller, TEXT_SELECT_OK), TEXT_SELECTION_OK);
    text_selection_get_status(&controller, &status);
    CHECK("snap left start", status.selection_start, 0U); CHECK("snap left end", status.selection_end, 3U);
    CHECK("snap left copy", text_selection_action(&controller, TEXT_SELECT_COPY), TEXT_SELECTION_OK);
    text_selection_get_status(&controller, &status); CHECK("snap left copy bytes", status.clipboard_bytes, 3U);
    return failures;
}

int main(void)
{
    int failures = test_state_machine() + test_copy_boundaries_and_failures() +
                   test_paste_and_active_replace() + test_fixed_extent_survives_movement() +
                   test_snapped_opaque_cursor();
    if (failures != 0) { printf("FAIL text_selection_controller failures=%d\n", failures); return 1; }
    printf("PASS text_selection_controller\n"); return 0;
}
