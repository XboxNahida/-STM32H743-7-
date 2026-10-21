#include "oscilloscope_zoom.h"
#include "oscilloscope_core.h"
#include "oscilloscope_driver.h"
#include <math.h>

static uint32_t rate_limit=1000000U;
void scope_zoom_set_rate_limit(uint32_t maximum)
{rate_limit=maximum==2000000U?2000000U:1000000U;}

static uint32_t samples_for(uint32_t rate,float time_div)
{
    double count=(double)rate*(double)time_div*10.0;
    if(count<(double)SCOPE_SMOOTH_MIN_SAMPLES)return SCOPE_SMOOTH_MIN_SAMPLES;
    if(count>(double)SCOPE_MAX_RECORD_LENGTH)return SCOPE_MAX_RECORD_LENGTH;
    /* 允许float往返误差，防止每次应用都多出16点。 */
    return (uint32_t)ceil((count-0.01)/16.0)*16U;
}

int scope_zoom_auto(float frequency,uint32_t *rate,uint32_t *samples,float *time_div)
{
    if(!rate||!samples||!time_div||!isfinite(frequency)||frequency<1.0f||frequency>200000.0f)return 0;
    if(!scope_zoom_horizontal(rate_limit,0.25f/frequency,frequency,rate,samples))return 0;
    *time_div=0.25f/frequency;
    return 1;
}

int scope_zoom_horizontal(uint32_t current_rate,float time_div,float frequency,
                          uint32_t *rate,uint32_t *samples)
{
    double span;
    uint32_t i,selected_rate=scope_rate_at(0U);
    if(!rate||!samples||current_rate<SCOPE_DRIVER_MIN_RATE_HZ||
       current_rate>SCOPE_DRIVER_MAX_RATE_HZ||!isfinite(time_div)||time_div<=0.0f||time_div>10.0f||
       !isfinite(frequency)||frequency<0.0f)return 0;
    span=(double)time_div*10.0;
    /* 在容量内尽量提高真实采样密度；极短窗口自动扩至512点。 */
    for(i=0U;i<SCOPE_RATE_COUNT;i++){
        uint32_t candidate=scope_rate_at(i);
        if(candidate>rate_limit)break;
        if((double)candidate*span<=(double)SCOPE_MAX_RECORD_LENGTH+0.01)selected_rate=candidate;
    }
    *rate=selected_rate;*samples=samples_for(selected_rate,time_div);
    return 1;
}

float scope_zoom_time_div(uint32_t rate,uint32_t samples)
{
    if(rate==0U)return 0.0f;
    return (float)((double)samples/(10.0*(double)rate));
}

int scope_zoom_pair(float requested_div,float other_div,uint32_t *rate,
                    uint32_t *samples,float *selected_div)
{
    uint32_t i,first=0U,chosen=0U,own_count,other_count;
    double other_span=(double)other_div*10.0;
    if(!rate||!samples||!selected_div||!isfinite(requested_div)||requested_div<=0.0f||
       !isfinite(other_div)||other_div<=0.0f)return 0;
    for(i=0U;i<SCOPE_RATE_COUNT;i++){
        uint32_t candidate=scope_rate_at(i);
        double n=other_span*(double)candidate;
        if(candidate>rate_limit)break;
        if(n<2.0-0.01||n>8192.0+0.01)continue;
        if(!first)first=candidate;
        if((double)requested_div*10.0*candidate<=8192.0+0.01)chosen=candidate;
    }
    if(!first){
        if(!scope_zoom_horizontal(rate_limit,requested_div,0.0f,rate,samples))return 0;
        *selected_div=fmaxf(2.0f/(10.0f*(float)*rate),
            fminf(requested_div,scope_zoom_time_div(*rate,*samples)));
        return 1;
    }
    if(!chosen)chosen=first;
    own_count=samples_for(chosen,requested_div);other_count=samples_for(chosen,other_div);
    *rate=chosen;*samples=own_count>other_count?own_count:other_count;
    *selected_div=fmaxf(2.0f/(10.0f*(float)chosen),
        fminf(requested_div,scope_zoom_time_div(chosen,own_count)));
    return 1;
}

int scope_zoom_drag_axis(int32_t dx,int32_t dy,scope_zoom_axis_t *axis)
{
    double x=fabs((double)dx),y=fabs((double)dy);
    if(!axis)return 0;
    if(*axis==SCOPE_ZOOM_HORIZONTAL||*axis==SCOPE_ZOOM_VERTICAL)return 1;
    if(fmax(x,y)<8.0)return 0;
    *axis=x>=y?SCOPE_ZOOM_HORIZONTAL:SCOPE_ZOOM_VERTICAL;return 1;
}

int scope_zoom_pinch(int32_t start_dx,int32_t start_dy,int32_t current_dx,
                     int32_t current_dy,scope_zoom_axis_t *axis,float *scale)
{
    double before,after,change_x,change_y;
    if(!axis||!scale)return 0;
    before=sqrt((double)start_dx*start_dx+(double)start_dy*start_dy);
    after=sqrt((double)current_dx*current_dx+(double)current_dy*current_dy);
    if(before<16.0||after<16.0||fabs(after-before)<8.0)return 0;
    change_x=fabs(fabs((double)current_dx)-fabs((double)start_dx));
    change_y=fabs(fabs((double)current_dy)-fabs((double)start_dy));
    *axis=change_x>=change_y?SCOPE_ZOOM_HORIZONTAL:SCOPE_ZOOM_VERTICAL;
    *scale=(float)(after/before);
    return 1;
}
