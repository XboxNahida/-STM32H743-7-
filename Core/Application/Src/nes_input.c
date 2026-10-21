#include "nes_input.h"

#include <stddef.h>
#include <string.h>

static nes_button_mask_t g_source_buttons[NES_INPUT_SOURCE_COUNT];
static nes_ir_binding_t g_ir_bindings[NES_INPUT_MAX_IR_BINDINGS];
static uint8_t g_ir_binding_count;
static uint8_t g_ir_active;
static nes_button_mask_t g_ir_last_buttons;
static uint32_t g_ir_last_frame_count;
static uint32_t g_ir_last_repeat_count;
static uint32_t g_ir_last_event_ms;

void nes_input_init(void)
{
    memset(g_source_buttons, 0, sizeof(g_source_buttons));
    memset(g_ir_bindings, 0, sizeof(g_ir_bindings));
    g_ir_binding_count = 0U;
    g_ir_active = 0U;
    g_ir_last_buttons = 0U;
    g_ir_last_frame_count = 0U;
    g_ir_last_repeat_count = 0U;
    g_ir_last_event_ms = 0U;
}

int nes_input_set_source(nes_input_source_t source,
                         nes_button_mask_t buttons)
{
    if ((uint32_t)source >= (uint32_t)NES_INPUT_SOURCE_COUNT)
    {
        return NES_INPUT_INVALID_ARG;
    }
    g_source_buttons[source] = buttons;
    return NES_INPUT_OK;
}

nes_button_mask_t nes_input_get_source(nes_input_source_t source)
{
    if ((uint32_t)source >= (uint32_t)NES_INPUT_SOURCE_COUNT)
    {
        return 0U;
    }
    return g_source_buttons[source];
}

nes_button_mask_t nes_input_get_buttons(void)
{
    nes_button_mask_t buttons = 0U;
    uint8_t source;

    for (source = 0U; source < (uint8_t)NES_INPUT_SOURCE_COUNT; source++)
    {
        buttons = (nes_button_mask_t)(buttons | g_source_buttons[source]);
    }
    return buttons;
}

static int nes_input_validate_touch_regions(
    const nes_touch_region_t *regions,
    uint8_t region_count)
{
    uint8_t region_index;

    for (region_index = 0U; region_index < region_count; region_index++)
    {
        if ((regions[region_index].width == 0U) ||
            (regions[region_index].height == 0U) ||
            (regions[region_index].buttons == 0U))
        {
            return NES_INPUT_INVALID_ARG;
        }
    }
    return NES_INPUT_OK;
}

int nes_input_map_touch(const nes_touch_point_t *points,
                        uint8_t point_count,
                        const nes_touch_region_t *regions,
                        uint8_t region_count,
                        nes_button_mask_t *buttons)
{
    nes_button_mask_t mapped = 0U;
    uint8_t point_index;
    uint8_t region_index;

    if ((buttons == NULL) ||
        (point_count > NES_INPUT_MAX_TOUCH_POINTS) ||
        (region_count > NES_INPUT_MAX_TOUCH_REGIONS) ||
        ((point_count != 0U) && (points == NULL)) ||
        ((region_count != 0U) && (regions == NULL)))
    {
        return NES_INPUT_INVALID_ARG;
    }
    if (nes_input_validate_touch_regions(regions, region_count) !=
        NES_INPUT_OK)
    {
        return NES_INPUT_INVALID_ARG;
    }

    for (point_index = 0U; point_index < point_count; point_index++)
    {
        for (region_index = 0U; region_index < region_count; region_index++)
        {
            int32_t dx = (int32_t)points[point_index].x -
                         (int32_t)regions[region_index].x;
            int32_t dy = (int32_t)points[point_index].y -
                         (int32_t)regions[region_index].y;

            if ((dx >= 0) && (dy >= 0) &&
                ((uint32_t)dx < (uint32_t)regions[region_index].width) &&
                ((uint32_t)dy < (uint32_t)regions[region_index].height))
            {
                mapped = (nes_button_mask_t)(
                    mapped | regions[region_index].buttons);
                break;
            }
        }
    }
    *buttons = mapped;
    return NES_INPUT_OK;
}

