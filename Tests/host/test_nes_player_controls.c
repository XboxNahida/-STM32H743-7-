#include <stdint.h>
#include <stdio.h>

#include "nes_player_controls.h"

static int expect(const char *name, int condition)
{
    if (!condition)
    {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

static int update_one(int16_t x, int16_t y, uint32_t tick,
                      nes_button_mask_t *buttons, uint8_t *pause)
{
    nes_touch_point_t point = { x, y };
    return nes_player_controls_update(&point, 1U, tick, buttons, pause);
}

int main(void)
{
    const nes_player_control_region_t *layout;
    nes_touch_point_t points[5];
    nes_button_mask_t buttons;
    uint8_t pause;
    uint8_t count;
    int failures = 0;

    nes_player_controls_init();
    layout = nes_player_controls_get_layout(&count);
    failures += expect("layout count", layout != NULL && count == 15U);
    failures += expect("select position",
        layout[NES_PLAYER_CONTROL_SELECT].x == 16 &&
        layout[NES_PLAYER_CONTROL_SELECT].y == 128);
    failures += expect("start position",
        layout[NES_PLAYER_CONTROL_START].x == 128 &&
        layout[NES_PLAYER_CONTROL_START].y == 128);
    failures += expect("pause top right",
        layout[NES_PLAYER_CONTROL_PAUSE].x == 872 &&
        layout[NES_PLAYER_CONTROL_PAUSE].y == 60 &&
        layout[NES_PLAYER_CONTROL_PAUSE].width == 104 &&
        layout[NES_PLAYER_CONTROL_PAUSE].height == 48);
    failures += expect("dpad center empty",
        layout[NES_PLAYER_CONTROL_UP].y == 248 &&
        layout[NES_PLAYER_CONTROL_LEFT].y == 320 &&
        layout[NES_PLAYER_CONTROL_RIGHT].y == 320 &&
        layout[NES_PLAYER_CONTROL_DOWN].y == 392);
    failures += expect("diagonal dpad positions",
        layout[NES_PLAYER_CONTROL_UP_LEFT].x == 16 &&
        layout[NES_PLAYER_CONTROL_UP_LEFT].y == 248 &&
        layout[NES_PLAYER_CONTROL_UP_RIGHT].x == 160 &&
        layout[NES_PLAYER_CONTROL_UP_RIGHT].y == 248 &&
        layout[NES_PLAYER_CONTROL_DOWN_LEFT].x == 16 &&
        layout[NES_PLAYER_CONTROL_DOWN_LEFT].y == 392 &&
        layout[NES_PLAYER_CONTROL_DOWN_RIGHT].x == 160 &&
        layout[NES_PLAYER_CONTROL_DOWN_RIGHT].y == 392);
    failures += expect("action diamond moved down",
        layout[NES_PLAYER_CONTROL_Y].x == 872 &&
        layout[NES_PLAYER_CONTROL_Y].y == 268 &&
        layout[NES_PLAYER_CONTROL_X].x == 800 &&
        layout[NES_PLAYER_CONTROL_X].y == 340 &&
        layout[NES_PLAYER_CONTROL_B].x == 944 &&
        layout[NES_PLAYER_CONTROL_B].y == 340 &&
        layout[NES_PLAYER_CONTROL_A].x == 872 &&
        layout[NES_PLAYER_CONTROL_A].y == 412);

    failures += expect("direct A",
        update_one(900, 430, 0U, &buttons, &pause) == 0 &&
        buttons == NES_BUTTON_A && pause == 0U);
    failures += expect("direct select",
        update_one(20, 140, 0U, &buttons, &pause) == 0 &&
        buttons == NES_BUTTON_SELECT);
    failures += expect("direct up left",
        update_one(20, 260, 0U, &buttons, &pause) == 0 &&
        buttons == (NES_BUTTON_UP | NES_BUTTON_LEFT));
    failures += expect("direct up right",
        update_one(170, 260, 0U, &buttons, &pause) == 0 &&
        buttons == (NES_BUTTON_UP | NES_BUTTON_RIGHT));
    failures += expect("direct down left",
        update_one(20, 400, 0U, &buttons, &pause) == 0 &&
        buttons == (NES_BUTTON_DOWN | NES_BUTTON_LEFT));
    failures += expect("direct down right",
        update_one(170, 400, 0U, &buttons, &pause) == 0 &&
        buttons == (NES_BUTTON_DOWN | NES_BUTTON_RIGHT));

    nes_player_controls_init();
    failures += expect("turbo X starts B",
        update_one(810, 350, 100U, &buttons, &pause) == 0 &&
        buttons == NES_BUTTON_B);
    failures += expect("turbo X on phase",
        update_one(810, 350, 159U, &buttons, &pause) == 0 &&
        buttons == NES_BUTTON_B);
    failures += expect("turbo X off phase",
        update_one(810, 350, 160U, &buttons, &pause) == 0 && buttons == 0U);
    failures += expect("turbo X next phase",
        update_one(810, 350, 220U, &buttons, &pause) == 0 &&
        buttons == NES_BUTTON_B);
    failures += expect("turbo release",
        nes_player_controls_update(NULL, 0U, 221U, &buttons, &pause) == 0 &&
        buttons == 0U);
    failures += expect("turbo repress immediate",
        update_one(810, 350, 999U, &buttons, &pause) == 0 &&
        buttons == NES_BUTTON_B);

    nes_player_controls_init();
    failures += expect("turbo Y starts A",
        update_one(880, 280, 10U, &buttons, &pause) == 0 &&
        buttons == NES_BUTTON_A);
    failures += expect("pause rising",
        update_one(900, 80, 20U, &buttons, &pause) == 0 && pause == 1U);
    failures += expect("pause held",
        update_one(900, 80, 21U, &buttons, &pause) == 0 && pause == 0U);
    failures += expect("pause release",
        nes_player_controls_update(NULL, 0U, 22U, &buttons, &pause) == 0 &&
        pause == 0U);
    failures += expect("pause second rising",
        update_one(900, 80, 23U, &buttons, &pause) == 0 && pause == 1U);

    nes_player_controls_init();
    points[0] = (nes_touch_point_t){ 20, 330 };
    points[1] = (nes_touch_point_t){ 900, 430 };
    points[2] = (nes_touch_point_t){ 810, 350 };
    points[3] = (nes_touch_point_t){ 900, 80 };
    points[4] = (nes_touch_point_t){ 20, 140 };
    failures += expect("five point merge",
        nes_player_controls_update(points, 5U, 500U, &buttons, &pause) == 0 &&
        buttons == (NES_BUTTON_LEFT | NES_BUTTON_A | NES_BUTTON_B |
                    NES_BUTTON_SELECT) && pause == 1U);
    failures += expect("too many points",
        nes_player_controls_update(points, 6U, 0U, &buttons, &pause) != 0);
    failures += expect("null output",
        nes_player_controls_update(NULL, 0U, 0U, NULL, &pause) != 0);

    if (failures == 0) printf("PASS nes_player_controls\n");
    return failures;
}
