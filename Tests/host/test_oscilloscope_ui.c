/* 真实LVGL/示波器界面/会话集成测试；仅ADC/DAC与触摸硬件为主机替身。 */
#include "oscilloscope_ui_private.h"
#include "oscilloscope_driver.h"
#include "oscilloscope_phy.h"
#include "gui_guider.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

lv_ui guider_ui;
static lv_port_touch_snapshot_t touch;
static scope_driver_status_t driver;
static uint32_t frame,source_sequence,starts;
static uint32_t dma_sequence_step=1U;
static uint8_t slow_ui;
static uint64_t sample_index;
static float input_hz=1000.0f;
static double input_phase_cycles;
static double input_noise_codes;
static uint16_t input_bias_code=32768U;
static uint8_t noise_input;
static float input2_hz;
static uint8_t square_input;
static uint8_t wait_for_dma;
static int io_failure;
void scope_phy_get_diagnostic(scope_phy_diagnostic_t *d)
{*d=(scope_phy_diagnostic_t){0U,0x0128U,0x1140U,1U,SCOPE_PHY_ISOLATE_READBACK};}
static lv_indev_data_t pointer_data;
static void read_pointer(lv_indev_t *indev,lv_indev_data_t *data)
{(void)indev;*data=pointer_data;}
static uint8_t render_buffer[1024U*600U*4U];
static uint8_t render_pixels[1024U*600U*3U];
/* 只在主机链接包装绘制调用，校验实际交给LVGL的文字，不修改板端接口。 */
static char captured_labels[4096];
static size_t captured_length;
static uint8_t capture_labels;
void __real_lv_draw_label(lv_layer_t *layer,const lv_draw_label_dsc_t *dsc,const lv_area_t *area);
void __wrap_lv_draw_label(lv_layer_t *layer,const lv_draw_label_dsc_t *dsc,const lv_area_t *area)
{
    if(capture_labels&&dsc->text){
        int n=snprintf(captured_labels+captured_length,sizeof(captured_labels)-captured_length,
                       "%s\n",dsc->text);
        assert(n>=0&&(size_t)n<sizeof(captured_labels)-captured_length);captured_length+=(size_t)n;
    }
    __real_lv_draw_label(layer,dsc,area);
}
static void capture_plot_labels(void)
{
    captured_length=0U;captured_labels[0]='\0';capture_labels=1U;
    lv_obj_invalidate(g_scope_ui.plot);lv_refr_now(NULL);capture_labels=0U;
}
static void flush(lv_display_t *display,const lv_area_t *area,uint8_t *pixels)
{
    int x,y;size_t offset=0U;
    for(y=area->y1;y<=area->y2;y++)for(x=area->x1;x<=area->x2;x++){
        size_t target=((size_t)y*1024U+(size_t)x)*3U;
        render_pixels[target]=pixels[offset+2U];render_pixels[target+1U]=pixels[offset+1U];
        render_pixels[target+2U]=pixels[offset];offset+=4U;
    }
    lv_display_flush_ready(display);
}
static void screenshot(const char *path)
{
    FILE *file;lv_obj_invalidate(lv_screen_active());lv_refr_now(NULL);
    file=fopen(path,"wb");assert(file);assert(fprintf(file,"P6\n1024 600\n255\n")>0);
    assert(fwrite(render_pixels,1U,sizeof(render_pixels),file)==sizeof(render_pixels));assert(fclose(file)==0);
}
static unsigned int waveform_pixels(int x1,int y1,int x2,int y2)
{
    int x,y;unsigned int n=0U;
    lv_obj_invalidate(lv_screen_active());lv_refr_now(NULL);
    for(y=y1;y<=y2;y++)for(x=x1;x<=x2;x++){
        size_t p=((size_t)y*1024U+(size_t)x)*3U;
        if(render_pixels[p]==255U&&render_pixels[p+1U]==214U&&render_pixels[p+2U]==107U)n++;
    }
    return n;
}
static unsigned int color_pixels(int x1,int y1,int x2,int y2,
                                 uint8_t red,uint8_t green,uint8_t blue)
{
    int x,y;unsigned int n=0U;
    lv_obj_invalidate(lv_screen_active());lv_refr_now(NULL);
    for(y=y1;y<=y2;y++)for(x=x1;x<=x2;x++){
        size_t p=((size_t)y*1024U+(size_t)x)*3U;
        if(render_pixels[p]==red&&render_pixels[p+1U]==green&&render_pixels[p+2U]==blue)n++;
    }
    return n;
}
static unsigned int spectrum_region_pixels(int x1,int y1,int x2,int y2)
{
    int x,y;unsigned count=0U;
    lv_obj_invalidate(lv_screen_active());lv_refr_now(NULL);
    for(y=y1;y<=y2;++y)for(x=x1;x<=x2;++x){
        size_t p=((size_t)y*1024U+(size_t)x)*3U;
        if(render_pixels[p]>100U&&render_pixels[p+1U]>80U&&
           render_pixels[p]>render_pixels[p+2U]*2U)++count;
    }
    return count;
}
static unsigned int spectrum_peak_pixels(int x1,int x2)
{return spectrum_region_pixels(x1,220,x2,400);}
static uint32_t plot_hash(void)
{
    int x,y;uint32_t hash=2166136261U;
    lv_obj_invalidate(lv_screen_active());lv_refr_now(NULL);
    for(y=158;y<452;y++)for(x=12;x<664;x++){
        size_t p=((size_t)y*1024U+(size_t)x)*3U;
        hash=(hash^render_pixels[p])*16777619U;
        hash=(hash^render_pixels[p+1U])*16777619U;
        hash=(hash^render_pixels[p+2U])*16777619U;
    }
    return hash;
}
int scope_driver_open(void){driver.opened=1U;return 0;}
void scope_driver_close(void){driver.opened=0U;driver.acquiring=0U;}
int scope_driver_start(uint32_t rate){driver.sample_rate_hz=rate;driver.acquiring=1U;starts++;return 0;}
int scope_driver_start_capture(uint32_t rate,uint32_t n)
{frame=n;source_sequence=0U;sample_index=0U;return scope_driver_start(rate);}
uint32_t scope_driver_frame_samples(void){return frame;}
void scope_driver_stop(void){driver.acquiring=0U;}
void scope_driver_disable_dac(void){driver.dac_enabled=0U;}
int scope_driver_set_dac(const uint16_t *p,uint32_t n,uint32_t rate)
{uint32_t i;driver.dac_enabled=1U;driver.dac_rate_hz=0U;
 for(i=1U;i<n;i++)if(p[i]!=p[0]){driver.dac_rate_hz=rate;break;}
 return 0;}
void scope_driver_get_status(scope_driver_status_t *s){*s=driver;}
int scope_driver_read(uint16_t *p,uint32_t capacity,uint32_t *rate,uint32_t *sequence)
{
    uint32_t i;if(!driver.acquiring||wait_for_dma)return 0;assert(capacity>=frame);
    for(i=0U;i<frame;i++,sample_index++)p[i]=noise_input?
        (uint16_t)((noise_input==2U&&driver.sample_rate_hz>10000U?62000U:30000U)+
            ((sample_index+source_sequence*7U)*17U)%(noise_input==2U?13U:23U)):
        (uint16_t)((double)input_bias_code+(square_input?
            (fmod((double)input_hz*(double)sample_index/(double)driver.sample_rate_hz,1.0)<0.2?12000.0:-12000.0):
            12000.0*sin(6.283185307179586*((double)input_hz*(double)sample_index/(double)driver.sample_rate_hz+input_phase_cycles)))+
            input_noise_codes*sin(6.283185307179586*20000.0*(double)sample_index/(double)driver.sample_rate_hz));
    if(noise_input==3U){for(i=0U;i<frame;i++)p[i]=30000U;p[0]=60000U;}
    *rate=driver.sample_rate_hz;source_sequence+=dma_sequence_step;*sequence=source_sequence;return 1;
}
int scope_driver_configure_io(const scope_io_config_t *c)
{
    if(io_failure&&((c->enabled[0]&&c->input[0]==SCOPE_INPUT_PC4)||
       (c->enabled[1]&&c->input[1]==SCOPE_INPUT_PC4)))return io_failure;
    return scope_io_valid(c)?0:-1;
}
int scope_driver_read_pair(uint16_t *p,uint16_t *p2,uint32_t n,uint32_t *r,uint32_t *q)
{int result=scope_driver_read(p,n,r,q);uint32_t i;
 if(result>0)for(i=0;i<frame;i++)p2[i]=input2_hz>0.0f?
    (uint16_t)(32768.0+9000.0*sin(6.283185307179586*(double)input2_hz*
        (double)(sample_index-frame+i)/(double)driver.sample_rate_hz)):(uint16_t)(65535U-p[i]);
 return result;}
void lv_port_indev_get_touch_snapshot(lv_port_touch_snapshot_t *s){*s=touch;}
void scope_ui_files(unsigned int operation){(void)operation;}
static void poll(void)
{
    uint32_t ms=driver.acquiring?1U+(uint32_t)(1000ULL*frame/driver.sample_rate_hz):10U;
    if(slow_ui&&ms<43U)ms=43U;
    lv_tick_inc(ms);oscilloscope_ui_poll();
}
static void tab_active(uint32_t index)
{
    lv_tabview_set_active(g_scope_ui.tabs,index,LV_ANIM_OFF);
    lv_obj_send_event(g_scope_ui.tabs,LV_EVENT_VALUE_CHANGED,NULL);
}
static void two_fingers(int x0,int y0,int x1,int y1)
{touch.count=2U;touch.x[0]=(int16_t)x0;touch.y[0]=(int16_t)y0;
 touch.x[1]=(int16_t)x1;touch.y[1]=(int16_t)y1;poll();}
