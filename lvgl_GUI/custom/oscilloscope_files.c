#include "oscilloscope_ui_private.h"
#include "oscilloscope_accurate.h"
#include "sdcard_manager.h"
#include "sdram_layout.h"
#include "lv_port_disp.h"
#include "ff.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SCOPE_SD_OWNER 0x5343U
#define SCOPE_DIRECTORY "0:/OSCILLOSCOPE"
static FIL scope_file;
static uint8_t file_opened;
static int io_open(void *context,const char *path,scope_file_mode_t mode)
{
    FRESULT r;(void)context;
    if(file_opened)return SCOPE_IO_ERROR;
    r=f_open(&scope_file,path,mode==SCOPE_FILE_READ?FA_READ:(FA_WRITE|FA_CREATE_NEW));
    if(r==FR_EXIST)return SCOPE_IO_EXISTS;
    if(r!=FR_OK)return SCOPE_IO_ERROR;
    file_opened=1U;return SCOPE_IO_OK;
}
static size_t io_read(void *context,void *buffer,size_t bytes)
{
    UINT count=0U;(void)context;
    if(!file_opened||bytes>65535U)return SIZE_MAX;
    if(f_read(&scope_file,buffer,(UINT)bytes,&count)!=FR_OK)return SIZE_MAX;
    return (size_t)count;
}
static size_t io_write(void *context,const void *buffer,size_t bytes)
{
    UINT count=0U;(void)context;
    if(!file_opened||bytes>65535U)return SIZE_MAX;
    if(f_write(&scope_file,buffer,(UINT)bytes,&count)!=FR_OK)return SIZE_MAX;
    return (size_t)count;
}
static int io_close(void *context)
{
    FRESULT r;(void)context;if(!file_opened)return 0;
    r=f_close(&scope_file);file_opened=0U;return r==FR_OK;
}
static int io_remove(void *context,const char *path)
{ (void)context;return f_unlink(path)==FR_OK; }
static int io_next(void *context,const char *directory,size_t index,char *name,size_t capacity)
{
    DIR dir;FILINFO info;FRESULT r;size_t n=0U,length;int result=0;
    (void)context;
    if(f_opendir(&dir,directory)!=FR_OK)return -1;
    for(;;){
        r=f_readdir(&dir,&info);if(r!=FR_OK){result=-1;break;}
        if(info.fname[0]=='\0')break;
        if((info.fattrib&AM_DIR)!=0U)continue;
        if(n++!=index)continue;
        length=strlen(info.fname);
        if(length>=capacity){result=-1;break;}
        memcpy(name,info.fname,length+1U);result=1;break;
    }
    if(f_closedir(&dir)!=FR_OK)result=-1;
    return result;
}
static int screen_row(void *context,uint32_t y,uint16_t *row,size_t width)
{
    uintptr_t address=(uintptr_t)context;
    if((address!=SDRAM_LVGL_FRAME_ADDR&&address!=SDRAM_LVGL_BACK_FRAME_ADDR)||
       width!=1024U||y>=600U)return 0;
    memcpy(row,(const void *)(address+(uintptr_t)y*2048U),width*sizeof(*row));return 1;
}
void scope_ui_files(unsigned int operation)
{
    scope_ui_t *u=&g_scope_ui;
    const scope_storage_io_t io={NULL,io_open,io_read,io_write,io_close,io_remove,io_next};
    scope_settings_t previous;
    char path[SCOPE_STORAGE_PATH_MAX],message[180];FRESULT r;int ok=0;
    disp_diagnostics_t display;
    if(operation==4U||operation==5U){
        if(u->session.running){scope_ui_status("请先停止采集，接GND/实测参考电压后校准");return;}
        ok=scope_session_calibrate_channel(&u->session,u->settings.measurement_source,
            operation==5U,u->reference_mv);
        if(ok==0){
            scope_accurate_cancel();u->accurate_busy=0U;u->accurate_ready=0U;
            u->settings.calibration=u->session.calibration;
            u->settings.calibration2=u->session.calibration2;
            u->average_frames=0U;u->history_count=0U;scope_ui_rebuild_display(0);
        }
        scope_ui_status(ok==0?"校准完成；请保存设置使校准持久化":"校准失败：无有效记录或参考电压/输入过低");
        return;
    }
    if((operation==0U||operation==1U)&&!u->session.has_record){scope_ui_status("没有真实采集记录可保存");return;}
    if(sdcard_manager_session_acquire(SCOPE_SD_OWNER)!=SDCARD_MANAGER_OK){
        scope_ui_status("SD卡未就绪或资源忙");return;
    }
    r=f_mkdir(SCOPE_DIRECTORY);
    if(r==FR_OK||r==FR_EXIST){
        if(operation==0U)ok=scope_storage_save_csv_pair(&io,SCOPE_DIRECTORY,u->session.raw,u->session.raw2,
           u->session.record_count,(float)u->session.actual_rate_hz,u->session.record_sequence,
           &u->session.record_io,&u->session.calibration,&u->session.calibration2,
           u->settings.measurement_mode,u->session.measurement_sequence,path,sizeof(path));
        if(operation==1U){
            /* 拒绝已有VBlank切换，固定前台地址；同步保存期间不推进LVGL。 */
            disp_get_diagnostics(&display);
            if(display.reload_pending){
                (void)sdcard_manager_session_release(SCOPE_SD_OWNER);
                scope_ui_status("屏幕刷新尚未完成，请稍后保存截图");return;
            }
            disp_disable_update();
            ok=scope_storage_save_bmp(&io,SCOPE_DIRECTORY,1024U,600U,screen_row,
                 (void *)(uintptr_t)disp_get_frame_address(),path,sizeof(path));
            if(display.flush_enabled)disp_enable_update();
        }
        if(operation==2U)ok=scope_storage_save_settings(&io,SCOPE_DIRECTORY,&u->settings,path,sizeof(path));
        if(operation==3U){
            previous=u->settings;
            ok=scope_storage_load_latest_settings(&io,SCOPE_DIRECTORY,&u->settings);
            if(ok){
                /* 配置文件不具备开启准确模式的权限，只有测量页按钮可开启。 */
                u->settings.measurement_mode=0U;
                scope_accurate_cancel();u->accurate_busy=0U;u->accurate_ready=0U;
                /* 恢复设置永不自动启动输出。 */
                (void)scope_session_dac(&u->session,NULL,0);
                if(scope_ui_apply_settings()!=0){u->settings=previous;(void)scope_ui_apply_settings();ok=0;}
            }
        }
    }
    if(file_opened)(void)io_close(NULL);
    if(sdcard_manager_session_release(SCOPE_SD_OWNER)!=SDCARD_MANAGER_OK)ok=0;
    if(ok&&operation!=3U){(void)snprintf(message,sizeof(message),"已保存 %s",path);scope_ui_status(message);}
    else scope_ui_status(ok?"设置/校准已恢复，DAC保持关闭":"文件操作失败；检查SD卡、空间及配置有效性");
}
