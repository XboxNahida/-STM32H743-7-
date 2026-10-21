#ifndef OSCILLOSCOPE_UI_PRIVATE_H
#define OSCILLOSCOPE_UI_PRIVATE_H
#include "oscilloscope_ui.h"
#include "oscilloscope_session.h"
#include "oscilloscope_storage.h"
#include "oscilloscope_accurate.h"
#include "lv_port_indev.h"
#include "oscilloscope_zoom.h"
#include "oscilloscope_driver.h"
#include "oscilloscope_format.h"
#define SCOPE_PLOT_WIDTH 652
#define SCOPE_HISTORY 4
typedef struct {
    scope_session_t session;
    scope_settings_t settings;
    lv_obj_t *screen,*plot,*status,*notice,*measure,*cursor_label,*entry,*entry_desktop;
    lv_obj_t *keyboard,*tabs,*run_label;
    lv_obj_t *accurate_button,*measure_page,*measure_summary;
    lv_obj_t *measure_values[10];
    float follow_plot_mv[2];
    uint32_t follow_valid;
    uint8_t accurate_source;
    lv_obj_t *auto_button,*channel_label,*channel_label2,*time_label,*footer,*readings[4],*reading_titles[4];
    scope_settings_t smooth_settings;
    scope_settings_t auto_settings;
    size_t accurate_samples;
    uint32_t accurate_attempt_sequence,pinch_release_ms,pinch_preview_ms,auto_sequence,auto_started_ms;
    float *display,*average,*fft;
    float *fft2; /* 流畅FFT复用闲置准确计算区，准确模式禁止显示FFT。 */
    float fft_span_hz;
    float fft_frequency_scale,fft_vertical_scale,fft_pan_x,fft_pan_y;
    uint8_t fft_linear;
    uint8_t fft_mask,keyboard_enabled;
    int16_t history[SCOPE_HISTORY][SCOPE_PLOT_WIDTH][2];
    int16_t sample_points[500][2];
    float sample_values[500];
    float sample_values2[500],display2[500];
    int16_t history2[SCOPE_HISTORY][SCOPE_PLOT_WIDTH][2];
    int16_t sample_points2[500][2];
    size_t sample_point_count;
    uint32_t average_frames,last_refresh_ms,status_until_ms;
    uint32_t last_plot_ms,last_fft_ms,last_accurate_ms,accurate_sequence;
    uint32_t fft_rate_hz,fft_sequence;
    uint32_t display_rate_hz,display_sequence;
    float reference_mv;
    size_t view_start,view_count,display_count,fft_count,measurement_count;
    size_t accurate_pending_count,accurate_record_count;
    lv_point_t pinch_start_a,pinch_start_b,pinch_last_a,pinch_last_b;
    float pinch_start_time_div,pinch_start_vertical_scale,pinch_start_fft_scale;
    float pinch_vertical_center_mv,pinch_start_vertical_offset_mv;
    float pinch_vertical_center2_mv,pinch_start_vertical_offset2_mv,plot_vertical_offset2_mv;
    float plot_vertical_offset_mv;
    float plot_vertical_scale,fft_horizontal_scale,drag_cursor_distance;
    float plot_vertical_scale2,pan_x2,pan_y2,channel_time_div[2],view_span[2],view_origin[2];
    float sampling_time_div;
    uint32_t average_frames2;
    size_t view_start2,view_count2,display_count2,sample_point_count2;
    uint8_t selected_channel;
    uint8_t trigger_source,auto_trigger_source;
    float trigger_hysteresis_mv,auto_hysteresis_mv,auto_time_div;
    lv_point_t drag_start,drag_last;
    float pan_x,pan_y,drag_pan_x,drag_pan_y,drag_cursor_x,drag_cursor_mv;
    uint32_t drag_draw_ms;
    uint8_t drag_active,drag_axis,drag_locked,drag_was_running;
    uint8_t touch_blocked;
    int8_t drag_time_candidate,drag_voltage_candidate,drag_cursor,drag_cursor_candidate;
    uint8_t pinch_active,pinch_axis,pinch_consumed;
    uint8_t pinch_was_running,accurate_resume_running,auto_active;
    uint8_t auto_was_running;
    uint8_t history_count,history_next,fft_enabled,initialized;
    uint8_t xy_enabled;
    uint8_t accurate_busy,accurate_ready;
    uint8_t accurate_have_info;
    scope_accurate_info_t accurate_info;
    uint8_t plot_pending;
} scope_ui_t;
extern scope_ui_t g_scope_ui;
lv_obj_t *scope_ui_button(lv_obj_t *parent,const char *text,int x,int y,int w,int h,
                          lv_event_cb_t callback,void *data);
void scope_ui_status(const char *text);
void scope_ui_adc_error(int code,const scope_driver_status_t *snapshot);
void scope_ui_controls(lv_obj_t *parent);
void scope_ui_sync_controls(void);
void scope_ui_keyboard_toggle(lv_event_t *e);
void scope_ui_select_channel(unsigned channel);
int scope_ui_apply_settings(void);
void scope_ui_rebuild_display(int new_frame);
float scope_ui_vertical_offset(void);
void scope_ui_files(unsigned int operation);
int scope_ui_set_accurate(int enable,int resume);
int scope_ui_measure_page_active(void);
#endif