static void one_finger(int x,int y)
{touch.count=1U;touch.x[0]=(int16_t)x;touch.y[0]=(int16_t)y;poll();}
static lv_obj_t *find_field(lv_obj_t *page,const char *caption)
{
    uint32_t j;for(j=0U;j+1U<lv_obj_get_child_count(page);j++){
        lv_obj_t *o=lv_obj_get_child(page,(int32_t)j);
        if(lv_obj_check_type(o,&lv_label_class)&&!strcmp(lv_label_get_text(o),caption))
            return lv_obj_get_child(page,(int32_t)j+1);
    }
    for(j=0U;j<lv_obj_get_child_count(page);j++){
        lv_obj_t *found=find_field(lv_obj_get_child(page,(int32_t)j),caption);if(found)return found;
    }
    return NULL;
}
static lv_obj_t *page_field(lv_obj_t *page,const char *caption)
{lv_obj_t *o=find_field(page,caption);assert(o);return o;}
static unsigned collect_fields(lv_obj_t *page,lv_obj_t **objects,unsigned count)
{
    uint32_t j;for(j=0U;j<lv_obj_get_child_count(page)&&count<128U;j++){
        lv_obj_t *o=lv_obj_get_child(page,(int32_t)j);
        if(lv_obj_check_type(o,&lv_textarea_class)||lv_obj_check_type(o,&lv_dropdown_class))objects[count++]=o;
        else count=collect_fields(o,objects,count);
    }return count;
}
static void apply_page(lv_obj_t *page)
{
    uint32_t j;char values[128][32];uint32_t selected[128];lv_obj_t *objects[128];unsigned count=0U;uint8_t changed[128];
    unsigned total=collect_fields(page,objects,0U);
    for(j=0U;j<total;j++){
        lv_obj_t *o=objects[j];
        if(lv_obj_check_type(o,&lv_textarea_class)){
            objects[count]=o;(void)snprintf(values[count],sizeof(values[count]),"%s",lv_textarea_get_text(o));selected[count++]=UINT32_MAX;
        }else if(lv_obj_check_type(o,&lv_dropdown_class)){
            objects[count]=o;selected[count++]=lv_dropdown_get_selected(o);
        }
    }
    scope_ui_sync_controls();
    for(j=0U;j<count;++j)changed[j]=(uint8_t)(selected[j]==UINT32_MAX?
        strcmp(values[j],lv_textarea_get_text(objects[j]))!=0:selected[j]!=lv_dropdown_get_selected(objects[j]));
    for(j=0U;j<count;++j)if(changed[j]){
        if(selected[j]==UINT32_MAX){lv_textarea_set_text(objects[j],values[j]);lv_obj_send_event(objects[j],LV_EVENT_READY,NULL);}
        else {lv_dropdown_set_selected(objects[j],selected[j]);lv_obj_send_event(objects[j],LV_EVENT_VALUE_CHANGED,NULL);}
    }
}
static lv_obj_t *screen_button(const char *name)
{
    uint32_t j;for(j=0U;j<lv_obj_get_child_count(g_scope_ui.screen);j++){
        lv_obj_t *o=lv_obj_get_child(g_scope_ui.screen,(int32_t)j);
        if(lv_obj_check_type(o,&lv_button_class)&&lv_obj_get_child_count(o)&&
           !strcmp(lv_label_get_text(lv_obj_get_child(o,0)),name))return o;
    }
    assert(0);return NULL;
}
static lv_obj_t *find_button(lv_obj_t *parent,const char *name)
{
    uint32_t j;for(j=0U;j<lv_obj_get_child_count(parent);++j){
        lv_obj_t *o=lv_obj_get_child(parent,(int32_t)j);
        if(lv_obj_check_type(o,&lv_button_class)&&lv_obj_get_child_count(o)&&
           strcmp(lv_label_get_text(lv_obj_get_child(o,0)),name)==0)return o;
    }
    for(j=0U;j<lv_obj_get_child_count(parent);++j){
        lv_obj_t *found=find_button(lv_obj_get_child(parent,(int32_t)j),name);if(found)return found;
    }
    return NULL;
}
static lv_obj_t *button_named(lv_obj_t *parent,const char *name)
{lv_obj_t *o=find_button(parent,name);assert(o);return o;}
int main(void)
{
    uint32_t i,before,seq;float time_div,mv_div,scale;int16_t history[4][SCOPE_PLOT_WIDTH][2];
    lv_obj_t *old_desktop,*new_entry;
    scope_ui_t *u=&g_scope_ui;
    lv_init();lv_display_create(1024,600);
    lv_display_set_color_format(lv_display_get_default(),LV_COLOR_FORMAT_XRGB8888);
    lv_display_set_buffers(lv_display_get_default(),render_buffer,NULL,sizeof(render_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(lv_display_get_default(),flush);
    guider_ui.DeskTop=lv_obj_create(NULL);
    guider_ui.DeskTop_tileview_DeskTop_tile_1=lv_obj_create(guider_ui.DeskTop);
    lv_obj_set_size(guider_ui.DeskTop_tileview_DeskTop_tile_1,1000,565);
    oscilloscope_ui_open();lv_obj_update_layout(u->screen);
    lv_obj_update_layout(guider_ui.DeskTop);
    assert(u->entry&&lv_obj_get_parent(u->entry)==guider_ui.DeskTop_tileview_DeskTop_tile_1);
    assert(lv_obj_get_x(u->entry)==850&&lv_obj_get_y(u->entry)==35);
    assert(lv_obj_check_type(lv_obj_get_child(u->entry,0),&lv_image_class));
    assert(lv_obj_get_x(u->plot)==12&&lv_obj_get_y(u->plot)==158);
    assert(lv_obj_get_width(u->plot)==652&&lv_obj_get_height(u->plot)==294);
    assert(lv_obj_get_x(u->tabs)==674&&lv_obj_get_y(u->tabs)==158);
    assert(lv_obj_get_width(u->tabs)==338&&lv_obj_get_height(u->tabs)==294);
    assert(lv_color_to_u32(lv_obj_get_style_bg_color(u->screen,0))==lv_color_to_u32(lv_color_hex(0x111e2b)));
    assert(u->session.running&&u->session.config.record_samples>=512U);
    for(i=0U;i<300U&&!u->session.has_record;i++)poll();
    assert(u->session.has_record);
    /* 两次标记抽样之间的窄脉冲必须出现在像素峰谷中。 */
    {
        scope_session_stop(&u->session);
        for(i=0U;i<u->session.record_count;i++)u->session.mv[i]=1650.0f;
        u->session.mv[1]=3000.0f;
        scope_ui_rebuild_display(0);
        int peak=32767;
        for(i=0U;i<SCOPE_PLOT_WIDTH;i++)if(u->history[(u->history_next+3U)%4U][i][0]<peak)
            peak=u->history[(u->history_next+3U)%4U][i][0];
        assert(peak<100);
        assert(scope_session_run(&u->session,lv_tick_get())==0);
        for(i=0U;i<60U;i++)poll();
    }
    /* 测量不能忽略500点以后的峰值；两路峰谷均覆盖完整记录。 */
    {
        scope_session_stop(&u->session);
        u->settings.io.enabled[1]=1U;assert(scope_ui_apply_settings()==0);
        assert(scope_session_run(&u->session,lv_tick_get())==0);
        for(i=0U;i<60U;i++)poll();
        scope_session_stop(&u->session);
        for(i=0U;i<u->session.record_count;i++){u->session.mv[i]=1650.0f;u->session.mv2[i]=1650.0f;}
        size_t spike=u->view_start+u->view_count-7U;
        u->session.mv[spike]=3000.0f;u->session.mv2[spike]=3000.0f;
        u->settings.vertical_offset2_mv=1650.0f;u->settings.measurement_source=0U;
        scope_ui_rebuild_display(0);lv_tick_inc(250U);poll();
        assert(u->measurement_count==u->view_count&&u->measurement_count>500U);
        assert(u->session.measurements.max_mv==3000.0f);
        int peak=32767;
        for(i=0U;i<SCOPE_PLOT_WIDTH;i++)if(u->history2[(u->history_next+3U)%4U][i][0]<peak)
            peak=u->history2[(u->history_next+3U)%4U][i][0];
        assert(peak<100);
        u->settings.io.enabled[1]=0U;assert(scope_ui_apply_settings()==0);
        assert(scope_session_run(&u->session,lv_tick_get())==0);
        for(i=0U;i<60U;i++)poll();
    }
    /* 游标读数必须在绘图区外；右侧说明不能与下一个字段重叠。 */
    {
        lv_area_t a,b;lv_obj_t *page=lv_obj_get_child(lv_tabview_get_content(u->tabs),5);
        lv_obj_t *field=page_field(page,"时间A 采样点");
        assert(lv_obj_get_parent(u->cursor_label)==u->screen);
        lv_obj_get_coords(u->plot,&a);lv_obj_get_coords(u->cursor_label,&b);
        assert(b.y1>a.y2);
        lv_obj_update_layout(page);
        lv_obj_get_coords(lv_obj_get_child(page,2),&a);
        lv_obj_get_coords(lv_obj_get_child(page,3),&b);
        assert(a.y2<b.y1&&lv_obj_check_type(field,&lv_textarea_class));
    }
    /* 单次完成后按运行必须连续采集；Normal停止/运行仍保留Normal。 */
    {
        lv_obj_t *single=NULL,*run=lv_obj_get_parent(u->run_label);
        for(i=0U;i<lv_obj_get_child_count(u->screen);i++){
            lv_obj_t *o=lv_obj_get_child(u->screen,(int32_t)i);
            if(lv_obj_check_type(o,&lv_button_class)&&lv_obj_get_child_count(o)&&
               !strcmp(lv_label_get_text(lv_obj_get_child(o,0)),"单次"))single=o;
        }
        assert(single);lv_obj_send_event(single,LV_EVENT_CLICKED,NULL);
        for(i=0U;i<300U&&u->session.running;i++)poll();
        assert(!u->session.running&&u->session.config.mode==SCOPE_MODE_SINGLE);
        lv_obj_send_event(run,LV_EVENT_CLICKED,NULL);
        assert(u->session.running&&u->session.config.mode==SCOPE_MODE_AUTO);
        before=u->session.record_sequence;for(i=0U;i<100U;i++)poll();
        assert(u->session.running&&u->session.record_sequence>before+1U);
        u->settings.trigger_mode=SCOPE_MODE_NORMAL;assert(scope_ui_apply_settings()==0);
        lv_obj_send_event(run,LV_EVENT_CLICKED,NULL);assert(!u->session.running);
        lv_obj_send_event(run,LV_EVENT_CLICKED,NULL);
        assert(u->session.running&&u->session.config.mode==SCOPE_MODE_NORMAL);
        u->settings.trigger_mode=SCOPE_MODE_AUTO;assert(scope_ui_apply_settings()==0);
    }
    /* 输入框获得焦点即弹键盘，不依赖松手后CLICKED；页面滚动不能吞掉编辑入口。 */
    {
        lv_obj_t *page=lv_obj_get_child(lv_tabview_get_content(u->tabs),2);
        lv_obj_t *field=page_field(page,"时基 us/div");
        tab_active(2U);lv_obj_send_event(field,LV_EVENT_FOCUSED,NULL);
        assert(u->keyboard&&lv_obj_is_valid(u->keyboard));
        assert(lv_keyboard_get_textarea(u->keyboard)==field);
        assert(lv_keyboard_get_mode(u->keyboard)==LV_KEYBOARD_MODE_NUMBER);
        lv_obj_send_event(u->keyboard,LV_EVENT_CANCEL,NULL);assert(!u->keyboard);
        lv_obj_send_event(field,LV_EVENT_CLICKED,NULL);assert(u->keyboard);
        lv_obj_send_event(u->keyboard,LV_EVENT_READY,NULL);assert(!u->keyboard);
        {
            lv_area_t a,b;lv_obj_t *toggle=screen_button("键盘开");
            int running=u->session.running;uint32_t old_starts=starts;
            lv_obj_get_coords(lv_obj_get_parent(u->run_label),&a);lv_obj_get_coords(toggle,&b);
            assert(a.x2<b.x1&&b.x1-a.x2<=12&&a.y1==b.y1);
            lv_obj_send_event(field,LV_EVENT_FOCUSED,NULL);assert(u->keyboard);
            lv_obj_send_event(toggle,LV_EVENT_CLICKED,NULL);assert(!u->keyboard);
            lv_obj_send_event(field,LV_EVENT_FOCUSED,NULL);
            lv_obj_send_event(field,LV_EVENT_CLICKED,NULL);assert(!u->keyboard);
            lv_obj_send_event(screen_button("键盘关"),LV_EVENT_CLICKED,NULL);
            assert(!u->keyboard&&u->session.running==running&&starts==old_starts);
        }
        /* 通过LVGL输入设备命中测试触发焦点，覆盖按下尚未松手的入口。 */
        {
            lv_area_t area;lv_indev_t *pointer=lv_indev_create();
            lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);
            lv_indev_set_read_cb(pointer,read_pointer);
            lv_obj_scroll_to_y(page,0,LV_ANIM_OFF);lv_obj_update_layout(u->screen);
            lv_obj_get_coords(field,&area);
            pointer_data.point.x=(area.x1+area.x2)/2;
            pointer_data.point.y=(area.y1+area.y2)/2;
            pointer_data.state=LV_INDEV_STATE_PRESSED;lv_indev_read(pointer);
            assert(u->keyboard&&lv_keyboard_get_textarea(u->keyboard)==field);
            pointer_data.point.x++;lv_indev_read(pointer);
            pointer_data.state=LV_INDEV_STATE_RELEASED;lv_indev_read(pointer);
            assert(u->keyboard);
            lv_obj_update_layout(u->screen);lv_obj_get_coords(u->keyboard,&area);
            assert(area.x1==0&&area.y1==340&&area.x2==1023&&area.y2==599);
            if(getenv("SCOPE_RENDER"))screenshot("tmp/scope_numeric_keyboard.ppm");
            lv_textarea_set_cursor_pos(field,LV_TEXTAREA_CURSOR_LAST);
            char expected_input[40];
            (void)snprintf(expected_input,sizeof(expected_input),"%s1",lv_textarea_get_text(field));
            pointer_data.point.x=100;pointer_data.point.y=376;
            pointer_data.state=LV_INDEV_STATE_PRESSED;lv_indev_read(pointer);
            pointer_data.state=LV_INDEV_STATE_RELEASED;lv_indev_read(pointer);
            assert(!strcmp(lv_textarea_get_text(field),expected_input));
            pointer_data.point.x=800;pointer_data.point.y=438;
            pointer_data.state=LV_INDEV_STATE_PRESSED;lv_indev_read(pointer);
            pointer_data.state=LV_INDEV_STATE_RELEASED;lv_indev_read(pointer);
            assert(!u->keyboard);lv_textarea_set_text(field,"0.0001");
            lv_indev_delete(pointer);
        }
        tab_active(0U);
    }
    for(i=0U;i<60U;i++)poll();
    if(getenv("SCOPE_RENDER"))screenshot("tmp/scope_axis_debug.ppm");
    assert(color_pixels(16,162,660,448,0xb5,0xc8,0xd8)>20U);
    /* 无边沿且无有效周期：真实ADC替身码值更新仍推动绘图，不受测量有效位影响。 */
    noise_input=1U;u->settings.trigger.level_mv=3200.0f;
    assert(scope_ui_apply_settings()==0);before=u->display_sequence;
    for(i=0U;i<60U;i++)poll();
    /* 实际水平页应用：扩大/缩小后字段、会话Fs/N、时基同步。 */
    {
        scope_settings_t saved=u->settings;
        lv_obj_t *page=lv_obj_get_child(lv_tabview_get_content(u->tabs),2);
        u->settings.sample_rate_hz=100000.0f;u->settings.time_per_div_s=0.0001f;
        assert(scope_ui_apply_settings()==0);scope_ui_sync_controls();tab_active(2U);
        lv_textarea_set_text(page_field(page,"时基 us/div"),"1000");apply_page(page);
        assert(u->session.config.sample_rate_hz==500000U&&u->session.config.record_samples==5008U);
        for(i=0U;i<lv_obj_get_child_count(page);i++){
            lv_obj_t *o=lv_obj_get_child(page,(int32_t)i);
            if(lv_obj_check_type(o,&lv_label_class))assert(strcmp(lv_label_get_text(o),"采样率 Sa/s"));
        }
        lv_textarea_set_text(page_field(page,"时基 us/div"),"100000");apply_page(page);
        assert(u->session.config.sample_rate_hz==5000U&&u->session.config.record_samples==5008U);
        lv_textarea_set_text(page_field(page,"时基 us/div"),"1");apply_page(page);
        assert(u->session.config.sample_rate_hz==1000000U&&u->session.config.record_samples==512U);
        assert(fabsf(u->settings.time_per_div_s-0.000001f)<1e-9f);
        u->settings=saved;assert(scope_ui_apply_settings()==0);scope_ui_sync_controls();tab_active(0U);
        for(i=0U;i<60U;i++)poll();
    }
    assert(!u->session.triggered&&u->display_sequence>before&&u->history_count>0U);
    assert(u->session.raw[0]>=30000U&&u->session.raw[0]<30023U);
    before=u->display_sequence;for(i=0U;i<60U;i++)poll();
    assert(u->display_sequence>before&&u->session.running);
    noise_input=0U;input_hz=0.0f;for(i=0U;i<60U;i++)poll();
    lv_tick_inc(200U);poll();
    assert(!(u->session.measurements.valid&SCOPE_VALID_FREQUENCY));
    assert(strcmp(lv_label_get_text(u->readings[0]),"--")==0&&u->history_count>0U);
    input_hz=1000.0f;u->settings.trigger.level_mv=1650.0f;assert(scope_ui_apply_settings()==0);
    for(i=0U;i<60U;i++)poll();
    if(getenv("SCOPE_RENDER")){
        lv_tick_inc(200U);poll();screenshot("tmp/scope_layout_ch1.ppm");
    }

    /* 标记始终来自当前原始采样位置，拟合不增加虚构的ADC点。 */
    {
        int16_t markers[500][2];size_t marker_count;
        u->settings.render_mode=0U;scope_ui_rebuild_display(0);
        assert(u->view_count>=512U&&u->sample_point_count==500U);
        marker_count=u->sample_point_count;memcpy(markers,u->sample_points,sizeof(markers));
        assert(u->sample_points[0][0]==0&&u->sample_points[marker_count-1U][0]==SCOPE_PLOT_WIDTH-1);
        assert(marker_count>1U&&u->sample_points[1][0]>0);
        u->settings.render_mode=1U;scope_ui_rebuild_display(0);
        assert(u->sample_point_count==marker_count);
        for(i=0U;i<marker_count;i++){
            assert(u->sample_points[i][0]==markers[i][0]);
            assert(abs(u->sample_points[i][1]-markers[i][1])<=1);
        }
        if(getenv("SCOPE_RENDER"))screenshot("tmp/scope_points_fitted.ppm");
        u->settings.display.coupling=SCOPE_COUPLING_GND;scope_ui_rebuild_display(0);
        for(i=0U;i<u->sample_point_count;i++)assert(u->sample_points[i][1]==u->sample_points[0][1]);
        u->settings.display.coupling=SCOPE_COUPLING_AC;u->settings.display.inverted=1;
        u->settings.render_mode=0U;scope_ui_rebuild_display(0);
        memcpy(markers,u->sample_points,sizeof(markers));
        u->settings.render_mode=1U;scope_ui_rebuild_display(0);
        for(i=0U;i<marker_count;i++)assert(abs(u->sample_points[i][1]-markers[i][1])<=1);
        u->settings.display.coupling=SCOPE_COUPLING_DC;u->settings.display.inverted=0;
        u->settings.render_mode=0U;scope_ui_rebuild_display(0);
        if(getenv("SCOPE_RENDER"))screenshot("tmp/scope_points_raw.ppm");
        u->settings.acquisition_mode=2U;u->average_frames=0U;
        for(i=0U;i<u->session.record_count;i++)u->session.mv[i]=1000.0f;
        scope_ui_rebuild_display(1);
        for(i=0U;i<u->session.record_count;i++)u->session.mv[i]=2000.0f;
        scope_ui_rebuild_display(1);
        assert(u->average_frames==2U&&fabsf(u->sample_values[0]-1500.0f)<0.01f);
        assert(fabsf(u->sample_values[0]-u->display[0])<0.01f);
        u->settings.display.coupling=SCOPE_COUPLING_AC;scope_ui_rebuild_display(0);
        assert(fabsf(u->sample_values[0])<0.01f&&fabsf(u->display[0])<0.01f);
        u->settings.acquisition_mode=0U;u->settings.display.coupling=SCOPE_COUPLING_DC;
        for(i=0U;i<60U;i++)poll();
    }

    /* 测量页是唯一开启入口，离页立即取消并恢复流畅。 */
    /* 参数区起手进入表格仍属于同一次按压，必须松手后才能开始波形手势。 */
    before=starts;
    one_finger(720,260);one_finger(620,260);one_finger(580,265);
    assert(!u->drag_active&&u->session.running&&u->pan_x==0.0f&&starts==before);
    two_fingers(500,220,600,300);assert(!u->pinch_active);
    touch.count=0U;poll();
    /* 起始主轴锁定后，后续歪划只沿该轴平移，运行状态松手恢复。 */
    {
        float x,y;uint32_t fs;size_t count;
        u->settings.cursor_enabled=0U;x=u->pan_x;y=u->pan_y;
        before=starts;fs=u->session.config.sample_rate_hz;count=u->session.config.record_samples;
        one_finger(210,260);one_finger(224,263);
        assert(u->drag_locked&&u->drag_axis==SCOPE_ZOOM_HORIZONTAL&&!u->session.running);
        one_finger(250,390);
        assert(u->pan_x>x+35.0f&&u->pan_y==y);
        touch.count=0U;poll();assert(u->session.running&&starts==before+1U);
        assert(u->session.config.sample_rate_hz==fs&&u->session.config.record_samples==count);
        x=u->pan_x;y=u->pan_y;
        for(i=0U;i<60U;i++)poll();
        assert(u->pan_x==x&&u->pan_y==y);
        one_finger(230,240);one_finger(232,254);one_finger(420,282);
        assert(u->drag_axis==SCOPE_ZOOM_VERTICAL&&u->pan_x==x&&u->pan_y>y+35.0f);
        touch.count=0U;poll();
        u->pan_x=0;u->pan_y=0;lv_obj_invalidate(u->plot);
    }
    /* 点击/抖动不跳转；游标交点按初始方向选时间或电压，偏斜/反向不换轴。 */
    {
        lv_obj_t *page=lv_obj_get_child(lv_tabview_get_content(u->tabs),5);
        float ax,ay,bx,by;int x,y;uint32_t config_rate=u->session.config.sample_rate_hz;
        scope_ui_sync_controls();tab_active(5U);
        lv_dropdown_set_selected(page_field(page,"游标"),1U);apply_page(page);
        assert(u->settings.cursor_enabled&&u->settings.cursor_x[1]<(float)u->view_count);
        ax=u->settings.cursor_x[0];ay=u->settings.cursor_mv[0];
        bx=u->settings.cursor_x[1];by=u->settings.cursor_mv[1];
        x=12+(int)((ax-(float)u->view_start)/(float)(u->view_count-1U)*(SCOPE_PLOT_WIDTH-1));
        y=158+(int)(147.0f-(ay-u->settings.vertical_offset_mv)*294.0f/8.0f*
            u->plot_vertical_scale/u->settings.mv_per_div);
        before=starts;one_finger(x,y);one_finger(x+4,y+3);touch.count=0U;poll();
        lv_obj_send_event(u->plot,LV_EVENT_CLICKED,NULL);
        assert(u->settings.cursor_x[0]==ax&&u->settings.cursor_mv[0]==ay&&starts==before);
        one_finger(x,y);one_finger(x+12,y+2);one_finger(x+32,y+120);
        assert(u->drag_cursor==0&&u->settings.cursor_x[0]>ax&&u->settings.cursor_mv[0]<ay);
        one_finger(x-20,y+100);assert(u->settings.cursor_x[0]<ax&&u->settings.cursor_mv[0]<ay);
        touch.count=0U;poll();assert(u->session.running);
        assert(u->settings.cursor_x[1]==bx&&u->settings.cursor_mv[1]==by);
        u->settings.cursor_x[0]=ax;u->settings.cursor_mv[0]=ay;
        one_finger(x,y);one_finger(x+2,y+12);one_finger(x+120,y+32);
        assert(u->drag_axis==SCOPE_ZOOM_VERTICAL&&u->drag_cursor==0);
        assert(u->settings.cursor_x[0]!=ax&&u->settings.cursor_mv[0]<ay);
        touch.count=0U;poll();assert(u->session.config.sample_rate_hz==config_rate);
        {
            float px=u->pan_x,py=u->pan_y,cx=u->settings.cursor_x[0],cy=u->settings.cursor_mv[0];
            one_finger(620,430);one_finger(680,390);
            assert(u->drag_cursor==-1&&u->pan_x==px&&u->pan_y==py);
            assert(u->settings.cursor_x[0]==cx&&u->settings.cursor_mv[0]==cy);
            touch.count=0U;poll();
        }
        lv_tick_inc(250U);poll();
        assert(strstr(lv_label_get_text(u->cursor_label),"dt"));
        {
            lv_area_t a,b,c;lv_obj_update_layout(u->screen);
            lv_obj_get_coords(u->footer,&a);lv_obj_get_coords(u->cursor_label,&b);
            lv_obj_get_coords(u->notice,&c);assert(a.y2<b.y1&&b.y2<c.y1);
        }
        if(getenv("SCOPE_RENDER"))screenshot("tmp/scope_drag_cursors.ppm");
        u->settings.cursor_enabled=0U;tab_active(0U);
        u->pan_x=200.0f;u->pan_y=60.0f;
        u->settings.cursor_x[0]=0.0f;u->settings.cursor_x[1]=1024.0f;
        u->settings.cursor_mv[0]=-100000.0f;u->settings.cursor_mv[1]=100000.0f;
        scope_ui_sync_controls();tab_active(5U);
        lv_dropdown_set_selected(page_field(page,"游标"),1U);apply_page(page);
        for(i=0U;i<2U;i++){
            float xx=(u->settings.cursor_x[i]-(float)u->view_start)/(float)(u->view_count-1U)*651.0f+u->pan_x;
            float yy=147.0f-(u->settings.cursor_mv[i]-u->settings.vertical_offset_mv)*294.0f/8.0f*
                u->plot_vertical_scale/u->settings.mv_per_div+u->pan_y;
            assert(xx>=0.0f&&xx<652.0f&&yy>=0.0f&&yy<294.0f);
        }
        u->settings.cursor_enabled=0U;u->pan_x=0.0f;u->pan_y=0.0f;tab_active(0U);
    }
    /* 真实像素裁剪：记录外留空，移动不改采样数据，ADC停止时拖动不启动。 */
    {
        float sample=u->session.mv[0];scope_session_stop(&u->session);before=starts;
        assert(waveform_pixels(12,158,663,451)>0U);
        one_finger(100,250);one_finger(800,252);touch.count=0U;poll();
        assert(!u->session.running&&starts==before&&u->pan_x==700.0f);
        assert(waveform_pixels(12,158,663,451)==0U&&u->session.mv[0]==sample);
        u->pan_x=200.0f;assert(waveform_pixels(12,158,200,451)==0U);
        assert(waveform_pixels(213,158,663,451)>0U);
        if(getenv("SCOPE_RENDER"))screenshot("tmp/scope_drag_pan.ppm");
        u->pan_x=0.0f;u->pan_y=0.0f;
        assert(scope_session_run(&u->session,lv_tick_get())==0);
    }
    /* 单指交接双指继承原运行状态；双指落一指不能触发新拖动。 */
    before=starts;one_finger(210,260);one_finger(225,261);
    assert(u->drag_locked&&!u->session.running);
    two_fingers(100,200,300,270);assert(u->pinch_active&&!u->drag_active&&u->pinch_was_running);
    touch.count=1U;poll();assert(u->session.running&&starts==before+1U);
    one_finger(260,270);one_finger(350,330);assert(!u->drag_active&&starts==before+1U);
    touch.count=0U;lv_tick_inc(200U);poll();u->pan_x=0.0f;
    assert(scope_ui_set_accurate(1,1)!=0);
    tab_active(4U);assert(scope_ui_measure_page_active());
    if(getenv("SCOPE_RENDER"))screenshot("tmp/scope_layout_measure.ppm");
    u->accurate_samples=8192U;assert(scope_ui_set_accurate(1,1)==0);
    /* 等待新记录时再应用，不能对旧记录启动准确任务并覆盖采集中stream_mv。 */
    assert(u->session.running&&scope_ui_apply_settings()==0);
    wait_for_dma=1U;poll();assert(!(u->session.running&&u->accurate_busy));wait_for_dma=0U;
    assert(u->session.config.record_samples==8192U);
    for(i=0U;i<100U&&!u->accurate_ready;i++)poll();
    assert(u->accurate_ready&&!u->session.running);
    {
        float x=u->pan_x,y=u->pan_y;
        one_finger(210,260);one_finger(260,265);touch.count=0U;poll();
        assert(!u->drag_active&&u->pan_x==x&&u->pan_y==y&&!u->session.running);
    }
    for(i=0U;i<u->sample_point_count;i++){
        size_t source=(size_t)((float)i*(float)(u->view_count-1U)/
            (float)(u->sample_point_count-1U)+0.5f);
        assert(fabsf(u->sample_values[i]-u->session.mv[u->view_start+source])<0.01f);
        assert(fabsf(u->sample_values[i]-u->display[i])<0.01f);
    }
    seq=u->accurate_attempt_sequence;
    for(i=0U;i<20U;i++)poll();
    assert(u->accurate_attempt_sequence==seq&&!u->accurate_busy);
    tab_active(0U);
    assert(!u->settings.measurement_mode&&!u->accurate_busy&&!u->accurate_ready);
    assert(u->session.running&&u->session.config.record_samples>=512U&&u->session.config.record_samples<=8192U);

    /* 双指缩放40ms内预览冻结记录，保留纵移且清除横移，松手才恢复采集。 */
    for(i=0U;i<10U;i++)poll();
    time_div=u->settings.time_per_div_s;before=starts;seq=u->session.record_sequence;
    {
        float pinch_scale=u->plot_vertical_scale,center_mv,center_y;
        scope_measurements_t current_measurement;
        assert(scope_measure(u->session.mv,u->session.record_count,
                             (float)u->session.actual_rate_hz,&current_measurement));
        center_mv=current_measurement.mean_mv*u->settings.display.probe_factor*
            (u->settings.display.inverted?-1.0f:1.0f);
        u->settings.vertical_offset_mv=0.0f;
        u->pan_y=24.0f;u->pan_x=15.0f;
        center_y=147.0f-center_mv*294.0f/8.0f*pinch_scale/u->settings.mv_per_div+u->pan_y;
        two_fingers(100,200,300,270);assert(u->pinch_active&&!u->session.running);
        two_fingers(90,170,310,400);
        assert(u->plot_vertical_scale>pinch_scale&&u->pan_y==24.0f&&u->pan_x==0.0f);
        assert(u->settings.vertical_offset_mv==0.0f);
        /* 均值锚点缩放前后保持同一屏幕位置，不能跳回表格中心。 */
        assert(fabsf((147.0f-(center_mv-u->plot_vertical_offset_mv)*294.0f/8.0f*
                      u->plot_vertical_scale/u->settings.mv_per_div+u->pan_y)-center_y)<0.1f);
        assert(starts==before&&u->settings.time_per_div_s==time_div&&u->session.record_sequence==seq);
        assert(u->display_sequence==seq);
        memcpy(history,u->history,sizeof(history));
    }
    touch.count=1U;poll();assert(!u->pinch_active&&u->session.running&&starts==before+1U);
    touch.count=0U;poll();
    assert(u->settings.time_per_div_s==time_div&&u->session.config.record_samples>=512U&&u->session.config.record_samples<=8192U);
    assert(u->pan_y==24.0f&&u->pan_x==0.0f);
    for(i=0U;i<20U;i++)poll();
    lv_tick_inc(200U);poll();

    /* 无法从记录取得有效均值时，纵向缩放以0mV为中心锚定。 */
    {
        float old_scale=u->plot_vertical_scale;
        float old_offset=scope_ui_vertical_offset(),zero_y;
        u->settings.vertical_offset_mv=200.0f;
        old_offset+=200.0f;
        zero_y=147.0f+old_offset*294.0f/8.0f*old_scale/u->settings.mv_per_div+u->pan_y;
        u->session.mv[0]=NAN;
        two_fingers(100,180,300,250);two_fingers(100,150,300,430);
        assert(u->settings.vertical_offset_mv==200.0f);
        assert(fabsf((147.0f+scope_ui_vertical_offset()*294.0f/8.0f*
                      u->plot_vertical_scale/u->settings.mv_per_div+u->pan_y)-zero_y)<0.1f);
        touch.count=0U;poll();
        u->session.mv[0]=0.0f;scope_session_refresh(&u->session);
        lv_tick_inc(200U);poll();
    }

    /* 绘图补偿后手动修改中心/量程必须按输入生效，不能叠加旧缩放中心。 */
    {
        lv_obj_t *page=lv_obj_get_child(lv_tabview_get_content(u->tabs),0);
        scope_ui_sync_controls();tab_active(0U);
        lv_textarea_set_text(page_field(page,"中心位置 mV"),"900");
        lv_textarea_set_text(page_field(page,"量程 mV/div"),"400");
        apply_page(page);
        assert(u->settings.vertical_offset_mv==900.0f&&u->plot_vertical_offset_mv==0.0f);
    }

    /* 波形横向缩放在按住期间更新时基/视窗和标签，松手才应用Fs/N。 */
    {
        size_t old_view=u->view_count,old_points=u->session.config.record_samples;
        uint32_t old_rate=u->session.config.sample_rate_hz;float old_div=u->settings.time_per_div_s;
        u->pan_x=90.0f;u->pan_y=24.0f;before=starts;seq=u->session.record_sequence;
        two_fingers(100,200,300,270);two_fingers(80,193,380,298);
        assert(u->settings.time_per_div_s<old_div&&u->view_count!=old_view);
        assert(u->pan_x==0.0f&&u->pan_y==24.0f&&starts==before);
        assert(u->session.config.sample_rate_hz==old_rate&&u->session.config.record_samples==old_points);
        assert(u->session.record_sequence==seq);
        {
            char expected[40];float shown=scope_zoom_time_div(u->display_rate_hz,(uint32_t)u->view_count);
            (void)snprintf(expected,sizeof(expected),"%.4g %s/div",
                (double)(shown<0.001f?shown*1e6f:shown<1.0f?shown*1000.0f:shown),
                shown<0.001f?"us":shown<1.0f?"ms":"s");
            assert(strcmp(lv_label_get_text(u->time_label),expected)==0);
        }
        touch.count=0U;poll();assert(starts==before+1U&&u->pan_y==24.0f&&u->pan_x==0.0f);
    }
    lv_tick_inc(200U);poll();

    /* FFT允许缓存视图横纵缩放和单指平移，过程中不重配ADC。 */
    {
        uint32_t old_rate=u->session.config.sample_rate_hz,old_points=(uint32_t)u->session.config.record_samples;
        uint32_t pixels,zero_spectrum_pixels;
        float time_x=u->pan_x,time_y=u->pan_y,time_scale=u->plot_vertical_scale;
        float old_fft_scale=1.0f;
        assert(u->fft_frequency_scale==1.0f&&u->fft_vertical_scale==1.0f);
        u->fft_enabled=1U;u->fft_mask=1U;u->fft_count=512U;u->fft_rate_hz=16000U;
        u->fft_frequency_scale=1.0f;u->fft_vertical_scale=0.1f;u->fft_pan_x=0.0f;u->fft_pan_y=24.0f;
        for(i=0U;i<=256U;i++)u->fft[i]=0.0f;
        u->last_fft_ms=lv_tick_get();zero_spectrum_pixels=plot_hash();
        for(i=0U;i<=256U;i++)u->fft[i]=NAN;
        assert(plot_hash()==zero_spectrum_pixels);
        for(i=0U;i<=256U;i++)u->fft[i]=1.0f;
        u->last_fft_ms=lv_tick_get();
        lv_obj_invalidate(u->plot);pixels=plot_hash();
        before=starts;two_fingers(100,200,300,270);two_fingers(80,193,380,298);
        for(i=0U;i<50U;i++)poll();
        assert(u->fft_frequency_scale>old_fft_scale&&u->fft_pan_y==24.0f);
        {uint32_t current=plot_hash();assert(current!=pixels);pixels=current;}
        if(getenv("SCOPE_RENDER"))screenshot("tmp/scope_fft_zoom_h.ppm");
        touch.count=0U;poll();
        assert(u->session.config.sample_rate_hz==old_rate&&u->session.config.record_samples==old_points);
        assert(starts==before+1U&&u->fft_frequency_scale>old_fft_scale);
        lv_tick_inc(200U);poll();u->fft_pan_y=16.0f;
        scale=u->fft_vertical_scale;two_fingers(100,180,300,250);two_fingers(100,150,300,430);
        for(i=0U;i<50U;i++)poll();
        assert(u->fft_vertical_scale>scale&&u->fft_pan_y==16.0f&&u->fft_pan_x==0.0f);
        {uint32_t current=plot_hash();assert(current!=pixels);pixels=current;}
        touch.count=0U;poll();assert(u->session.config.sample_rate_hz==old_rate);
        lv_tick_inc(200U);poll();
        old_fft_scale=u->fft_frequency_scale;
        one_finger(210,260);one_finger(250,280);touch.count=0U;poll();
        assert(u->fft_pan_x!=0.0f&&u->fft_pan_y==16.0f&&u->fft_frequency_scale==old_fft_scale);
        {uint32_t current=plot_hash();assert(current!=pixels);pixels=current;}
        if(getenv("SCOPE_RENDER"))screenshot("tmp/scope_fft_pan_h.ppm");
        lv_tick_inc(200U);poll();u->fft_pan_x=0.0f;u->fft_frequency_scale=1.0f;
        for(i=0U;i<=256U;i++)u->fft[i]=1.0f;
        u->last_fft_ms=lv_tick_get();pixels=plot_hash();
        if(getenv("SCOPE_RENDER"))screenshot("tmp/scope_fft_pan_v_start.ppm");
        one_finger(210,260);one_finger(215,300);touch.count=0U;poll();
        if(getenv("SCOPE_RENDER"))screenshot("tmp/scope_fft_pan_v_end.ppm");
        assert(u->fft_pan_y>16.0f&&u->fft_pan_x==0.0f);
        assert(plot_hash()!=pixels);
        assert(u->session.config.sample_rate_hz==old_rate&&u->session.config.record_samples==old_points);
        assert(u->pan_x==time_x&&u->pan_y==time_y&&u->plot_vertical_scale==time_scale);
        u->fft_enabled=0U;u->fft_pan_x=u->fft_pan_y=0.0f;u->fft_frequency_scale=1.0f;
    }

    /* 已知高频也能继续缩小波形；Fs随窗口下降，不再被16倍频率卡住。 */
    lv_tick_inc(200U);poll();u->settings.sample_rate_hz=1000000.0f;
    u->settings.time_per_div_s=0.00005f;assert(scope_ui_apply_settings()==0);
    for(i=0U;i<20U;i++)poll();
    u->session.measurement_sequence=u->session.record_sequence;
    u->session.measurements.valid|=SCOPE_VALID_FREQUENCY;u->session.measurements.frequency_hz=200000.0f;
    two_fingers(80,190,480,290);two_fingers(180,215,380,265);touch.count=0U;poll();
    assert(u->settings.time_per_div_s>0.00009f&&u->session.config.sample_rate_hz==1000000U);
    for(i=0U;i<20U;i++)poll();

    lv_tick_inc(200U);poll();
    mv_div=u->settings.mv_per_div;time_div=u->settings.time_per_div_s;
    scale=u->plot_vertical_scale;before=starts;
    two_fingers(100,190,160,330);two_fingers(90,170,180,380);
    assert(u->plot_vertical_scale>scale);
    touch.count=0U;poll();assert(u->plot_vertical_scale>scale);
    assert(u->settings.mv_per_div==mv_div&&u->settings.time_per_div_s==time_div&&starts==before+1U);
    for(i=0U;i<20U;i++)poll();

    /* 纵向允许继续缩小越过旧0.05倍软门槛，不修改采样配置。 */
    lv_tick_inc(200U);poll();u->plot_vertical_scale=0.05f;
    before=u->session.config.sample_rate_hz;seq=(uint32_t)u->session.config.record_samples;
    two_fingers(140,180,160,420);two_fingers(145,240,155,360);touch.count=0U;poll();
    assert(u->plot_vertical_scale<0.03f&&u->settings.mv_per_div==mv_div);
    assert(u->session.config.sample_rate_hz==before&&u->session.config.record_samples==seq);
    for(i=0U;i<20U;i++)poll();

    lv_tick_inc(200U);poll();scope_session_stop(&u->session);before=starts;
    {
        lv_obj_t *page=lv_obj_get_child(lv_tabview_get_content(u->tabs),2);
        float old_span=scope_zoom_time_div(u->session.actual_rate_hz,(uint32_t)u->session.record_count);
        char title[40];tab_active(2U);scope_ui_sync_controls();
        lv_textarea_set_text(page_field(page,"时基 us/div"),"100000");apply_page(page);
        lv_tick_inc(200U);poll();
        assert(!u->session.running&&starts==before);
        assert(u->settings.sample_rate_hz==5000.0f&&u->settings.trigger.window_samples==5008U);
        (void)snprintf(title,sizeof(title),"%.4g %s/div",
            (double)(old_span<0.001f?old_span*1e6f:old_span<1.0f?old_span*1000.0f:old_span),
            old_span<0.001f?"us":old_span<1.0f?"ms":"s");
        assert(strcmp(lv_label_get_text(u->time_label),title)==0);
        assert(strstr(lv_label_get_text(u->notice),"原Fs"));tab_active(0U);
    }
    two_fingers(100,200,300,270);two_fingers(80,193,380,298);touch.count=0U;poll();
    assert(!u->session.running&&starts==before);
    for(i=0U;i<20U;i++)poll();
    tab_active(4U);assert(scope_ui_set_accurate(1,1)==0);
    for(i=0U;i<10U&&!u->accurate_busy;i++)poll();
    assert(u->accurate_busy);
    tab_active(0U);assert(!u->session.running&&!u->settings.measurement_mode);
    assert(!u->accurate_busy&&!u->accurate_ready);

    /* 自动按钮在宽频输入下实际切Fs/N，并在停止状态完成后保持停止。 */
    for(i=0U;i<2U;i++){
        uint32_t tries;input_hz=i?200000.0f:1.0f;
        lv_obj_send_event(u->auto_button,LV_EVENT_CLICKED,NULL);
        for(tries=0U;tries<10000U&&u->auto_active;tries++)poll();
        lv_tick_inc(200U);poll(); /* 流畅测量的正常刷新节拍为200ms。 */
        assert(!u->auto_active&&!u->session.running);
        assert(u->session.config.record_samples>=512U&&u->session.config.record_samples<=8192U);
        assert(u->session.actual_rate_hz==(i?1000000U:2000U));
        assert(u->view_count*input_hz/(float)u->session.actual_rate_hz>=2.0f&&
               u->view_count*input_hz/(float)u->session.actual_rate_hz<=3.0f);
        assert(u->settings.trigger_mode==SCOPE_MODE_AUTO&&u->session.triggered);
        {
            size_t p;int low=32767,high=-32767,height=(int)lv_obj_get_height(u->plot);
            for(p=0U;p<u->sample_point_count;p++){
                if(u->sample_points[p][1]<low)low=u->sample_points[p][1];
                if(u->sample_points[p][1]>high)high=u->sample_points[p][1];
            }
            assert(high-low>height*8/10&&low>=0&&high<height);
        }
        assert(u->session.measurements.valid&SCOPE_VALID_FREQUENCY);
        assert(fabsf(u->session.measurements.frequency_hz-input_hz)<input_hz*0.02f);
    }
    /* 50Hz从峰顶开始，512点高速帧几乎全是平顶噪声，不能提前锁到20kHz。 */
    input_hz=50.0f;input_noise_codes=600.0;
    for(unsigned phase=0U;phase<16U;++phase){
        input_phase_cycles=(double)phase/16.0;
        lv_obj_send_event(u->auto_button,LV_EVENT_CLICKED,NULL);
        for(i=0U;i<10000U&&u->auto_active;i++)poll();
        lv_tick_inc(250U);poll();
        assert(!u->auto_active&&u->session.has_record);
        if(!(u->session.measurements.valid&SCOPE_VALID_FREQUENCY)||fabsf(u->session.measurements.frequency_hz-50.0f)>=1.0f)
            printf("bad 50Hz phase=%u f=%f rate=%u\n",phase,(double)u->session.measurements.frequency_hz,u->session.actual_rate_hz);
        assert((u->session.measurements.valid&SCOPE_VALID_FREQUENCY)&&
            fabsf(u->session.measurements.frequency_hz-50.0f)<1.0f);
        assert(u->view_count*50.0f/(float)u->session.actual_rate_hz>=2.0f&&
            u->view_count*50.0f/(float)u->session.actual_rate_hz<=3.0f);
    }
    {
        scope_io_config_t old_io=u->settings.io;
        float old_other=u->channel_time_div[1];
        u->settings.io=(scope_io_config_t){{1U,1U},{SCOPE_INPUT_PA5,SCOPE_INPUT_PC4},SCOPE_OUTPUT_PA4};
        u->channel_time_div[1]=0.0001f;
        lv_obj_send_event(u->auto_button,LV_EVENT_CLICKED,NULL);
        for(i=0U;i<10000U&&u->auto_active;i++)poll();
        assert(!u->auto_active&&u->view_count*50.0f/(float)u->session.actual_rate_hz>=2.0f);
        u->settings.io=old_io;u->channel_time_div[1]=old_other;
    }
    input_phase_cycles=0.0;input_noise_codes=0.0;
    input_bias_code=28000U;input_hz=4000.0f;
    lv_obj_send_event(u->auto_button,LV_EVENT_CLICKED,NULL);
    for(i=0U;i<10000U&&u->auto_active;i++){
        if(u->auto_active==2U)input_bias_code=24000U;
        poll();
    }
    assert(!u->auto_active&&fabsf(u->settings.vertical_offset_mv-
           (float)input_bias_code*3300.0f/65535.0f)<10.0f);
    {
        lv_area_t plot_area;
        lv_obj_get_coords(u->plot,&plot_area);
        {
            size_t point;int closest=lv_area_get_height(&plot_area);
            for(point=0U;point<u->sample_point_count;point++){
                int distance=abs((int)u->sample_points[point][1]+(int)u->pan_y-
                                 lv_area_get_height(&plot_area)/2);
                if(distance<closest)closest=distance;
            }
            assert(closest<8);
        }
        assert(color_pixels(plot_area.x1+2,plot_area.y1+2,
                            plot_area.x2-2,plot_area.y2-2,0xb5,0xc8,0xd8)>20U);
        if(getenv("SCOPE_RENDER")){
            lv_tick_inc(200U);poll(); /* 状态/读数沿用正常200ms刷新周期。 */
            screenshot("tmp/scope_center_auto.ppm");
        }
    }
    input_bias_code=32768U;
    u->settings.display.coupling=SCOPE_COUPLING_AC;input_hz=1000.0f;
    lv_obj_send_event(u->auto_button,LV_EVENT_CLICKED,NULL);
    for(i=0U;i<1000U&&u->auto_active;i++)poll();
    {
        float low=INFINITY,high=-INFINITY;size_t p;
        for(p=0U;p<u->sample_point_count;p++){
            low=fminf(low,u->sample_values[p]);high=fmaxf(high,u->sample_values[p]);
        }
        assert(!u->auto_active&&fabsf(u->settings.vertical_offset_mv-(low+high)*0.5f)<0.1f);
    }
    u->settings.display.coupling=SCOPE_COUPLING_DC;u->settings.display.inverted=1;
    lv_obj_send_event(u->auto_button,LV_EVENT_CLICKED,NULL);
    for(i=0U;i<1000U&&u->auto_active;i++)poll();
    assert(!u->auto_active&&u->settings.vertical_offset_mv<-1500.0f);
    /* 直流没有有效周期时，Auto仍把真实CH1像素放回视野并恢复原时基。 */
    u->settings.display.inverted=0;u->settings.mv_per_div=0.000001f;
    u->settings.vertical_offset_mv=-100000.0f;input_hz=0.0f;
    lv_obj_send_event(u->auto_button,LV_EVENT_CLICKED,NULL);
    for(i=0U;i<10000U&&u->auto_active;i++)poll();
    assert(!u->auto_active&&!u->session.running);
    assert(fabsf(u->settings.vertical_offset_mv-1650.0f)<5.0f);
    assert(u->settings.mv_per_div>=1.0e-6f&&u->plot_vertical_scale==1.0f);
    {
        lv_area_t plot_area;int center_y;
        lv_obj_get_coords(u->plot,&plot_area);
        center_y=plot_area.y1+lv_area_get_height(&plot_area)/2;
        assert(waveform_pixels(plot_area.x1+8,center_y-2,
                               plot_area.x2-8,center_y+2)>200U);
    }
    assert(strstr(lv_label_get_text(u->notice),"无有效周期"));
    /* 不对称方波以峰谷中点居中，不能用占空比相关的均值当屏幕中心。 */
    input_hz=1000.0f;square_input=1U;
    lv_obj_send_event(u->auto_button,LV_EVENT_CLICKED,NULL);
    for(i=0U;i<1000U&&u->auto_active;i++)poll();
    assert(!u->auto_active&&fabsf(u->settings.vertical_offset_mv-1650.0f)<5.0f);
    assert(u->session.triggered&&u->session.config.hysteresis_mv>0.0f);
    lv_tick_inc(250U);poll();
    assert(u->session.measurements.mean_mv<u->settings.vertical_offset_mv-100.0f);
    square_input=0U;noise_input=1U;
    lv_obj_send_event(u->auto_button,LV_EVENT_CLICKED,NULL);
    for(i=0U;i<10000U&&u->auto_active;i++)poll();
    assert(!u->auto_active&&u->sample_point_count>=2U);
    {
        size_t p;int low=32767,high=-32767,height=(int)lv_obj_get_height(u->plot);
        for(p=0U;p<u->sample_point_count;p++){
            if(u->sample_points[p][1]<low)low=u->sample_points[p][1];
            if(u->sample_points[p][1]>high)high=u->sample_points[p][1];
        }
        assert(low>=0&&high<height&&high-low>height*8/10);
    }
    /* PA5源在不同采样时间下读数变化：按恢复后真实新记录居中，而非低速搜索帧。 */
    noise_input=2U;
    u->settings.sample_rate_hz=85000.0f;u->settings.time_per_div_s=500.0f/850000.0f;
    assert(scope_ui_apply_settings()==0);
    u->channel_time_div[0]=u->settings.time_per_div_s;
    assert(scope_session_run(&u->session,lv_tick_get())==0);
    lv_obj_send_event(u->auto_button,LV_EVENT_CLICKED,NULL);
    for(i=0U;i<10000U&&u->auto_active;i++)poll();
    assert(!u->auto_active&&u->session.running);
    for(i=0U;i<30U;i++)poll();
    {
        scope_measurements_t final;
        assert(scope_measure(u->session.mv,u->session.record_count,(float)u->session.actual_rate_hz,&final));
        assert(u->session.actual_rate_hz==1000000U);
        assert(fabsf(u->settings.vertical_offset_mv-(final.min_mv+final.max_mv)*0.5f)<0.1f);
        if(getenv("SCOPE_RENDER"))screenshot("tmp/scope_pa5_noise_final.ppm");
    }
    scope_session_stop(&u->session);noise_input=0U;
    input_hz=1000.0f;
    /* CH2硬件失败必须保留真实原因，回滚CH1采集，不能提示成DAC电压越界。 */
    {
        lv_obj_t *page=lv_obj_get_child(lv_tabview_get_content(u->tabs),1);
        assert(scope_session_run(&u->session,lv_tick_get())==0);
        scope_ui_sync_controls();tab_active(1U);io_failure=-13;
        lv_dropdown_set_selected(page_field(page,"通道开关"),1U);apply_page(page);
        assert(strstr(lv_label_get_text(u->notice),"PC4")&&
               strstr(lv_label_get_text(u->notice),"-13")&&
               !strstr(lv_label_get_text(u->notice),"DAC"));
        assert(strstr(lv_label_get_text(u->notice),"隔离读回失败")&&
               strstr(lv_label_get_text(u->notice),"0128")&&strstr(lv_label_get_text(u->notice),"1140"));
        assert(u->session.running&&!u->settings.io.enabled[1]);
        u->settings.io.enabled[1]=1U;assert(scope_ui_apply_settings()==-13);
        u->settings.io.enabled[1]=0U;io_failure=0;
        assert(scope_ui_apply_settings()==0);scope_ui_sync_controls();
        assert(scope_session_run(&u->session,lv_tick_get())==0);
        io_failure=-4;driver.adc_stage=SCOPE_ADC_STAGE_CAL2;
        driver.adc_state[1]=0x10U;driver.adc_cr[1]=0x20U;
        lv_dropdown_set_selected(page_field(page,"通道开关"),1U);apply_page(page);
        assert(strstr(lv_label_get_text(u->notice),"CH2校准")&&
               strstr(lv_label_get_text(u->notice),"-4"));
        assert(u->session.running&&!u->settings.io.enabled[1]);io_failure=0;
        scope_session_stop(&u->session);tab_active(0U);
    }
    /* 两路同帧、独立变换、XY成对采样和引脚速率限制。 */
    {
        scope_settings_t previous=u->settings;uint32_t pixels;
        lv_obj_t *page=lv_obj_get_child(lv_tabview_get_content(u->tabs),1);
        u->settings.sample_rate_hz=100000.0f;u->settings.time_per_div_s=0.0005f;
        u->settings.mv_per_div=500.0f;u->settings.vertical_offset_mv=1650.0f;
        u->channel_time_div[0]=u->settings.time_per_div_s;
        u->channel_time_div[1]=u->settings.time_per_div_s;
        u->plot_vertical_offset_mv=0.0f;u->plot_vertical_offset2_mv=0.0f;
        u->pan_x=0.0f;u->pan_y=0.0f;u->settings.cursor_enabled=0U;
        scope_ui_sync_controls();tab_active(1U);
        lv_dropdown_set_selected(page_field(page,"通道开关"),1U);
        lv_textarea_set_text(page_field(page,"中心位置 mV"),"1650");apply_page(page);
        assert(u->settings.io.enabled[1]==1U&&u->settings.vertical_offset2_mv==1650.0f);
        assert(scope_session_run(&u->session,lv_tick_get())==0);
        for(i=0U;i<10U;i++)poll();
        scope_session_stop(&u->session);scope_ui_rebuild_display(0);
        u->channel_time_div[0]=u->settings.time_per_div_s;
        u->channel_time_div[1]=u->settings.time_per_div_s;scope_ui_rebuild_display(0);
        lv_tick_inc(250U);poll();
        assert(u->session.record_io.enabled[0]&&u->session.record_io.enabled[1]);
        assert(u->sample_point_count<=500U);
        for(i=0U;i<u->sample_point_count;i++)
            assert(fabsf(u->sample_values[i]+u->sample_values2[i]-3300.0f)<1.0f);
        /* 时域选中一路后，平移/纵缩放/横缩放不得改变另一路。 */
        {
            int16_t second[500][2];float channel1_pan,channel1_scale;size_t other_count;
            memcpy(second,u->sample_points2,sizeof(second));tab_active(0U);
            one_finger(230,240);one_finger(250,241);touch.count=0U;poll();
            assert(u->pan_x>0.0f);channel1_pan=u->pan_x;
            tab_active(1U);one_finger(230,240);one_finger(232,270);touch.count=0U;poll();
            assert(u->pan_x==channel1_pan&&u->pan_y==0.0f);
            channel1_scale=u->plot_vertical_scale;
            two_fingers(280,230,285,330);two_fingers(278,190,287,370);touch.count=0U;poll();
            assert(u->plot_vertical_scale==channel1_scale);lv_tick_inc(200U);poll();
            assert(memcmp(second,u->sample_points2,sizeof(second))!=0);
            other_count=u->view_count;
            two_fingers(230,270,350,275);two_fingers(170,268,410,277);touch.count=0U;poll();
            assert(u->view_count==other_count);lv_tick_inc(200U);poll();
            /* CH2缩放和平移后，首次开启游标应落在CH2可见窗口内。 */
            {
                lv_obj_t *cursor_page=lv_obj_get_child(lv_tabview_get_content(u->tabs),5);
                size_t k;
                u->settings.cursor_x[0]=1000.0f;u->settings.cursor_x[1]=1024.0f;
                u->settings.cursor_mv[0]=1.0e6f;u->settings.cursor_mv[1]=1.0e6f;
                scope_ui_sync_controls();tab_active(5U);
                lv_dropdown_set_selected(page_field(cursor_page,"游标"),1U);apply_page(cursor_page);
                for(k=0U;k<2U;k++){
                    float x=(u->settings.cursor_x[k]-u->view_origin[1])/
                        (u->view_span[1]-1.0f)*(SCOPE_PLOT_WIDTH-1)+u->pan_x2;
                    float y=(float)lv_obj_get_height(u->plot)*0.5f-
                        (u->settings.cursor_mv[k]-u->settings.vertical_offset2_mv-u->plot_vertical_offset2_mv)*
                        ((float)lv_obj_get_height(u->plot)/8.0f)*u->plot_vertical_scale2/u->settings.mv_per_div2+u->pan_y2;
                    assert(u->settings.cursor_enabled);
                    assert(x>=0.0f&&x<SCOPE_PLOT_WIDTH&&y>=0.0f&&y<lv_obj_get_height(u->plot));
                }
                u->settings.cursor_enabled=0U;scope_ui_sync_controls();
            }
            tab_active(0U);u->pan_x=0.0f;
        }
        /* CH2输入不同频率：Auto选CH2触发，不改CH1显示参数。 */
        {
            float old_div=u->channel_time_div[0],old_scale=u->plot_vertical_scale;
            float old_center=u->settings.vertical_offset_mv,old_mv=u->settings.mv_per_div;
            size_t p;uint32_t previous_sequence;
            input2_hz=2300.0f;tab_active(1U);u->settings.measurement_source=1U;
            lv_obj_send_event(u->auto_button,LV_EVENT_CLICKED,NULL);
            for(i=0U;i<10000U&&u->auto_active;i++)poll();
            assert(!u->auto_active&&!u->session.running&&u->session.triggered);
            assert(u->session.config.trigger_source==1U);
            assert(u->channel_time_div[0]==old_div&&u->plot_vertical_scale==old_scale&&
                   u->settings.vertical_offset_mv==old_center&&u->settings.mv_per_div==old_mv);
            assert(u->view_count2*input2_hz/u->session.actual_rate_hz>=2.0f&&
                   u->view_count2*input2_hz/u->session.actual_rate_hz<=3.0f);
            lv_tick_inc(250U);poll();
            assert(fabsf(u->session.measurements.frequency_hz-input2_hz)<input2_hz*0.02f);
            assert(strstr(lv_label_get_text(u->footer),"T: CH2 "));
            if(getenv("SCOPE_RENDER"))screenshot("tmp/scope_ch2_auto.ppm");
            assert(scope_session_run(&u->session,lv_tick_get())==0);
            previous_sequence=u->session.record_sequence;
            for(p=0U;p<20U;p++){
                poll();
                if(u->session.record_sequence!=previous_sequence){
                    size_t trigger=u->session.record_trigger_index;
                    assert(u->session.triggered&&trigger>0U);
                    assert(u->session.mv2[trigger-1U]<u->session.config.level_mv&&
                           u->session.mv2[trigger]>=u->session.config.level_mv);
                    previous_sequence=u->session.record_sequence;
                }
            }
            scope_session_stop(&u->session);input2_hz=0.0f;u->settings.measurement_source=0U;
            u->trigger_source=0U;u->trigger_hysteresis_mv=0.0f;tab_active(0U);
            u->settings.sample_rate_hz=100000.0f;u->settings.time_per_div_s=0.0005f;
            assert(scope_ui_apply_settings()==0);
            assert(scope_session_run(&u->session,lv_tick_get())==0);
            for(p=0U;p<50U;p++)poll();
            scope_session_stop(&u->session);
            u->channel_time_div[0]=u->settings.time_per_div_s;
            u->channel_time_div[1]=u->settings.time_per_div_s;
            assert(scope_ui_apply_settings()==0);
        }
        /* 同帧双FFT：共轴、独立变换；禁用一路后不画旧谱，不借另一通道。 */
        {
            lv_obj_t *math_page=lv_obj_get_child(lv_tabview_get_content(u->tabs),6);
            lv_obj_t *source=page_field(math_page,"FFT通道");
            size_t peak=1U;uint32_t frozen;float probe=u->settings.display2.probe_factor;
            assert(lv_dropdown_get_option_count(source)==3U);
            u->settings.fft_source=2U;u->settings.display.probe_factor=1.0f;
            u->settings.display.coupling=SCOPE_COUPLING_AC;
            u->settings.display2.coupling=SCOPE_COUPLING_AC;u->settings.display2.probe_factor=0.5f;
            assert(scope_ui_apply_settings()==0);u->fft_enabled=1U;
            lv_tick_inc(600U);poll();
            assert(u->fft_mask==3U&&u->fft_count>=4U&&u->fft_count<=SCOPE_FFT_MAX_LENGTH);
            assert(u->fft_sequence==u->session.record_sequence&&u->fft_rate_hz==u->session.actual_rate_hz);
            for(i=2U;i<=u->fft_count/2U;i++)if(u->fft[i]>u->fft[peak])peak=i;
            assert(u->fft[peak]>1.0f&&fabsf(u->fft2[peak]/u->fft[peak]-0.5f)<0.01f);
            assert(color_pixels(16,162,660,425,0xff,0xd6,0x6b)>50U);
            assert(color_pixels(16,162,660,425,0x66,0xd9,0xf0)>50U);
            if(getenv("SCOPE_RENDER"))screenshot("tmp/scope_dual_fft.ppm");
            frozen=u->fft_sequence;two_fingers(230,270,350,275);
            lv_tick_inc(600U);two_fingers(170,268,410,277);assert(u->fft_sequence==frozen);
            touch.count=0U;poll();lv_tick_inc(200U);poll();
            u->fft_frequency_scale=1.0f;u->settings.display2.probe_factor=probe;
            u->settings.io.enabled[1]=0U;assert(scope_ui_apply_settings()==0);
            assert(scope_session_run(&u->session,lv_tick_get())==0);
            for(i=0U;i<10U;i++)poll();
            scope_session_stop(&u->session);lv_tick_inc(600U);poll();
            assert(u->settings.fft_source==2U&&u->fft_mask==1U);
            assert(color_pixels(16,162,660,425,0x66,0xd9,0xf0)==0U);
            u->settings.io.enabled[1]=1U;assert(scope_ui_apply_settings()==0);
            assert(scope_session_run(&u->session,lv_tick_get())==0);
            for(i=0U;i<10U;i++)poll();
            scope_session_stop(&u->session);u->fft_enabled=0U;
            scope_ui_rebuild_display(0);
        }
        u->xy_enabled=1U;u->pan_x=0.0f;u->pan_y=0.0f;
        u->settings.vertical_offset_mv=0.0f;u->settings.vertical_offset2_mv=0.0f;
        u->plot_vertical_offset_mv=0.0f;u->plot_vertical_offset2_mv=0.0f;
        u->settings.mv_per_div=500.0f;u->settings.mv_per_div2=500.0f;
        u->plot_vertical_scale=1.0f;u->fft_horizontal_scale=1.0f;
        scope_ui_rebuild_display(0);pixels=plot_hash();
        u->settings.display2.inverted=1;scope_ui_rebuild_display(0);
        assert(plot_hash()!=pixels);
        u->settings.display2.inverted=0;scope_ui_rebuild_display(0);
        if(getenv("SCOPE_RENDER"))screenshot("tmp/scope_dual_xy.ppm");
        before=starts;time_div=u->settings.time_per_div_s;
        two_fingers(230,270,350,275);two_fingers(170,268,410,277);
        assert(u->fft_horizontal_scale>1.0f&&u->settings.time_per_div_s==time_div);
        assert(plot_hash()!=pixels);touch.count=0U;poll();assert(starts==before);
        lv_tick_inc(200U);poll();u->fft_horizontal_scale=1.0f;
        {
            float offset1=u->plot_vertical_offset_mv;
            two_fingers(280,230,285,330);two_fingers(278,190,287,370);
            assert(u->plot_vertical_scale>1.0f&&u->plot_vertical_offset_mv==offset1);
            touch.count=0U;poll();lv_tick_inc(200U);poll();u->plot_vertical_scale=1.0f;
        }
        u->xy_enabled=0U;scope_ui_rebuild_display(0);
        if(getenv("SCOPE_RENDER"))screenshot("tmp/scope_dual_time.ppm");
        u->settings.io.enabled[0]=0U;u->settings.sample_rate_hz=2000000.0f;
        u->settings.time_per_div_s=0.00001f;
        assert(scope_ui_apply_settings()==0);
        assert(u->session.io.enabled[0]==0U&&u->session.io.enabled[1]==1U);
        assert(u->settings.sample_rate_hz==2000000.0f);
        assert(scope_session_run(&u->session,lv_tick_get())==0);
        for(i=0U;i<10U;i++)poll();
        scope_session_stop(&u->session);
        u->settings.fft_source=2U;u->fft_enabled=1U;lv_tick_inc(600U);poll();
        assert(u->fft_mask==2U&&u->fft_count<=SCOPE_FFT_MAX_LENGTH);
        u->settings.fft_source=1U;assert(scope_ui_apply_settings()==0);
        lv_tick_inc(600U);poll();assert(u->fft_mask==2U);
        u->fft_enabled=0U;
        tab_active(4U);u->accurate_samples=512U;
        assert(scope_ui_set_accurate(1,1)==0);
        assert(u->settings.measurement_source==1U);
        for(i=0U;i<100U&&!u->accurate_ready;i++)poll();
        assert(u->accurate_ready);tab_active(0U);assert(!u->settings.measurement_mode);
        u->settings=previous;assert(scope_ui_apply_settings()==0);
    }
    /* 拖动中切屏或销毁，不在松手时偷偷重新打开ADC。 */
    {
        lv_obj_t *content=lv_tabview_get_content(u->tabs);uint32_t page_index;
        for(page_index=0U;page_index<lv_obj_get_child_count(content);page_index++){
            lv_obj_t *page=lv_obj_get_child(content,(int32_t)page_index);uint32_t child;
            tab_active(page_index);lv_obj_update_layout(u->screen);
            for(child=1U;child<lv_obj_get_child_count(page);child++){
                lv_area_t a,b;lv_obj_get_coords(lv_obj_get_child(page,(int32_t)child-1),&a);
                lv_obj_get_coords(lv_obj_get_child(page,(int32_t)child),&b);
                if(page_index!=4U)assert(a.y2<b.y1||a.x2<b.x1||b.x2<a.x1);
                lv_obj_t *o=lv_obj_get_child(page,(int32_t)child);
                if(lv_obj_check_type(o,&lv_label_class)){
                    assert(strcmp(lv_label_get_text(o),"采样率 Sa/s"));
                    assert(strcmp(lv_label_get_text(o),"点速率 Sa/s"));
                }
            }
            if(getenv("SCOPE_RENDER")){
                char path[64];(void)snprintf(path,sizeof(path),"tmp/scope_page_%lu.ppm",(unsigned long)page_index);
                lv_obj_scroll_to_y(page,0,LV_ANIM_OFF);screenshot(path);
            }
        }
        tab_active(0U);
    }
    assert(scope_session_run(&u->session,lv_tick_get())==0);before=starts;
    one_finger(210,260);one_finger(225,261);assert(u->drag_locked);
    lv_screen_load(guider_ui.DeskTop);touch.count=0U;poll();
    assert(!u->session.opened&&!u->drag_active&&starts==before);
    oscilloscope_ui_open();lv_obj_update_layout(u->screen);
    one_finger(210,260);one_finger(225,261);before=starts;assert(u->drag_locked);
    lv_obj_delete(u->screen);touch.count=0U;poll();
    assert(!u->session.opened&&!u->drag_active&&starts==before);
    oscilloscope_ui_open();lv_obj_update_layout(u->screen);
    tab_active(4U);assert(scope_ui_set_accurate(1,1)==0);
    lv_obj_delete(u->screen);
    assert(!u->settings.measurement_mode&&!u->accurate_busy&&!u->session.opened);
    oscilloscope_ui_open();assert(!u->settings.measurement_mode&&u->session.running);
    before=lv_obj_get_child_count(guider_ui.DeskTop_tileview_DeskTop_tile_1);
    oscilloscope_ui_attach_desktop(guider_ui.DeskTop);
    assert(lv_obj_get_child_count(guider_ui.DeskTop_tileview_DeskTop_tile_1)==before);
    /* 桌面重建允许新旧屏同时存在；旧图标稍后删除不能清除新图标身份。 */
    old_desktop=guider_ui.DeskTop;guider_ui.DeskTop=lv_obj_create(NULL);
    guider_ui.DeskTop_tileview_DeskTop_tile_1=lv_obj_create(guider_ui.DeskTop);
    lv_obj_set_size(guider_ui.DeskTop_tileview_DeskTop_tile_1,1000,565);
    oscilloscope_ui_attach_desktop(guider_ui.DeskTop);new_entry=u->entry;
    lv_obj_delete(old_desktop);assert(u->entry==new_entry);
    lv_screen_load(guider_ui.DeskTop);poll();assert(!u->session.opened);
    lv_obj_send_event(new_entry,LV_EVENT_CLICKED,NULL);
    assert(lv_screen_active()==u->screen&&u->session.opened&&u->session.running);
    /* 新进入APP及双路切换：每43ms轮询，DMA每次跳7块，仍有新记录且Auto完成。 */
    slow_ui=1U;dma_sequence_step=8U;input_hz=1000.0f;
    u->settings.sample_rate_hz=85000.0f;u->settings.time_per_div_s=500.0f/850000.0f;
    u->settings.io.enabled[0]=1U;u->settings.io.enabled[1]=0U;
    u->settings.io.input[0]=SCOPE_INPUT_PC4;u->settings.io.input[1]=SCOPE_INPUT_PA5;
    u->settings.trigger_mode=SCOPE_MODE_AUTO;assert(scope_ui_apply_settings()==0);
    before=u->session.record_sequence;
    for(i=0U;i<5U;i++)poll();
    assert(u->session.running&&u->session.record_sequence>=before+5U&&u->sample_point_count>1U);
    lv_obj_send_event(u->auto_button,LV_EVENT_CLICKED,NULL);
    for(i=0U;i<1000U&&u->auto_active;i++)poll();
    assert(!u->auto_active&&u->session.running);
    assert(u->view_count*input_hz/u->session.actual_rate_hz>=2.0f&&
           u->view_count*input_hz/u->session.actual_rate_hz<=3.0f);
    u->settings.io.enabled[1]=1U;assert(scope_ui_apply_settings()==0);
    before=u->session.record_sequence;
    for(i=0U;i<20U;i++)poll();
    assert(u->session.running&&u->session.record_sequence>=before+20U&&u->sample_point_count2>1U);
    /* 双路曲线移入视野并松手后应恢复采集，关闭/重开CH2也继续刷新。 */
    {
        float ch1_pan=u->pan_y;uint32_t fs=u->session.config.sample_rate_hz;
        scope_ui_select_channel(1U);u->settings.cursor_enabled=0U;u->pan_y2=-250.0f;
        one_finger(250,220);one_finger(251,240);one_finger(251,390);
        assert(u->drag_locked&&!u->session.running&&u->pan_y2>-100.0f&&u->pan_y==ch1_pan);
        touch.count=0U;poll();assert(u->session.running&&u->session.config.sample_rate_hz==fs);
        before=u->session.record_sequence;for(i=0U;i<5U;i++)poll();
        assert(u->session.record_sequence>=before+5U);
        u->settings.io.enabled[1]=0U;assert(scope_ui_apply_settings()==0);
        for(i=0U;i<3U;i++)poll();
        u->settings.io.enabled[1]=1U;assert(scope_ui_apply_settings()==0);
        before=u->session.record_sequence;for(i=0U;i<5U;i++)poll();
        assert(u->session.running&&u->session.record_sequence>=before+5U);
        u->pan_y2=0.0f;
    }
    if(getenv("SCOPE_RENDER"))screenshot("tmp/scope_dual_refresh_final.ppm");
    /* CH2单独首次绘制不依赖CH1旧视窗，必须显示坐标轴。 */
    {
        u->settings.io.enabled[0]=0U;assert(scope_ui_apply_settings()==0);
        for(i=0U;i<5U;i++)poll();
        u->view_count=0U;scope_ui_rebuild_display(0);
        assert(u->view_count2>=512U);
        assert(color_pixels(16,162,660,448,0xb5,0xc8,0xd8)>20U);
        u->settings.io.enabled[0]=1U;assert(scope_ui_apply_settings()==0);
    }
    /* 极端独立窗口：CH2扩大不能把CH1的51.2us/div默默扩大500倍。 */
    {
        scope_settings_t saved=u->settings;
        float saved_div0=u->channel_time_div[0],saved_div1=u->channel_time_div[1];
        unsigned selected=u->selected_channel;
        scope_ui_select_channel(0U);
        u->channel_time_div[0]=0.0000512f;u->channel_time_div[1]=0.0000512f;
        u->settings.time_per_div_s=0.0000512f;
        assert(scope_ui_apply_settings()==0);
        for(i=0U;i<5U;i++)poll();
        float first_div=u->channel_time_div[0];
        scope_ui_select_channel(1U);u->settings.time_per_div_s=0.25f;
        assert(scope_ui_apply_settings()==0);
        for(i=0U;i<5U;i++)poll();
        assert(u->channel_time_div[0]==first_div&&u->view_count>=2U&&u->view_count2>=2U);
        assert(fabsf((float)u->view_count/(10.0f*u->session.actual_rate_hz)-first_div)<=1.0f/(float)u->session.actual_rate_hz);
        assert(u->channel_time_div[1]<=0.164f&&u->session.config.record_samples==8192U);
        /* 无周期Auto不能把另一窗口需要的、当前视野外尖峰用于缩小本通道。 */
        scope_ui_select_channel(0U);noise_input=3U;
        lv_obj_send_event(u->auto_button,LV_EVENT_CLICKED,NULL);
        for(i=0U;i<10000U&&u->auto_active;i++)poll();
        assert(!u->auto_active&&u->view_count>=2U&&u->session.record_count==8192U);
        assert(fabsf(u->settings.vertical_offset_mv-30000.0f*3300.0f/65535.0f)<0.1f);
        assert(u->settings.mv_per_div<1.0f);noise_input=0U;
        u->settings=saved;u->selected_channel=(uint8_t)selected;
        u->channel_time_div[0]=saved_div0;u->channel_time_div[1]=saved_div1;
        assert(scope_ui_apply_settings()==0);scope_ui_sync_controls();
    }
    {
        static const unsigned counts[]={512U,513U,799U,800U,2048U,4096U,8191U,8192U};
        size_t case_index;
        for(case_index=0U;case_index<sizeof(counts)/sizeof(counts[0]);case_index++){
            u->settings.sample_rate_hz=100000.0f;
            u->settings.time_per_div_s=(float)counts[case_index]/1000000.0f;
            u->channel_time_div[1]=u->settings.time_per_div_s;
            assert(scope_ui_apply_settings()==0);poll();
            two_fingers(200,240,300,240);two_fingers(200,240,340,240);
            assert(u->pinch_active&&!u->session.running);
            touch.count=0U;poll();assert(u->session.running);
            before=u->session.record_sequence;for(i=0U;i<5U;i++)poll();
            assert(u->session.record_sequence>=before+5U);
        }
        two_fingers(200,220,300,300);two_fingers(200,200,300,330);
        io_failure=-4;driver.adc_stage=SCOPE_ADC_STAGE_CAL2;
        touch.count=0U;poll();
        assert(!u->session.running&&strstr(lv_label_get_text(u->notice),"-4")&&
               strstr(lv_label_get_text(u->notice),"CH2校准"));
        io_failure=0;assert(scope_session_run(&u->session,lv_tick_get())==0);
    }
    /* 复现实机1MSa/s、2512点、1kHz双路：FFT不能只取不足一周期的256点。 */
    scope_session_stop(&u->session);u->fft_enabled=1U;u->xy_enabled=0U;
    u->fft_frequency_scale=u->fft_vertical_scale=1.0f;u->fft_pan_x=u->fft_pan_y=0.0f;
    u->settings.fft_source=2U;u->settings.display.coupling=SCOPE_COUPLING_DC;
    u->settings.display2.coupling=SCOPE_COUPLING_DC;
    u->settings.display.probe_factor=u->settings.display2.probe_factor=1.0f;
    u->session.has_record=1U;u->session.record_count=2512U;u->session.actual_rate_hz=1000000U;
    u->session.record_io.enabled[0]=u->session.record_io.enabled[1]=1U;
    u->view_start=u->view_start2=0U;u->view_count=u->view_count2=2512U;
    for(i=0U;i<2512U;i++){
        u->session.mv[i]=1650.0f+500.0f*sinf(6.283185307f*(float)i/1000.0f);
        u->session.mv2[i]=1400.0f+250.0f*sinf(6.283185307f*(float)i/1000.0f);
    }
    lv_tick_inc(600U);poll();
    assert(u->fft_mask==3U&&u->fft_count==2048U);
    assert(u->fft[2]>400.0f&&fabsf(u->fft2[2]/u->fft[2]-0.5f)<0.01f);
    assert(u->fft[1]<u->fft[2]&&u->fft[0]<50.0f);
    if(getenv("SCOPE_RENDER"))screenshot("tmp/scope_fft_1k_full_window.ppm");
    u->session.record_io.enabled[0]=0U;u->view_count=512U;
    lv_tick_inc(600U);poll();
    assert(u->fft_mask==2U&&u->fft_count==2048U);
    u->session.record_io.enabled[0]=1U;u->view_count=2512U;
    u->view_start=1500U;u->view_count=1012U;u->view_count2=512U;
    lv_tick_inc(600U);poll();
    assert(u->fft_mask==3U&&u->fft_count==2048U);
    assert(fabsf(u->fft2[2]/u->fft[2]-0.5f)<0.01f);
    u->view_start=0U;u->view_count=u->view_count2=2512U;
    /* 1%弱谐波在dBV轴可见；切线性及FFT复位不影响时域或ADC。 */
    {
        lv_obj_t *math_page=lv_obj_get_child(lv_tabview_get_content(u->tabs),6);
        float time_x=u->pan_x,time_y=u->pan_y,time_scale=u->plot_vertical_scale;
        u->fft_count=2048U;u->fft_mask=1U;u->fft_span_hz=16000.0f;u->fft_rate_hz=128000U;
        u->fft_frequency_scale=u->fft_vertical_scale=1.0f;u->fft_pan_x=u->fft_pan_y=0.0f;
        memset(u->fft,0,1025U*sizeof(float));u->fft[32]=1000.0f;u->fft[96]=10.0f;
        assert(u->fft_linear==0U);
        capture_plot_labels();
        assert(strstr(captured_labels,"CH1 峰 2 kHz / -3.01 dBV"));
        assert(strstr(captured_labels,"df=62.5 Hz | N=2048"));
        if(getenv("SCOPE_RENDER"))screenshot("tmp/scope_fft_dbv_harmonic.ppm");
        assert(spectrum_peak_pixels(287,303)>5U);
        before=starts;
        lv_obj_send_event(button_named(math_page,"dBV / 线性"),LV_EVENT_CLICKED,NULL);
        assert(u->fft_linear==1U&&starts==before);
        capture_plot_labels();
        assert(strstr(captured_labels,"CH1 峰 2 kHz / 707.11 mV"));
        assert(strstr(captured_labels,"线性 mV RMS"));
        assert(spectrum_peak_pixels(287,303)==0U);
        if(getenv("SCOPE_RENDER"))screenshot("tmp/scope_fft_linear_harmonic.ppm");
        lv_obj_send_event(button_named(math_page,"dBV / 线性"),LV_EVENT_CLICKED,NULL);
        assert(u->fft_linear==0U&&starts==before);
        u->fft_frequency_scale=7.0f;u->fft_vertical_scale=3.0f;u->fft_pan_x=93.0f;u->fft_pan_y=-42.0f;
        lv_obj_send_event(button_named(math_page,"FFT复位"),LV_EVENT_CLICKED,NULL);
        assert(u->fft_frequency_scale==1.0f&&u->fft_vertical_scale==1.0f);
        assert(u->fft_pan_x==0.0f&&u->fft_pan_y==0.0f&&starts==before);
        assert(u->pan_x==time_x&&u->pan_y==time_y&&u->plot_vertical_scale==time_scale);
        memset(u->fft,0,1025U*sizeof(float));u->fft[7]=NAN;u->fft[8]=INFINITY;u->fft[9]=-5.0f;
        assert(waveform_pixels(100,240,630,390)==0U);
        /* 放大到约56像素/bin，半bin处应在斜坡中部，而非最近邻平台。 */
        memset(u->fft,0,1025U*sizeof(float));u->fft[3]=1000.0f;
        u->fft_linear=1U;u->fft_span_hz=640.0f;
        assert(spectrum_region_pixels(216,300,224,350)>5U);
        u->fft_linear=0U;
        assert(scope_session_run(&u->session,lv_tick_get())==0);before=starts;
        lv_obj_send_event(button_named(math_page,"dBV / 线性"),LV_EVENT_CLICKED,NULL);
        lv_obj_send_event(button_named(math_page,"FFT复位"),LV_EVENT_CLICKED,NULL);
        assert(u->session.running&&driver.acquiring&&starts==before);
        lv_obj_send_event(button_named(math_page,"dBV / 线性"),LV_EVENT_CLICKED,NULL);
        assert(u->session.running&&starts==before);scope_session_stop(&u->session);
    }
    /* 宽频谱中的单bin窄峰不能被像素抽点漏掉，平移只施加一次。 */
    u->fft_count=8192U;u->fft_mask=1U;u->fft_span_hz=500000.0f;u->fft_rate_hz=1000000U;
    u->fft_frequency_scale=1.0f;u->fft_vertical_scale=1.0f;u->fft_pan_x=u->fft_pan_y=0.0f;
    memset(u->fft,0,4097U*sizeof(float));u->fft[631U]=100.0f;
    if(getenv("SCOPE_RENDER"))screenshot("tmp/scope_fft_narrow_peak.ppm");
    assert(spectrum_peak_pixels(160,176)>0U);
    u->fft_pan_x=50.0f;
    assert(spectrum_peak_pixels(210,226)>0U);
    assert(spectrum_peak_pixels(160,176)==0U);
    u->fft_pan_x=0.0f;
    u->fft_enabled=0U;
    u->settings.mv_per_div=2070.0f;
    lv_tick_inc(250U);poll();
    assert(strstr(lv_label_get_text(u->channel_label),"2.07 V/div"));
    {
        lv_obj_t *page=lv_obj_get_child(lv_tabview_get_content(u->tabs),4);
        lv_obj_t *channel=page_field(page,"常驻1通道");
        lv_obj_t *item=page_field(page,"常驻1项目");
        assert(channel&&item&&lv_dropdown_get_option_count(item)==10U);
        lv_dropdown_set_selected(channel,1U);lv_dropdown_set_selected(item,1U);
        before=starts;apply_page(page);
        lv_tick_inc(250U);poll();
        assert(starts==before&&u->settings.reading_source[0]==1U&&u->settings.reading_item[0]==1U);
        assert(strcmp(lv_label_get_text(u->reading_titles[0]),"CH2 / 周期")==0);
        assert(strstr(lv_label_get_text(u->readings[0]),"ms"));
        assert(lv_color_eq(lv_obj_get_style_text_color(u->reading_titles[0],0),lv_color_hex(0x66d9f0)));
        u->session.record_io.enabled[1]=0U;lv_tick_inc(250U);poll();
        assert(strcmp(lv_label_get_text(u->readings[0]),"--")==0);
        u->session.record_io.enabled[1]=1U;
        u->session.record_io.enabled[0]=0U;u->settings.measurement_source=0U;
        lv_tick_inc(250U);poll();
        assert(strstr(lv_label_get_text(u->readings[0]),"ms"));
        u->session.record_io.enabled[0]=1U;
        if(getenv("SCOPE_RENDER")){
            tab_active(4U);lv_obj_scroll_to_y(page,0,LV_ANIM_OFF);
            screenshot("tmp/scope_measure_cards.ppm");
            lv_obj_scroll_to_view(channel,LV_ANIM_OFF);screenshot("tmp/scope_measure_card_settings.ppm");
            tab_active(0U);
        }
    }
    {
        char formatted[48];
        scope_format_quantity(formatted,sizeof(formatted),500000.0,SCOPE_UNIT_HZ);
        assert(strcmp(formatted,"500 kHz")==0);
        scope_format_quantity(formatted,sizeof(formatted),-0.00000125,SCOPE_UNIT_SECONDS);
        assert(strcmp(formatted,"-1.25 us")==0);
        scope_format_fixed(formatted,sizeof(formatted),0.000000001,9U);
        assert(strcmp(formatted,"0.000000001")==0);
        scope_format_quantity(formatted,sizeof(formatted),1234.567,SCOPE_UNIT_HZ);
        assert(strcmp(formatted,"1.23 kHz")==0);
        scope_format_quantity_precision(formatted,sizeof(formatted),1234.567,SCOPE_UNIT_HZ,5U);
        assert(strcmp(formatted,"1.23457 kHz")==0);
    }
    {
        scope_settings_t previous=u->settings;
        lv_obj_t *page=lv_obj_get_child(lv_tabview_get_content(u->tabs),7);
        lv_obj_t *wave=page_field(page,"设定波形");
        assert(wave&&lv_dropdown_get_option_count(wave)==11U);
        scope_settings_defaults(&u->settings);scope_ui_sync_controls();
        u->settings.dac.offset_mv=1650.1234f;scope_ui_sync_controls();
        assert(strcmp(lv_textarea_get_text(page_field(page,"偏置 mV")),"1650.12")==0);
        lv_dropdown_set_selected(wave,SCOPE_WAVE_HARMONIC);
        lv_textarea_set_text(page_field(page,"8次幅值 %"),"20");
        lv_textarea_set_text(page_field(page,"8次相位 度"),"90");
        apply_page(page);
        lv_obj_send_event(button_named(page,"开启输出"),LV_EVENT_CLICKED,NULL);
        assert(u->session.dac_enabled&&u->settings.dac.harmonic_percent[7]==20.0f);
        assert(u->settings.dac.harmonic_phase_deg[7]==90.0f);
        assert(u->settings.dac.offset_mv==1650.1234f);
        assert(u->session.dac_count>=256U*u->session.dac_cycles);
        assert(fabsf(u->session.dac_frequency_hz-1000.0f)<0.01f);
        {
            float amplitude=u->settings.dac.amplitude_mv;
            for(unsigned shape=0U;shape<=SCOPE_WAVE_HARMONIC;shape++)if(shape!=SCOPE_WAVE_DC){
                uint16_t low=4095U,high=0U;
                lv_dropdown_set_selected(wave,SCOPE_WAVE_DC);
                lv_obj_send_event(wave,LV_EVENT_VALUE_CHANGED,NULL);
                assert(u->settings.dac.amplitude_mv==0.0f && u->session.dac_enabled);
                assert(u->session.dac_rate_hz==0U);
                lv_dropdown_set_selected(wave,shape);
                lv_obj_send_event(wave,LV_EVENT_VALUE_CHANGED,NULL);
                assert(u->settings.dac.amplitude_mv==amplitude && u->session.dac_enabled);
                for(size_t sample=0U;sample<u->session.dac_count;sample++){
                    if(u->session.dac_table[sample]<low)low=u->session.dac_table[sample];
                    if(u->session.dac_table[sample]>high)high=u->session.dac_table[sample];
                }
                assert((uint32_t)high-(uint32_t)low>100U);
            }
            lv_dropdown_set_selected(wave,SCOPE_WAVE_DC);
            lv_obj_send_event(wave,LV_EVENT_VALUE_CHANGED,NULL);
            lv_tick_inc(250U);poll();assert(strstr(lv_label_get_text(u->footer),"1.65 V"));
            lv_textarea_set_text(page_field(page,"偏置 mV"),"100");
            lv_obj_send_event(page_field(page,"偏置 mV"),LV_EVENT_READY,NULL);
            lv_dropdown_set_selected(wave,SCOPE_WAVE_HARMONIC);
            lv_obj_send_event(wave,LV_EVENT_VALUE_CHANGED,NULL);
            assert(u->session.dac_enabled && u->settings.dac.amplitude_mv==200.0f);
            u->settings.dac.offset_mv=1650.1234f;u->settings.dac.amplitude_mv=amplitude;
            scope_ui_sync_controls();
        }
        if(getenv("SCOPE_RENDER")){
            tab_active(7U);lv_obj_scroll_to_y(page,0,LV_ANIM_OFF);screenshot("tmp/scope_dac_functions.ppm");
            lv_obj_scroll_to_view(page_field(page,"8次相位 度"),LV_ANIM_OFF);
            screenshot("tmp/scope_dac_harmonics.ppm");tab_active(0U);
        }
        lv_textarea_set_text(page_field(page,"频率 Hz"),"4000");
        lv_obj_send_event(page_field(page,"频率 Hz"),LV_EVENT_READY,NULL);
        assert(u->session.dac_enabled&&u->settings.dac.frequency_hz==1000.0f);
        assert(strstr(lv_label_get_text(u->notice),"3.91"));
        driver.dac_enabled=0U;driver.dac_rate_hz=0U;driver.last_error=-9;
        driver.dac_underruns=8U;driver.dac_recoveries=7U;
        lv_tick_inc(250U);poll();
        assert(!u->session.dac_enabled);
        assert(strstr(lv_label_get_text(u->notice),"DAC")&&strstr(lv_label_get_text(u->notice),"-9"));
        assert(strstr(lv_label_get_text(u->footer),"恢7"));
        lv_obj_send_event(button_named(page,"开启输出"),LV_EVENT_CLICKED,NULL);
        assert(u->session.dac_enabled);
        driver.dac_underruns=0U;driver.dac_recoveries=0U;driver.last_error=0;
        lv_obj_send_event(button_named(page,"关闭输出"),LV_EVENT_CLICKED,NULL);
        assert(!u->session.dac_enabled);
        u->settings=previous;scope_ui_sync_controls();
    }
    {
        scope_settings_t old=u->settings;
        scope_session_stop(&u->session);scope_ui_select_channel(0U);
        u->settings.cursor_enabled=2U;u->settings.measurement_source=0U;
        assert(scope_ui_apply_settings()==0);
        u->session.has_record=1U;u->session.record_count=2048U;u->session.actual_rate_hz=100000U;
        u->session.record_io.enabled[0]=1U;u->channel_time_div[0]=0.002f;
        for(unsigned frame_index=0U;frame_index<2U;++frame_index){
            float period=frame_index?200.0f:100.0f,amplitude=frame_index?250.0f:500.0f;
            for(i=0U;i<2048U;++i)u->session.mv[i]=1650.0f+amplitude*sinf(6.283185307f*(float)i/period);
            scope_ui_rebuild_display(0);
            assert(fabsf(u->settings.cursor_x[1]-u->settings.cursor_x[0]-period)<0.1f);
            assert(fabsf(u->settings.cursor_mv[1]-u->settings.cursor_mv[0]-2.0f*amplitude)<0.1f);
            lv_tick_inc(250U);poll();
        }
        u->settings.display.coupling=SCOPE_COUPLING_AC;
        u->settings.display.probe_factor=10.0f;u->settings.display.inverted=1;
        scope_ui_rebuild_display(0);
        assert(u->follow_plot_mv[0]>2400.0f&&u->follow_plot_mv[0]<2600.0f);
        assert(fabsf(u->follow_plot_mv[1]-u->follow_plot_mv[0]+5000.0f)<0.1f);
        u->settings.acquisition_mode=2U;u->settings.average_count=2U;u->average_frames=0U;
        u->settings.display.coupling=SCOPE_COUPLING_DC;u->settings.display.probe_factor=1.0f;u->settings.display.inverted=0;
        for(i=0U;i<2048U;++i)u->session.mv[i]=1650.0f+500.0f*sinf(6.283185307f*(float)i/100.0f);
        scope_ui_rebuild_display(1);
        for(i=0U;i<2048U;++i)u->session.mv[i]=1650.0f+250.0f*sinf(6.283185307f*(float)i/100.0f);
        scope_ui_rebuild_display(1);
        assert(fabsf(u->settings.cursor_mv[1]-u->settings.cursor_mv[0]-750.0f)<0.1f);
        u->settings.acquisition_mode=0U;
        for(i=0U;i<2048U;++i)u->session.mv[i]=1650.0f;
        scope_ui_rebuild_display(0);lv_tick_inc(250U);poll();
        assert(!(u->follow_valid&SCOPE_VALID_PERIOD));
        assert(strstr(lv_label_get_text(u->cursor_label),"dt --"));
        assert(u->settings.cursor_mv[0]==u->settings.cursor_mv[1]);
        u->settings=old;scope_ui_sync_controls();
    }
    {
        scope_settings_t old=u->settings;float old_input2=input2_hz;
        tab_active(4U);scope_ui_select_channel(1U);
        u->settings.cursor_enabled=2U;u->settings.measurement_source=0U;
        u->accurate_samples=2048U;input2_hz=2000.0f;
        assert(scope_ui_set_accurate(1,1)==0);
        for(i=0U;i<100U&&!u->accurate_ready;i++)poll();
        assert(u->accurate_ready&&u->accurate_source==1U&&u->settings.measurement_source==1U);
        assert(fabsf(u->session.measurements.frequency_hz-2000.0f)<20.0f);
        assert(fabsf(u->settings.cursor_x[1]-u->settings.cursor_x[0]-
            u->session.measurements.period_s*(float)u->session.actual_rate_hz)<0.1f);
        {
            lv_obj_t *cursor=page_field(lv_obj_get_child(lv_tabview_get_content(u->tabs),5),"游标");
            lv_dropdown_set_selected(cursor,1U);lv_obj_send_event(cursor,LV_EVENT_VALUE_CHANGED,NULL);
            lv_obj_t *source=page_field(u->measure_page,"测量通道");
            lv_dropdown_set_selected(source,0U);lv_obj_send_event(source,LV_EVENT_VALUE_CHANGED,NULL);
        }
        poll();assert(!u->accurate_ready);
        for(i=0U;i<100U&&!u->accurate_ready;i++)poll();
        assert(u->accurate_ready&&u->accurate_source==0U);
        assert(fabsf(u->session.measurements.frequency_hz-input_hz)<20.0f);
        {
            lv_obj_t *source=page_field(u->measure_page,"测量通道");
            lv_dropdown_set_selected(source,1U);lv_obj_send_event(source,LV_EVENT_VALUE_CHANGED,NULL);
            assert(u->settings.measurement_source==1U&&u->smooth_settings.measurement_source==1U);
        }
        tab_active(0U);assert(u->settings.measurement_source==1U);
        u->settings=old;input2_hz=old_input2;assert(scope_ui_apply_settings()==0);
    }
    {
        lv_obj_t *page=lv_obj_get_child(lv_tabview_get_content(u->tabs),0);
        lv_obj_t *range=page_field(page,"量程 mV/div");
        before=starts;mv_div=u->settings.mv_per_div;
        lv_textarea_set_text(range,"350");lv_obj_send_event(range,LV_EVENT_VALUE_CHANGED,NULL);
        assert(u->settings.mv_per_div==mv_div&&starts==before);
        lv_obj_send_event(range,LV_EVENT_READY,NULL);
        assert(u->settings.mv_per_div==350.0f&&starts==before);
        lv_obj_send_event(range,LV_EVENT_DEFOCUSED,NULL);assert(starts==before);
        lv_textarea_set_text(range,"-1");lv_obj_send_event(range,LV_EVENT_READY,NULL);
        assert(u->settings.mv_per_div==350.0f);
        lv_obj_t *save=lv_obj_get_child(lv_tabview_get_content(u->tabs),9);
        assert(scope_session_run(&u->session,lv_tick_get())==0);before=starts;
        lv_obj_send_event(button_named(save,"保存设置"),LV_EVENT_CLICKED,NULL);assert(starts==before);
        for(i=0U;i<10U;i++)assert(!find_button(lv_obj_get_child(lv_tabview_get_content(u->tabs),(int32_t)i),"应用"));
    }
    {
        lv_obj_t *page=lv_obj_get_child(lv_tabview_get_content(u->tabs),4);
        if(u->settings.measurement_mode)(void)scope_ui_set_accurate(0,0);
        scope_session_stop(&u->session);u->selected_channel=0U;u->settings.cursor_enabled=2U;
        u->settings.acquisition_mode=0U;u->settings.display.coupling=SCOPE_COUPLING_DC;
        u->settings.display.probe_factor=1.0f;u->settings.display.inverted=0;
        u->settings.vertical_offset_mv=1650.0f;u->settings.mv_per_div=150.0f;
        u->plot_vertical_offset_mv=0.0f;u->plot_vertical_scale=1.0f;u->pan_x=u->pan_y=0.0f;
        u->channel_time_div[0]=0.00025f;u->session.actual_rate_hz=100000U;
        u->session.record_count=2048U;u->session.has_record=1U;
        for(i=0U;i<2048U;i++)u->session.mv[i]=1650.0f+500.0f*sinf(6.283185307f*(float)i/100.0f);
        scope_ui_rebuild_display(0);lv_tick_inc(250U);poll();
        assert(button_named(page,"读数")&&button_named(page,"设置")&&button_named(page,"常驻"));
        lv_obj_send_event(button_named(page,"读数"),LV_EVENT_CLICKED,NULL);
        tab_active(4U);lv_obj_update_layout(page);assert(lv_obj_get_scroll_bottom(page)==0);
        lv_obj_send_event(button_named(page,"常驻"),LV_EVENT_CLICKED,NULL);
        lv_obj_update_layout(page);assert(lv_obj_get_scroll_bottom(page)==0);
        if(getenv("SCOPE_RENDER"))screenshot("tmp/scope_measure_compact_cards.ppm");
        lv_obj_send_event(button_named(page,"读数"),LV_EVENT_CLICKED,NULL);
        if(getenv("SCOPE_RENDER"))screenshot("tmp/scope_measure_compact_readings.ppm");
        tab_active(0U);
    }
    io_failure=-13;lv_obj_send_event(u->auto_button,LV_EVENT_CLICKED,NULL);
    assert(!u->auto_active&&strstr(lv_label_get_text(u->notice),"-13"));io_failure=0;
    puts("PASS oscilloscope_ui: continuous after single, keyboard toggle, cursor/layout, dual FFT, locked gestures, measurement lifetime, auto 1Hz/200kHz, independent CH1/CH2, CH2 cursors/trigger, noise/DC auto fit, final-rate noise centering, dual refresh with skipped DMA, dual drag/re-enable, ADC failure stage, pinch boundaries/restart");return 0;
}
