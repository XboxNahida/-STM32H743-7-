#include "drawing_app_ui.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "drawing_bmp_writer.h"
#include "drawing_bmp_reader.h"
#include "drawing_history.h"

#define DRAWING_CANVAS_WIDTH       866U
#define DRAWING_CANVAS_HEIGHT      454U
#define DRAWING_STROKE_CAPACITY    128U
#define DRAWING_POINT_CAPACITY     8192U
#define DRAWING_DEFAULT_WIDTH      8U
#define DRAWING_BACKGROUND_HEX     0xF4F2EDU
#define DRAWING_ACTIVE_HEX         0xE8BD4BU
#define DRAWING_BUTTON_HEX         0x252A2DU
#define DRAWING_LIGHT_TEXT_HEX     0xF4F6F7U
#define DRAWING_DARK_TEXT_HEX      0x171A1CU

typedef struct
{
    lv_ui *ui;
    lv_obj_t *canvas;
    lv_obj_t *status_label;
    lv_obj_t *swatches[DRAWING_COLOR_COUNT];
    void *canvas_buffer;
    void *base_buffer;
    drawing_stroke_t *strokes;
    drawing_point_t *points;
    drawing_history_t history;
    uint32_t canvas_stride;
    uint8_t selected_color;
    uint8_t width;
    uint8_t eraser;
    uint8_t save_pending;
    uint8_t save_overwrite;
    char opened_path[512];
    lv_obj_t *save_dialog;
} drawing_session_t;

static const uint32_t g_drawing_color_hex[DRAWING_COLOR_COUNT] = {
    0x1F2325U, 0xE3AD37U, 0xD95D58U,
    0x3D9A6BU, 0x2990B7U, 0xF4F2EDU
};

static drawing_session_t *g_drawing_session;
static char g_drawing_pending_path[512];

static lv_color_t drawing_color_from_rgb565(uint16_t color)
{
    return lv_color_make((uint8_t)(((color >> 11) & 0x1FU) << 3),
                         (uint8_t)(((color >> 5) & 0x3FU) << 2),
                         (uint8_t)((color & 0x1FU) << 3));
}

