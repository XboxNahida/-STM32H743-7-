#ifndef MUSIC_PLAYER_VISUAL_H
#define MUSIC_PLAYER_VISUAL_H

#include <stdbool.h>
#include <stdint.h>

#include "lvgl.h"

#define MUSIC_VISUAL_BAR_COUNT 20U
#define MUSIC_VISUAL_SPECTRUM_SIZE 250
#define MUSIC_VISUAL_INNER_RADIUS 77

typedef struct
{
    lv_obj_t *spectrum;
    lv_obj_t *cover;
    lv_obj_t *logo;
    lv_obj_t *logo_title;
    lv_obj_t *fade_objects[4];
    uint16_t radii[MUSIC_VISUAL_BAR_COUNT];
    uint16_t intro_values[MUSIC_VISUAL_BAR_COUNT];
    uint32_t intro_started_ms;
    uint32_t last_phase;
    uint8_t intro_active;
    uint8_t intro_bars_visible;
    uint8_t was_playing;
} music_visual_t;

void music_visual_init(music_visual_t *visual);
void music_visual_create_background(lv_obj_t *parent);
lv_obj_t *music_visual_create_image_button(lv_obj_t *parent,
                                            const lv_image_dsc_t *source,
                                            lv_obj_t **image_out);
void music_visual_create_spectrum(music_visual_t *visual,
                                  lv_obj_t *parent,
                                  const lv_image_dsc_t *cover_source);
void music_visual_start_intro(music_visual_t *visual,
                              lv_obj_t *parent,
                              lv_obj_t *title_box,
                              lv_obj_t *icon_box,
                              lv_obj_t *control_box,
                              lv_obj_t *handle_box,
                              uint32_t now_ms);
void music_visual_update_spectrum(music_visual_t *visual,
                                  uint32_t elapsed_ms,
                                  bool playing,
                                  uint32_t now_ms);

#endif
