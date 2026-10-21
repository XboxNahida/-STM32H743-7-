#include "nes_player_controls.h"

#include <stddef.h>

#define NES_PLAYER_TURBO_PHASE_MS 60U

typedef struct
{
    uint32_t started_ms;
    uint8_t held;
} nes_player_turbo_t;

static const nes_player_control_region_t g_layout[] =
{
    {  16, 128, 104, 48 },
    { 128, 128, 104, 48 },
    {  88, 248,  72, 72 },
    {  88, 392,  72, 72 },
    {  16, 320,  72, 72 },
    { 872,  60, 104, 48 },
    { 160, 320,  72, 72 },
    { 872, 268,  72, 72 },
    { 800, 340,  72, 72 },
    { 944, 340,  72, 72 },
    { 872, 412,  72, 72 },
    {  16, 248,  72, 72 },
    { 160, 248,  72, 72 },
    {  16, 392,  72, 72 },
    { 160, 392,  72, 72 }
};

static const nes_touch_region_t g_direct_regions[] =
{
    {  16, 128, 104, 48, NES_BUTTON_SELECT },
    { 128, 128, 104, 48, NES_BUTTON_START },
    {  88, 248,  72, 72, NES_BUTTON_UP },
    {  88, 392,  72, 72, NES_BUTTON_DOWN },
    {  16, 320,  72, 72, NES_BUTTON_LEFT },
    { 160, 320,  72, 72, NES_BUTTON_RIGHT },
    { 944, 340,  72, 72, NES_BUTTON_B },
    { 872, 412,  72, 72, NES_BUTTON_A },
    {  16, 248,  72, 72,
        (nes_button_mask_t)(NES_BUTTON_UP | NES_BUTTON_LEFT) },
    { 160, 248,  72, 72,
        (nes_button_mask_t)(NES_BUTTON_UP | NES_BUTTON_RIGHT) },
    {  16, 392,  72, 72,
        (nes_button_mask_t)(NES_BUTTON_DOWN | NES_BUTTON_LEFT) },
    { 160, 392,  72, 72,
        (nes_button_mask_t)(NES_BUTTON_DOWN | NES_BUTTON_RIGHT) }
};

static nes_player_turbo_t g_turbo_a;
static nes_player_turbo_t g_turbo_b;
static uint8_t g_pause_held;

static uint8_t nes_player_point_inside(
    const nes_touch_point_t *point,
    const nes_player_control_region_t *region)
{
    int32_t dx = (int32_t)point->x - (int32_t)region->x;
    int32_t dy = (int32_t)point->y - (int32_t)region->y;
    return ((dx >= 0) && (dy >= 0) &&
            ((uint32_t)dx < region->width) &&
            ((uint32_t)dy < region->height)) ? 1U : 0U;
}

static uint8_t nes_player_region_touched(const nes_touch_point_t *points,
                                         uint8_t point_count,
                                         nes_player_control_id_t id)
{
    uint8_t index;
    for (index = 0U; index < point_count; ++index)
    {
        if (nes_player_point_inside(&points[index], &g_layout[id]) != 0U)
        {
            return 1U;
        }
    }
    return 0U;
}

static void nes_player_apply_turbo(nes_player_turbo_t *turbo,
                                   uint8_t touched,
                                   uint32_t now_ms,
                                   nes_button_mask_t button,
                                   nes_button_mask_t *buttons)
{
    if (touched == 0U)
    {
        turbo->held = 0U;
        return;
    }
    if (turbo->held == 0U)
    {
        turbo->held = 1U;
        turbo->started_ms = now_ms;
    }
    if ((((uint32_t)(now_ms - turbo->started_ms) /
          NES_PLAYER_TURBO_PHASE_MS) & 1U) == 0U)
    {
        *buttons = (nes_button_mask_t)(*buttons | button);
    }
}

void nes_player_controls_init(void)
{
    g_turbo_a.held = 0U;
    g_turbo_a.started_ms = 0U;
    g_turbo_b.held = 0U;
    g_turbo_b.started_ms = 0U;
    g_pause_held = 0U;
}

const nes_player_control_region_t *nes_player_controls_get_layout(
    uint8_t *count)
{
    if (count != NULL)
    {
        *count = (uint8_t)NES_PLAYER_CONTROL_COUNT;
    }
    return g_layout;
}

int nes_player_controls_update(const nes_touch_point_t *points,
                               uint8_t point_count,
                               uint32_t now_ms,
                               nes_button_mask_t *buttons,
                               uint8_t *pause_toggle)
{
    uint8_t turbo_a;
    uint8_t turbo_b;
    uint8_t pause;

    if ((buttons == NULL) || (pause_toggle == NULL) ||
        (point_count > NES_INPUT_MAX_TOUCH_POINTS) ||
        ((point_count != 0U) && (points == NULL)))
    {
        return NES_INPUT_INVALID_ARG;
    }
    if (nes_input_map_touch(points, point_count, g_direct_regions,
        (uint8_t)(sizeof(g_direct_regions) / sizeof(g_direct_regions[0])),
        buttons) != NES_INPUT_OK)
    {
        return NES_INPUT_INVALID_ARG;
    }
    turbo_a = nes_player_region_touched(points, point_count,
                                        NES_PLAYER_CONTROL_Y);
    turbo_b = nes_player_region_touched(points, point_count,
                                        NES_PLAYER_CONTROL_X);
    pause = nes_player_region_touched(points, point_count,
                                      NES_PLAYER_CONTROL_PAUSE);
    nes_player_apply_turbo(&g_turbo_a, turbo_a, now_ms,
                           NES_BUTTON_A, buttons);
    nes_player_apply_turbo(&g_turbo_b, turbo_b, now_ms,
                           NES_BUTTON_B, buttons);
    *pause_toggle = ((pause != 0U) && (g_pause_held == 0U)) ? 1U : 0U;
    g_pause_held = pause;
    return NES_INPUT_OK;
}
