#include "oscilloscope_ui_private.h"
#include "gui_guider.h"
#include "oscilloscope_phy.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { lv_obj_t *object;void *value;double scale;uint8_t type,size; } scope_field_t;
static scope_field_t fields[96];
static size_t field_count;
static uint8_t syncing_controls;
static float dac_wave_amplitude_mv;
static void commit_field(lv_event_t *e);
static void apply_command(intptr_t command,scope_field_t *field);
static uint16_t preview_codes[256];
static uint8_t preview_valid;
static scope_dac_config_t preview_config;
static float preview_vref;
static lv_obj_t *preview;
static lv_obj_t *record_summary;
static lv_obj_t *dac_summary;
static lv_obj_t *measure_sections[3],*measure_buttons[3];
static int row_y;
static void draw_preview(lv_event_t *event)
{
    lv_area_t a;lv_layer_t *layer=lv_event_get_layer(event);lv_draw_line_dsc_t d;size_t i;
    if(!preview_valid)return;
    lv_obj_get_coords(preview,&a);lv_draw_line_dsc_init(&d);
    d.color=lv_color_hex(0xc1a1ff);d.width=1;
    for(i=1U;i<256U;i++){
        d.p1.x=a.x1+(int)((i-1U)*(size_t)lv_area_get_width(&a)/256U);
        d.p2.x=a.x1+(int)(i*(size_t)lv_area_get_width(&a)/256U);
        d.p1.y=a.y2-(int)((uint32_t)preview_codes[i-1U]*(uint32_t)(lv_area_get_height(&a)-1)/4095U);
        d.p2.y=a.y2-(int)((uint32_t)preview_codes[i]*(uint32_t)(lv_area_get_height(&a)-1)/4095U);
        lv_draw_line(layer,&d);
    }
}
static lv_obj_t *text(lv_obj_t *p,const char *s,int x,int y)
{
    lv_obj_t *o=lv_label_create(p);lv_obj_set_pos(o,x,y);lv_label_set_text(o,s);
    lv_obj_set_style_text_font(o,&lv_font_myyh_16,0);
    lv_obj_set_style_text_color(o,lv_color_hex(0xd7e4ee),0);
    lv_obj_set_width(o,226);return o;
}
static void close_keyboard(lv_event_t *e)
{
    if(g_scope_ui.keyboard){
        lv_obj_t *keyboard=g_scope_ui.keyboard,*field=lv_keyboard_get_textarea(keyboard);
        if(field&&(!e||lv_event_get_code(e)!=LV_EVENT_CANCEL))lv_obj_send_event(field,LV_EVENT_READY,NULL);
        else if(field)scope_ui_sync_controls();
        if(g_scope_ui.keyboard==keyboard){lv_obj_delete(keyboard);g_scope_ui.keyboard=NULL;}
    }
}
static void edit(lv_event_t *e)
{
    if(!g_scope_ui.keyboard_enabled)return;
    if(!g_scope_ui.keyboard){
        g_scope_ui.keyboard=lv_keyboard_create(g_scope_ui.screen);
        lv_obj_set_size(g_scope_ui.keyboard,1024,260);
        lv_obj_align(g_scope_ui.keyboard,LV_ALIGN_BOTTOM_MID,0,0);
        lv_keyboard_set_mode(g_scope_ui.keyboard,LV_KEYBOARD_MODE_NUMBER);
        lv_obj_add_event_cb(g_scope_ui.keyboard,close_keyboard,LV_EVENT_READY,NULL);
        lv_obj_add_event_cb(g_scope_ui.keyboard,close_keyboard,LV_EVENT_CANCEL,NULL);
    }
    lv_keyboard_set_textarea(g_scope_ui.keyboard,lv_event_get_target_obj(e));
}
void scope_ui_keyboard_toggle(lv_event_t *e)
{
    g_scope_ui.keyboard_enabled=(uint8_t)!g_scope_ui.keyboard_enabled;
    if(!g_scope_ui.keyboard_enabled)close_keyboard(NULL);
    lv_label_set_text(lv_obj_get_child(lv_event_get_target_obj(e),0),
        g_scope_ui.keyboard_enabled?"键盘开":"键盘关");
}
static void number(lv_obj_t *p,const char *caption,int x,void *value,uint8_t type)
{
    lv_obj_t *o,*title;char buf[32];double n,scale=1.0;
    if(field_count>=sizeof(fields)/sizeof(fields[0]))return;
    if(type==0U)n=(double)*(float *)value;
    else if(type==2U)n=(double)*(size_t *)value;
    else n=(double)*(uint32_t *)value;
    if(value==&g_scope_ui.settings.time_per_div_s)scale=1000000.0;
    (void)x;scope_format_fixed(buf,sizeof(buf),n*scale,2U);title=text(p,caption,0,row_y);
    lv_obj_update_layout(title);row_y+=(int)lv_obj_get_height(title)+6;
    o=lv_textarea_create(p);lv_obj_set_pos(o,0,row_y);lv_obj_set_size(o,226,40);row_y+=52;
    lv_textarea_set_one_line(o,true);lv_textarea_set_max_length(o,18U);
    lv_obj_set_style_text_font(o,&lv_font_myyh_20,0);lv_textarea_set_text(o,buf);
    lv_obj_set_style_bg_color(o,lv_color_hex(0x223748),0);
    lv_obj_set_style_text_color(o,lv_color_hex(0xeff7fc),0);
    lv_obj_set_style_border_color(o,lv_color_hex(0x405569),0);
    lv_obj_add_event_cb(o,edit,LV_EVENT_CLICKED,NULL);
    lv_obj_add_event_cb(o,edit,LV_EVENT_FOCUSED,NULL);
    fields[field_count].object=o;fields[field_count].value=value;fields[field_count].scale=scale;
    fields[field_count].type=type;
    lv_obj_add_event_cb(o,commit_field,LV_EVENT_READY,&fields[field_count]);
    lv_obj_add_event_cb(o,commit_field,LV_EVENT_DEFOCUSED,&fields[field_count]);
    field_count++;
}
static void choice_impl(lv_obj_t *p,const char *caption,int x,const char *options,void *value,size_t bytes)
{
    lv_obj_t *o,*title;uint32_t selected=0U;
    if(field_count>=sizeof(fields)/sizeof(fields[0]))return;
    (void)x;memcpy(&selected,value,bytes);title=text(p,caption,0,row_y);
    lv_obj_update_layout(title);row_y+=(int)lv_obj_get_height(title)+6;
    o=lv_dropdown_create(p);lv_obj_set_pos(o,0,row_y);lv_obj_set_size(o,226,40);row_y+=52;
    lv_obj_set_style_bg_color(o,lv_color_hex(0x223748),0);
    lv_obj_set_style_text_color(o,lv_color_hex(0xeff7fc),0);
    lv_obj_set_style_border_color(o,lv_color_hex(0x405569),0);
    lv_obj_set_style_bg_color(lv_dropdown_get_list(o),lv_color_hex(0x223748),0);
    lv_obj_set_style_text_color(lv_dropdown_get_list(o),lv_color_hex(0xeff7fc),0);
    lv_obj_set_style_text_font(o,&lv_font_myyh_20,0);
    lv_dropdown_set_options(o,options);lv_dropdown_set_selected(o,selected);
    fields[field_count].object=o;fields[field_count].value=value;fields[field_count].type=3U;
    fields[field_count].size=(uint8_t)bytes;
    lv_obj_add_event_cb(o,commit_field,LV_EVENT_VALUE_CHANGED,&fields[field_count]);
    field_count++;
}
#define choice(p,c,x,o,v) choice_impl((p),(c),(x),(o),(v),sizeof(*(v)))
static int read_fields(scope_field_t *only)
{
    size_t i;char *end;char original[32];float v;uint32_t n;size_t z;
    for(i=0;i<field_count;i++){
        scope_field_t *f=&fields[i];
        if(only&&only!=f)continue;
        if(f->type==3U){n=lv_dropdown_get_selected(f->object);memcpy(f->value,&n,f->size);continue;}
        if(f->type==0U){
            scope_format_fixed(original,sizeof(original),(double)*(float *)f->value*f->scale,2U);
            if(strcmp(original,lv_textarea_get_text(f->object))==0)continue;
        }
        v=strtof(lv_textarea_get_text(f->object),&end);
        if(end==lv_textarea_get_text(f->object)||*end||!isfinite(v))return 0;
        if(f->type==0U){v=(float)((double)v/f->scale);memcpy(f->value,&v,sizeof(v));continue;}
        if(v<0.0f||v>10000000.0f||floorf(v)!=v)return 0;
        if(f->type==2U){z=(size_t)v;memcpy(f->value,&z,sizeof(z));}
        else {n=(uint32_t)v;memcpy(f->value,&n,sizeof(n));}
    }
    return 1;
}
void scope_ui_sync_controls(void)
{
    size_t i;uint32_t n;double v;char buf[128],rate[32],frequency[32];
    syncing_controls=1U;
    for(i=0;i<field_count;i++){
        scope_field_t *f=&fields[i];
        if(f->type==3U){n=0U;memcpy(&n,f->value,f->size);lv_dropdown_set_selected(f->object,n);continue;}
        if(f->type==0U)v=(double)*(float *)f->value;
        else if(f->type==2U)v=(double)*(size_t *)f->value;
        else {memcpy(&n,f->value,sizeof(n));v=(double)n;}
        scope_format_fixed(buf,sizeof(buf),v*f->scale,2U);lv_textarea_set_text(f->object,buf);
    }
    if(preview&&lv_obj_is_valid(preview)){
        if(!preview_valid||preview_vref!=g_scope_ui.settings.calibration.vref_mv||
           memcmp(&preview_config,&g_scope_ui.settings.dac,sizeof(preview_config))!=0){
            preview_config=g_scope_ui.settings.dac;preview_vref=g_scope_ui.settings.calibration.vref_mv;
            preview_valid=(uint8_t)scope_make_dac(&preview_config,preview_vref,
                preview_codes,sizeof(preview_codes)/sizeof(preview_codes[0]));
        }
        lv_obj_invalidate(preview);
    }
    if(record_summary&&lv_obj_is_valid(record_summary)){
        scope_format_quantity(rate,sizeof(rate),g_scope_ui.settings.sample_rate_hz,SCOPE_UNIT_RATE);
        (void)snprintf(buf,sizeof(buf),"%lu点 / %s / %s",
            (unsigned long)g_scope_ui.settings.trigger.window_samples,
            rate,
            g_scope_ui.settings.measurement_mode?"准确":"流畅");
        lv_label_set_text(record_summary,buf);
    }
    if(dac_summary&&lv_obj_is_valid(dac_summary)){
        if(g_scope_ui.session.dac_enabled&&g_scope_ui.session.dac_rate_hz==0U){
            scope_format_quantity(rate,sizeof(rate),g_scope_ui.settings.dac.offset_mv,SCOPE_UNIT_MV);
            (void)snprintf(buf,sizeof(buf),"直流保持 / %s",rate);
        }else if(g_scope_ui.session.dac_enabled){
            scope_format_quantity(rate,sizeof(rate),g_scope_ui.session.dac_rate_hz,SCOPE_UNIT_RATE);
            scope_format_quantity(frequency,sizeof(frequency),g_scope_ui.session.dac_frequency_hz,SCOPE_UNIT_HZ);
            (void)snprintf(buf,sizeof(buf),"实际 %s / %s / %.1f点/周期",frequency,rate,
                g_scope_ui.session.dac_cycles?(double)g_scope_ui.session.dac_count/g_scope_ui.session.dac_cycles:0.0);
        }
        else (void)snprintf(buf,sizeof(buf),"输出关闭；采样率自动选择");
        lv_label_set_text(dac_summary,buf);
    }
    if(g_scope_ui.accurate_button){
        lv_label_set_text(lv_obj_get_child(g_scope_ui.accurate_button,0),
            g_scope_ui.settings.measurement_mode?"关闭准确模式":"开启准确模式");
    }
    syncing_controls=0U;
}
int scope_ui_measure_page_active(void)
{
    return g_scope_ui.tabs&&g_scope_ui.measure_page&&
        lv_obj_get_child(lv_tabview_get_content(g_scope_ui.tabs),
            (int32_t)lv_tabview_get_tab_active(g_scope_ui.tabs))==g_scope_ui.measure_page;
}
static void measurement_toggle(lv_event_t *e)
{
    char *end;unsigned long n;size_t i;(void)e;
    if(!g_scope_ui.settings.measurement_mode){
        for(i=0U;i<field_count;i++)if(fields[i].value==&g_scope_ui.accurate_samples){
            const char *input=lv_textarea_get_text(fields[i].object);
            n=strtoul(input,&end,10);
            if(end==input||*end||n<512UL||n>8192UL){scope_ui_status("准确记录点数范围512~8192");return;}
            g_scope_ui.accurate_samples=(size_t)n;break;
        }
    }
    if(scope_ui_set_accurate(!g_scope_ui.settings.measurement_mode,1)!=0)
        scope_ui_status("准确模式切换失败，请检查采集状态");
}
static void tab_changed(lv_event_t *e)
{
    (void)e;
    close_keyboard(NULL);
    if(lv_tabview_get_tab_active(g_scope_ui.tabs)<2U)
        scope_ui_select_channel(lv_tabview_get_tab_active(g_scope_ui.tabs));
    if(!scope_ui_measure_page_active()&&g_scope_ui.settings.measurement_mode)
        (void)scope_ui_set_accurate(0,1);
}
static void apply_readings(void)
{
    memcpy(g_scope_ui.smooth_settings.reading_item,g_scope_ui.settings.reading_item,
        sizeof(g_scope_ui.settings.reading_item));
    memcpy(g_scope_ui.smooth_settings.reading_source,g_scope_ui.settings.reading_source,
        sizeof(g_scope_ui.settings.reading_source));
    scope_ui_status("常驻显示已更新，颜色跟随通道；保存设置后保留");
    g_scope_ui.last_refresh_ms=lv_tick_get()-200U;
}
static void apply(lv_event_t *e)
{apply_command((intptr_t)lv_event_get_user_data(e),NULL);}
static void apply_command(intptr_t command,scope_field_t *field)
{
    scope_settings_t previous=g_scope_ui.settings;float old_reference=g_scope_ui.reference_mv;
    float old_plot_offset=g_scope_ui.plot_vertical_offset_mv;
    float old_plot_offset2=g_scope_ui.plot_vertical_offset2_mv;
    int was_running=g_scope_ui.session.running;
    int r,dac_failure=0,hardware_changed;
    float requested,frequency=0.0f;char message[180];
    scope_phy_diagnostic_t phy={0};
    scope_driver_status_t adc={0};
    if(command==2){
        r=scope_session_dac(&g_scope_ui.session,NULL,0);
        scope_ui_status(r==0?"DAC输出已关闭":"DAC关闭失败，请检查驱动状态");return;
    }
    r=field?read_fields(field):1;
    if(field&&r&&field->value==&g_scope_ui.settings.dac.waveform){
        if(g_scope_ui.settings.dac.waveform==SCOPE_WAVE_DC){
            if(previous.dac.waveform!=SCOPE_WAVE_DC)
                dac_wave_amplitude_mv=previous.dac.amplitude_mv;
            g_scope_ui.settings.dac.amplitude_mv=0.0f;
        }else if(previous.dac.waveform==SCOPE_WAVE_DC&&g_scope_ui.settings.dac.amplitude_mv==0.0f){
            float headroom=2.0f*fminf(g_scope_ui.settings.dac.offset_mv,
                g_scope_ui.settings.calibration.vref_mv-g_scope_ui.settings.dac.offset_mv);
            g_scope_ui.settings.dac.amplitude_mv=fminf(dac_wave_amplitude_mv,fmaxf(0.0f,headroom));
        }
    }
    if(field&&r&&memcmp(&previous,&g_scope_ui.settings,sizeof(previous))==0){scope_ui_sync_controls();return;}
    if(r&&scope_io_valid(&g_scope_ui.settings.io)&&
       memcmp(&previous.io,&g_scope_ui.settings.io,sizeof(previous.io))!=0&&
       g_scope_ui.settings.sample_rate_hz>(float)scope_io_max_rate(&g_scope_ui.settings.io))
        g_scope_ui.settings.sample_rate_hz=(float)scope_io_max_rate(&g_scope_ui.settings.io);
    if(!r||!scope_settings_valid(&g_scope_ui.settings)){
        unsigned h=scope_dac_highest_harmonic(&g_scope_ui.settings.dac);
        int frequency_error=h&&g_scope_ui.settings.dac.frequency_hz>31250.0f/(float)h;
        g_scope_ui.settings=previous;g_scope_ui.reference_mv=old_reference;
        if(frequency_error){char limit[32];
            scope_format_quantity(limit,sizeof(limit),31250.0/(double)h,SCOPE_UNIT_HZ);
            (void)snprintf(message,sizeof(message),"DAC频率超限：最高%u次谐波，基频上限%s",h,limit);
            scope_ui_status(message);
        }else scope_ui_status("参数无效：未应用，请检查范围");
        scope_ui_sync_controls();return;
    }
    requested=g_scope_ui.settings.time_per_div_s;
    if(previous.vertical_offset2_mv!=g_scope_ui.settings.vertical_offset2_mv||
       previous.mv_per_div2!=g_scope_ui.settings.mv_per_div2||
       previous.display2.coupling!=g_scope_ui.settings.display2.coupling||
       previous.display2.probe_factor!=g_scope_ui.settings.display2.probe_factor||
       previous.display2.inverted!=g_scope_ui.settings.display2.inverted)
        g_scope_ui.plot_vertical_offset2_mv=0.0f;
    if(previous.vertical_offset_mv!=g_scope_ui.settings.vertical_offset_mv||
       previous.mv_per_div!=g_scope_ui.settings.mv_per_div||
       previous.display.coupling!=g_scope_ui.settings.display.coupling||
       previous.display.probe_factor!=g_scope_ui.settings.display.probe_factor||
       previous.display.inverted!=g_scope_ui.settings.display.inverted)
        g_scope_ui.plot_vertical_offset_mv=0.0f;
    if(g_scope_ui.session.measurement_sequence==g_scope_ui.session.record_sequence&&
       (g_scope_ui.session.measurements.valid&SCOPE_VALID_FREQUENCY))
        frequency=g_scope_ui.session.measurements.frequency_hz;
    hardware_changed=previous.time_per_div_s!=g_scope_ui.settings.time_per_div_s||
        previous.trigger_mode!=g_scope_ui.settings.trigger_mode||previous.holdoff_ms!=g_scope_ui.settings.holdoff_ms||
        memcmp(&previous.io,&g_scope_ui.settings.io,sizeof(previous.io))||
        memcmp(&previous.trigger,&g_scope_ui.settings.trigger,sizeof(previous.trigger))||
        memcmp(&previous.calibration,&g_scope_ui.settings.calibration,sizeof(previous.calibration))||
        memcmp(&previous.calibration2,&g_scope_ui.settings.calibration2,sizeof(previous.calibration2));
    if(hardware_changed)r=scope_ui_apply_settings();
    else {
        r=0;g_scope_ui.history_count=0U;g_scope_ui.average_frames=0U;g_scope_ui.average_frames2=0U;
        scope_ui_rebuild_display(0);g_scope_ui.last_refresh_ms=lv_tick_get()-200U;
    }
    if(!r&&previous.cursor_enabled!=1U&&g_scope_ui.settings.cursor_enabled==1U&&g_scope_ui.session.record_count>1U){
        scope_ui_t *u=&g_scope_ui;unsigned c=u->selected_channel;int k;
        float count=fmaxf(1.0f,u->view_span[c]-1.0f),pan=c?u->pan_x2:u->pan_x;
        float half=(c?u->settings.mv_per_div2:u->settings.mv_per_div)*4.0f/
            (c?u->plot_vertical_scale2:u->plot_vertical_scale);
        float center=(c?u->settings.vertical_offset2_mv+u->plot_vertical_offset2_mv:scope_ui_vertical_offset())+
            (c?u->pan_y2:u->pan_y)*half*2.0f/(float)lv_obj_get_height(u->plot);
        float lo=fmaxf(0.0f,u->view_origin[c]-pan*count/(SCOPE_PLOT_WIDTH-1));
        float hi=fminf((float)(u->session.record_count-1U),
            u->view_origin[c]+((SCOPE_PLOT_WIDTH-1)-pan)*count/(SCOPE_PLOT_WIDTH-1));
        /* 默认B=1024常在流畅记录外；按平移后的可见交集安置，保留有效输入。 */
        if(lo<=hi&&(u->settings.cursor_x[0]<lo||u->settings.cursor_x[0]>hi||
                    u->settings.cursor_x[1]<lo||u->settings.cursor_x[1]>hi)){
            for(k=0;k<2;k++)u->settings.cursor_x[k]=lo+(hi-lo)*(k?0.7f:0.3f);
        }
        for(k=0;k<2;k++)if(fabsf(u->settings.cursor_mv[k]-center)>=half)
            u->settings.cursor_mv[k]=fmaxf(-1.0e9f,fminf(1.0e9f,center+half*(k?-0.4f:0.4f)));
    }
    if(command==1&&r==0){
        r=scope_session_dac(&g_scope_ui.session,&g_scope_ui.settings.dac,1);dac_failure=r!=0;
    }
    if(r==-13)scope_phy_get_diagnostic(&phy);
    if(r)scope_driver_get_status(&adc);
    if(r){g_scope_ui.settings=previous;g_scope_ui.plot_vertical_offset_mv=old_plot_offset;
        g_scope_ui.plot_vertical_offset2_mv=old_plot_offset2;
        if(hardware_changed&&scope_ui_apply_settings()==0&&was_running&&g_scope_ui.session.opened&&!g_scope_ui.session.running)
            (void)scope_session_run(&g_scope_ui.session,lv_tick_get());}
    if(!r&&field&&previous.measurement_mode&&g_scope_ui.settings.measurement_mode){
        uintptr_t begin=(uintptr_t)&g_scope_ui.settings,address=(uintptr_t)field->value;
        size_t bytes=field->type==3U?field->size:field->type==2U?sizeof(size_t):sizeof(uint32_t);
        if(address>=begin&&address-begin<=sizeof(g_scope_ui.settings)-bytes){
            memcpy((uint8_t *)&g_scope_ui.smooth_settings+(address-begin),field->value,bytes);
            /* 准确模式的记录长度临时扩展，退出时只带回用户修改的字段。 */
            if(address>=(uintptr_t)&g_scope_ui.settings.dac&&
               address<(uintptr_t)&g_scope_ui.settings.dac+sizeof(g_scope_ui.settings.dac))
                g_scope_ui.smooth_settings.dac=g_scope_ui.settings.dac;
            if(g_scope_ui.smooth_settings.sample_rate_hz>(float)scope_io_max_rate(&g_scope_ui.smooth_settings.io))
                g_scope_ui.smooth_settings.sample_rate_hz=(float)scope_io_max_rate(&g_scope_ui.smooth_settings.io);
            if(g_scope_ui.smooth_settings.trigger.pre_samples>g_scope_ui.smooth_settings.trigger.window_samples*9U/10U)
                g_scope_ui.smooth_settings.trigger.pre_samples=g_scope_ui.smooth_settings.trigger.window_samples*9U/10U;
        }
    }
    scope_ui_sync_controls();
    if(r==0&&command==0&&hardware_changed&&!g_scope_ui.settings.measurement_mode){
        char time_text[32],rate_text[32];
        scope_format_quantity(time_text,sizeof(time_text),g_scope_ui.settings.time_per_div_s,SCOPE_UNIT_SECONDS);
        scope_format_quantity(rate_text,sizeof(rate_text),g_scope_ui.settings.sample_rate_hz,SCOPE_UNIT_RATE);
        (void)snprintf(message,sizeof(message),"已应用 %s/div / %s / %lu点%s%s",time_text,rate_text,
            (unsigned long)g_scope_ui.settings.trigger.window_samples,
            fabsf(g_scope_ui.settings.time_per_div_s-requested)>requested*0.01f?
                "；已达采样率/512～8192点边界":"",
            !g_scope_ui.session.running&&g_scope_ui.session.has_record?
                "；停止中，旧记录按原Fs显示，运行后更新":
            frequency>0.0f&&g_scope_ui.settings.sample_rate_hz<frequency*16.0f?
                "；采样点/周期偏少，可能混叠":"");
        scope_ui_status(message);
    }else if(r==-13&&!dac_failure){
        const char *step=phy.stage==SCOPE_PHY_NO_RESPONSE?"MDIO无应答":
            phy.stage==SCOPE_PHY_ID_MISMATCH?"型号不符":
            phy.stage==SCOPE_PHY_CONTROL_READ?"控制寄存器读取失败":
            phy.stage==SCOPE_PHY_RESETTING?"仍在复位":"隔离读回失败";
        (void)snprintf(message,sizeof(message),"PC4 PHY失败(-13) %s ID:%04X:%04X BMCR:%04X",
            step,(unsigned)phy.id1,(unsigned)phy.id2,(unsigned)phy.bmcr);scope_ui_status(message);
    }else if(r&&!dac_failure&&(r==-4||r==-5||r==-7||r==-10)){
        scope_ui_adc_error(r,&adc);
    }else if(r){
        const char *reason=dac_failure?"DAC输出失败：检查幅值/偏置、更新率及DMA":
            r==-13?"PC4启用失败：PHY识别/隔离未通过，保留原通道":
            r==-12?"输入IO配置失败：检查通道重复、运行状态或输出冲突":
            r==-4?"ADC时钟/采样配置失败":r==-5?"ADC DMA启动失败":
            r==-10?"ADC停止/恢复未完成":"ADC配置/启动失败";
        (void)snprintf(message,sizeof(message),"%s (%d)",reason,r);scope_ui_status(message);
    }else scope_ui_status("设置已生效");
}
static void commit_field(lv_event_t *e)
{
    scope_field_t *field=(scope_field_t *)lv_event_get_user_data(e);
    uintptr_t address,begin;size_t i;
    if(syncing_controls||!field)return;
    if(field->value==&g_scope_ui.accurate_samples){
        size_t old=g_scope_ui.accurate_samples;
        if(!read_fields(field)||g_scope_ui.accurate_samples<512U||g_scope_ui.accurate_samples>8192U){
            g_scope_ui.accurate_samples=old;scope_ui_status("准确记录点数范围512~8192");
        }else scope_ui_status("准确点数已设置，下次开启准确模式生效");
        scope_ui_sync_controls();return;
    }
    if(field->value==&g_scope_ui.reference_mv){
        float old=g_scope_ui.reference_mv;
        if(!read_fields(field)||g_scope_ui.reference_mv<=0.0f||g_scope_ui.reference_mv>3600.0f){
            g_scope_ui.reference_mv=old;scope_ui_status("参考电压范围大于0且不超过3600mV");
        }else scope_ui_status("参考电压已设置");
        scope_ui_sync_controls();return;
    }
    address=(uintptr_t)field->value;begin=(uintptr_t)&g_scope_ui.settings.dac;
    apply_command(g_scope_ui.session.dac_enabled&&((address>=begin&&address<begin+sizeof(g_scope_ui.settings.dac))||
        field->value==&g_scope_ui.settings.calibration.vref_mv)?1:0,field);
    for(i=0U;i<4U;++i)if(field->value==&g_scope_ui.settings.reading_item[i]||field->value==&g_scope_ui.settings.reading_source[i]){
        apply_readings();break;
    }
}
static void file_action(lv_event_t *e)
{
    intptr_t op=(intptr_t)lv_event_get_user_data(e);
    if(op==5){
        size_t i;char *end;float reference;
        for(i=0;i<field_count;i++)if(fields[i].value==&g_scope_ui.reference_mv){
            const char *value=lv_textarea_get_text(fields[i].object);
            reference=strtof(value,&end);
            if(end==value||*end||!isfinite(reference)||reference<=0.0f||reference>3600.0f){
                scope_ui_status("参考电压无效");return;
            }
            g_scope_ui.reference_mv=reference;break;
        }
    }
    if(op==2&&!scope_settings_valid(&g_scope_ui.settings)){
        scope_ui_status("设置未保存：参数无效");return;
    }
    scope_ui_files((unsigned int)op);scope_ui_sync_controls();
}
static lv_obj_t *tab(lv_obj_t *view,const char *name)
{
    lv_obj_t *p=lv_tabview_add_tab(view,name),*b=lv_tabview_get_tab_button(view,-1);
    row_y=0;
    lv_obj_set_style_bg_color(p,lv_color_hex(0x192b3b),0);lv_obj_set_style_bg_opa(p,LV_OPA_COVER,0);
    lv_obj_set_style_pad_all(p,8,0);lv_obj_set_scroll_dir(p,LV_DIR_VER);
    lv_obj_set_style_border_width(p,0,0);
    lv_obj_set_flex_grow(b,0);lv_obj_set_size(b,76,48);
    lv_obj_set_style_radius(b,7,0);lv_obj_set_style_shadow_width(b,0,0);
    lv_obj_set_style_bg_color(b,lv_color_hex(0x203243),0);
    lv_obj_set_style_bg_color(b,lv_color_hex(0x355367),LV_STATE_CHECKED);
    lv_obj_set_style_border_width(b,0,0);
    lv_obj_set_style_border_width(b,1,LV_STATE_CHECKED);
    lv_obj_set_style_border_color(b,lv_color_hex(0x8de0cb),LV_STATE_CHECKED);
    lv_obj_set_style_border_side(b,LV_BORDER_SIDE_FULL,LV_STATE_CHECKED);
    lv_obj_set_style_text_color(b,lv_color_hex(0xe8f0f6),0);
    lv_obj_set_style_text_color(b,lv_color_hex(0xe8f0f6),LV_STATE_CHECKED);
    lv_obj_set_style_text_font(b,&lv_font_myyh_16,0);return p;
}
static lv_obj_t *page_button(lv_obj_t *p,const char *name,lv_event_cb_t cb,void *data)
{
    lv_obj_t *b=scope_ui_button(p,name,0,row_y,226,42,cb,data);row_y+=54;return b;
}
static void page_note(lv_obj_t *p,const char *s)
{lv_obj_t *l=text(p,s,0,row_y);lv_obj_update_layout(l);row_y+=(int)lv_obj_get_height(l)+12;}
static void display_fft(lv_event_t *e)
{
    (void)e;
    if(g_scope_ui.settings.measurement_mode){scope_ui_status("请先关闭准确模式");return;}
    g_scope_ui.xy_enabled=0U;g_scope_ui.fft_enabled=(uint8_t)!g_scope_ui.fft_enabled;
    scope_ui_rebuild_display(0);
}
static void display_xy(lv_event_t *e)
{
    (void)e;
    if(g_scope_ui.settings.measurement_mode){scope_ui_status("请先关闭准确模式");return;}
    g_scope_ui.xy_enabled=(uint8_t)!g_scope_ui.xy_enabled;g_scope_ui.fft_enabled=0U;
    scope_ui_rebuild_display(0);
    if(g_scope_ui.xy_enabled)scope_ui_status("XY相位图：X=CH1，Y=CH2，需要两路同步记录");
}
static void fft_mode(lv_event_t *e)
{
    (void)e;g_scope_ui.fft_linear=(uint8_t)!g_scope_ui.fft_linear;
    g_scope_ui.fft_vertical_scale=1.0f;g_scope_ui.fft_pan_y=0.0f;
    lv_obj_invalidate(g_scope_ui.plot);
    scope_ui_status(g_scope_ui.fft_linear?"FFT：线性RMS幅值":"FFT：dBV对数RMS幅值，默认范围80dB");
}
static void fft_reset(lv_event_t *e)
{
    (void)e;g_scope_ui.fft_frequency_scale=g_scope_ui.fft_vertical_scale=1.0f;
    g_scope_ui.fft_pan_x=g_scope_ui.fft_pan_y=0.0f;
    g_scope_ui.last_fft_ms=lv_tick_get()-500U;
    lv_obj_invalidate(g_scope_ui.plot);scope_ui_status("FFT视图已复位，频宽按当前主峰适配");
}
static void measure_section(lv_event_t *e)
{
    unsigned i,selected=(unsigned)(uintptr_t)lv_event_get_user_data(e);
    close_keyboard(NULL);
    for(i=0U;i<3U;i++){
        if(i==selected){lv_obj_remove_flag(measure_sections[i],LV_OBJ_FLAG_HIDDEN);lv_obj_add_state(measure_buttons[i],LV_STATE_CHECKED);}
        else {lv_obj_add_flag(measure_sections[i],LV_OBJ_FLAG_HIDDEN);lv_obj_remove_state(measure_buttons[i],LV_STATE_CHECKED);}
    }
}
static lv_obj_t *measure_panel(lv_obj_t *page,unsigned index)
{
    lv_obj_t *panel=lv_obj_create(page);measure_sections[index]=panel;
    lv_obj_set_pos(panel,0,38);lv_obj_set_size(panel,226,238);
    lv_obj_set_style_pad_all(panel,0,0);lv_obj_set_style_border_width(panel,0,0);
    lv_obj_set_style_bg_opa(panel,LV_OPA_TRANSP,0);lv_obj_remove_flag(panel,LV_OBJ_FLAG_SCROLLABLE);
    if(index)lv_obj_add_flag(panel,LV_OBJ_FLAG_HIDDEN);
    row_y=0;return panel;
}
void scope_ui_controls(lv_obj_t *parent)
{
    scope_settings_t *s=&g_scope_ui.settings;lv_obj_t *v,*p,*bar,*content,*mp;
    field_count=0U;g_scope_ui.keyboard=NULL;g_scope_ui.reference_mv=1000.0f;
    dac_wave_amplitude_mv=s->dac.amplitude_mv>0.0f?s->dac.amplitude_mv:1000.0f;
    preview=NULL;record_summary=NULL;dac_summary=NULL;
    v=lv_tabview_create(parent);g_scope_ui.tabs=v;
    lv_obj_set_pos(v,674,158);lv_obj_set_size(v,338,294);
    lv_tabview_set_tab_bar_position(v,LV_DIR_LEFT);lv_tabview_set_tab_bar_size(v,88);
    lv_obj_set_style_bg_color(v,lv_color_hex(0x192b3b),0);
    lv_obj_set_style_bg_opa(v,LV_OPA_COVER,0);lv_obj_set_style_radius(v,10,0);
    lv_obj_set_style_border_width(v,1,0);lv_obj_set_style_border_color(v,lv_color_hex(0x30485d),0);
    lv_obj_set_style_pad_all(v,0,0);lv_obj_set_style_text_font(v,&lv_font_myyh_16,0);
    bar=lv_tabview_get_tab_bar(v);lv_obj_set_style_pad_all(bar,6,0);
    lv_obj_set_style_pad_row(bar,6,0);lv_obj_set_scroll_dir(bar,LV_DIR_VER);
    lv_obj_set_style_bg_color(bar,lv_color_hex(0x192b3b),0);
    lv_obj_set_style_bg_opa(bar,LV_OPA_COVER,0);
    content=lv_tabview_get_content(v);lv_obj_set_scroll_dir(content,LV_DIR_NONE);
    p=tab(v,"CH1");
    lv_obj_set_style_text_color(lv_tabview_get_tab_button(v,-1),lv_color_hex(0xffd66b),0);
    lv_obj_set_style_text_color(lv_tabview_get_tab_button(v,-1),lv_color_hex(0xffd66b),LV_STATE_CHECKED);
    choice(p,"通道开关",0,"关闭\n开启",&s->io.enabled[0]);
    choice(p,"输入引脚",0,"PA5 (慢通道)\nPC4 (高速)",&s->io.input[0]);
    number(p,"量程 mV/div",0,&s->mv_per_div,0);
    number(p,"中心位置 mV",0,&s->vertical_offset_mv,0);
    choice(p,"软件耦合",0,"DC\nAC\nGND",&s->display.coupling);
    number(p,"探头倍率",0,&s->display.probe_factor,0);
    choice(p,"反相",0,"关闭\n开启",&s->display.inverted);
    p=tab(v,"CH2");
    lv_obj_set_style_text_color(lv_tabview_get_tab_button(v,-1),lv_color_hex(0x66d9f0),0);
    lv_obj_set_style_text_color(lv_tabview_get_tab_button(v,-1),lv_color_hex(0x66d9f0),LV_STATE_CHECKED);
    choice(p,"通道开关",0,"关闭\n开启",&s->io.enabled[1]);
    choice(p,"输入引脚",0,"PA5 (慢通道)\nPC4 (高速)",&s->io.input[1]);
    number(p,"量程 mV/div",0,&s->mv_per_div2,0);
    number(p,"中心位置 mV",0,&s->vertical_offset2_mv,0);
    choice(p,"软件耦合",0,"DC\nAC\nGND",&s->display2.coupling);
    number(p,"探头倍率",0,&s->display2.probe_factor,0);
    choice(p,"反相",0,"关闭\n开启",&s->display2.inverted);
    page_note(p,"保留摄像头；PC4仅用于不使用以太网时，PHY隔离成功才允许采集。两路输入不可重复。");
    p=tab(v,"水平");
    number(p,"时基 us/div",0,&s->time_per_div_s,0);
    record_summary=text(p,"",0,row_y);row_y+=56;
    choice(p,"波形显示",0,"原始\n拟合(保形)",&s->render_mode);
    choice(p,"网格",0,"关闭\n开启",&s->grid_enabled);
    choice(p,"余辉(4帧)",0,"关闭\n开启",&s->persistence);
    choice(p,"采集模式",0,"采样\n峰值\n指数平均",&s->acquisition_mode);
    number(p,"平均次数",0,&s->average_count,1);
    page_note(p,"采样率自动选固定档位；整条记录512~8192点，允许局部放大查看更少真实点。");
    p=tab(v,"触发");
    choice(p,"触发模式",0,"Auto\nNormal\nSingle",&s->trigger_mode);
    choice(p,"边沿",0,"上升沿\n下降沿",&s->trigger.edge);
    number(p,"电平 mV(输入)",0,&s->trigger.level_mv,0);
    number(p,"预触发点(最多90%)",0,&s->trigger.pre_samples,2);
    number(p,"释抑 ms",0,&s->holdoff_ms,1);
    p=tab(v,"测量");g_scope_ui.measure_page=p;mp=p;
    lv_obj_remove_flag(p,LV_OBJ_FLAG_SCROLLABLE);
    {
        unsigned i;const char *names[]={"读数","设置","常驻"};
        for(i=0U;i<3U;i++){
            measure_buttons[i]=scope_ui_button(p,names[i],(int)i*76,0,72,30,
                measure_section,(void *)(uintptr_t)i);
            lv_obj_set_style_bg_color(measure_buttons[i],lv_color_hex(0x355367),LV_STATE_CHECKED);
        }
        lv_obj_add_state(measure_buttons[0],LV_STATE_CHECKED);
    }
    p=measure_panel(mp,0U);
    choice(p,"测量通道",0,"CH1\nCH2",&s->measurement_source);
    lv_obj_add_flag(lv_obj_get_child(p,0),LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(lv_obj_get_child(p,1),0,0);lv_obj_set_size(lv_obj_get_child(p,1),88,32);
    g_scope_ui.accurate_button=scope_ui_button(p,"开启准确模式",96,0,130,32,measurement_toggle,NULL);
    g_scope_ui.measure=text(p,"等待真实采集",0,36);
    {
        unsigned i;const char *names[]={"频率","周期","峰峰值","有效值","最小值","最大值","平均值","占空比","上升时间","下降时间"};
        for(i=0U;i<10U;i++){
            int x=(int)(i%2U)*114,y=60+(int)(i/2U)*35;
            lv_obj_t *label=text(p,names[i],x,y);lv_obj_set_width(label,112);
            g_scope_ui.measure_values[i]=text(p,"--",x,y+16);
            lv_obj_set_width(g_scope_ui.measure_values[i],112);
        }
    }
    p=measure_panel(mp,1U);
    number(p,"准确记录点数",0,&g_scope_ui.accurate_samples,2);
    g_scope_ui.measure_summary=text(p,"流畅模式：测量可见真实记录",0,row_y);row_y+=65;
    page_note(p,"游标页选择测量跟随：时间差=周期，电压差=峰峰值；数值随选中通道更新。离开测量页关闭准确模式。");
    p=measure_panel(mp,2U);
    {
        size_t card;char caption[40];lv_obj_t *o;
        for(card=0U;card<4U;++card){
            row_y=(int)card*58;
            (void)snprintf(caption,sizeof(caption),"常驻%lu通道",(unsigned long)card+1U);
            choice(p,caption,0,"CH1\nCH2",&s->reading_source[card]);
            o=lv_obj_get_child(p,-2);lv_obj_set_pos(o,0,(int)card*58);lv_obj_set_width(o,88);
            o=lv_obj_get_child(p,-1);lv_obj_set_pos(o,0,(int)card*58+20);lv_obj_set_size(o,88,34);
            row_y=(int)card*58;
            (void)snprintf(caption,sizeof(caption),"常驻%lu项目",(unsigned long)card+1U);
            choice(p,caption,0,"频率\n周期\n峰峰值\n有效值\n最小值\n最大值\n平均值\n占空比\n上升时间\n下降时间",
                &s->reading_item[card]);
            o=lv_obj_get_child(p,-2);lv_obj_set_pos(o,96,(int)card*58);lv_obj_set_width(o,130);
            o=lv_obj_get_child(p,-1);lv_obj_set_pos(o,96,(int)card*58+20);lv_obj_set_size(o,130,34);
        }
    }
    p=tab(v,"游标");
    choice(p,"游标",0,"关闭\n手动\n测量跟随",&s->cursor_enabled);
    page_note(p,"手动：拖动A/B线。测量跟随：时间差=周期，电压差=峰峰值，自动跟随选中通道。");
    number(p,"时间A 采样点",0,&s->cursor_x[0],0);
    number(p,"时间B 采样点",0,&s->cursor_x[1],0);
    number(p,"电压A mV",0,&s->cursor_mv[0],0);
    number(p,"电压B mV",0,&s->cursor_mv[1],0);
    p=tab(v,"数学");
    page_button(p,"波形 / FFT",display_fft,NULL);
    page_button(p,"XY相位图",display_xy,NULL);
    choice(p,"FFT通道",0,"CH1\nCH2\nCH1+CH2",&s->fft_source);
    scope_ui_button(p,"dBV / 线性",0,row_y,110,42,fft_mode,NULL);
    scope_ui_button(p,"FFT复位",116,row_y,110,42,fft_reset,NULL);row_y+=54;
    page_note(p,"XY：X=CH1，Y=CH2，成对真实采样点；支持电压缩放和平移，时间游标只适用于时域。");
    page_note(p,"FFT默认dBV，可切线性；均为正弦等效RMS幅值。去直流/Hann窗，最多8192真实点、每500ms更新。df=采样率/点数；连线不提高分辨率。缩放和平移独立于时域/XY，准确模式关闭FFT。");
    p=tab(v,"DAC");
    lv_obj_set_style_text_color(lv_tabview_get_tab_button(v,-1),lv_color_hex(0xc1a1ff),0);
    lv_obj_set_style_text_color(lv_tabview_get_tab_button(v,-1),lv_color_hex(0xc1a1ff),LV_STATE_CHECKED);
    page_note(p,"输出固定PA4 / DAC1");
    choice(p,"设定波形",0,"正弦\n方波\n三角\n锯齿\n直流\n半正弦脉冲\n梯形\n全波整流正弦\n指数波\nsinc\n谐波合成",&s->dac.waveform);
    number(p,"频率 Hz",0,&s->dac.frequency_hz,0);
    number(p,"峰峰值 mV",0,&s->dac.amplitude_mv,0);
    number(p,"偏置 mV",0,&s->dac.offset_mv,0);
    number(p,"占空比 %",0,&s->dac.duty_percent,0);
    page_note(p,"自动选采样率，最高启用谐波至少32点/周期。普通波最高31.25kHz，8次合成最高3.91kHz；超限拒绝应用。");
    dac_summary=text(p,"输出关闭；采样率自动选择",0,row_y);row_y+=80;
    page_button(p,"开启输出",apply,(void *)(intptr_t)1);
    page_button(p,"关闭输出",apply,(void *)(intptr_t)2);
    page_note(p,"DAC设定预览(非测量)");
    preview=lv_obj_create(p);lv_obj_set_pos(preview,0,row_y);lv_obj_set_size(preview,226,60);
    lv_obj_set_style_bg_color(preview,lv_color_hex(0x0b1420),0);
    lv_obj_remove_flag(preview,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(preview,draw_preview,LV_EVENT_DRAW_MAIN,NULL);
    row_y+=76;
    page_note(p,"占空比控制方波、三角、脉冲、梯形及指数波。合成总峰峰值由上方幅值控制，各次比例归一化后保持；全零比例输出偏置直流。");
    {unsigned h;char caption[40];for(h=0U;h<SCOPE_DAC_HARMONICS;++h){
        (void)snprintf(caption,sizeof(caption),"%u次幅值 %%",h+1U);
        number(p,caption,0,&s->dac.harmonic_percent[h],0);
        (void)snprintf(caption,sizeof(caption),"%u次相位 度",h+1U);
        number(p,caption,0,&s->dac.harmonic_phase_deg[h],0);
    }}
    page_note(p,"谐波比例0~100%，相位-360~360度；仅谐波合成模式使用。");
    p=tab(v,"校准");
    page_button(p,"零点校准",file_action,(void *)(intptr_t)4);
    number(p,"实测参考 mV",0,&g_scope_ui.reference_mv,0);
    page_button(p,"增益校准",file_action,(void *)(intptr_t)5);
    number(p,"实测VDDA mV",0,&s->calibration.vref_mv,0);
    number(p,"CH2 VDDA mV",0,&s->calibration2.vref_mv,0);
    page_note(p,"校准通道沿用测量页所选通道。停止采集后校准；零点接GND，增益接外部已知电压。");
    p=tab(v,"保存");
    page_button(p,"CSV",file_action,NULL);
    page_button(p,"截图BMP",file_action,(void *)(intptr_t)1);
    page_button(p,"保存设置",file_action,(void *)(intptr_t)2);
    page_button(p,"恢复设置",file_action,(void *)(intptr_t)3);
    lv_obj_add_event_cb(v,tab_changed,LV_EVENT_VALUE_CHANGED,NULL);
    scope_ui_sync_controls();
}
