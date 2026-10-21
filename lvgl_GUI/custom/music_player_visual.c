#include "music_player_visual.h"

#include <stddef.h>
#include <string.h>

#include "gui_guider.h"
#include "music_spectrum_1.h"

#define MUSIC_VISUAL_INTRO_BARS_MS 1800U
#define MUSIC_VISUAL_CONTENT_IN_MS 3000U
#define MUSIC_VISUAL_INTRO_DONE_MS 5000U
#define MUSIC_VISUAL_MAX_RADIUS ((MUSIC_VISUAL_SPECTRUM_SIZE / 2) - 2)

LV_IMAGE_DECLARE(img_lv_demo_music_logo);

static const uint16_t g_music_visual_random[MUSIC_VISUAL_BAR_COUNT] =
{
    994U, 285U, 553U, 11U, 792U, 707U, 966U, 641U, 852U, 827U,
    44U, 352U, 146U, 581U, 490U, 80U, 729U, 58U, 695U, 940U
};

static int music_visual_time_reached(uint32_t now_ms, uint32_t deadline_ms)
{
    return ((int32_t)(now_ms - deadline_ms) >= 0) ? 1 : 0;
}

static uint8_t music_visual_fade_value(uint32_t elapsed_ms,
                                       uint32_t start_ms,
                                       uint32_t duration_ms)
{
    uint32_t value;

    if (elapsed_ms <= start_ms)
    {
        return LV_OPA_TRANSP;
    }
    if (elapsed_ms >= (start_ms + duration_ms))
    {
        return LV_OPA_COVER;
    }
    value = ((elapsed_ms - start_ms) * LV_OPA_COVER) / duration_ms;
    return (uint8_t)value;
}

static lv_color_t music_visual_bar_color(uint16_t radius)
{
    if (radius < 80U)
    {
        return lv_color_hex(0xe9dbfc);
    }
    if (radius < 100U)
    {
        return lv_color_mix(lv_color_hex(0x6f8af6),
                            lv_color_hex(0xe9dbfc),
                            (uint8_t)((radius - 80U) * 255U / 20U));
    }
    return lv_color_hex(0x6f8af6);
}

static int32_t music_visual_cos(int32_t degree, int32_t radius)
{
    return (lv_trigo_sin((int16_t)(degree + 90)) * radius) /
           LV_TRIGO_SIN_MAX;
}

static int32_t music_visual_sin(int32_t degree, int32_t radius)
{
    return (lv_trigo_sin((int16_t)degree) * radius) /
           LV_TRIGO_SIN_MAX;
}

static void music_visual_draw_quad(lv_layer_t *layer,
                                   const lv_point_precise_t points[4],
                                   lv_color_t color)
{
    lv_draw_triangle_dsc_t triangle;

    lv_draw_triangle_dsc_init(&triangle);
    triangle.base.layer = layer;
    triangle.color = color;
    triangle.opa = LV_OPA_COVER;
    triangle.p[0] = points[0];
    triangle.p[1] = points[1];
    triangle.p[2] = points[2];
    lv_draw_triangle(layer, &triangle);
    triangle.p[1] = points[2];
    triangle.p[2] = points[3];
    lv_draw_triangle(layer, &triangle);
}

static void music_visual_draw_bar(lv_layer_t *layer,
                                  int32_t center_x,
                                  int32_t center_y,
                                  uint32_t index,
                                  uint16_t inner_radius,
                                  uint16_t outer_radius)
{
    const int32_t degree_step = 180 / (int32_t)MUSIC_VISUAL_BAR_COUNT;
    int32_t degree = (int32_t)index * degree_step + 90;
    int32_t degree_space = (outer_radius > 256U) ?
                           ((int32_t)outer_radius >> 7) : 1;
    lv_point_precise_t points[4];
    lv_color_t color = music_visual_bar_color(outer_radius);
    uint32_t point;

    if (degree_space >= (degree_step / 2))
    {
        degree_space = degree_step / 2 - 1;
    }
    points[0].x = center_x +
        music_visual_cos(degree + degree_space, outer_radius);
    points[0].y = center_y +
        music_visual_sin(degree + degree_space, outer_radius);
    points[1].x = center_x +
        music_visual_cos(degree + degree_space, inner_radius);
    points[1].y = center_y +
        music_visual_sin(degree + degree_space, inner_radius);
    degree += degree_step - degree_space;
    points[2].x = center_x + music_visual_cos(degree, inner_radius);
    points[2].y = center_y + music_visual_sin(degree, inner_radius);
    points[3].x = center_x + music_visual_cos(degree, outer_radius);
    points[3].y = center_y + music_visual_sin(degree, outer_radius);
    music_visual_draw_quad(layer, points, color);

    for (point = 0U; point < 4U; point++)
    {
        points[point].x = center_x - (points[point].x - center_x);
    }
    music_visual_draw_quad(layer, points, color);
}

