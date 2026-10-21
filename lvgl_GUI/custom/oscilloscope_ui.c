#include "oscilloscope_ui_private.h"
#include "gui_guider.h"
#include "oscilloscope_driver.h"
#include "oscilloscope_accurate.h"
#include "oscilloscope_workspace.h"
#include "assets/oscilloscope_icon.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

scope_ui_t g_scope_ui;
static void cancel_auto(int restore);
static void update_time_label(void);
static void refresh_follow_measurement(scope_ui_t *u);
static unsigned gesture_channel(const scope_ui_t *u)
{return !u->fft_enabled&&!u->xy_enabled?u->selected_channel:0U;}
static float channel_time(const scope_ui_t *u,unsigned c)
{return u->channel_time_div[c]>0.0f?u->channel_time_div[c]:u->settings.time_per_div_s;}
static float *channel_pan_x(scope_ui_t *u)
{return u->fft_enabled?&u->fft_pan_x:(gesture_channel(u)?&u->pan_x2:&u->pan_x);}
static float *channel_pan_y(scope_ui_t *u)
{return u->fft_enabled?&u->fft_pan_y:(gesture_channel(u)?&u->pan_y2:&u->pan_y);}
static float *channel_scale(scope_ui_t *u)
{return u->fft_enabled?&u->fft_vertical_scale:(gesture_channel(u)?&u->plot_vertical_scale2:&u->plot_vertical_scale);}
static float *horizontal_scale(scope_ui_t *u)
{return u->fft_enabled?&u->fft_frequency_scale:&u->fft_horizontal_scale;}
static int channel_x(const scope_ui_t *u,unsigned c,float x)
{
    size_t start=c?u->view_start2:u->view_start,count=c?u->view_count2:u->view_count;
    float span=u->view_span[c],value=x;
    if(span>1.0f)value=((float)start-u->view_origin[c]+x*(float)(count-1U)/(SCOPE_PLOT_WIDTH-1U))*
        (SCOPE_PLOT_WIDTH-1U)/(span-1.0f);
    return (int)fmaxf(-32760.0f,fminf(32760.0f,value));
}
void scope_ui_select_channel(unsigned channel)
{
    scope_ui_t *u=&g_scope_ui;unsigned c;
    if(channel>1U||u->drag_active||u->pinch_active)return;
    if(channel!=u->selected_channel)cancel_auto(1);
    u->selected_channel=(uint8_t)channel;
    if(u->settings.cursor_enabled==2U)u->settings.measurement_source=channel;
    u->settings.time_per_div_s=channel_time(u,channel);
    scope_ui_sync_controls();
    for(c=0U;c<2U;c++){
        lv_obj_t *l=c?u->channel_label2:u->channel_label;
        if(l)lv_obj_set_style_border_color(lv_obj_get_parent(l),
            lv_color_hex(c==channel?(c?0x66d9f0U:0xffd66bU):0x30485dU),0);
    }
    update_time_label();
    if(u->plot){refresh_follow_measurement(u);lv_obj_invalidate(u->plot);}
}
static void reset_processing(void)
{
    scope_ui_t *u=&g_scope_ui;scope_workspace_t *w=scope_workspace_get();
    scope_accurate_cancel();u->accurate_busy=0U;u->accurate_ready=0U;u->accurate_have_info=0U;
    u->history_count=0U;u->history_next=0U;u->average_frames=0U;u->average_frames2=0U;
    u->fft_count=0U;u->fft_mask=0U;u->plot_pending=0U;u->measurement_count=0U;u->display_count=0U;
    u->sample_point_count=0U;
    u->accurate_attempt_sequence=0U;
    u->display=w?w->display:NULL;u->average=w?w->average:NULL;u->fft=w?w->bins:NULL;
    u->fft2=w?w->accurate_out:NULL;u->fft_span_hz=0.0f;
}
static lv_obj_t *label(lv_obj_t *p,const char *text,int x,int y)
{
    lv_obj_t *o=lv_label_create(p);lv_obj_set_pos(o,x,y);
    lv_obj_set_style_text_color(o,lv_color_hex(0xeff7fc),0);
    lv_obj_set_style_text_font(o,&lv_font_myyh_20,0);lv_label_set_text(o,text);return o;
}
static void update_time_label(void)
{
    scope_ui_t *u=&g_scope_ui;float div;char text[48],value[36];
    if(!u->time_label)return;
    div=u->history_count&&u->session.has_record&&u->display_rate_hz?
        u->view_span[u->selected_channel]/(10.0f*(float)u->display_rate_hz):
        channel_time(u,u->selected_channel);
    scope_format_quantity(value,sizeof(value),div,SCOPE_UNIT_SECONDS);
    (void)snprintf(text,sizeof(text),"%s/div",value);
    lv_label_set_text(u->time_label,text);
}
lv_obj_t *scope_ui_button(lv_obj_t *p,const char *t,int x,int y,int w,int h,
                          lv_event_cb_t cb,void *data)
{
    lv_obj_t *o=lv_button_create(p),*l;
    lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,h);
    lv_obj_set_style_bg_color(o,lv_color_hex(0x263a4a),0);
    lv_obj_set_style_bg_color(o,lv_color_hex(0x355367),LV_STATE_PRESSED);
    lv_obj_set_style_border_color(o,lv_color_hex(0x405569),0);
    lv_obj_set_style_border_width(o,1,0);lv_obj_set_style_shadow_width(o,0,0);
    lv_obj_set_style_radius(o,9,0);l=label(o,t,0,0);
    lv_obj_set_style_text_font(l,&lv_font_myyh_16,0);lv_obj_center(l);
    if(cb)lv_obj_add_event_cb(o,cb,LV_EVENT_CLICKED,data);
    return o;
}
void scope_ui_status(const char *text)
{
    if(g_scope_ui.notice){lv_label_set_text(g_scope_ui.notice,text);
        g_scope_ui.status_until_ms=lv_tick_get()+5000U;lv_obj_remove_flag(g_scope_ui.notice,LV_OBJ_FLAG_HIDDEN);}
    else if(g_scope_ui.status)lv_label_set_text(g_scope_ui.status,text);
}
void scope_ui_adc_error(int code,const scope_driver_status_t *snapshot)
{
    static const char *const stages[]={"采样","速率范围","定时器","采样时序","停止ADC",
        "CH1初始化","公共时钟","CH1通道","CH2初始化","CH2通道","CH2校准",
        "双ADC模式","DMA配置","ADC启动","定时器启动"};
    scope_driver_status_t current;char text[180];unsigned stage;
    if(!snapshot){scope_driver_get_status(&current);snapshot=&current;}
    stage=snapshot->adc_stage;
    if(stage>=sizeof(stages)/sizeof(stages[0]))stage=0U;
    (void)snprintf(text,sizeof(text),"ADC失败(%d) %s S:%lX/%lX CR:%lX/%lX",code,stages[stage],
        (unsigned long)snapshot->adc_state[0],(unsigned long)snapshot->adc_state[1],
        (unsigned long)snapshot->adc_cr[0],(unsigned long)snapshot->adc_cr[1]);
    scope_ui_status(text);
}
static void line_width(lv_layer_t *layer,int x1,int y1,int x2,int y2,
                       uint32_t color,uint8_t opa,uint8_t width)
{
    lv_draw_line_dsc_t d;lv_draw_line_dsc_init(&d);
    d.p1.x=x1;d.p1.y=y1;d.p2.x=x2;d.p2.y=y2;
    d.color=lv_color_hex(color);d.width=width;d.opa=opa;lv_draw_line(layer,&d);
}
static void line(lv_layer_t *layer,int x1,int y1,int x2,int y2,uint32_t color,uint8_t opa)
{line_width(layer,x1,y1,x2,y2,color,opa,1U);}
float scope_ui_vertical_offset(void)
{
    return g_scope_ui.settings.vertical_offset_mv+g_scope_ui.plot_vertical_offset_mv;
}
static int voltage_y(float mv,int height)
{
    float v=(float)height*0.5f-(mv-scope_ui_vertical_offset())*
             ((float)height/8.0f)*g_scope_ui.plot_vertical_scale/g_scope_ui.settings.mv_per_div;
    /* 保留离屏坐标，平移后可查看；缓存为int16，绘制时再裁剪。 */
    if(v<-32760.0f)v=-32760.0f;
    if(v>32760.0f)v=32760.0f;
    return (int)v;
}
static int voltage_y2(float mv,int height)
{
    scope_ui_t *u=&g_scope_ui;
    float v=(float)height*0.5f-(mv-u->settings.vertical_offset2_mv-u->plot_vertical_offset2_mv)*
        ((float)height/8.0f)*(u->xy_enabled?u->plot_vertical_scale:u->plot_vertical_scale2)/u->settings.mv_per_div2;
    return (int)fmaxf(-32760.0f,fminf(32760.0f,v));
}
static const float *measurement_input(scope_ui_t *u,uint32_t source)
{
    if(source>1U||!u->session.record_io.enabled[source])return NULL;
    return source?u->session.mv2:u->session.mv;
}
static const float *visible_measurement_input(scope_ui_t *u,unsigned source)
{
    const float *raw=measurement_input(u,source);scope_workspace_t *w=scope_workspace_get();
    size_t start=source?u->view_start2:u->view_start,count=source?u->view_count2:u->view_count;
    if(!raw||count<2U||start>=u->session.record_count||count>u->session.record_count-start)return NULL;
    if(!u->settings.measurement_mode&&u->settings.acquisition_mode==2U&&
       (source?u->average_frames2:u->average_frames))return source?w->average2:w->average;
    return raw+start;
}
static float displayed_center2(const scope_ui_t *u)
{
    double sum=0.0;size_t i;float center;
    if(!u->session.has_record||!u->session.record_io.enabled[1]||!u->session.record_count)return 0.0f;
    for(i=0U;i<u->session.record_count;i++){
        if(!isfinite(u->session.mv2[i]))return 0.0f;
        sum+=u->session.mv2[i];
    }
    center=u->settings.display2.coupling==SCOPE_COUPLING_DC?
        (float)(sum/(double)u->session.record_count)*u->settings.display2.probe_factor*
        (u->settings.display2.inverted?-1.0f:1.0f):0.0f;
    return isfinite(center)?center:0.0f;
}
static float displayed_signal_center(const scope_ui_t *u)
{
    double sum=0.0;size_t i;float center=0.0f;
    if(!u||!u->session.has_record||!u->session.mv||!u->session.record_count)return 0.0f;
    /* 手势起点只求一次均值，不执行频率/FFT等测量。 */
    for(i=0U;i<u->session.record_count;i++){
        if(!isfinite(u->session.mv[i]))return 0.0f;
        sum+=(double)u->session.mv[i];
    }
    if(u->settings.display.coupling==SCOPE_COUPLING_DC)
        center=(float)(sum/(double)u->session.record_count)*u->settings.display.probe_factor*
            (u->settings.display.inverted?-1.0f:1.0f);
    return isfinite(center)?center:0.0f;
}
static void preview_vertical_zoom(scope_ui_t *u,float scale)
{
    float *vertical=channel_scale(u);
    *vertical=fmaxf(0.001f,fminf(1000.0f,u->pinch_start_vertical_scale*scale));
    if(!u->fft_enabled&&!u->xy_enabled&&!u->selected_channel){
        /* y(C)不变：C-O_new=(C-O_old)*scale_old/scale_new，pan_y保持独立。 */
        u->plot_vertical_offset_mv=u->pinch_vertical_center_mv-
            (u->pinch_vertical_center_mv-u->pinch_start_vertical_offset_mv)*
            u->pinch_start_vertical_scale/u->plot_vertical_scale-u->settings.vertical_offset_mv;
    }
    if(!u->fft_enabled&&(u->xy_enabled||u->selected_channel)){
        u->plot_vertical_offset2_mv=u->pinch_vertical_center2_mv-
            (u->pinch_vertical_center2_mv-u->pinch_start_vertical_offset2_mv)*
            u->pinch_start_vertical_scale/(*vertical)-u->settings.vertical_offset2_mv;
    }
    *channel_pan_x(u)=0.0f;u->history_count=0U;
    if(!u->fft_enabled)scope_ui_rebuild_display(0);
}
static void draw_waveform_axes(lv_layer_t *layer,const lv_area_t *plot,int height)
{
    scope_ui_t *u=&g_scope_ui;
    int k,x,y;unsigned c=u->selected_channel;float sample,time_s,mv;
    char text[32];lv_area_t area;lv_draw_label_dsc_t label_dsc;
    lv_draw_rect_dsc_t background;
    if(!u->session.record_io.enabled[c])c=u->session.record_io.enabled[0]?0U:1U;
    if(!u->session.has_record||(c?u->view_count2:u->view_count)<2U||u->display_rate_hz==0U||height<2)return;
    lv_draw_rect_dsc_init(&background);background.bg_color=lv_color_hex(0x0b1420);
    background.bg_opa=LV_OPA_COVER;background.radius=0;
    lv_draw_label_dsc_init(&label_dsc);label_dsc.font=&lv_font_myyh_16;
    label_dsc.color=lv_color_hex(0xb5c8d8);label_dsc.opa=LV_OPA_COVER;
    label_dsc.text_local=1U;label_dsc.align=LV_TEXT_ALIGN_CENTER;
    for(k=0;k<3;k++){
        x=k*(SCOPE_PLOT_WIDTH-1)/2;
        sample=u->view_origin[c]+((float)x-(c?u->pan_x2:u->pan_x))*(u->view_span[c]-1.0f)/
            (float)(SCOPE_PLOT_WIDTH-1U);
        time_s=(sample-(float)u->session.record_trigger_index)/
            (float)u->display_rate_hz;
        scope_format_quantity(text,sizeof(text),time_s,SCOPE_UNIT_SECONDS);
        area.x1=plot->x1+x-54;area.x2=area.x1+107;
        if(area.x1<plot->x1+2){area.x1=plot->x1+2;area.x2=area.x1+107;}
        if(area.x2>plot->x2-2){area.x2=plot->x2-2;area.x1=area.x2-107;}
        area.y1=plot->y2-21;area.y2=plot->y2-2;
        lv_draw_rect(layer,&background,&area);
        label_dsc.align=LV_TEXT_ALIGN_CENTER;
        label_dsc.text=text;lv_draw_label(layer,&label_dsc,&area);
        line(layer,plot->x1+x,plot->y2-24,plot->x1+x,plot->y2-20,0xb5c8d8,220);
        y=k*(height-1)/2;
        mv=(u->session.record_io.enabled[0]?scope_ui_vertical_offset():
            u->settings.vertical_offset2_mv+u->plot_vertical_offset2_mv)+
            ((float)height*0.5f+(u->session.record_io.enabled[0]?u->pan_y:u->pan_y2)-(float)y)*
            (u->session.record_io.enabled[0]?u->settings.mv_per_div:u->settings.mv_per_div2)*8.0f/
            ((float)height*(u->session.record_io.enabled[0]?u->plot_vertical_scale:u->plot_vertical_scale2));
        scope_format_quantity(text,sizeof(text),mv,SCOPE_UNIT_MV);
        area.x1=plot->x1+3;area.x2=plot->x1+94;
        area.y1=plot->y1+y-10;
        if(area.y1<plot->y1+2)area.y1=plot->y1+2;
        if(area.y1+18>plot->y2-24)area.y1=plot->y2-42;
        area.y2=area.y1+18;label_dsc.align=LV_TEXT_ALIGN_LEFT;
        lv_draw_rect(layer,&background,&area);
        label_dsc.text=text;lv_draw_label(layer,&label_dsc,&area);
        line(layer,plot->x1+94,plot->y1+y,plot->x1+98,plot->y1+y,0xb5c8d8,220);
        if(u->session.record_io.enabled[0]&&u->session.record_io.enabled[1]){
            mv=u->settings.vertical_offset2_mv+u->plot_vertical_offset2_mv+
                ((float)height*0.5f+u->pan_y2-(float)y)*u->settings.mv_per_div2*8.0f/
                ((float)height*u->plot_vertical_scale2);
            scope_format_quantity(text,sizeof(text),mv,SCOPE_UNIT_MV);
            area.x1=plot->x2-94;area.x2=plot->x2-3;label_dsc.align=LV_TEXT_ALIGN_RIGHT;
            label_dsc.color=lv_color_hex(0x66d9f0);lv_draw_rect(layer,&background,&area);
            label_dsc.text=text;lv_draw_label(layer,&label_dsc,&area);
            label_dsc.color=lv_color_hex(0xb5c8d8);
        }
    }
}
static const char *accurate_reason_text(scope_accurate_reason_t reason)
{
    switch(reason){
    case SCOPE_ACCURATE_DC:return "准确模式：直流输入，动态频率无效";
    case SCOPE_ACCURATE_TOO_SHORT:return "准确模式：记录周期不足，动态频率无效";
    case SCOPE_ACCURATE_AMBIGUOUS:return "准确模式：多音/非谐波信号，动态频率有歧义";
    default:return "准确模式：结果有效";
    }
}
static float fft_value(const float *bins,size_t bin)
{return isfinite(bins[bin])&&bins[bin]>0.0f?bins[bin]:0.0f;}
static float fft_pixel(const float *bins,size_t last,float center,float half_width)
{
    size_t first,end,j,left;float value=0.0f,t;
    if(center<0.0f||center>(float)last)return 0.0f;
    left=(size_t)center;t=center-(float)left;
    value=fft_value(bins,left);
    if(left<last)value=value*(1.0f-t)+fft_value(bins,left+1U)*t;
    /* 放大时连接真实频点；缩小时保留像素覆盖的窄峰。 */
    first=(size_t)fmaxf(0.0f,ceilf(center-half_width));
    end=(size_t)fminf((float)last,floorf(center+half_width));
    if(first<=end)for(j=first;j<=end;++j)value=fmaxf(value,fft_value(bins,j));
    return value;
}
static float fft_dbv(float peak_mv)
{return peak_mv>0.000001f?20.0f*log10f(peak_mv*0.00070710678f):-180.0f;}
static void fft_text(lv_layer_t *layer,const lv_area_t *area,const char *text,
                     uint32_t color,lv_text_align_t align)
{
    lv_draw_label_dsc_t desc;lv_draw_label_dsc_init(&desc);
    desc.font=&lv_font_myyh_16;desc.color=lv_color_hex(color);desc.text_local=1U;
    desc.text=text;desc.align=align;lv_draw_label(layer,&desc,area);
}
static void draw_fft(lv_layer_t *layer,const lv_area_t *area)
{
    scope_ui_t *u=&g_scope_ui;lv_area_t graph=*area,label,clip=layer->_clip_area;
    size_t bin,peaks[2]={0U,0U};float values[2]={0.0f,0.0f},maximum=0.0f;
    float top,bottom,range,bin_count,position,amplitude,value,axis_factor=1.0f;
    const char *axis_unit="mV";int c,k,x,y,old_y=0;
    int width,height;char text[160],frequency[32],magnitude[32],resolution[32];
    if(!u->fft_rate_hz||u->fft_count<4U)return;
    graph.x1+=68;graph.x2-=10;graph.y1+=46;graph.y2-=26;
    width=lv_area_get_width(&graph);height=lv_area_get_height(&graph);
    if(width<2||height<2)return;
    for(c=0;c<2;++c)if(u->fft_mask&(1U<<c)){
        const float *bins=c&&u->fft_mask==3U?u->fft2:u->fft;
        for(bin=1U;bin<=u->fft_count/2U;++bin)if(fft_value(bins,bin)>values[c]){
            values[c]=fft_value(bins,bin);peaks[c]=bin;
        }
        maximum=fmaxf(maximum,values[c]);
    }
    if(u->fft_linear){
        top=fmaxf(0.001f,maximum*0.70710678f)*1.12f/u->fft_vertical_scale;
        bottom=0.0f;
    }else{
        top=maximum>0.0f?10.0f*ceilf((fft_dbv(maximum)+3.0f)/10.0f):0.0f;
        bottom=top-80.0f/u->fft_vertical_scale;
    }
    range=top-bottom;
    if(u->fft_linear){
        float extent=fmaxf(fabsf(top+u->fft_pan_y*range/(float)(height-1)),
                           fabsf(bottom+u->fft_pan_y*range/(float)(height-1)));
        if(extent>=1000000.0f){axis_factor=0.000001f;axis_unit="kV";}
        else if(extent>=1000.0f){axis_factor=0.001f;axis_unit="V";}
        else if(extent<1.0f){axis_factor=1000.0f;axis_unit="uV";}
    }
    bin_count=u->fft_span_hz>0.0f?u->fft_span_hz*(float)u->fft_count/(float)u->fft_rate_hz:
        (float)(u->fft_count/2U);
    if(u->settings.grid_enabled){
        for(k=0;k<=8;++k){x=graph.x1+k*(width-1)/8;
            line(layer,x,graph.y1,x,graph.y2,0x213443,255);}
        for(k=0;k<=4;++k){y=graph.y1+k*(height-1)/4;
            line(layer,graph.x1,y,graph.x2,y,0x213443,255);}
    }
    /* 仅裁剪频谱线，图例与刻度保持清晰。 */
    layer->_clip_area.x1=LV_MAX(clip.x1,graph.x1);layer->_clip_area.x2=LV_MIN(clip.x2,graph.x2);
    layer->_clip_area.y1=LV_MAX(clip.y1,graph.y1);layer->_clip_area.y2=LV_MIN(clip.y2,graph.y2);
    for(c=0;c<2;++c)if(u->fft_mask&(1U<<c)){
        const float *bins=c&&u->fft_mask==3U?u->fft2:u->fft;
        float half_bin=bin_count/(2.0f*(float)(width-1)*u->fft_frequency_scale);
        for(x=0;x<width;++x){
            position=bin_count*0.5f+((float)x-(float)(width-1)*0.5f-u->fft_pan_x)*
                bin_count/((float)(width-1)*u->fft_frequency_scale);
            amplitude=fft_pixel(bins,u->fft_count/2U,position,half_bin);
            value=u->fft_linear?amplitude*0.70710678f:fft_dbv(amplitude);
            value=(float)(height-1)*(top-value)/range+u->fft_pan_y;
            y=(int)fmaxf(-16000.0f,fminf(16000.0f,value));
            if(x)line(layer,graph.x1+x-1,graph.y1+old_y,graph.x1+x,graph.y1+y,
                       c?0x66d9f0U:0xffd66bU,255);
            old_y=y;
        }
    }
    layer->_clip_area=clip;
    for(c=0;c<2;++c)if(u->fft_mask&(1U<<c)){
        scope_format_quantity(frequency,sizeof(frequency),
            (double)peaks[c]*(double)u->fft_rate_hz/(double)u->fft_count,SCOPE_UNIT_HZ);
        if(u->fft_linear)scope_format_quantity(magnitude,sizeof(magnitude),values[c]*0.70710678f,SCOPE_UNIT_MV);
        else (void)snprintf(magnitude,sizeof(magnitude),"%.2f dBV",(double)fft_dbv(values[c]));
        if(values[c]>0.0f)(void)snprintf(text,sizeof(text),"CH%d 峰 %s / %s",c+1,frequency,magnitude);
        else (void)snprintf(text,sizeof(text),"CH%d 峰 --",c+1);
        label=*area;label.x1+=4+c*(SCOPE_PLOT_WIDTH/2);label.x2=label.x1+SCOPE_PLOT_WIDTH/2-5;
        label.y1+=1;label.y2=label.y1+19;fft_text(layer,&label,text,c?0x66d9f0U:0xffd66bU,LV_TEXT_ALIGN_LEFT);
    }
    scope_format_quantity(resolution,sizeof(resolution),(double)u->fft_rate_hz/(double)u->fft_count,SCOPE_UNIT_HZ);
    if(u->fft_linear)(void)snprintf(magnitude,sizeof(magnitude),"线性 %s RMS",axis_unit);
    else (void)snprintf(magnitude,sizeof(magnitude),"dBV RMS");
    (void)snprintf(text,sizeof(text),"%s | df=%s | N=%lu | Hann / 去直流",
        magnitude,resolution,(unsigned long)u->fft_count);
    label=*area;label.x1+=4;label.y1+=22;label.y2=label.y1+19;
    fft_text(layer,&label,text,0xb5c8d8U,LV_TEXT_ALIGN_LEFT);
    for(k=0;k<=4;++k){
        value=top-range*(float)k/4.0f+u->fft_pan_y*range/(float)(height-1);
        if(u->fft_linear)scope_format_fixed(text,sizeof(text),value*axis_factor,2U);
        else (void)snprintf(text,sizeof(text),"%.2f",(double)value);
        label=*area;label.x2=graph.x1-5;label.y1=graph.y1+k*(height-1)/4-9;label.y2=label.y1+19;
        fft_text(layer,&label,text,0xb5c8d8U,LV_TEXT_ALIGN_RIGHT);
    }
    for(k=0;k<3;++k){
        position=bin_count*0.5f+((float)k*(float)(width-1)*0.5f-(float)(width-1)*0.5f-u->fft_pan_x)*
            bin_count/((float)(width-1)*u->fft_frequency_scale);
        if(position<0.0f||position>(float)(u->fft_count/2U))(void)snprintf(text,sizeof(text),"--");
        else scope_format_quantity(text,sizeof(text),(double)position*u->fft_rate_hz/u->fft_count,SCOPE_UNIT_HZ);
        label=graph;label.x1=graph.x1+k*(width-130)/2;label.x2=label.x1+129;
        label.y1=graph.y2+5;label.y2=area->y2;
        fft_text(layer,&label,text,0xb5c8d8U,k==0?LV_TEXT_ALIGN_LEFT:k==1?LV_TEXT_ALIGN_CENTER:LV_TEXT_ALIGN_RIGHT);
    }
}
static void draw(lv_event_t *e)
{
    scope_ui_t *u=&g_scope_ui;
    lv_area_t a,saved_clip;lv_layer_t *layer=lv_event_get_layer(e);
    int x,y,h,k,c,old_y=0,px=(int)u->pan_x,py=(int)u->pan_y;
    lv_obj_get_coords(u->plot,&a);h=lv_area_get_height(&a);
    saved_clip=layer->_clip_area;
    layer->_clip_area.x1=LV_MAX(saved_clip.x1,a.x1);
    layer->_clip_area.y1=LV_MAX(saved_clip.y1,a.y1);
    layer->_clip_area.x2=LV_MIN(saved_clip.x2,a.x2);
    layer->_clip_area.y2=LV_MIN(saved_clip.y2,a.y2);
    if(u->settings.grid_enabled&&!u->fft_enabled){
        for(k=1;k<10;k++){x=k*SCOPE_PLOT_WIDTH/10;
            line(layer,a.x1+x,a.y1,a.x1+x,a.y2,0x213443,255);
        }
        for(k=1;k<8;k++){y=k*h/8;line(layer,a.x1,a.y1+y,a.x2,a.y1+y,0x213443,255);}
        line(layer,a.x1,a.y1+h/2,a.x2,a.y1+h/2,0x3a5264,255);
    }
    if(u->xy_enabled){
        if(u->session.has_record&&u->session.record_io.enabled[0]&&u->session.record_io.enabled[1]){
            size_t i;int previous_x=0,previous_y=0;lv_draw_rect_dsc_t dot;lv_area_t point;
            lv_draw_rect_dsc_init(&dot);dot.bg_color=lv_color_hex(0x66d9f0);
            dot.bg_opa=LV_OPA_COVER;dot.radius=LV_RADIUS_CIRCLE;
            for(i=0U;i<u->sample_point_count;i++){
                float vx=(float)(SCOPE_PLOT_WIDTH-1)*0.5f+
                    (u->sample_values[i]-scope_ui_vertical_offset())*
                    ((float)SCOPE_PLOT_WIDTH/10.0f)*u->fft_horizontal_scale/u->settings.mv_per_div;
                x=(int)fmaxf(-32760.0f,fminf(32760.0f,vx))+px;
                y=voltage_y2(u->sample_values2[i],h)+py;
                if(i)line_width(layer,a.x1+previous_x,a.y1+previous_y,a.x1+x,a.y1+y,0x66d9f0,255,2U);
                point.x1=a.x1+x-1;point.x2=point.x1+2;point.y1=a.y1+y-1;point.y2=point.y1+2;
                lv_draw_rect(layer,&dot,&point);previous_x=x;previous_y=y;
            }
        }
        {
            lv_draw_label_dsc_t desc;lv_area_t axis=a;char text[160];
            lv_draw_label_dsc_init(&desc);desc.font=&lv_font_myyh_16;
            desc.color=lv_color_hex(0xb5c8d8);desc.text_local=1U;
            (void)snprintf(text,sizeof(text),"XY: X=CH1 / Y=CH2");
            axis.x1+=120;axis.x2-=120;desc.align=LV_TEXT_ALIGN_CENTER;
            axis.y1=a.y1+3;axis.y2=axis.y1+20;desc.text=text;lv_draw_label(layer,&desc,&axis);
            for(k=0;k<3;k++){
                float mv;int tick=k*(SCOPE_PLOT_WIDTH-1)/2;
                mv=scope_ui_vertical_offset()+((float)tick-u->pan_x-(float)(SCOPE_PLOT_WIDTH-1)*0.5f)*
                    10.0f*u->settings.mv_per_div/((float)SCOPE_PLOT_WIDTH*u->fft_horizontal_scale);
                scope_format_quantity(text,sizeof(text),mv,SCOPE_UNIT_MV);
                axis.x1=a.x1+tick-65;axis.x2=axis.x1+130;
                if(axis.x1<a.x1+2){axis.x1=a.x1+2;axis.x2=axis.x1+130;}
                if(axis.x2>a.x2-2){axis.x2=a.x2-2;axis.x1=axis.x2-130;}
                axis.y1=a.y2-21;axis.y2=a.y2-2;desc.align=LV_TEXT_ALIGN_CENTER;desc.text=text;
                lv_draw_label(layer,&desc,&axis);
                tick=k*(h-1)/2;
                mv=u->settings.vertical_offset2_mv+u->plot_vertical_offset2_mv+
                    ((float)h*0.5f+u->pan_y-(float)tick)*u->settings.mv_per_div2*8.0f/
                    ((float)h*u->plot_vertical_scale);
                scope_format_quantity(text,sizeof(text),mv,SCOPE_UNIT_MV);
                axis.x1=a.x1+3;axis.x2=axis.x1+110;axis.y1=a.y1+tick-10;
                if(axis.y1<a.y1+26)axis.y1=a.y1+26;
                if(axis.y1>a.y2-44)axis.y1=a.y2-44;
                axis.y2=axis.y1+18;desc.align=LV_TEXT_ALIGN_LEFT;lv_draw_label(layer,&desc,&axis);
            }
        }
    }else if(u->fft_enabled&&u->session.has_record&&u->fft_count){
        draw_fft(layer,&a);
    }else{
        for(c=0;c<2;c++){
          uint32_t color=c?0x66d9f0U:0xffd66bU;
          if(!u->session.record_io.enabled[c])continue;
          px=(int)(c?u->pan_x2:u->pan_x);py=(int)(c?u->pan_y2:u->pan_y);
          for(k=(int)u->history_count-1;k>=0;k--){
            unsigned int row=(u->history_next+SCOPE_HISTORY-1U-(unsigned int)k)%SCOPE_HISTORY;
            uint8_t opacity=(k==0)?255U:65U;
            for(x=0;x<SCOPE_PLOT_WIDTH;x++){
                int lo=c?u->history2[row][x][0]:u->history[row][x][0];
                int hi=c?u->history2[row][x][1]:u->history[row][x][1];
                int xx=channel_x(u,(unsigned)c,(float)x)+px;
                if(x)line_width(layer,a.x1+channel_x(u,(unsigned)c,(float)(x-1))+px,
                    a.y1+old_y+py,a.x1+xx,a.y1+lo+py,color,opacity,k==0?2U:1U);
                if(hi!=lo)line_width(layer,a.x1+xx,a.y1+lo+py,a.x1+xx,a.y1+hi+py,color,opacity,k==0?2U:1U);
                old_y=lo;
            }
        }
        /* 最多500个缓存点，仅最新真实记录；拟合曲线不生成额外采样标记。 */
        if(u->history_count){
            lv_draw_rect_dsc_t point;lv_area_t area;size_t i;
            lv_draw_rect_dsc_init(&point);point.bg_color=lv_color_hex(color);
            point.bg_opa=LV_OPA_COVER;point.radius=LV_RADIUS_CIRCLE;
            for(i=0U;i<(c?u->sample_point_count2:u->sample_point_count);i++){
                area.x1=a.x1+(c?u->sample_points2[i][0]:u->sample_points[i][0])+px-1;area.x2=area.x1+2;
                area.y1=a.y1+(c?u->sample_points2[i][1]:u->sample_points[i][1])+py-1;area.y2=area.y1+2;
                lv_draw_rect(layer,&point,&area);
            }
        }
        }
        px=(int)*channel_pan_x(u);py=(int)*channel_pan_y(u);
        y=u->session.record_io.enabled[0]?
            voltage_y(u->settings.trigger.level_mv*u->settings.display.probe_factor,h)+py:
            voltage_y2(u->settings.trigger.level_mv*u->settings.display2.probe_factor,h)+py;
        line(layer,a.x1,a.y1+y,a.x2,a.y1+y,0xa77353,150);
    }
    if(u->settings.cursor_enabled&&!u->fft_enabled&&!u->xy_enabled&&
       (!u->settings.measurement_mode||u->settings.cursor_enabled==2U)){
        unsigned source=u->selected_channel;float count=fmaxf(1.0f,u->view_span[source]-1.0f);
        for(k=0;k<2;k++){
            x=(int)((u->settings.cursor_x[k]-u->view_origin[source])/count*(SCOPE_PLOT_WIDTH-1))+px;
            float mv=u->settings.cursor_enabled==2U?u->follow_plot_mv[k]:u->settings.cursor_mv[k];
            int time_valid=u->settings.cursor_enabled!=2U||(u->follow_valid&SCOPE_VALID_PERIOD);
            int voltage_valid=u->settings.cursor_enabled!=2U||(u->follow_valid&(k?SCOPE_VALID_MAX:SCOPE_VALID_MIN));
            y=(source?voltage_y2(mv,h):voltage_y(mv,h))+py;
            if(time_valid&&x>=0&&x<SCOPE_PLOT_WIDTH)line(layer,a.x1+x,a.y1,a.x1+x,a.y2,0x87dba7,220);
            if(voltage_valid)line(layer,a.x1,a.y1+y,a.x2,a.y1+y,0x87dba7,170);
            {
                lv_draw_label_dsc_t d;lv_area_t tag;
                lv_draw_label_dsc_init(&d);d.font=&lv_font_myyh_16;
                d.color=lv_color_hex(0x87dba7);d.text=k?"B":"A";
                if(time_valid&&x>=0&&x<SCOPE_PLOT_WIDTH){
                    tag.x1=a.x1+LV_MIN(x+3,SCOPE_PLOT_WIDTH-16);tag.x2=tag.x1+15;
                    tag.y1=a.y1+4;tag.y2=tag.y1+20;lv_draw_label(layer,&d,&tag);}
                if(voltage_valid&&y>=0&&y<h){tag.x1=a.x1+4;tag.x2=tag.x1+15;
                    tag.y1=a.y1+LV_MIN(y+2,h-22);tag.y2=tag.y1+20;lv_draw_label(layer,&d,&tag);}
            }
        }
    }
    if(!u->fft_enabled&&!u->xy_enabled)draw_waveform_axes(layer,&a,h);
    layer->_clip_area=saved_clip;
}
void scope_ui_rebuild_display(int new_frame)
{
    scope_ui_t *u=&g_scope_ui;scope_workspace_t *w=scope_workspace_get();
    size_t i,j,a,b,n,pre,count,start,points,draw_points;unsigned c;
    unsigned row=u->history_next;int h,lo,hi,y;float requested;
    if(!u->session.has_record||!u->display||!w)return;
    n=u->session.record_count;pre=u->session.record_trigger_index;
    if(n<2U||n>SCOPE_MAX_RECORD_LENGTH)return;
    h=(int)lv_obj_get_height(u->plot);if(h<2)h=338;
    if(!u->settings.persistence||u->settings.measurement_mode){u->history_count=0U;row=0U;}
    for(c=0U;c<2U;c++){
        float *samples=c?u->sample_values2:u->sample_values;
        float *display=c?u->display2:u->display;
        float *average=c?w->average2:u->average;
        /* FFT与绘图均在主循环顺序运行，复用FFT虚部；准确算法使用独立工作区。 */
        float *full=w->fft_imag;
        const float *input=c?u->session.mv2:u->session.mv;
        const scope_display_t *transform=c?&u->settings.display2:&u->settings.display;
        uint32_t *frames=c?&u->average_frames2:&u->average_frames;
        int16_t (*markers)[2]=c?u->sample_points2:u->sample_points;
        int16_t (*history)[2]=c?u->history2[row]:u->history[row];
        if(!u->session.record_io.enabled[c]){
            if(c)u->sample_point_count2=0U;else u->sample_point_count=0U;
            continue;
        }
        requested=(u->xy_enabled?u->settings.time_per_div_s:channel_time(u,c))*10.0f*(float)u->session.actual_rate_hz;
        count=requested>=(float)n-0.5f?n:(requested<2.0f?2U:(size_t)(requested+0.5f));
        if(u->settings.measurement_mode||count>n)count=n;
        start=pre-pre*count/n;if(start+count>n)start=n-count;
        points=count>500U?500U:count;draw_points=u->settings.render_mode?500U:points;
        if((c?u->view_start2:u->view_start)!=start||(c?u->view_count2:u->view_count)!=count){
            *frames=0U;u->history_count=0U;
        }
        if(c){u->view_start2=start;u->view_count2=count;u->display_count2=draw_points;u->sample_point_count2=points;}
        else {u->view_start=start;u->view_count=count;u->display_count=draw_points;u->sample_point_count=points;}
        u->view_span[c]=(float)count;u->view_origin[c]=(float)start;
        if(!u->settings.measurement_mode&&!u->xy_enabled&&requested>(float)n+0.5f){
            u->view_span[c]=requested;u->view_origin[c]=(float)pre-(float)pre*requested/(float)n;
        }
        if(!u->settings.measurement_mode&&u->settings.acquisition_mode==2U){
            if(new_frame){
                uint32_t next=*frames+1U;if(next>u->settings.average_count)next=u->settings.average_count;
                for(i=0U;i<count;i++)average[i]=next==1U?input[start+i]:average[i]+(input[start+i]-average[i])/(float)next;
                *frames=next;
            }
            if(*frames)input=average;else input+=start;
        }else input+=start;
        if(!scope_transform(input,count,transform,full)){
            scope_ui_status("显示换算失败，请减小探头倍率/增益");return;
        }
        if(!scope_resample_view(full,count,0U,count,samples,points,0)||
           !scope_resample_view(full,count,0U,count,display,draw_points,(int)u->settings.render_mode))return;
        if(!scope_resample_view(full,count,0U,count,w->fft_real,SCOPE_PLOT_WIDTH,
                               (int)u->settings.render_mode))return;
        for(i=0U;i<points;i++){
            size_t index=(size_t)((double)i*(double)(count-1U)/(double)(points-1U)+0.5);
            markers[i][0]=(int16_t)channel_x(u,c,(float)index*(SCOPE_PLOT_WIDTH-1U)/(float)(count-1U));
            markers[i][1]=(int16_t)(c?voltage_y2(samples[i],h):voltage_y(samples[i],h));
        }
        /* 每一原始点都进入一个像素列；压缩显示不能漏掉抽点间的峰谷。 */
        for(i=0U;i<SCOPE_PLOT_WIDTH;i++){
            a=i*count/SCOPE_PLOT_WIDTH;b=(i+1U)*count/SCOPE_PLOT_WIDTH;
            if(b<=a)b=a+1U;
            if(b>count)b=count;
            lo=c?voltage_y2(w->fft_real[i],h):voltage_y(w->fft_real[i],h);hi=lo;
            for(j=a;j<b;j++){
                y=c?voltage_y2(full[j],h):voltage_y(full[j],h);
                if(y<lo)lo=y;
                if(y>hi)hi=y;
            }
            history[i][0]=(int16_t)lo;history[i][1]=(int16_t)hi;
        }
    }
    u->display_rate_hz=u->session.actual_rate_hz;u->display_sequence=u->session.record_sequence;
    u->history_next=(uint8_t)((row+1U)%SCOPE_HISTORY);
    if(u->history_count<SCOPE_HISTORY)u->history_count++;
    refresh_follow_measurement(u);
    lv_obj_invalidate(u->plot);
}
int scope_ui_apply_settings(void)
{
    scope_ui_t *u=&g_scope_ui;scope_session_config_t c;int running=u->session.running;
    uint32_t rate,samples;size_t old_count,old_pre;int result;
    /* 加入PA5后从2M降到1M是正常规划，不能在规划前当成参数错误拒绝。 */
    if(scope_io_valid(&u->settings.io)&&isfinite(u->settings.sample_rate_hz)&&
       u->settings.sample_rate_hz<=(float)SCOPE_DRIVER_MAX_RATE_HZ&&
       u->settings.sample_rate_hz>(float)scope_io_max_rate(&u->settings.io))
        u->settings.sample_rate_hz=(float)scope_io_max_rate(&u->settings.io);
    if(!scope_settings_valid(&u->settings))return -1;
    scope_zoom_set_rate_limit(scope_io_max_rate(&u->settings.io));
    if(!u->settings.measurement_mode&&u->auto_active!=1U){
        unsigned other=1U-u->selected_channel;
        float selected_div;
        old_count=u->settings.trigger.window_samples;old_pre=u->settings.trigger.pre_samples;
        if(!scope_zoom_horizontal((uint32_t)u->settings.sample_rate_hz,u->settings.time_per_div_s,
            0.0f,&rate,&samples))return -1;
        selected_div=fmaxf(2.0f/(10.0f*(float)rate),
            fminf(u->settings.time_per_div_s,scope_zoom_time_div(rate,samples)));
        if(u->settings.io.enabled[0]&&u->settings.io.enabled[1]){
            float other_div=channel_time(u,other);
            /* 只有输入上限变化才允许扩展另一通道不再可实现的最窄窗口。 */
            other_div=fmaxf(scope_zoom_time_div(scope_io_max_rate(&u->settings.io),2U),other_div);
            other_div=fminf(scope_zoom_time_div(100U,8192U),other_div);
            if(!scope_zoom_pair(u->settings.time_per_div_s,other_div,&rate,&samples,&selected_div))return -1;
            u->channel_time_div[other]=other_div;
        }
        u->settings.sample_rate_hz=(float)rate;u->settings.trigger.window_samples=samples;
        u->settings.trigger.pre_samples=old_pre*samples/old_count;
        u->settings.time_per_div_s=selected_div;
    }else if(u->settings.measurement_mode){
        /* 准确模式保留请求点数，采样时钟同样只能取固定档位。 */
        uint32_t index;
        rate=scope_rate_at(0U);
        for(index=0U;index<SCOPE_RATE_COUNT;index++){
            uint32_t candidate=scope_rate_at(index);
            if(candidate>(uint32_t)u->settings.sample_rate_hz)break;
            rate=candidate;
        }
        u->settings.sample_rate_hz=(float)rate;
    }
    if(u->settings.sample_rate_hz<(float)SCOPE_DRIVER_MIN_RATE_HZ||
       u->settings.sample_rate_hz>(float)SCOPE_DRIVER_MAX_RATE_HZ||
       u->settings.trigger.pre_samples>u->settings.trigger.window_samples*90U/100U)return -1;
    if(u->auto_active!=1U&&!u->settings.measurement_mode){
        unsigned c;
        for(c=0U;c<2U;c++)if(u->channel_time_div[c]<=0.0f)
            u->channel_time_div[c]=u->sampling_time_div>0.0f?u->sampling_time_div:u->settings.time_per_div_s;
        u->channel_time_div[u->selected_channel]=u->settings.time_per_div_s;
        if(!u->auto_active&&!running&&!u->pinch_was_running&&u->session.has_record&&u->session.actual_rate_hz)
            u->channel_time_div[u->selected_channel]=fminf(u->settings.time_per_div_s,
                scope_zoom_time_div(u->session.actual_rate_hz,(uint32_t)u->session.record_count));
    }
    u->sampling_time_div=u->settings.time_per_div_s;
    c.sample_rate_hz=(uint32_t)u->settings.sample_rate_hz;c.mode=(scope_mode_t)u->settings.trigger_mode;
    c.edge=u->settings.trigger.edge;c.level_mv=u->settings.trigger.level_mv;
    c.holdoff_ms=u->settings.holdoff_ms;
    c.pre_samples=u->settings.trigger.pre_samples;
    c.record_samples=u->settings.trigger.window_samples;
    c.trigger_source=u->settings.io.enabled[u->trigger_source]?u->trigger_source:
        (u->settings.io.enabled[0]?0U:1U);
    c.hysteresis_mv=u->trigger_hysteresis_mv;
    if(running)scope_session_stop(&u->session);
    if(u->session.opened){
        result=scope_driver_configure_io(&u->settings.io);
        if(result){u->session.last_error=result;return result;}
    }
    u->session.io=u->settings.io;
    if(!u->settings.io.enabled[u->settings.measurement_source])
        u->settings.measurement_source=u->settings.io.enabled[0]?0U:1U;
    if(u->settings.fft_source<2U&&!u->settings.io.enabled[u->settings.fft_source])
        u->settings.fft_source=u->settings.io.enabled[0]?0U:1U;
    if(scope_session_configure(&u->session,&c)!=0)return -1;
    scope_accurate_cancel();u->accurate_busy=0U;u->accurate_ready=0U;
    u->accurate_attempt_sequence=u->settings.measurement_mode?u->session.record_sequence:0U;
    u->session.calibration=u->settings.calibration;
    u->session.calibration2=u->settings.calibration2;
    u->session.dac_sample_rate_hz=u->settings.dac_sample_rate_hz;
    u->average_frames=0U;u->average_frames2=0U;u->history_count=0U;u->fft_count=0U;u->fft_mask=0U;
    u->accurate_have_info=0U;u->measurement_count=0U;
    if(u->session.has_record)(void)scope_session_refresh(&u->session);
    if(running||u->settings.measurement_mode){
        result=scope_session_run(&u->session,lv_tick_get());if(result)return result;
    }
    scope_ui_rebuild_display(0);return 0;
}
int scope_ui_set_accurate(int enable,int resume)
{
    scope_ui_t *u=&g_scope_ui;int r=0;
    if(enable){
        if(u->settings.measurement_mode)return 0;
        if(!scope_ui_measure_page_active()||!u->session.opened||u->pinch_active||u->drag_active||
           u->accurate_samples<512U||u->accurate_samples>8192U)return -1;
        cancel_auto(1);u->smooth_settings=u->settings;
        u->accurate_resume_running=u->session.running;
        u->settings.measurement_mode=1U;u->fft_enabled=0U;u->xy_enabled=0U;
        u->settings.trigger.pre_samples=u->settings.trigger.pre_samples*u->accurate_samples/
            u->settings.trigger.window_samples;
        u->settings.trigger.window_samples=u->accurate_samples;
        r=scope_ui_apply_settings();
        /* 只采一条新记录再停机计算；保留原来的运行/停止状态。 */
        u->accurate_attempt_sequence=u->session.record_sequence;
        if(!r&&!u->session.running)r=scope_session_run(&u->session,lv_tick_get());
        if(!r)scope_ui_status("准确模式：等待一条完整记录，离开测量页自动关闭");
        else (void)scope_ui_set_accurate(0,1);
    }else{
        scope_accurate_cancel();u->accurate_busy=0U;u->accurate_ready=0U;u->accurate_have_info=0U;
        if(u->settings.measurement_mode){
            scope_session_stop(&u->session);u->settings=u->smooth_settings;
            u->settings.measurement_mode=0U;r=scope_ui_apply_settings();
            if(!r&&resume&&u->accurate_resume_running&&u->session.opened)
                r=scope_session_run(&u->session,lv_tick_get());
            u->accurate_resume_running=0U;
        }
    }
    scope_ui_sync_controls();return r;
}
static int touch_in_plot(int x,int y)
{
    lv_area_t a;lv_obj_get_coords(g_scope_ui.plot,&a);
    return x>=a.x1&&x<=a.x2&&y>=a.y1&&y<=a.y2;
}
/* 按下仅命中候选线；初始位移锁轴后，才确定时间/电压游标或波形平移。 */
static int poll_drag(uint32_t now)
{
    scope_ui_t *u=&g_scope_ui;lv_port_touch_snapshot_t t;lv_area_t a;
    scope_zoom_axis_t axis;int dx,dy,k,r=0;float count,x,y,best_x=25.0f,best_y=25.0f;
    lv_port_indev_get_touch_snapshot(&t);
    if(u->settings.measurement_mode||u->pinch_active||u->pinch_consumed||u->touch_blocked)return 0;
    if(!u->drag_active){
        if(t.count!=1U||!touch_in_plot(t.x[0],t.y[0]))return 0;
        cancel_auto(1);u->drag_active=1U;u->drag_locked=0U;u->drag_axis=SCOPE_ZOOM_NONE;
        u->drag_start.x=t.x[0];u->drag_start.y=t.y[0];u->drag_last=u->drag_start;
        u->drag_pan_x=*channel_pan_x(u);u->drag_pan_y=*channel_pan_y(u);
        u->drag_time_candidate=-1;u->drag_voltage_candidate=-1;u->drag_cursor=-1;
        u->drag_cursor_candidate=-1;u->drag_cursor_distance=25.0f;
        lv_obj_get_coords(u->plot,&a);
        count=fmaxf(1.0f,u->view_span[u->selected_channel]-1.0f);
        if(u->settings.cursor_enabled==1U&&!u->fft_enabled&&!u->xy_enabled&&u->session.has_record)for(k=0;k<2;k++){
            x=(u->settings.cursor_x[k]-u->view_origin[u->selected_channel])/count*(SCOPE_PLOT_WIDTH-1)+*channel_pan_x(u);
            y=(float)(u->selected_channel?voltage_y2(u->settings.cursor_mv[k],lv_area_get_height(&a)):
                voltage_y(u->settings.cursor_mv[k],lv_area_get_height(&a)))+*channel_pan_y(u);
            if(x>=0.0f&&x<SCOPE_PLOT_WIDTH&&fabsf((float)(t.x[0]-a.x1)-x)<best_x){
                best_x=fabsf((float)(t.x[0]-a.x1)-x);u->drag_time_candidate=(int8_t)k;}
            if(y>=0.0f&&y<(float)lv_area_get_height(&a)&&fabsf((float)(t.y[0]-a.y1)-y)<best_y){
                best_y=fabsf((float)(t.y[0]-a.y1)-y);u->drag_voltage_candidate=(int8_t)k;}
        }
        if(u->settings.cursor_enabled==1U&&!u->fft_enabled&&!u->xy_enabled){
            if(u->drag_time_candidate>=0){u->drag_cursor_candidate=u->drag_time_candidate;
                u->drag_cursor_distance=best_x;}
            if(u->drag_voltage_candidate>=0&&best_y<u->drag_cursor_distance){
                u->drag_cursor_candidate=u->drag_voltage_candidate;u->drag_cursor_distance=best_y;}
        }
        return 0;
    }
    if(t.count!=1U){
        int locked=u->drag_locked,running=u->drag_was_running;
        u->drag_active=0U;u->drag_locked=0U;
        if(locked){
            lv_obj_invalidate(u->plot);scope_ui_sync_controls();
            if(running&&u->session.opened)r=scope_session_run(&u->session,now);
            if(r)scope_ui_adc_error(r,NULL);
        }
        return locked;
    }
    u->drag_last.x=t.x[0];u->drag_last.y=t.y[0];
    dx=u->drag_last.x-u->drag_start.x;dy=u->drag_last.y-u->drag_start.y;
    axis=(scope_zoom_axis_t)u->drag_axis;
    if(!scope_zoom_drag_axis(dx,dy,&axis))return 0;
    u->drag_axis=(uint8_t)axis;
    if(!u->drag_locked){
        u->drag_locked=1U;u->drag_was_running=u->session.running;
        if(u->session.running)scope_session_stop(&u->session);
        u->plot_pending=0U;u->drag_draw_ms=now-40U;
        u->drag_cursor=u->settings.cursor_enabled==1U&&!u->fft_enabled&&!u->xy_enabled?
            u->drag_cursor_candidate:(axis==SCOPE_ZOOM_HORIZONTAL?u->drag_time_candidate:u->drag_voltage_candidate);
        if(u->drag_cursor>=0){
            u->drag_cursor_x=u->settings.cursor_x[u->drag_cursor];
            u->drag_cursor_mv=u->settings.cursor_mv[u->drag_cursor];}
    }
    lv_obj_get_coords(u->plot,&a);
    if(u->settings.cursor_enabled==1U&&!u->fft_enabled&&!u->xy_enabled){
        if(u->drag_cursor>=0){
            k=u->drag_cursor;count=fmaxf(1.0f,u->view_span[u->selected_channel]-1.0f);
            u->settings.cursor_x[k]=fmaxf(0.0f,fminf((float)(u->session.record_count-1U),
                u->drag_cursor_x+(float)dx*count/(SCOPE_PLOT_WIDTH-1)));
            u->settings.cursor_mv[k]=fmaxf(-1.0e9f,fminf(1.0e9f,u->drag_cursor_mv-
                (float)dy*(u->selected_channel?u->settings.mv_per_div2:u->settings.mv_per_div)*8.0f/
                ((float)lv_area_get_height(&a)*(*channel_scale(u)))));
        }
    }else if(u->drag_cursor>=0){
        k=u->drag_cursor;
        if(axis==SCOPE_ZOOM_HORIZONTAL){
            count=(float)(u->view_count>1U?u->view_count-1U:1U);
            u->settings.cursor_x[k]=fmaxf(0.0f,fminf((float)(u->session.record_count-1U),
                u->drag_cursor_x+(float)dx*count/(SCOPE_PLOT_WIDTH-1)));
        }else u->settings.cursor_mv[k]=fmaxf(-1.0e9f,fminf(1.0e9f,u->drag_cursor_mv-
            (float)dy*u->settings.mv_per_div*8.0f/
            ((float)lv_area_get_height(&a)*u->plot_vertical_scale)));
    }else if(axis==SCOPE_ZOOM_HORIZONTAL)
        *channel_pan_x(u)=fmaxf(-10.0f*SCOPE_PLOT_WIDTH,fminf(10.0f*SCOPE_PLOT_WIDTH,u->drag_pan_x+(float)dx));
    else *channel_pan_y(u)=fmaxf(-10.0f*(float)lv_area_get_height(&a),
        fminf(10.0f*(float)lv_area_get_height(&a),u->drag_pan_y+(float)dy));
    if((uint32_t)(now-u->drag_draw_ms)>=40U){u->drag_draw_ms=now;lv_obj_invalidate(u->plot);}
    return 1;
}
/* 双指期间冻结采集，每40ms重画缓存；松开任一指后应用横向采样配置。 */
static int poll_pinch(uint32_t now)
{
    scope_ui_t *u=&g_scope_ui;lv_port_touch_snapshot_t t;
    scope_zoom_axis_t axis=SCOPE_ZOOM_NONE;float scale=1.0f,requested,frequency=0.0f;
    uint32_t rate,samples;int changed,r=0;
    lv_port_indev_get_touch_snapshot(&t);
    if(u->settings.measurement_mode)return 0;
    if(t.count==0U)u->touch_blocked=0U;
    /* 界外起手后滑入，仍属于原按压；必须全部松手才能开始表格手势。 */
    if(!u->pinch_active&&!u->drag_active&&!u->pinch_consumed&&t.count>0U&&
       (!touch_in_plot(t.x[0],t.y[0])||(t.count>1U&&!touch_in_plot(t.x[1],t.y[1]))))
        u->touch_blocked=1U;
    if(u->touch_blocked)return 0;
    if(!u->pinch_active){
        if(u->pinch_consumed){
            if(t.count==0U&&(uint32_t)(now-u->pinch_release_ms)>=150U)u->pinch_consumed=0U;
            return 0;
        }
        if(t.count<2U||!touch_in_plot(t.x[0],t.y[0])||!touch_in_plot(t.x[1],t.y[1]))return 0;
        cancel_auto(1);u->pinch_active=1U;u->pinch_consumed=1U;u->pinch_axis=SCOPE_ZOOM_NONE;
        u->pinch_start_a.x=t.x[0];u->pinch_start_a.y=t.y[0];
        u->pinch_start_b.x=t.x[1];u->pinch_start_b.y=t.y[1];
        u->pinch_last_a=u->pinch_start_a;u->pinch_last_b=u->pinch_start_b;
        if(!u->fft_enabled&&!u->xy_enabled){
            if(u->channel_time_div[0]<=0.0f)u->channel_time_div[0]=u->settings.time_per_div_s;
            if(u->channel_time_div[1]<=0.0f)u->channel_time_div[1]=u->settings.time_per_div_s;
        }
        u->pinch_start_time_div=channel_time(u,gesture_channel(u));
        u->pinch_start_vertical_scale=*channel_scale(u);
        u->pinch_vertical_center_mv=u->fft_enabled?0.0f:displayed_signal_center(u);
        u->pinch_start_vertical_offset_mv=scope_ui_vertical_offset();
        u->pinch_vertical_center2_mv=displayed_center2(u);
        u->pinch_start_vertical_offset2_mv=u->settings.vertical_offset2_mv+u->plot_vertical_offset2_mv;
        u->pinch_start_fft_scale=*horizontal_scale(u);
        u->pinch_preview_ms=now-40U;
        u->pinch_was_running=u->drag_locked?u->drag_was_running:u->session.running;
        u->drag_active=0U;u->drag_locked=0U;
        if(u->session.running)scope_session_stop(&u->session);
        u->plot_pending=0U;return 1;
    }
    if(t.count>=2U){
        u->pinch_last_a.x=t.x[0];u->pinch_last_a.y=t.y[0];
        u->pinch_last_b.x=t.x[1];u->pinch_last_b.y=t.y[1];
        changed=scope_zoom_pinch(
            u->pinch_start_b.x-u->pinch_start_a.x,u->pinch_start_b.y-u->pinch_start_a.y,
            u->pinch_last_b.x-u->pinch_last_a.x,u->pinch_last_b.y-u->pinch_last_a.y,&axis,&scale);
        if(changed){
            if(u->pinch_axis==SCOPE_ZOOM_NONE)u->pinch_axis=(uint8_t)axis;
            else axis=(scope_zoom_axis_t)u->pinch_axis;
            if((uint32_t)(now-u->pinch_preview_ms)>=40U){
                u->pinch_preview_ms=now;
                if(axis==SCOPE_ZOOM_VERTICAL){
                    preview_vertical_zoom(u,scale);
                }else{
                    *channel_pan_x(u)=0.0f;
                    if(u->fft_enabled||u->xy_enabled)*horizontal_scale(u)=fmaxf(0.01f,fminf(100.0f,u->pinch_start_fft_scale*scale));
                    else {u->settings.time_per_div_s=fmaxf(1e-9f,fminf(10.0f,u->pinch_start_time_div/scale));
                        u->channel_time_div[u->selected_channel]=u->settings.time_per_div_s;
                        u->history_count=0U;scope_ui_rebuild_display(0);update_time_label();}
                }
                lv_obj_invalidate(u->plot);
            }
        }
        return 1;
    }
    u->pinch_active=0U;u->pinch_release_ms=now;
    changed=scope_zoom_pinch(u->pinch_start_b.x-u->pinch_start_a.x,
        u->pinch_start_b.y-u->pinch_start_a.y,u->pinch_last_b.x-u->pinch_last_a.x,
        u->pinch_last_b.y-u->pinch_last_a.y,&axis,&scale);
    if(changed){
        if(u->pinch_axis!=SCOPE_ZOOM_NONE)axis=(scope_zoom_axis_t)u->pinch_axis;
        if(axis==SCOPE_ZOOM_VERTICAL){
            preview_vertical_zoom(u,scale);
            scope_ui_status("纵向缩放已刷新：只改变绘图比例");
        }else if(u->fft_enabled||u->xy_enabled){
            *channel_pan_x(u)=0.0f;
            *horizontal_scale(u)=fmaxf(0.01f,fminf(100.0f,u->pinch_start_fft_scale*scale));
            lv_obj_invalidate(u->plot);scope_ui_status(u->xy_enabled?"XY横轴电压已缩放":"FFT频率范围已缩放，ADC采样配置不变");
        }else{
            *channel_pan_x(u)=0.0f;
            requested=fmaxf(1e-9f,fminf(10.0f,u->pinch_start_time_div/scale));
            if(u->pinch_was_running){
                if(u->session.measurement_sequence==u->session.record_sequence&&
                   (u->session.measurements.valid&SCOPE_VALID_FREQUENCY))
                    frequency=u->session.measurements.frequency_hz;
                if(scope_zoom_horizontal((uint32_t)u->settings.sample_rate_hz,requested,frequency,&rate,&samples)){
                    u->settings.sample_rate_hz=(float)rate;
                    u->settings.time_per_div_s=requested;
                    u->channel_time_div[u->selected_channel]=u->settings.time_per_div_s;
                    r=scope_ui_apply_settings();
                    scope_ui_status(fabsf(u->settings.time_per_div_s-requested)>requested*0.01f?
                        "横向缩放已到硬件/512～8192点边界":
                        (frequency>0.0f&&(float)rate<frequency*16.0f?
                        "横向缩放已刷新；采样点/周期偏少，可能混叠":
                        "横向缩放已应用，等待新采样填满时间窗"));
                }else r=-1;
            }else if(u->session.has_record){
                /* 停止时只能裁剪现有记录，不能把旧数据解释成新采样率。 */
                u->channel_time_div[u->selected_channel]=fmaxf(2.0f/(10.0f*(float)u->session.actual_rate_hz),
                    fminf(requested,(float)u->session.record_count/(10.0f*(float)u->session.actual_rate_hz)));
                u->settings.time_per_div_s=u->channel_time_div[u->selected_channel];
                u->history_count=0U;scope_ui_rebuild_display(0);
                scope_ui_status("停止状态：已刷新现有记录视窗，保持停止");
            }
        }
    }
    if(u->pinch_was_running&&u->session.opened){
        int resumed=scope_session_run(&u->session,now);
        if(resumed)r=resumed;
    }
    u->pinch_was_running=0U;scope_ui_sync_controls();
    if(r)scope_ui_adc_error(r,NULL);
    return 0;
}
static void cancel_auto(int restore)
{
    scope_ui_t *u=&g_scope_ui;int running;
    if(!u->auto_active)return;
    running=u->auto_was_running;u->auto_active=0U;
    if(restore){
        scope_session_stop(&u->session);u->settings=u->auto_settings;
        u->trigger_source=u->auto_trigger_source;u->trigger_hysteresis_mv=u->auto_hysteresis_mv;
        u->channel_time_div[u->selected_channel]=u->auto_time_div;
        u->sampling_time_div=u->settings.time_per_div_s;
        if(scope_ui_apply_settings()==0&&running&&u->session.opened)
            (void)scope_session_run(&u->session,lv_tick_get());
        scope_ui_sync_controls();
    }else u->settings=u->auto_settings;
}
static int auto_capture(uint32_t rate,uint32_t now)
{
    scope_ui_t *u=&g_scope_ui;int result;
    u->settings.sample_rate_hz=(float)rate;
    u->settings.time_per_div_s=scope_zoom_time_div(rate,SCOPE_SMOOTH_MIN_SAMPLES);
    u->settings.trigger_mode=SCOPE_MODE_AUTO;
    u->auto_sequence=u->session.record_sequence;u->auto_started_ms=now;
    result=scope_ui_apply_settings();if(result)return result;
    if(!u->session.running)return scope_session_run(&u->session,now);
    return 0;
}
static int auto_measure(scope_ui_t *u,scope_measurements_t *m,int visible)
{
    unsigned c=u->selected_channel;size_t start=0U,count=u->session.record_count;
    const float *input=measurement_input(u,c);
    if(!input||!u->session.has_record)return 0;
    if(visible){start=c?u->view_start2:u->view_start;count=c?u->view_count2:u->view_count;}
    
    if(count<2U||start+count>u->session.record_count)return 0;
    return scope_measure(input+start,count,(float)u->session.actual_rate_hz,m);
}
static int auto_fit_wave(scope_ui_t *u,scope_settings_t *settings,const scope_measurements_t *m)
{
    unsigned c=u->selected_channel;float center,span,step;
    const scope_display_t *d=c?&settings->display2:&settings->display;
    const scope_calibration_t *cal=c?&settings->calibration2:&settings->calibration;
    if(!(m->valid&SCOPE_VALID_MIN)||!(m->valid&SCOPE_VALID_MAX)||!isfinite(m->min_mv)||
       !isfinite(m->max_mv)||!isfinite(m->mean_mv))return 0;
    center=(m->min_mv+m->max_mv)*0.5f;span=m->max_mv-m->min_mv;
    if(d->coupling==SCOPE_COUPLING_AC)center-=m->mean_mv;
    if(d->coupling==SCOPE_COUPLING_GND){center=0.0f;span=0.0f;}
    center*=d->probe_factor*(d->inverted?-1.0f:1.0f);span*=d->probe_factor;
    /* 至少给8个真实ADC码留空间，直流也不会被放大到浮点数噪声。 */
    step=cal->vref_mv*cal->gain*d->probe_factor/65535.0f;
    span=fmaxf(span,8.0f*step);
    if(c){settings->mv_per_div2=fmaxf(1e-6f,fminf(1e9f,span/7.2f));
        settings->vertical_offset2_mv=center;u->plot_vertical_offset2_mv=0.0f;}
    else {settings->mv_per_div=fmaxf(1e-6f,fminf(1e9f,span/7.2f));
        settings->vertical_offset_mv=center;u->plot_vertical_offset_mv=0.0f;}
    *channel_scale(u)=1.0f;*channel_pan_x(u)=0.0f;*channel_pan_y(u)=0.0f;
    return 1;
}
static void auto_trigger(scope_ui_t *u,const scope_measurements_t *m)
{
    unsigned c=u->selected_channel;
    const scope_calibration_t *cal=c?&u->settings.calibration2:&u->settings.calibration;
    u->trigger_source=(uint8_t)c;u->settings.trigger_mode=SCOPE_MODE_AUTO;
    u->settings.trigger.edge=SCOPE_EDGE_RISING;
    u->settings.trigger.level_mv=(m->min_mv+m->max_mv)*0.5f;
    u->trigger_hysteresis_mv=fmaxf(m->vpp_mv*0.05f,2.0f*cal->vref_mv*cal->gain/65535.0f);
    u->settings.holdoff_ms=0U;
}
static int auto_recapture_original(uint32_t now)
{
    scope_ui_t *u=&g_scope_ui;int result;
    /* PA5采样时间随Fs改变：恢复最终采样配置后，必须重取真实数据再拟合。 */
    scope_session_stop(&u->session);u->settings=u->auto_settings;
    u->settings.time_per_div_s=u->auto_time_div;
    u->settings.trigger_mode=SCOPE_MODE_AUTO;
    u->channel_time_div[u->selected_channel]=u->auto_time_div;
    u->auto_active=4U;u->auto_sequence=u->session.record_sequence;u->auto_started_ms=now;
    u->trigger_source=u->selected_channel;u->trigger_hysteresis_mv=0.0f;
    result=scope_ui_apply_settings();if(result)return result;
    return scope_session_run(&u->session,now);
}
static int poll_auto(uint32_t now,int new_record)
{
    scope_ui_t *u=&g_scope_ui;scope_measurements_t m;
    uint32_t rate,samples,timeout;float time_div;char text[100];int result;
    if(!u->auto_active)return 0;
    timeout=1500U+(uint32_t)(1200ULL*u->session.config.record_samples/u->session.config.sample_rate_hz);
    if((uint32_t)(now-u->auto_started_ms)>timeout){
        if(u->auto_active!=4U&&auto_recapture_original(now)==0)
            scope_ui_status("未找到稳定周期：正在原时基重新采集并居中");
        else {cancel_auto(1);scope_ui_status("自动设置未取得最终新记录，已恢复；请检查采样/丢块");}
        return 1;
    }
    if(!new_record||u->auto_sequence==u->session.record_sequence)return 0;
    if(u->auto_active==4U){
        scope_ui_rebuild_display(0);
        if(!auto_measure(u,&m,1)||!auto_fit_wave(u,&u->settings,&m)){
            cancel_auto(1);scope_ui_status("最终采样数据无效，已恢复");return 1;
        }
        scope_session_stop(&u->session);
        u->settings.trigger_mode=u->auto_settings.trigger_mode;
        u->trigger_source=u->auto_trigger_source;u->trigger_hysteresis_mv=u->auto_hysteresis_mv;
        result=scope_ui_apply_settings();
        if(!result&&u->auto_was_running)result=scope_session_run(&u->session,now);
        if(result){cancel_auto(1);scope_ui_status("自动居中后采样恢复失败");return 1;}
        u->auto_active=0U;scope_ui_sync_controls();
        scope_ui_status("无有效周期：已按原时基新记录放大居中");return 0;
    }
    if(u->auto_active>=2U){
        /* 搜索帧只用于选Fs；以最终采样配置的新记录更新直流中心和幅度。 */
        scope_ui_rebuild_display(0);
        if(auto_measure(u,&m,1)){
            (void)auto_fit_wave(u,&u->settings,&m);auto_trigger(u,&m);
            u->session.config.level_mv=u->settings.trigger.level_mv;
            u->session.config.hysteresis_mv=u->trigger_hysteresis_mv;
        }
        if(u->auto_active==2U){
            u->auto_active=3U;u->auto_sequence=u->session.record_sequence;u->auto_started_ms=now;
            return 1;
        }
        u->history_count=0U;scope_ui_rebuild_display(0);
        if(!u->auto_was_running)scope_session_stop(&u->session);
        u->auto_active=0U;scope_ui_sync_controls();
        scope_ui_status("自动设置完成：至少512真实点，居中放大，中点回滞触发");return 0;
    }
    if(auto_measure(u,&m,0)&&
       (m.valid&SCOPE_VALID_FREQUENCY)&&m.vpp_mv>=1.0f&&
       m.frequency_hz*(float)u->session.record_count/(float)u->session.actual_rate_hz>=2.0f&&
       scope_zoom_auto(m.frequency_hz,&rate,&samples,&time_div)){
        (void)auto_fit_wave(u,&u->settings,&m);auto_trigger(u,&m);
        u->settings.sample_rate_hz=(float)rate;u->settings.time_per_div_s=time_div;
        u->channel_time_div[u->selected_channel]=time_div;
        u->settings.trigger.pre_samples=samples/5U;
        u->settings.trigger.window_samples=samples;
        u->auto_active=2U;u->auto_sequence=u->session.record_sequence;u->auto_started_ms=now;
        if(scope_ui_apply_settings()!=0){cancel_auto(1);scope_ui_status("自动设置应用失败，已恢复");return 1;}
        scope_ui_sync_controls();return 1;
    }
    rate=u->session.config.sample_rate_hz;
    if(rate<=100U){
        if(auto_recapture_original(now)!=0){cancel_auto(1);scope_ui_status("原时基重新采集失败，已恢复");}
        else scope_ui_status("无有效周期：正在原时基重新采集并居中");
        return 1;
    }
    rate=scope_rate_previous(rate);
    if(auto_capture(rate,now)!=0){cancel_auto(1);scope_ui_status("自动搜索采集失败，已恢复");}
    else { (void)snprintf(text,sizeof(text),"自动搜索：%lu Sa/s，至少512点；低频需要等待完整记录",(unsigned long)rate);
        scope_ui_status(text);scope_ui_sync_controls();}
    return 1;
}
static void action(lv_event_t *e)
{
    scope_ui_t *u=&g_scope_ui;intptr_t id=(intptr_t)lv_event_get_user_data(e);int r=0;
    if(u->settings.measurement_mode&&id!=0){scope_ui_status("请先关闭准确模式或离开测量页");return;}
    if((u->pinch_active||u->drag_active)&&id!=0)return;
    cancel_auto(1);
    if(id==0){scope_accurate_cancel();u->accurate_busy=0U;u->accurate_ready=0U;
        (void)scope_ui_set_accurate(0,0);
        scope_session_close(&u->session);lv_screen_load(guider_ui.DeskTop);return;}
    if(id==1){scope_accurate_cancel();u->accurate_busy=0U;u->accurate_ready=0U;
        if(u->session.running)scope_session_stop(&u->session);
        else {
            if(u->settings.trigger_mode==SCOPE_MODE_SINGLE){
                u->settings.trigger_mode=SCOPE_MODE_AUTO;r=scope_ui_apply_settings();
            }
            if(!r)r=scope_session_open(&u->session);
            reset_processing();
            if(!r)r=scope_session_run(&u->session,lv_tick_get());}}
    if(id==2){scope_accurate_cancel();u->accurate_busy=0U;u->accurate_ready=0U;
        u->settings.trigger_mode=SCOPE_MODE_SINGLE;r=scope_ui_apply_settings();
        if(!r)r=scope_session_open(&u->session);
        reset_processing();
        if(!r)r=scope_session_run(&u->session,lv_tick_get());}
    if(id==3){
        if(!u->settings.io.enabled[u->selected_channel])scope_ui_select_channel(u->settings.io.enabled[0]?0U:1U);
        u->fft_enabled=0U;u->xy_enabled=0U;
        *channel_pan_x(u)=0.0f;*channel_pan_y(u)=0.0f;*channel_scale(u)=1.0f;
        if(u->selected_channel)u->plot_vertical_offset2_mv=0.0f;else u->plot_vertical_offset_mv=0.0f;
        lv_obj_invalidate(u->plot);
        u->auto_time_div=channel_time(u,u->selected_channel);
        u->auto_trigger_source=u->trigger_source;u->auto_hysteresis_mv=u->trigger_hysteresis_mv;
        u->trigger_source=u->selected_channel;u->trigger_hysteresis_mv=0.0f;
        u->auto_settings=u->settings;u->auto_was_running=u->session.running;u->auto_active=1U;
        u->settings.trigger.pre_samples=100U;u->settings.trigger.window_samples=SCOPE_SMOOTH_MIN_SAMPLES;
        r=auto_capture(scope_io_max_rate(&u->settings.io),lv_tick_get());
        if(r)cancel_auto(1);
        else scope_ui_status("自动搜索：按固定采样档位寻找真实周期");
    }
    if(id==4){u->xy_enabled=0U;u->fft_enabled=(uint8_t)!u->fft_enabled;scope_ui_rebuild_display(0);}
    scope_ui_sync_controls();
    if(r){char text[100];
        (void)snprintf(text,sizeof(text),"操作失败(%d)：检查采样/输入参数",r);scope_ui_status(text);}
}
static void screen_event(lv_event_t *e)
{
    if(lv_event_get_code(e)==LV_EVENT_SCREEN_UNLOAD_START){
        (void)scope_ui_set_accurate(0,0);cancel_auto(0);g_scope_ui.pinch_active=0U;
        g_scope_ui.drag_active=0U;g_scope_ui.drag_locked=0U;
        scope_accurate_cancel();g_scope_ui.accurate_busy=0U;
        g_scope_ui.accurate_ready=0U;scope_session_close(&g_scope_ui.session);reset_processing();}
    if(lv_event_get_code(e)==LV_EVENT_DELETE){
        if(g_scope_ui.settings.measurement_mode)g_scope_ui.settings=g_scope_ui.smooth_settings;
        g_scope_ui.settings.measurement_mode=0U;g_scope_ui.accurate_resume_running=0U;
        cancel_auto(0);g_scope_ui.pinch_active=0U;g_scope_ui.drag_active=0U;g_scope_ui.drag_locked=0U;
        scope_accurate_cancel();scope_session_close(&g_scope_ui.session);reset_processing();
        g_scope_ui.screen=NULL;g_scope_ui.plot=NULL;g_scope_ui.status=NULL;g_scope_ui.notice=NULL;
        g_scope_ui.tabs=NULL;g_scope_ui.measure_page=NULL;g_scope_ui.accurate_button=NULL;
        g_scope_ui.measure_summary=NULL;
    }
}
static void open_event(lv_event_t *e){(void)e;oscilloscope_ui_open();}
static void entry_deleted(lv_event_t *e)
{
    /* 动画重建时旧桌面可能晚于新图标删除，只清理本次销毁对象。 */
    if(lv_event_get_target_obj(e)==g_scope_ui.entry){
        g_scope_ui.entry=NULL;g_scope_ui.entry_desktop=NULL;}
}
void oscilloscope_ui_attach_desktop(lv_obj_t *desktop)
{
    lv_obj_t *tile,*image,*caption;
    if(!desktop||!lv_obj_is_valid(desktop))return;
    if(g_scope_ui.entry_desktop==desktop&&g_scope_ui.entry&&lv_obj_is_valid(g_scope_ui.entry))return;
    tile=guider_ui.DeskTop_tileview_DeskTop_tile_1;
    if(!tile||!lv_obj_is_valid(tile)||lv_obj_get_screen(tile)!=desktop)return;
    /* 接在第二页小游戏(x750)后；透明点击容器与其他APP的80px图标/文字对齐。 */
    g_scope_ui.entry=lv_obj_create(tile);
    lv_obj_set_pos(g_scope_ui.entry,850,35);lv_obj_set_size(g_scope_ui.entry,80,114);
    lv_obj_set_style_bg_opa(g_scope_ui.entry,LV_OPA_TRANSP,0);
    lv_obj_set_style_border_width(g_scope_ui.entry,0,0);
    lv_obj_set_style_pad_all(g_scope_ui.entry,0,0);
    lv_obj_remove_flag(g_scope_ui.entry,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(g_scope_ui.entry,LV_OBJ_FLAG_CLICKABLE);
    image=lv_image_create(g_scope_ui.entry);lv_image_set_src(image,&scope_icon);
    lv_obj_set_size(image,80,80);lv_obj_set_pos(image,0,0);
    caption=label(g_scope_ui.entry,"示波器",0,84);lv_obj_set_size(caption,80,30);
    lv_obj_set_style_text_color(caption,lv_color_hex(0xffffff),0);
    lv_obj_set_style_text_align(caption,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_add_event_cb(g_scope_ui.entry,open_event,LV_EVENT_CLICKED,NULL);
    g_scope_ui.entry_desktop=desktop;
    lv_obj_add_event_cb(g_scope_ui.entry,entry_deleted,LV_EVENT_DELETE,NULL);
}
static void display_event(lv_event_t *e)
{
    lv_obj_t *screen=(lv_obj_t *)lv_event_get_param(e);
    if(screen==guider_ui.DeskTop)oscilloscope_ui_attach_desktop(screen);
}
void oscilloscope_ui_init(void)
{
    if(g_scope_ui.initialized)return;
    scope_session_init(&g_scope_ui.session);scope_settings_defaults(&g_scope_ui.settings);
    g_scope_ui.accurate_samples=SCOPE_RECORD_LENGTH;g_scope_ui.plot_vertical_scale=1.0f;
    g_scope_ui.plot_vertical_scale2=1.0f;
    g_scope_ui.fft_horizontal_scale=1.0f;
    g_scope_ui.fft_frequency_scale=1.0f;g_scope_ui.fft_vertical_scale=1.0f;
    g_scope_ui.keyboard_enabled=1U;
    g_scope_ui.initialized=1U;
    lv_display_add_event_cb(lv_display_get_default(),display_event,LV_EVENT_SCREEN_LOADED,NULL);
    oscilloscope_ui_attach_desktop(guider_ui.DeskTop);
}
static lv_obj_t *card(lv_obj_t *parent,int x,int y,int w,int h,uint32_t border)
{
    lv_obj_t *o=lv_obj_create(parent);lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,h);
    lv_obj_set_style_bg_color(o,lv_color_hex(0x192b3b),0);
    lv_obj_set_style_border_color(o,lv_color_hex(border),0);
    lv_obj_set_style_border_width(o,1,0);lv_obj_set_style_radius(o,8,0);
    lv_obj_set_style_pad_all(o,0,0);lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE);return o;
}
static void select_page(lv_event_t *e)
{
    uint32_t index=(uint32_t)(uintptr_t)lv_event_get_user_data(e);
    if(index<2U)scope_ui_select_channel(index);
    lv_tabview_set_active(g_scope_ui.tabs,index,LV_ANIM_OFF);
    lv_obj_send_event(g_scope_ui.tabs,LV_EVENT_VALUE_CHANGED,NULL);
    lv_obj_scroll_to_view(lv_tabview_get_tab_button(g_scope_ui.tabs,(int32_t)index),LV_ANIM_OFF);
}
static lv_obj_t *card_text(lv_obj_t *p,const char *s,int x,int y,int w,uint32_t color)
{
    lv_obj_t *o=label(p,s,x,y);lv_obj_set_width(o,w);
    lv_obj_set_style_text_font(o,&lv_font_myyh_16,0);
    lv_obj_set_style_text_color(o,lv_color_hex(color),0);
    lv_label_set_long_mode(o,LV_LABEL_LONG_CLIP);return o;
}
void oscilloscope_ui_open(void)
{
    scope_ui_t *u=&g_scope_ui;int i;lv_obj_t *o,*b;
    static const char *measure_names[]={"CH1 / 频率","CH1 / 峰峰值","CH1 / 有效值","CH1 / 平均值"};
    oscilloscope_ui_init();
    if(u->screen&&lv_obj_is_valid(u->screen)){lv_screen_load(u->screen);
        if(scope_session_open(&u->session)==0){
            reset_processing();
            (void)scope_ui_apply_settings();scope_ui_sync_controls();
            if(scope_session_run(&u->session,lv_tick_get())!=0)scope_ui_status("采样启动失败");
        }else scope_ui_status("ADC打开失败：资源忙或上次恢复未完成");
        return;}
    u->screen=lv_obj_create(NULL);lv_obj_set_style_bg_color(u->screen,lv_color_hex(0x111e2b),0);
    lv_obj_set_style_pad_all(u->screen,0,0);lv_obj_set_style_border_width(u->screen,0,0);
    lv_obj_set_style_text_font(u->screen,&lv_font_myyh_20,0);
    lv_obj_remove_flag(u->screen,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(u->screen,screen_event,LV_EVENT_ALL,NULL);
    label(u->screen,"示波器",12,34);
    card_text(u->screen,"片上 ADC PA5/PC4 / DAC PA4",12,8,550,0x8fa7bb);
    u->status=card_text(u->screen,"运行 / 等待采集 / 流畅",12,65,600,0x8de0cb);
    b=scope_ui_button(u->screen,"停止",606,34,100,52,action,(void *)(intptr_t)1);
    u->run_label=lv_obj_get_child(b,0);
    scope_ui_button(u->screen,u->keyboard_enabled?"键盘开":"键盘关",714,34,100,52,scope_ui_keyboard_toggle,NULL);
    scope_ui_button(u->screen,"单次",822,34,82,52,action,(void *)(intptr_t)2);
    u->auto_button=scope_ui_button(u->screen,"自动设置",912,34,100,52,action,(void *)(intptr_t)3);
    o=card(u->screen,12,100,402,48,0xffd66b);
    card_text(o,"CH1",8,13,45,0xffd66b);
    u->channel_label=card_text(o,"PA5",60,13,330,0xd7e4ee);
    lv_obj_add_flag(o,LV_OBJ_FLAG_CLICKABLE);lv_obj_add_event_cb(o,select_page,LV_EVENT_CLICKED,NULL);
    o=card(u->screen,422,100,402,48,0x30485d);
    card_text(o,"CH2",8,13,45,0x66d9f0);
    u->channel_label2=card_text(o,"关闭 / PC4",60,13,330,0x8fa7bb);
    lv_obj_add_flag(o,LV_OBJ_FLAG_CLICKABLE);lv_obj_add_event_cb(o,select_page,LV_EVENT_CLICKED,(void *)(uintptr_t)1);
    o=card(u->screen,832,100,180,48,0x30485d);
    u->time_label=card_text(o,"时基",8,13,164,0x8de0cb);
    lv_obj_add_flag(o,LV_OBJ_FLAG_CLICKABLE);lv_obj_add_event_cb(o,select_page,LV_EVENT_CLICKED,(void *)(uintptr_t)2);
    u->plot=lv_obj_create(u->screen);lv_obj_set_pos(u->plot,12,158);lv_obj_set_size(u->plot,SCOPE_PLOT_WIDTH,294);
    lv_obj_set_style_bg_color(u->plot,lv_color_hex(0x0b1420),0);
    lv_obj_set_style_radius(u->plot,10,0);
    lv_obj_set_style_pad_all(u->plot,0,0);lv_obj_set_style_border_width(u->plot,1,0);
    lv_obj_set_style_border_color(u->plot,lv_color_hex(0x273b4c),0);
    lv_obj_remove_flag(u->plot,LV_OBJ_FLAG_SCROLLABLE);lv_obj_add_flag(u->plot,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(u->plot,draw,LV_EVENT_DRAW_MAIN,NULL);
    u->cursor_label=card_text(u->screen,"",162,540,840,0xd7e4ee);
    for(i=0;i<4;i++){
        o=card(u->screen,12+i*252,462,244,54,0x192b3b);
        u->reading_titles[i]=card_text(o,measure_names[i],7,4,230,0xffd66b);
        u->readings[i]=label(o,"--",7,27);lv_obj_set_width(u->readings[i],230);
        lv_obj_add_flag(o,LV_OBJ_FLAG_CLICKABLE);lv_obj_add_event_cb(o,select_page,LV_EVENT_CLICKED,(void *)(uintptr_t)4);
    }
    scope_ui_button(u->screen,"返回桌面",12,556,130,36,action,NULL);
    u->notice=card_text(u->screen,"",162,561,840,0x8de0cb);lv_obj_add_flag(u->notice,LV_OBJ_FLAG_HIDDEN);
    /* 底部状态栏与原例布局一致，不把长调试串塞入标题。 */
    u->footer=card_text(u->screen,"等待采集 / 校准：默认",12,520,1000,0x8fa7bb);
    lv_label_set_long_mode(u->footer,LV_LABEL_LONG_DOT);lv_obj_set_height(u->footer,20);
    scope_ui_controls(u->screen);lv_screen_load(u->screen);
    if(scope_session_open(&u->session)!=0){scope_ui_status("ADC资源忙或初始化失败");return;}
    reset_processing();
    (void)scope_ui_apply_settings();
    scope_ui_sync_controls();
    if(scope_session_run(&u->session,lv_tick_get())!=0)scope_ui_status("采样启动失败");
}
static const char *const measurement_names[]={"频率","周期","峰峰值","有效值","最小值",
    "最大值","平均值","占空比","上升时间","下降时间"};
static void format_measurement(const scope_measurements_t *m,size_t item,char *out,size_t size,unsigned decimals)
{
    static const uint32_t flags[]={SCOPE_VALID_FREQUENCY,SCOPE_VALID_PERIOD,SCOPE_VALID_VPP,
        SCOPE_VALID_RMS,SCOPE_VALID_MIN,SCOPE_VALID_MAX,SCOPE_VALID_MEAN,SCOPE_VALID_DUTY,
        SCOPE_VALID_RISE,SCOPE_VALID_FALL};
    static const scope_unit_t units[]={SCOPE_UNIT_HZ,SCOPE_UNIT_SECONDS,SCOPE_UNIT_MV,
        SCOPE_UNIT_MV,SCOPE_UNIT_MV,SCOPE_UNIT_MV,SCOPE_UNIT_MV,SCOPE_UNIT_PERCENT,
        SCOPE_UNIT_SECONDS,SCOPE_UNIT_SECONDS};
    float numbers[10];
    if(!m||item>=10U||!(m->valid&flags[item])){(void)snprintf(out,size,"--");return;}
    numbers[0]=m->frequency_hz;numbers[1]=m->period_s;numbers[2]=m->vpp_mv;
    numbers[3]=m->rms_mv;numbers[4]=m->min_mv;numbers[5]=m->max_mv;
    numbers[6]=m->mean_mv;numbers[7]=m->duty_percent;numbers[8]=m->rise_s;numbers[9]=m->fall_s;
    scope_format_quantity_precision(out,size,numbers[item],units[item],decimals);
}
static void update_readings(const scope_measurements_t *first,const scope_measurements_t *second)
{
    scope_ui_t *u=&g_scope_ui;size_t i;char title[56],value[40];
    for(i=0U;i<4U;++i){
        uint32_t source=u->settings.reading_source[i],item=u->settings.reading_item[i];
        lv_color_t color=lv_color_hex(source?0x66d9f0U:0xffd66bU);
        if(source>1U||item>=10U)continue;
        (void)snprintf(title,sizeof(title),"CH%lu / %s",(unsigned long)source+1U,measurement_names[item]);
        lv_label_set_text(u->reading_titles[i],title);
        lv_obj_set_style_text_color(u->reading_titles[i],color,0);
        lv_obj_set_style_text_color(u->readings[i],color,0);
        format_measurement(u->settings.io.enabled[source]?(source?second:first):NULL,item,value,sizeof(value),2U);
        lv_label_set_text(u->readings[i],value);
    }
}
static void show_measurements(const scope_measurements_t *m)
{
    scope_ui_t *u=&g_scope_ui;char value[40],text[128];size_t i;
    for(i=0U;i<10U;i++){
        format_measurement(m,i,value,sizeof(value),5U);
        lv_label_set_text(u->measure_values[i],value);
    }
    (void)snprintf(text,sizeof(text),
        "CH%lu / %s / %lu点",
        (unsigned long)(u->settings.measurement_source+1U),
        u->settings.measurement_mode?"准确":"流畅",(unsigned long)u->measurement_count);
    lv_label_set_text(u->measure,text);
}
static void update_channel_readings(const scope_measurements_t *primary)
{
    scope_ui_t *u=&g_scope_ui;
    scope_measurements_t other;unsigned source=u->settings.measurement_source,opposite=1U-source;
    const scope_measurements_t *other_result=NULL;size_t i,start,n;
    if(!u->settings.measurement_mode&&u->settings.io.enabled[opposite]&&measurement_input(u,opposite)){
        for(i=0U;i<4U;++i)if(u->settings.reading_source[i]==opposite){
            start=opposite?u->view_start2:u->view_start;n=opposite?u->view_count2:u->view_count;
            if(n<2U||start+n>u->session.record_count){start=0U;n=u->session.record_count;}
            if(scope_measure(visible_measurement_input(u,opposite),n,(float)u->session.actual_rate_hz,&other))
                other_result=&other;
            break;
        }
    }
    update_readings(source?other_result:primary,source?primary:other_result);
}
static void follow_measurement_cursors(scope_ui_t *u,const scope_measurements_t *m)
{
    size_t start=u->selected_channel?u->view_start2:u->view_start;
    size_t count=u->selected_channel?u->view_count2:u->view_count;
    const float *input=visible_measurement_input(u,u->selected_channel);
    const scope_display_t *display=u->selected_channel?&u->settings.display2:&u->settings.display;
    float a=(float)start,b=a,period,mid;double mean=0.0;size_t i;
    u->follow_valid=0U;
    if(!input||count<2U||start+count>u->session.record_count)return;
    if(m->valid&SCOPE_VALID_PERIOD){
        period=m->period_s*(float)u->session.actual_rate_hz;
        mid=(m->min_mv+m->max_mv)*0.5f;
        for(i=1U;i<count;++i)if(input[i-1U]<=mid&&input[i]>mid){
            a=(float)(start+i-1U)+(mid-input[i-1U])/(input[i]-input[i-1U]);break;
        }
        a=fmaxf((float)start,fminf(a,(float)(start+count-1U)-period));b=a+period;
    }
    u->settings.cursor_x[0]=a;u->settings.cursor_x[1]=b;
    if(m->valid&SCOPE_VALID_MIN)u->settings.cursor_mv[0]=m->min_mv;
    if(m->valid&SCOPE_VALID_MAX)u->settings.cursor_mv[1]=m->max_mv;
    u->follow_valid=m->valid;
    if(display->coupling==SCOPE_COUPLING_AC){
        for(i=0U;i<count;i++)mean+=input[i];
        mean/=(double)count;
    }
    for(i=0U;i<2U;i++)u->follow_plot_mv[i]=display->coupling==SCOPE_COUPLING_GND?0.0f:
        (float)(((double)u->settings.cursor_mv[i]-mean)*display->probe_factor*(display->inverted?-1.0:1.0));
    lv_obj_invalidate(u->plot);
}
static void refresh_follow_measurement(scope_ui_t *u)
{
    size_t start,count;const float *input;
    if(u->settings.cursor_enabled!=2U)return;
    u->follow_valid=0U;u->settings.measurement_source=u->selected_channel;
    input=visible_measurement_input(u,u->selected_channel);
    if(!u->session.has_record||!input)return;
    if(u->settings.measurement_mode){
        if(!u->accurate_ready||u->accurate_source!=u->selected_channel)return;
    }else{
        start=u->selected_channel?u->view_start2:u->view_start;
        count=u->selected_channel?u->view_count2:u->view_count;
        if(count<2U||start+count>u->session.record_count)return;
        if(!scope_measure(input,count,(float)u->session.actual_rate_hz,&u->session.measurements))return;
        u->measurement_count=count;u->session.measurement_sequence=u->session.record_sequence;
    }
    follow_measurement_cursors(u,&u->session.measurements);
}
void oscilloscope_ui_poll(void)
{
    scope_ui_t *u=&g_scope_ui;char text[600],value[4][48];scope_measurements_t m;scope_driver_status_t d;
    float dt,dv;int result;uint32_t now=lv_tick_get();size_t n,start,limit;
    /* 动画解锁可能重建桌面，screen事件不保证转发给display。 */
    oscilloscope_ui_attach_desktop(guider_ui.DeskTop);
    if(!u->screen)return;
    if(lv_screen_active()!=u->screen){if(u->session.opened){scope_accurate_cancel();
        (void)scope_ui_set_accurate(0,0);cancel_auto(0);u->pinch_active=0U;
        u->drag_active=0U;u->drag_locked=0U;
        scope_session_close(&u->session);reset_processing();}return;}
    if(u->settings.measurement_mode&&!scope_ui_measure_page_active())
        (void)scope_ui_set_accurate(0,1);
    if(u->settings.cursor_enabled==2U)u->settings.measurement_source=u->selected_channel;
    if(u->settings.measurement_mode&&u->accurate_source!=u->settings.measurement_source){
        scope_accurate_cancel();u->accurate_busy=0U;u->accurate_ready=0U;u->accurate_have_info=0U;
        u->follow_valid=0U;u->accurate_attempt_sequence=u->session.record_sequence-1U;
        u->accurate_source=(uint8_t)u->settings.measurement_source;
    }
    if(poll_pinch(now))return;
    if(poll_drag(now))return;
    /* 持续消费保持跨帧触发连续性；绘图25Hz，硬件Fs保持不变。 */
    result=scope_session_poll(&u->session,now);
    if(result<0){cancel_auto(0);(void)scope_ui_set_accurate(0,0);
        reset_processing();scope_ui_adc_error(result,NULL);return;}
    if(result>0){u->plot_pending=1U;
        if(u->settings.measurement_mode)scope_session_stop(&u->session);}
    if(poll_auto(now,result>0))return;
    if(u->plot_pending&&(uint32_t)(now-u->last_plot_ms)>=40U){
        u->last_plot_ms=now;u->plot_pending=0U;scope_ui_rebuild_display(1);
    }
    if(u->settings.measurement_mode==1U&&u->session.has_record){
        if(u->accurate_busy){
            result=scope_accurate_step();
            if(result!=0){
                u->accurate_busy=0U;
                u->accurate_have_info=(uint8_t)(result>0&&
                    scope_accurate_result(&u->session.measurements,&u->accurate_info));
                u->accurate_ready=u->accurate_have_info;
                if(u->accurate_ready){u->session.measurement_sequence=u->accurate_sequence;
                    u->accurate_record_count=u->accurate_pending_count;refresh_follow_measurement(u);}
            }
        }else if(!u->session.running&&u->accurate_attempt_sequence!=u->session.record_sequence&&
                 measurement_input(u,u->settings.measurement_source)){
            u->last_accurate_ms=now;
            u->accurate_attempt_sequence=u->session.record_sequence;
            u->accurate_source=(uint8_t)u->settings.measurement_source;
            u->accurate_busy=(uint8_t)scope_accurate_begin(measurement_input(u,u->settings.measurement_source),u->session.record_count,
                                                       (float)u->session.actual_rate_hz);
            if(!u->accurate_busy)u->accurate_have_info=0U;
            u->accurate_sequence=u->session.record_sequence;
            u->accurate_pending_count=u->session.record_count;
        }
    }
    /* 准确任务拥有FFT工作区期间，显示FFT保持旧频谱与旧Fs。 */
    if(!u->settings.measurement_mode&&u->fft_enabled&&u->session.has_record&&!u->accurate_busy&&
       (uint32_t)(now-u->last_fft_ms)>=500U){
        u->last_fft_ms=now;
        if(u->settings.fft_source==2U){
            unsigned channel;size_t end=0U;
            start=u->session.record_count;
            /* 双谱用有效通道可见区间的并集，共用N/Fs，保证幅值与频率可比较。 */
            for(channel=0U;channel<2U;++channel)if(measurement_input(u,channel)){
                size_t first=channel?u->view_start2:u->view_start;
                size_t count=channel?u->view_count2:u->view_count;
                if(first<start)start=first;
                if(first+count>end)end=first+count;
            }
            limit=end>start?end-start:0U;
        }else{
            start=u->settings.fft_source==1U?u->view_start2:u->view_start;
            limit=u->settings.fft_source==1U?u->view_count2:u->view_count;
        }
        if(start>=u->session.record_count){start=0U;limit=u->session.record_count;}
        if(limit>u->session.record_count-start)limit=u->session.record_count-start;
        n=1U;while(n*2U<=limit)n*=2U;
        u->fft_mask=0U;u->fft_count=0U;
        if(n>=4U){
            unsigned int channel;
            for(channel=0U;channel<2U;channel++){
                const float *input=measurement_input(u,channel);
                float *bins=u->fft_mask?u->fft2:u->fft;
                size_t capacity=SCOPE_MAX_RECORD_LENGTH/2U+1U;
                if((u->settings.fft_source!=2U&&u->settings.fft_source!=channel)||!input)continue;
                if(scope_transform(input+start,n,channel?&u->settings.display2:&u->settings.display,u->display)){
                    /* 显示交流幅值谱：先去均值，避免直流经Hann窗泄漏遮住基波。 */
                    size_t sample;double mean=0.0;
                    for(sample=0U;sample<n;++sample)mean+=u->display[sample];
                    mean/=(double)n;
                    for(sample=0U;sample<n;++sample)u->display[sample]-=(float)mean;
                    if(scope_fft(u->display,n,bins,capacity))u->fft_mask|=(uint8_t)(1U<<channel);
                }
            }
        }
        if(u->fft_mask){
            u->fft_count=n;u->fft_rate_hz=u->session.actual_rate_hz;
            u->fft_sequence=u->session.record_sequence;
            if(u->fft_frequency_scale==1.0f&&u->fft_pan_x==0.0f){
                unsigned channel;size_t peak=1U;float frequency,span,decade=1.0f;
                for(channel=0U;channel<2U;++channel)if(u->fft_mask&(1U<<channel)){
                    const float *bins=channel&&u->fft_mask==3U?u->fft2:u->fft;
                    size_t j,best=1U;
                    for(j=2U;j<=n/2U;++j)if(bins[j]>bins[best])best=j;
                    if(best>peak)peak=best;
                }
                frequency=(float)peak*(float)u->fft_rate_hz/(float)n;
                span=frequency*8.0f;
                while(decade*10.0f<span)decade*=10.0f;
                span=span<=decade?decade:span<=2.0f*decade?2.0f*decade:
                    span<=5.0f*decade?5.0f*decade:10.0f*decade;
                u->fft_span_hz=fminf(span,(float)u->fft_rate_hz*0.5f);
            }
        }
        lv_obj_invalidate(u->plot);
    }
    if((uint32_t)(now-u->last_refresh_ms)<200U)return;
    u->last_refresh_ms=now;scope_driver_get_status(&d);
    if(u->session.dac_enabled&&!d.dac_enabled){
        char reason[112];
        (void)snprintf(reason,sizeof(reason),"DAC已停止 (%ld)：欠%lu DMA%lu，检查输出参数或重新开启",
            (long)d.last_error,(unsigned long)d.dac_underruns,(unsigned long)d.dac_dma_errors);
        scope_ui_status(reason);
    }
    if(u->session.dac_enabled!=d.dac_enabled||u->session.dac_rate_hz!=d.dac_rate_hz){
        u->session.dac_enabled=d.dac_enabled;u->session.dac_rate_hz=d.dac_rate_hz;
        scope_ui_sync_controls();
    }
    if(u->status_until_ms&&(int32_t)(now-u->status_until_ms)>=0){
        u->status_until_ms=0U;lv_obj_add_flag(u->notice,LV_OBJ_FLAG_HIDDEN);}
    (void)snprintf(text,sizeof(text),"%s / %s / %s",
       u->session.running?"运行":"停止",u->session.triggered?"已触发":"等待/自动",
       u->settings.measurement_mode?"准确":"流畅");
    lv_label_set_text(u->status,text);
    lv_label_set_text(u->run_label,u->session.running?"停止":"运行");
    lv_obj_set_style_bg_color(lv_obj_get_parent(u->run_label),
        lv_color_hex(u->session.running?0x1c5145:0x643b2c),0);
    lv_obj_set_style_border_color(lv_obj_get_parent(u->run_label),
        lv_color_hex(u->session.running?0x5ed5aa:0xefad7e),0);
    scope_format_quantity(value[0],sizeof(value[0]),u->settings.mv_per_div,SCOPE_UNIT_MV);
    (void)snprintf(text,sizeof(text),"%s / %s/div / %s",u->settings.io.enabled[0]?"开启":"关闭",
        value[0],u->settings.io.input[0]==SCOPE_INPUT_PA5?"PA5":"PC4");
    lv_label_set_text(u->channel_label,text);
    scope_format_quantity(value[0],sizeof(value[0]),u->settings.mv_per_div2,SCOPE_UNIT_MV);
    (void)snprintf(text,sizeof(text),"%s / %s/div / %s",u->settings.io.enabled[1]?"开启":"关闭",
        value[0],u->settings.io.input[1]==SCOPE_INPUT_PA5?"PA5":"PC4");
    lv_label_set_text(u->channel_label2,text);
    update_readings(NULL,NULL);
    /* 旧冻结帧可能仍在等待新触发；标题必须遵守实际绘制记录的Fs。 */
    update_time_label();
    scope_format_quantity(value[0],sizeof(value[0]),u->session.actual_rate_hz,SCOPE_UNIT_RATE);
    scope_format_quantity(value[1],sizeof(value[1]),u->settings.trigger.level_mv,SCOPE_UNIT_MV);
    if(d.dac_enabled&&d.dac_rate_hz==0U)
        scope_format_quantity(value[2],sizeof(value[2]),u->settings.dac.offset_mv,SCOPE_UNIT_MV);
    else scope_format_quantity(value[2],sizeof(value[2]),
       (d.dac_enabled&&u->session.dac_count)?
       (double)d.dac_rate_hz*u->session.dac_cycles/u->session.dac_count:0.0,SCOPE_UNIT_HZ);
    (void)snprintf(text,sizeof(text),"%s / %lu点 / T: CH%lu %s / DAC %s %s / 校准: %s / 丢%lu 欠%lu 恢%lu DMA%lu ADC%lu",
       value[0],
       (unsigned long)u->session.record_count,
       (unsigned long)(u->session.config.trigger_source+1U),
       value[1],
       d.dac_enabled?"开启":"关闭",
       value[2],
       u->settings.calibration.zero_code!=0.0f||u->settings.calibration.gain!=1.0f?"用户":"默认",
       (unsigned long)d.dropped_frames,(unsigned long)d.dac_underruns,
       (unsigned long)d.dac_recoveries,
       (unsigned long)d.dac_dma_errors,(unsigned long)d.adc_errors);
    lv_label_set_text(u->footer,text);
    if(!u->session.has_record)return;
    if(u->settings.cursor_enabled==2U)u->settings.measurement_source=u->selected_channel;
    if(!measurement_input(u,u->settings.measurement_source)){
        update_channel_readings(NULL);show_measurements(NULL);
        lv_label_set_text(u->measure,"所选通道暂无记录");return;
    }
    if(u->settings.measurement_mode==1U&&!u->accurate_ready){
        size_t i;for(i=0U;i<4U;i++)lv_label_set_text(u->readings[i],"--");
        show_measurements(NULL);lv_label_set_text(u->measure,"准确模式：等待计算");
        lv_label_set_text(u->measure_summary,u->session.running?"等待完整记录；可离开此页立即取消":"正在计算；可离开此页立即取消");
        lv_label_set_text(u->cursor_label,"");return;
    }
    if(u->settings.measurement_mode==0U&&u->settings.cursor_enabled!=2U){
        start=u->settings.measurement_source?u->view_start2:u->view_start;
        n=u->settings.measurement_source?u->view_count2:u->view_count;
        if(n<2U||start+n>u->session.record_count){start=0U;n=u->session.record_count;}
        
        /* 连续原始点和真实Fs；不能用均匀抽点后的显示数组测频。 */
        if(scope_measure(visible_measurement_input(u,u->settings.measurement_source),n,
            (float)u->session.actual_rate_hz,&u->session.measurements)){
            u->session.measurement_sequence=u->session.record_sequence;u->measurement_count=n;
        }
    }else if(u->settings.measurement_mode)u->measurement_count=u->accurate_record_count;
    m=u->session.measurements;
    if(u->settings.cursor_enabled==2U)follow_measurement_cursors(u,&m);
    show_measurements(&m);
    update_channel_readings(&m);
    lv_label_set_text(u->measure_summary,u->settings.measurement_mode?
        (u->accurate_have_info?accurate_reason_text(u->accurate_info.reason):"准确结果已就绪；离页即关闭"):
        "流畅模式：测量可见真实记录；完整采集至少512点");
    if((u->settings.measurement_mode&&u->settings.cursor_enabled!=2U)||u->xy_enabled||u->fft_enabled){lv_label_set_text(u->cursor_label,"");return;}
    dt=u->display_rate_hz?fabsf(u->settings.cursor_x[1]-u->settings.cursor_x[0])/(float)u->display_rate_hz:0.0f;
    dv=fabsf(u->settings.cursor_mv[1]-u->settings.cursor_mv[0]);
    scope_format_quantity(value[0],sizeof(value[0]),dt,SCOPE_UNIT_SECONDS);
    scope_format_quantity(value[1],sizeof(value[1]),dv,SCOPE_UNIT_MV);
    scope_format_quantity(value[2],sizeof(value[2]),dt>0?1.0/(double)dt:0.0,SCOPE_UNIT_HZ);
    if(u->settings.cursor_enabled==2U){
        if(!(u->follow_valid&SCOPE_VALID_PERIOD)){(void)snprintf(value[0],sizeof(value[0]),"--");(void)snprintf(value[2],sizeof(value[2]),"--");}
        if(!(u->follow_valid&SCOPE_VALID_VPP))(void)snprintf(value[1],sizeof(value[1]),"--");
    }
    (void)snprintf(text,sizeof(text),"游标 dt %s  dV %s  1/dt %s",value[0],value[1],value[2]);
    lv_label_set_text(u->cursor_label,u->settings.cursor_enabled?text:"");
}
