#ifndef OSCILLOSCOPE_ZOOM_H
#define OSCILLOSCOPE_ZOOM_H
#include <stdint.h>
#define SCOPE_SMOOTH_MIN_SAMPLES 512U
#define SCOPE_RATE_COUNT 14U
/* ADC/DAC共用内部1-2-5档位；不保存用户任意点速率。 */
static inline uint32_t scope_rate_at(uint32_t index)
{
    static const uint32_t rates[SCOPE_RATE_COUNT]={100U,200U,500U,1000U,2000U,5000U,
        10000U,20000U,50000U,100000U,200000U,500000U,1000000U,2000000U};
    return rates[index<SCOPE_RATE_COUNT?index:0U];
}
static inline uint32_t scope_rate_previous(uint32_t rate)
{
    uint32_t i,previous=scope_rate_at(0U);
    for(i=1U;i<SCOPE_RATE_COUNT&&scope_rate_at(i)<rate;i++)previous=scope_rate_at(i);
    return previous;
}
void scope_zoom_set_rate_limit(uint32_t maximum);

typedef enum { SCOPE_ZOOM_NONE, SCOPE_ZOOM_HORIZONTAL, SCOPE_ZOOM_VERTICAL } scope_zoom_axis_t;
/* 返回真实时基；流畅记录至少512点、16点对齐，最大8192点。 */
float scope_zoom_time_div(uint32_t sample_rate_hz,uint32_t record_samples);
int scope_zoom_auto(float frequency_hz,uint32_t *sample_rate_hz,
                    uint32_t *record_samples,float *time_per_div_s);
int scope_zoom_horizontal(uint32_t current_rate,float time_per_div_s,
                          float frequency_hz,uint32_t *sample_rate_hz,
                          uint32_t *record_samples);
/* 整条采集记录至少512点；两路独立视窗允许局部放大至至少2个真实点。 */
int scope_zoom_pair(float requested_div,float other_div,uint32_t *sample_rate_hz,
                    uint32_t *record_samples,float *selected_div);
int scope_zoom_pinch(int32_t start_dx,int32_t start_dy,int32_t current_dx,
                     int32_t current_dy,scope_zoom_axis_t *axis,float *scale);
/* 首次主方向达到8px后锁定；后续位移和反向不改变轴，调用方在松手重置。 */
int scope_zoom_drag_axis(int32_t dx,int32_t dy,scope_zoom_axis_t *axis);
#endif