static void drawing_set_button_selected(lv_obj_t *button, uint8_t selected)
{
    lv_obj_t *label;

    if (button == NULL)
    {
        return;
    }
    lv_obj_set_style_bg_color(button,
        lv_color_hex((selected != 0U) ? DRAWING_ACTIVE_HEX :
                                      DRAWING_BUTTON_HEX),
        LV_PART_MAIN | LV_STATE_DEFAULT);
    label = lv_obj_get_child(button, 0);
    if (label != NULL)
    {
        lv_obj_set_style_text_color(label,
            lv_color_hex((selected != 0U) ? DRAWING_DARK_TEXT_HEX :
                                          DRAWING_LIGHT_TEXT_HEX),
            LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

static void drawing_update_controls(drawing_session_t *session)
{
    uint32_t index;
    uint8_t can_undo;
    uint8_t can_redo;

    drawing_set_button_selected(session->ui->screen_drawing_pen_btn,
                                (session->eraser == 0U) ? 1U : 0U);
    drawing_set_button_selected(session->ui->screen_drawing_eraser_btn,
                                session->eraser);
    for (index = 0U; index < DRAWING_COLOR_COUNT; index++)
    {
        lv_obj_set_style_border_width(session->swatches[index],
            ((session->eraser == 0U) &&
             (index == session->selected_color)) ? 3 : 1,
            LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    can_undo = (session->history.visible_count != 0U) ? 1U : 0U;
    can_redo = (session->history.visible_count <
                session->history.stroke_count) ? 1U : 0U;
    if (can_undo != 0U)
    {
        lv_obj_remove_state(session->ui->screen_drawing_undo_btn,
                            LV_STATE_DISABLED);
    }
    else
    {
        lv_obj_add_state(session->ui->screen_drawing_undo_btn,
                         LV_STATE_DISABLED);
    }
    if (can_redo != 0U)
    {
        lv_obj_remove_state(session->ui->screen_drawing_redo_btn,
                            LV_STATE_DISABLED);
    }
    else
    {
        lv_obj_add_state(session->ui->screen_drawing_redo_btn,
                         LV_STATE_DISABLED);
    }
}

static void drawing_draw_segment(lv_layer_t *layer,
                                 const drawing_stroke_t *stroke,
                                 const drawing_point_t *from,
                                 const drawing_point_t *to)
{
    lv_draw_line_dsc_t line;

    lv_draw_line_dsc_init(&line);
    line.p1.x = from->x;
    line.p1.y = from->y;
    line.p2.x = to->x;
    line.p2.y = to->y;
    line.color = drawing_color_from_rgb565(stroke->color_rgb565);
    line.width = stroke->width;
    line.opa = LV_OPA_COVER;
    line.round_start = 1U;
    line.round_end = 1U;
    lv_draw_line(layer, &line);
}

static void drawing_render_all(drawing_session_t *session)
{
    lv_layer_t layer;
    uint32_t stroke_index;

    if (session->base_buffer != NULL)
        memcpy(session->canvas_buffer, session->base_buffer,
               (size_t)session->canvas_stride * DRAWING_CANVAS_HEIGHT);
    else
        lv_canvas_fill_bg(session->canvas,
                          lv_color_hex(DRAWING_BACKGROUND_HEX), LV_OPA_COVER);
    lv_canvas_init_layer(session->canvas, &layer);
    for (stroke_index = 0U;
         stroke_index < drawing_history_visible_count(&session->history);
         stroke_index++)
    {
        const drawing_stroke_t *stroke = drawing_history_get_stroke(
            &session->history, stroke_index);
        uint32_t point_index;

        if ((stroke == NULL) || (stroke->point_count == 0U))
        {
            continue;
        }
        if (stroke->point_count == 1U)
        {
            const drawing_point_t *point = drawing_history_get_point(
                &session->history, stroke, 0U);
            drawing_draw_segment(&layer, stroke, point, point);
            continue;
        }
        for (point_index = 1U; point_index < stroke->point_count;
             point_index++)
        {
            const drawing_point_t *from = drawing_history_get_point(
                &session->history, stroke, point_index - 1U);
            const drawing_point_t *to = drawing_history_get_point(
                &session->history, stroke, point_index);
            drawing_draw_segment(&layer, stroke, from, to);
        }
    }
    lv_canvas_finish_layer(session->canvas, &layer);
}

static void drawing_render_latest(drawing_session_t *session,
                                  uint32_t previous_point_count)
{
    const drawing_stroke_t *stroke;
    const drawing_point_t *from;
    const drawing_point_t *to;
    lv_layer_t layer;

    stroke = drawing_history_get_stroke(
        &session->history, session->history.visible_count - 1U);
    if ((stroke == NULL) || (stroke->point_count == 0U) ||
        (stroke->point_count == previous_point_count))
    {
        return;
    }
    to = drawing_history_get_point(&session->history, stroke,
                                   stroke->point_count - 1U);
    from = (stroke->point_count == 1U) ? to : drawing_history_get_point(
        &session->history, stroke, stroke->point_count - 2U);
    if ((from == NULL) || (to == NULL))
    {
        return;
    }
    lv_canvas_init_layer(session->canvas, &layer);
    drawing_draw_segment(&layer, stroke, from, to);
    lv_canvas_finish_layer(session->canvas, &layer);
}

static uint8_t drawing_get_local_point(lv_event_t *event,
                                       drawing_session_t *session,
                                       int16_t *x, int16_t *y)
{
    lv_indev_t *indev = lv_event_get_indev(event);
    lv_point_t point;
    lv_area_t area;
    int32_t local_x;
    int32_t local_y;

    if ((indev == NULL) || (x == NULL) || (y == NULL))
    {
        return 0U;
    }
    lv_indev_get_point(indev, &point);
    lv_obj_get_coords(session->canvas, &area);
    local_x = point.x - area.x1;
    local_y = point.y - area.y1;
    if ((local_x < 0) || (local_y < 0) ||
        (local_x >= (int32_t)DRAWING_CANVAS_WIDTH) ||
        (local_y >= (int32_t)DRAWING_CANVAS_HEIGHT))
    {
        return 0U;
    }
    *x = (int16_t)local_x;
    *y = (int16_t)local_y;
    return 1U;
}

static void drawing_canvas_event(lv_event_t *event)
{
    drawing_session_t *session = lv_event_get_user_data(event);
    lv_event_code_t code = lv_event_get_code(event);
    int16_t x;
    int16_t y;

    if (session == NULL)
    {
        return;
    }
    if ((code == LV_EVENT_PRESSED) &&
        (drawing_get_local_point(event, session, &x, &y) != 0U))
    {
        uint16_t color = lv_color_to_u16(lv_color_hex(
            (session->eraser != 0U) ? DRAWING_BACKGROUND_HEX :
            g_drawing_color_hex[session->selected_color]));
        if (drawing_history_begin(&session->history, x, y, color,
                                  session->width, session->eraser) ==
            DRAWING_HISTORY_OK)
        {
            drawing_render_latest(session, 0U);
            drawing_update_controls(session);
        }
    }
    else if ((code == LV_EVENT_PRESSING) &&
             (session->history.active != 0U) &&
             (drawing_get_local_point(event, session, &x, &y) != 0U))
    {
        const drawing_stroke_t *stroke = drawing_history_get_stroke(
            &session->history, session->history.visible_count - 1U);
        uint32_t previous_count = (stroke != NULL) ? stroke->point_count : 0U;
        if (drawing_history_append(&session->history, x, y) ==
            DRAWING_HISTORY_OK)
        {
            drawing_render_latest(session, previous_count);
        }
    }
    else if (((code == LV_EVENT_RELEASED) ||
              (code == LV_EVENT_PRESS_LOST)) &&
             (session->history.active != 0U))
    {
        (void)drawing_history_end(&session->history);
        drawing_update_controls(session);
    }
}

static void drawing_tool_event(lv_event_t *event)
{
    drawing_session_t *session = lv_event_get_user_data(event);
    lv_obj_t *target;

    if ((session == NULL) ||
        (lv_event_get_code(event) != LV_EVENT_CLICKED))
    {
        return;
    }
    target = lv_event_get_target_obj(event);
    if (target == session->ui->screen_drawing_pen_btn)
    {
        session->eraser = 0U;
    }
    else if (target == session->ui->screen_drawing_eraser_btn)
    {
        session->eraser = 1U;
    }
    drawing_update_controls(session);
}

static void drawing_swatch_event(lv_event_t *event)
{
    drawing_session_t *session = lv_event_get_user_data(event);
    lv_obj_t *target;
    uint32_t index;

    if ((session == NULL) ||
        (lv_event_get_code(event) != LV_EVENT_CLICKED))
    {
        return;
    }
    target = lv_event_get_target_obj(event);
    for (index = 0U; index < DRAWING_COLOR_COUNT; index++)
    {
        if (target == session->swatches[index])
        {
            session->selected_color = (uint8_t)index;
            session->eraser = 0U;
            drawing_update_controls(session);
            return;
        }
    }
}

static void drawing_width_event(lv_event_t *event)
{
    drawing_session_t *session = lv_event_get_user_data(event);
    int32_t value;

    if ((session == NULL) ||
        (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED))
    {
        return;
    }
    value = lv_slider_get_value(session->ui->screen_drawing_width_slider);
    if (value < 1) value = 1;
    if (value > 32) value = 32;
    session->width = (uint8_t)value;
}

static void drawing_action_event(lv_event_t *event)
{
    drawing_session_t *session = lv_event_get_user_data(event);
    lv_obj_t *target;
    int result = DRAWING_HISTORY_EMPTY;

    if ((session == NULL) ||
        (lv_event_get_code(event) != LV_EVENT_CLICKED) ||
        (session->history.active != 0U))
    {
        return;
    }
    target = lv_event_get_target_obj(event);
    if (target == session->ui->screen_drawing_undo_btn)
    {
        result = drawing_history_undo(&session->history);
    }
    else if (target == session->ui->screen_drawing_redo_btn)
    {
        result = drawing_history_redo(&session->history);
    }
    else if (target == session->ui->screen_drawing_clear_btn)
    {
        drawing_history_clear(&session->history);
        result = DRAWING_HISTORY_OK;
    }
    if (result == DRAWING_HISTORY_OK)
    {
        drawing_render_all(session);
        drawing_update_controls(session);
    }
}

static void drawing_set_save_status(drawing_session_t *session,
                                    const char *text, uint32_t color)
{
    lv_label_set_text(session->status_label, text);
    lv_obj_set_style_text_color(session->status_label, lv_color_hex(color),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void drawing_save_async(void *user_data)
{
    drawing_session_t *session = user_data;
    char path[DRAWING_BMP_PATH_CAPACITY];
    const char *name;
    int result;

    if ((session == NULL) || (g_drawing_session != session))
    {
        return;
    }
    session->save_pending = 0U;
    lv_refr_now(NULL);
    if ((session->save_overwrite != 0U) && (session->opened_path[0] != '\0'))
    {
        result = drawing_bmp_save_overwrite(
            (const uint8_t *)session->canvas_buffer,
            DRAWING_CANVAS_WIDTH, DRAWING_CANVAS_HEIGHT,
            session->canvas_stride, session->opened_path);
        if (result == DRAWING_BMP_OK)
        {
            strncpy(path, session->opened_path, sizeof(path) - 1U);
            path[sizeof(path) - 1U] = '\0';
        }
    }
    else
        result = drawing_bmp_save_next(
            (const uint8_t *)session->canvas_buffer,
            DRAWING_CANVAS_WIDTH, DRAWING_CANVAS_HEIGHT,
            session->canvas_stride, path, sizeof(path));
    if (result == DRAWING_BMP_OK)
    {
        strncpy(session->opened_path, path, sizeof(session->opened_path) - 1U);
        session->opened_path[sizeof(session->opened_path) - 1U] = '\0';
        name = strrchr(path, '/');
        if (name == NULL) name = path;
        else name++;
        lv_label_set_text_fmt(session->status_label,
            "\xE5\xB7\xB2\xE4\xBF\x9D\xE5\xAD\x98: %s", name);
        lv_obj_set_style_text_color(session->status_label,
            lv_color_hex(0x3D9A6BU), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    else if (result == DRAWING_BMP_NOT_READY)
    {
        drawing_set_save_status(session,
            "SD\xE5\x8D\xA1\xE6\x9C\xAA\xE5\xB0\xB1\xE7\xBB\xAA",
            0xD95D58U);
    }
    else
    {
        drawing_set_save_status(session,
            "\xE4\xBF\x9D\xE5\xAD\x98\xE5\xA4\xB1\xE8\xB4\xA5",
            0xD95D58U);
    }
}

static void drawing_start_save(drawing_session_t *session, uint8_t overwrite)
{
    session->save_overwrite = overwrite;
    session->save_pending = 1U;
    drawing_set_save_status(session,
        "\xE6\xAD\xA3\xE5\x9C\xA8\xE4\xBF\x9D\xE5\xAD\x98...", 0xE3AD37U);
    if (lv_async_call(drawing_save_async, session) != LV_RESULT_OK)
    {
        session->save_pending = 0U;
        drawing_set_save_status(session,
            "\xE4\xBF\x9D\xE5\xAD\x98\xE5\xA4\xB1\xE8\xB4\xA5", 0xD95D58U);
    }
}

static void drawing_save_choice_event(lv_event_t *event)
{
    drawing_session_t *session = lv_event_get_user_data(event);
    intptr_t choice;
    if ((session == NULL) || (lv_event_get_code(event) != LV_EVENT_CLICKED)) return;
    choice = (intptr_t)lv_obj_get_user_data(lv_event_get_current_target(event));
    if ((session->save_dialog != NULL) && lv_obj_is_valid(session->save_dialog))
        lv_obj_delete(session->save_dialog);
    session->save_dialog = NULL;
    if (choice >= 0) drawing_start_save(session, (choice == 1) ? 1U : 0U);
}

static lv_obj_t *drawing_choice_button(drawing_session_t *session,
    const char *text, int32_t x, intptr_t choice)
{
    lv_obj_t *button = lv_button_create(session->save_dialog);
    lv_obj_t *label = lv_label_create(button);
    lv_obj_set_pos(button, x, 105);
    lv_obj_set_size(button, 135, 48);
    lv_obj_set_style_radius(button, 6, 0);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &lv_font_myyh_20, 0);
    lv_obj_center(label);
    lv_obj_set_user_data(button, (void *)choice);
    lv_obj_add_event_cb(button, drawing_save_choice_event,
                        LV_EVENT_CLICKED, session);
    return button;
}

static void drawing_show_save_dialog(drawing_session_t *session)
{
    lv_obj_t *label;
    session->save_dialog = lv_obj_create(session->ui->screen_drawing);
    lv_obj_set_size(session->save_dialog, 540, 180);
    lv_obj_center(session->save_dialog);
    lv_obj_set_style_bg_color(session->save_dialog, lv_color_hex(0x202830), 0);
    label = lv_label_create(session->save_dialog);
    lv_label_set_text(label, "保存已打开的画板文件");
    lv_obj_set_style_text_font(label, &lv_font_myyh_20, 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 18);
    (void)drawing_choice_button(session, "取消", 25, -1);
    (void)drawing_choice_button(session, "覆盖原文件", 195, 1);
    (void)drawing_choice_button(session, "另存新文件", 365, 0);
}

static void drawing_save_event(lv_event_t *event)
{
    drawing_session_t *session = lv_event_get_user_data(event);

    if ((session == NULL) ||
        (lv_event_get_code(event) != LV_EVENT_CLICKED) ||
        (session->save_pending != 0U))
    {
        return;
    }
    if (session->opened_path[0] != '\0') drawing_show_save_dialog(session);
    else drawing_start_save(session, 0U);
}

static void drawing_screen_delete_event(lv_event_t *event)
{
    drawing_session_t *session = lv_event_get_user_data(event);

    if ((session == NULL) ||
        (lv_event_get_code(event) != LV_EVENT_DELETE))
    {
        return;
    }
    if (session->save_pending != 0U)
    {
        (void)lv_async_call_cancel(drawing_save_async, session);
        session->save_pending = 0U;
    }
    if ((session->save_dialog != NULL) && lv_obj_is_valid(session->save_dialog))
    {
        lv_obj_delete(session->save_dialog);
        session->save_dialog = NULL;
    }
    if ((session->canvas != NULL) && lv_obj_is_valid(session->canvas))
    {
        lv_obj_delete(session->canvas);
        session->canvas = NULL;
    }
    lv_free(session->points);
    lv_free(session->strokes);
    lv_free(session->canvas_buffer);
    lv_free(session->base_buffer);
    if (g_drawing_session == session)
    {
        g_drawing_session = NULL;
    }
    lv_free(session);
}

static void drawing_show_allocation_error(lv_ui *ui)
{
    lv_obj_t *label = lv_label_create(ui->screen_drawing);
    lv_label_set_text(label, "\xE7\x94\xBB\xE5\xB8\x83\xE5\x86\x85\xE5\xAD\x98\xE4\xB8\x8D\xE8\xB6\xB3");
    lv_obj_set_style_text_font(label, &lv_font_myyh_20,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label, lv_color_hex(0xD95D58U),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(label);
}

int drawing_app_ui_attach(lv_ui *ui,
                          lv_obj_t *swatches[DRAWING_COLOR_COUNT])
{
    drawing_session_t *session;
    uint32_t index;
    uint32_t stride;
    size_t canvas_size;

    if ((ui == NULL) || (ui->screen_drawing == NULL) ||
        (ui->screen_drawing_canvas == NULL) || (swatches == NULL) ||
        (g_drawing_session != NULL))
    {
        return 1;
    }
    for (index = 0U; index < DRAWING_COLOR_COUNT; index++)
    {
        if (swatches[index] == NULL)
        {
            return 1;
        }
    }

    session = lv_malloc(sizeof(*session));
    if (session == NULL)
    {
        drawing_show_allocation_error(ui);
        return 1;
    }
    (void)memset(session, 0, sizeof(*session));
    stride = lv_draw_buf_width_to_stride(DRAWING_CANVAS_WIDTH,
                                         LV_COLOR_FORMAT_RGB565);
    canvas_size = (size_t)stride * DRAWING_CANVAS_HEIGHT;
    session->canvas_buffer = lv_malloc(canvas_size);
    session->base_buffer = lv_malloc(canvas_size);
    session->strokes = lv_malloc(sizeof(*session->strokes) *
                                 DRAWING_STROKE_CAPACITY);
    session->points = lv_malloc(sizeof(*session->points) *
                                DRAWING_POINT_CAPACITY);
    if ((session->canvas_buffer == NULL) || (session->base_buffer == NULL) ||
        (session->strokes == NULL) ||
        (session->points == NULL))
    {
        lv_free(session->points);
        lv_free(session->strokes);
        lv_free(session->canvas_buffer);
        lv_free(session->base_buffer);
        lv_free(session);
        drawing_show_allocation_error(ui);
        return 1;
    }

    session->ui = ui;
    session->canvas = ui->screen_drawing_canvas;
    session->selected_color = 0U;
    session->width = DRAWING_DEFAULT_WIDTH;
    session->canvas_stride = stride;
    if (g_drawing_pending_path[0] != '\0')
    {
        strncpy(session->opened_path, g_drawing_pending_path,
                sizeof(session->opened_path) - 1U);
        session->opened_path[sizeof(session->opened_path) - 1U] = '\0';
    }
    for (index = 0U; index < DRAWING_COLOR_COUNT; index++)
    {
        session->swatches[index] = swatches[index];
    }
    (void)drawing_history_init(&session->history, session->strokes,
        DRAWING_STROKE_CAPACITY, session->points, DRAWING_POINT_CAPACITY);
    lv_canvas_set_buffer(session->canvas, session->canvas_buffer,
                         DRAWING_CANVAS_WIDTH, DRAWING_CANVAS_HEIGHT,
                         LV_COLOR_FORMAT_RGB565);
    if ((g_drawing_pending_path[0] != '\0') &&
        (drawing_bmp_read_rgb565(g_drawing_pending_path,
            (uint8_t *)session->base_buffer, DRAWING_CANVAS_WIDTH,
            DRAWING_CANVAS_HEIGHT, stride) == 0))
        memcpy(session->canvas_buffer, session->base_buffer, canvas_size);
    else
    {
        lv_canvas_fill_bg(session->canvas,
                          lv_color_hex(DRAWING_BACKGROUND_HEX), LV_OPA_COVER);
        memcpy(session->base_buffer, session->canvas_buffer, canvas_size);
    }
    g_drawing_pending_path[0] = '\0';
    session->status_label = lv_label_create(ui->screen_drawing_cont_1);
    lv_label_set_text(session->status_label, "");
    lv_obj_set_pos(session->status_label, 105, 5);
    lv_obj_set_size(session->status_label, 340, 30);
    lv_obj_set_style_text_align(session->status_label, LV_TEXT_ALIGN_RIGHT,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(session->status_label, &lv_font_myyh_20,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(session->canvas, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(session->canvas, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(session->canvas, drawing_canvas_event,
                        LV_EVENT_ALL, session);
    lv_obj_add_event_cb(ui->screen_drawing_pen_btn, drawing_tool_event,
                        LV_EVENT_CLICKED, session);
    lv_obj_add_event_cb(ui->screen_drawing_eraser_btn, drawing_tool_event,
                        LV_EVENT_CLICKED, session);
    for (index = 0U; index < DRAWING_COLOR_COUNT; index++)
    {
        lv_obj_add_event_cb(session->swatches[index], drawing_swatch_event,
                            LV_EVENT_CLICKED, session);
    }
    lv_obj_add_event_cb(ui->screen_drawing_width_slider,
                        drawing_width_event, LV_EVENT_VALUE_CHANGED, session);
    lv_obj_add_event_cb(ui->screen_drawing_undo_btn, drawing_action_event,
                        LV_EVENT_CLICKED, session);
    lv_obj_add_event_cb(ui->screen_drawing_redo_btn, drawing_action_event,
                        LV_EVENT_CLICKED, session);
    lv_obj_add_event_cb(ui->screen_drawing_clear_btn, drawing_action_event,
                        LV_EVENT_CLICKED, session);
    lv_obj_add_event_cb(ui->screen_drawing_save_btn, drawing_save_event,
                        LV_EVENT_CLICKED, session);
    lv_obj_add_event_cb(ui->screen_drawing, drawing_screen_delete_event,
                        LV_EVENT_DELETE, session);
    g_drawing_session = session;
    drawing_update_controls(session);
    return 0;
}

int drawing_app_ui_request_open_file(const char *path)
{
    size_t length;
    if ((path == NULL) || (g_drawing_session != NULL) ||
        (guider_ui.DeskTop == NULL) ||
        (lv_screen_active() != guider_ui.DeskTop)) return 1;
    length = strlen(path);
    if ((length == 0U) || (length >= sizeof(g_drawing_pending_path))) return 1;
    memcpy(g_drawing_pending_path, path, length + 1U);
    ui_load_scr_animation(&guider_ui, &guider_ui.screen_drawing,
        guider_ui.screen_drawing_del, &guider_ui.DeskTop_del,
        setup_scr_screen_drawing, LV_SCR_LOAD_ANIM_OVER_TOP,
        200, 0, true, true);
    return 0;
}