static void music_visual_spectrum_event(lv_event_t *event)
{
    music_visual_t *visual = (music_visual_t *)lv_event_get_user_data(event);
    lv_event_code_t code = lv_event_get_code(event);

    if (visual == NULL)
    {
        return;
    }
    if (code == LV_EVENT_REFR_EXT_DRAW_SIZE)
    {
        lv_event_set_ext_draw_size(event,
            (visual->intro_active != 0U) ? 565 : 0);
    }
    else if (((code == LV_EVENT_DRAW_MAIN) &&
              (visual->intro_active != 0U)) ||
             (code == LV_EVENT_DRAW_POST))
    {
        lv_obj_t *object = lv_event_get_target(event);
        lv_layer_t *layer = lv_event_get_layer(event);
        lv_area_t area;
        int32_t center_x;
        int32_t center_y;
        int32_t cover_scale;
        uint16_t inner_radius;
        uint32_t index;

        if ((object == NULL) || (layer == NULL) ||
            ((visual->intro_active != 0U) &&
             (visual->intro_bars_visible == 0U)))
        {
            return;
        }
        lv_obj_get_coords(object, &area);
        center_x = (area.x1 + area.x2) / 2;
        center_y = (area.y1 + area.y2) / 2;
        cover_scale = lv_image_get_scale(visual->cover);
        if (cover_scale < 1)
        {
            cover_scale = 1;
        }
        inner_radius = (uint16_t)(((uint32_t)MUSIC_VISUAL_INNER_RADIUS *
                                   (uint32_t)cover_scale) / 256U);
        for (index = 0U; index < MUSIC_VISUAL_BAR_COUNT; index++)
        {
            uint16_t outer_radius = (visual->intro_active != 0U) ?
                (uint16_t)(MUSIC_VISUAL_INNER_RADIUS +
                           visual->intro_values[index]) :
                visual->radii[index];
            if ((visual->intro_active == 0U) &&
                (outer_radius > MUSIC_VISUAL_MAX_RADIUS))
            {
                outer_radius = MUSIC_VISUAL_MAX_RADIUS;
            }
            music_visual_draw_bar(layer, center_x, center_y, index,
                                  inner_radius, outer_radius);
        }
    }
}