static void nes_input_reset_ir_state(void)
{
    g_ir_active = 0U;
    g_ir_last_buttons = 0U;
    g_ir_last_frame_count = 0U;
    g_ir_last_repeat_count = 0U;
    g_ir_last_event_ms = 0U;
    g_source_buttons[NES_INPUT_SOURCE_IR] = 0U;
}

int nes_input_set_ir_map(const nes_ir_binding_t *bindings,
                         uint8_t count)
{
    uint8_t index;
    uint8_t compare_index;

    if ((count > NES_INPUT_MAX_IR_BINDINGS) ||
        ((count != 0U) && (bindings == NULL)))
    {
        return NES_INPUT_INVALID_ARG;
    }
    for (index = 0U; index < count; index++)
    {
        if (bindings[index].buttons == 0U)
        {
            return NES_INPUT_INVALID_ARG;
        }
        for (compare_index = 0U; compare_index < index; compare_index++)
        {
            if ((bindings[index].address ==
                 bindings[compare_index].address) &&
                (bindings[index].command ==
                 bindings[compare_index].command))
            {
                return NES_INPUT_INVALID_ARG;
            }
        }
    }

    memset(g_ir_bindings, 0, sizeof(g_ir_bindings));
    if (count != 0U)
    {
        memcpy(g_ir_bindings, bindings,
               (size_t)count * sizeof(g_ir_bindings[0]));
    }
    g_ir_binding_count = count;
    nes_input_reset_ir_state();
    return NES_INPUT_OK;
}

static nes_button_mask_t nes_input_find_ir_buttons(uint8_t address,
                                                    uint8_t command)
{
    uint8_t index;

    for (index = 0U; index < g_ir_binding_count; index++)
    {
        if ((g_ir_bindings[index].address == address) &&
            (g_ir_bindings[index].command == command))
        {
            return g_ir_bindings[index].buttons;
        }
    }
    return 0U;
}

int nes_input_update_ir(const nes_ir_snapshot_t *snapshot,
                        uint32_t now_ms)
{
    uint8_t new_frame;
    uint8_t new_repeat;

    if (snapshot == NULL)
    {
        return NES_INPUT_INVALID_ARG;
    }
    if (snapshot->frame_valid == 0U)
    {
        g_ir_active = 0U;
        g_ir_last_buttons = 0U;
        g_ir_last_frame_count = snapshot->frame_count;
        g_ir_last_repeat_count = snapshot->repeat_count;
        g_source_buttons[NES_INPUT_SOURCE_IR] = 0U;
        return NES_INPUT_OK;
    }

    new_frame = (snapshot->frame_count != g_ir_last_frame_count) ? 1U : 0U;
    new_repeat = (snapshot->repeat_count != g_ir_last_repeat_count) ? 1U : 0U;
    g_ir_last_frame_count = snapshot->frame_count;
    g_ir_last_repeat_count = snapshot->repeat_count;

    if (new_frame != 0U)
    {
        g_ir_last_buttons = nes_input_find_ir_buttons(snapshot->address,
                                                      snapshot->command);
        g_ir_active = (g_ir_last_buttons != 0U) ? 1U : 0U;
        g_source_buttons[NES_INPUT_SOURCE_IR] = g_ir_last_buttons;
        g_ir_last_event_ms = now_ms;
    }
    else if (new_repeat != 0U)
    {
        g_ir_active = (g_ir_last_buttons != 0U) ? 1U : 0U;
        g_source_buttons[NES_INPUT_SOURCE_IR] = g_ir_last_buttons;
        g_ir_last_event_ms = now_ms;
    }
    else if ((g_ir_active != 0U) &&
             ((uint32_t)(now_ms - g_ir_last_event_ms) >
              NES_INPUT_IR_RELEASE_MS))
    {
        g_ir_active = 0U;
        g_source_buttons[NES_INPUT_SOURCE_IR] = 0U;
    }
    return NES_INPUT_OK;
}
