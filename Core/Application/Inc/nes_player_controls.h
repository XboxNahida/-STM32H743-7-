#ifndef NES_PLAYER_CONTROLS_H
#define NES_PLAYER_CONTROLS_H

#include <stdint.h>

#include "nes_input.h"

typedef enum
{
    NES_PLAYER_CONTROL_SELECT = 0,
    NES_PLAYER_CONTROL_START,
    NES_PLAYER_CONTROL_UP,
    NES_PLAYER_CONTROL_DOWN,
    NES_PLAYER_CONTROL_LEFT,
    NES_PLAYER_CONTROL_PAUSE,
    NES_PLAYER_CONTROL_RIGHT,
    NES_PLAYER_CONTROL_Y,
    NES_PLAYER_CONTROL_X,
    NES_PLAYER_CONTROL_B,
    NES_PLAYER_CONTROL_A,
    NES_PLAYER_CONTROL_UP_LEFT,
    NES_PLAYER_CONTROL_UP_RIGHT,
    NES_PLAYER_CONTROL_DOWN_LEFT,
    NES_PLAYER_CONTROL_DOWN_RIGHT,
    NES_PLAYER_CONTROL_COUNT
} nes_player_control_id_t;

typedef struct
{
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
} nes_player_control_region_t;

void nes_player_controls_init(void);
const nes_player_control_region_t *nes_player_controls_get_layout(
    uint8_t *count);
int nes_player_controls_update(const nes_touch_point_t *points,
                               uint8_t point_count,
                               uint32_t now_ms,
                               nes_button_mask_t *buttons,
                               uint8_t *pause_toggle);

#endif