static void music_visual_update_intro(music_visual_t *visual,
                                      uint32_t now_ms)
{
    uint32_t elapsed_ms = now_ms - visual->intro_started_ms;
    uint8_t content_opa;
    uint32_t index;

    if ((elapsed_ms >= MUSIC_VISUAL_INTRO_BARS_MS) &&
        (visual->intro_bars_visible == 0U))
    {
        visual->intro_bars_visible = 1U;
    }

    for (index = 0U; index < MUSIC_VISUAL_BAR_COUNT; index++)
    {
        uint32_t delay_ms = MUSIC_VISUAL_INTRO_BARS_MS +
                            (g_music_visual_random[index] % 200U);
        uint32_t duration_ms = 2500U +
                               (g_music_visual_random[index] % 500U);
        uint32_t value;

        if (elapsed_ms <= delay_ms)
        {
            value = 1024U;
        }
        else if (elapsed_ms >= (delay_ms + duration_ms))
        {
            value = 5U;
        }
        else
        {
            value = 1024U -
                ((elapsed_ms - delay_ms) * 1019U) / duration_ms;
        }
        visual->intro_values[index] = (uint16_t)value;
    }
    content_opa = music_visual_fade_value(elapsed_ms,
                                          MUSIC_VISUAL_CONTENT_IN_MS,
                                          1000U);
    for (index = 0U; index < 4U; index++)
    {
        lv_obj_set_style_opa(visual->fade_objects[index],
                             content_opa, LV_PART_MAIN);
    }
    lv_obj_set_style_opa(visual->cover, content_opa, LV_PART_MAIN);
    if (elapsed_ms < 2800U)
    {
        lv_image_set_scale(visual->logo, 256U);
    }
    else if (elapsed_ms < 3200U)
    {
        lv_image_set_scale(visual->logo,
            (uint16_t)(256U - ((elapsed_ms - 2800U) * 246U) / 400U));
    }
    else
    {
        lv_obj_add_flag(visual->logo, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_set_style_opa(visual->logo_title,
        (lv_opa_t)(LV_OPA_COVER -
        music_visual_fade_value(elapsed_ms, 2000U, 500U)), LV_PART_MAIN);
    lv_obj_invalidate(visual->spectrum);
    if (music_visual_time_reached(elapsed_ms,
                                  MUSIC_VISUAL_INTRO_DONE_MS) != 0)
    {
        visual->intro_active = 0U;
        lv_obj_add_flag(visual->logo_title, LV_OBJ_FLAG_HIDDEN);
        lv_obj_refresh_ext_draw_size(visual->spectrum);
    }
}

static void music_visual_update_radii(music_visual_t *visual,
                                      uint32_t spectrum_index,
                                      uint16_t cover_scale)
{
    static const uint8_t band_widths[4] = {20U, 8U, 4U, 2U};
    uint16_t inner_radius = (uint16_t)(
        ((uint32_t)MUSIC_VISUAL_INNER_RADIUS * cover_scale) / 256U);
    uint32_t band;
    uint32_t index;

    for (index = 0U; index < MUSIC_VISUAL_BAR_COUNT; index++)
    {
        visual->radii[index] = inner_radius + 5U;
    }
    for (band = 0U; band < 4U; band++)
    {
        uint32_t width = band_widths[band];
        uint32_t frequency;

        for (frequency = 0U; frequency < width; frequency++)
        {
            int32_t target = (int32_t)(5U * band) -
                             (int32_t)(width / 2U) +
                             (int32_t)frequency;
            int32_t modifier = music_visual_cos(
                (int32_t)(frequency * 360U / width) + 180, 180) + 180;
            uint32_t amplitude = spectrum_1[spectrum_index][band];

            while (target < 0)
            {
                target += (int32_t)MUSIC_VISUAL_BAR_COUNT;
            }
            target %= (int32_t)MUSIC_VISUAL_BAR_COUNT;
            visual->radii[target] = (uint16_t)(
                visual->radii[target] +
                ((amplitude * (uint32_t)modifier) >> 9));
        }
    }
}

void music_visual_init(music_visual_t *visual)
{
    uint32_t index;

    if (visual == NULL)
    {
        return;
    }
    memset(visual, 0, sizeof(*visual));
    visual->last_phase = UINT32_MAX;
    for (index = 0U; index < MUSIC_VISUAL_BAR_COUNT; index++)
    {
        visual->radii[index] = MUSIC_VISUAL_INNER_RADIUS + 5U;
        visual->intro_values[index] = 1024U;
    }
}

void music_visual_create_background(lv_obj_t *parent)
{
    if (parent != NULL)
    {
        lv_obj_set_style_bg_color(parent, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, LV_PART_MAIN);
    }
}

lv_obj_t *music_visual_create_image_button(lv_obj_t *parent,
                                            const lv_image_dsc_t *source,
                                            lv_obj_t **image_out)
{
    lv_obj_t *button;
    lv_obj_t *image;

    if ((parent == NULL) || (source == NULL))
    {
        return NULL;
    }
    button = lv_button_create(parent);
    lv_obj_remove_style_all(button);
    lv_obj_set_size(button, source->header.w, source->header.h);
    image = lv_image_create(button);
    lv_image_set_src(image, source);
    lv_obj_center(image);
    lv_obj_clear_flag(image, LV_OBJ_FLAG_CLICKABLE);
    if (image_out != NULL)
    {
        *image_out = image;
    }
    return button;
}

void music_visual_create_spectrum(music_visual_t *visual,
                                  lv_obj_t *parent,
                                  const lv_image_dsc_t *cover_source)
{
    if ((visual == NULL) || (parent == NULL) || (cover_source == NULL))
    {
        return;
    }
    visual->spectrum = lv_obj_create(parent);
    lv_obj_remove_style_all(visual->spectrum);
    lv_obj_set_size(visual->spectrum,
                    MUSIC_VISUAL_SPECTRUM_SIZE,
                    MUSIC_VISUAL_SPECTRUM_SIZE);
    lv_obj_clear_flag(visual->spectrum, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(visual->spectrum, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(visual->spectrum, music_visual_spectrum_event,
                        LV_EVENT_ALL, visual);
    visual->cover = lv_image_create(visual->spectrum);
    lv_image_set_src(visual->cover, cover_source);
    lv_image_set_antialias(visual->cover, false);
    lv_obj_center(visual->cover);
    lv_obj_clear_flag(visual->cover, LV_OBJ_FLAG_CLICKABLE);
}

void music_visual_start_intro(music_visual_t *visual,
                              lv_obj_t *parent,
                              lv_obj_t *title_box,
                              lv_obj_t *icon_box,
                              lv_obj_t *control_box,
                              lv_obj_t *handle_box,
                              uint32_t now_ms)
{
    uint32_t index;

    if ((visual == NULL) || (parent == NULL) ||
        (visual->spectrum == NULL) || (visual->cover == NULL))
    {
        return;
    }
    visual->fade_objects[0] = title_box;
    visual->fade_objects[1] = icon_box;
    visual->fade_objects[2] = control_box;
    visual->fade_objects[3] = handle_box;
    for (index = 0U; index < 4U; index++)
    {
        lv_obj_set_style_opa(visual->fade_objects[index],
                             LV_OPA_TRANSP, LV_PART_MAIN);
    }
    lv_obj_set_style_opa(visual->cover, LV_OPA_TRANSP, LV_PART_MAIN);
    visual->logo = lv_image_create(parent);
    lv_image_set_src(visual->logo, &img_lv_demo_music_logo);
    /* Keep intro objects out of the player's Grid layout. */
    lv_obj_add_flag(visual->logo, LV_OBJ_FLAG_FLOATING);
    lv_obj_align_to(visual->logo, visual->spectrum,
                    LV_ALIGN_CENTER, 0, 0);
    lv_obj_move_foreground(visual->logo);
    visual->logo_title = lv_label_create(parent);
    lv_obj_add_flag(visual->logo_title, LV_OBJ_FLAG_FLOATING);
    lv_label_set_text(visual->logo_title, "LVGL Demo\nMusic player");
    lv_obj_set_style_text_align(visual->logo_title,
                                LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(visual->logo_title,
                               &lv_font_montserratMedium_16, LV_PART_MAIN);
    lv_obj_align_to(visual->logo_title, visual->logo,
                    LV_ALIGN_OUT_LEFT_MID, -20, 0);
    lv_obj_move_foreground(visual->logo_title);
    visual->intro_started_ms = now_ms;
    visual->intro_active = 1U;
    visual->intro_bars_visible = 0U;
    lv_obj_refresh_ext_draw_size(visual->spectrum);
}

void music_visual_update_spectrum(music_visual_t *visual,
                                  uint32_t elapsed_ms,
                                  bool playing,
                                  uint32_t now_ms)
{
    uint32_t phase;
    uint32_t spectrum_length =
        (uint32_t)(sizeof(spectrum_1) / sizeof(spectrum_1[0]));

    if ((visual == NULL) || (visual->spectrum == NULL))
    {
        return;
    }
    if (visual->intro_active != 0U)
    {
        music_visual_update_intro(visual, now_ms);
        return;
    }
    phase = playing ? (elapsed_ms / 33U) : 0U;
    if ((phase == visual->last_phase) &&
        (visual->was_playing == (uint8_t)(playing ? 1U : 0U)))
    {
        return;
    }
    visual->last_phase = phase;
    visual->was_playing = playing ? 1U : 0U;
    if (playing)
    {
        uint32_t spectrum_index = phase % spectrum_length;
        uint16_t cover_scale =
            (uint16_t)(256U + spectrum_1[spectrum_index][0]);

        lv_image_set_scale(visual->cover, cover_scale);
        music_visual_update_radii(visual, spectrum_index, cover_scale);
    }
    else
    {
        uint32_t index;

        for (index = 0U; index < MUSIC_VISUAL_BAR_COUNT; index++)
        {
            visual->radii[index] = MUSIC_VISUAL_INNER_RADIUS + 5U;
        }
        lv_image_set_scale(visual->cover, 256U);
    }
    lv_obj_invalidate(visual->spectrum);
}
