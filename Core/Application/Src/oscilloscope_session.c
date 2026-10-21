#include "oscilloscope_session.h"
#include "oscilloscope_zoom.h"
#include "oscilloscope_driver.h"
#include "oscilloscope_workspace.h"
#include <math.h>
#include <string.h>

void scope_session_init(scope_session_t *s)
{
    if(!s)return;
    memset(s,0,sizeof(*s));
    s->config.sample_rate_hz=100000U;
    s->config.mode=SCOPE_MODE_AUTO;
    s->config.level_mv=1650.0f;
    s->config.pre_samples=512U;
    s->config.record_samples=SCOPE_RECORD_LENGTH;
    s->record_count=SCOPE_RECORD_LENGTH;
    s->calibration.vref_mv=3300.0f;
    s->calibration.gain=1.0f;
    s->calibration2=s->calibration;
    s->io=(scope_io_config_t){{1U,0U},{SCOPE_INPUT_PA5,SCOPE_INPUT_PC4},SCOPE_OUTPUT_PA4};
    s->dac_sample_rate_hz=200000U;
}
int scope_session_open(scope_session_t *s)
{
    int r;scope_driver_status_t status;scope_workspace_t *w;
    if(!s)return -1;
    if(!scope_workspace_acquire()){s->last_error=-11;return -11;}
    w=scope_workspace_get();
    s->raw=w->raw;s->mv=w->mv;s->stream=w->stream;
    s->stream_mv=w->stream_mv;s->incoming=w->incoming;
    s->raw2=w->raw2;s->stream2=w->stream2;s->incoming2=w->incoming2;s->mv2=w->mv2;
    r=scope_driver_open(); s->last_error=r;
    scope_driver_get_status(&status);
    s->opened=status.opened;s->dac_enabled=status.dac_enabled;
    if(r!=0&&!s->opened){scope_workspace_release();s->has_record=0U;
        s->raw=NULL;s->mv=NULL;s->stream=NULL;s->stream_mv=NULL;s->incoming=NULL;
        s->raw2=NULL;s->stream2=NULL;s->incoming2=NULL;s->mv2=NULL;}
    return r;
}
int scope_session_configure(scope_session_t *s,const scope_session_config_t *c)
{
    if(!s||!c||c->sample_rate_hz<SCOPE_DRIVER_MIN_RATE_HZ||
       c->sample_rate_hz>scope_io_max_rate(&s->io)||
       (unsigned)c->mode>(unsigned)SCOPE_MODE_SINGLE||
       (unsigned)c->edge>(unsigned)SCOPE_EDGE_FALLING||
       !isfinite(c->level_mv)||fabsf(c->level_mv)>3300000.0f||
       c->record_samples<SCOPE_MIN_RECORD_LENGTH||c->record_samples>SCOPE_MAX_RECORD_LENGTH||
       c->pre_samples>c->record_samples*90U/100U||c->holdoff_ms>60000U||
       c->trigger_source>1U||!isfinite(c->hysteresis_mv)||c->hysteresis_mv<0.0f||
       c->hysteresis_mv>3300000.0f)return -1;
    if(s->running)scope_session_stop(s);
    s->config=*c; return 0;
}
int scope_session_run(scope_session_t *s,uint32_t now)
{
    int r;
    if(!s||!s->opened)return -1;
    if(s->running)scope_session_stop(s);
    r=scope_driver_configure_io(&s->io);if(r){s->last_error=r;return r;}
    r=scope_driver_start_capture(s->config.sample_rate_hz,(uint32_t)s->config.record_samples); s->last_error=r;
    if(r!=0)return r;
    s->stream_count=0U; s->received=0U; s->next_trigger=0U;
    s->source_sequence=0U; s->wait_since_ms=now; s->running=1U;
    return 0;
}
void scope_session_stop(scope_session_t *s)
{
    if(!s)return;
    if(s->opened)scope_driver_stop();
    s->running=0U;
}
void scope_session_close(scope_session_t *s)
{
    scope_driver_status_t status;
    if(!s)return;
    if(s->opened){
        scope_session_stop(s);scope_driver_disable_dac();scope_driver_close();
        scope_driver_get_status(&status);
        s->opened=status.opened;s->dac_enabled=status.dac_enabled;
        if(status.opened)s->last_error=status.last_error?status.last_error:-10;
    }
    if(!s->opened){s->has_record=0U;s->stream_count=0U;
        s->raw=NULL;s->mv=NULL;s->stream=NULL;s->stream_mv=NULL;s->incoming=NULL;
        s->raw2=NULL;s->stream2=NULL;s->incoming2=NULL;s->mv2=NULL;
        scope_workspace_release();}
}
int scope_session_refresh(scope_session_t *s)
{
    if(!s||!s->has_record)return -1;
    if(!scope_convert_adc(s->raw,s->record_count,
        s->record_io.enabled[1]&&!s->record_io.enabled[0]?&s->calibration2:&s->calibration,s->mv))return -1;
    if(s->record_io.enabled[1]&&!scope_convert_adc(s->raw2,s->record_count,&s->calibration2,s->mv2))return -1;
    return 0;
}
int scope_session_measure(scope_session_t *s)
{
    if(!s||!s->has_record)return -1;
    s->measurement_sequence=s->record_sequence;
    return scope_measure(s->mv,s->record_count,(float)s->actual_rate_hz,&s->measurements)?0:-1;
}
static int capture(scope_session_t *s,size_t start,uint32_t sequence,uint32_t now,int triggered)
{
    (void)sequence; /* DMA序号每次启动清零，不能作为会话记录身份。 */
    s->record_count=s->config.record_samples;
    memcpy(s->raw,s->stream+start,s->record_count*sizeof(*s->raw));
    if(s->io.enabled[1])memcpy(s->raw2,s->stream2+start,s->record_count*sizeof(*s->raw2));
    s->record_io=s->io;
    s->actual_rate_hz=s->stream_rate_hz;
    s->has_record=1U;
    s->record_sequence=(s->record_sequence==UINT32_MAX)?1U:s->record_sequence+1U;
    s->triggered=(uint8_t)triggered;
    s->record_trigger_index=triggered?s->config.pre_samples:s->record_count/2U;
    s->wait_since_ms=now;
    if(scope_session_refresh(s)!=0)return -2;
    if(s->config.mode==SCOPE_MODE_SINGLE)scope_session_stop(s);
    return 1;
}
static int capture_error(scope_session_t *s)
{
    scope_session_close(s);s->last_error=-2;return -2;
}
static void hysteresis_trigger(const scope_session_t *s,size_t first,size_t last,scope_trigger_result_t *hit)
{
    size_t i;int armed=0,rising=s->config.edge==SCOPE_EDGE_RISING;
    float level=s->config.level_mv,band=s->config.hysteresis_mv,value;
    hit->found=0;hit->index=0U;hit->start=0U;
    /* 从保留的真实流开始寻找完整回滞过程，再筛选可形成记录的边沿。 */
    for(i=0U;i<=last;i++){
        value=s->stream_mv[i];
        if(rising?value<=level-band:value>=level+band)armed=1;
        if(armed&&(rising?value>=level:value<=level)){
            armed=0;
            if(i>=first&&i>=s->config.pre_samples){
                hit->found=1;hit->index=i;hit->start=i-s->config.pre_samples;return;
            }
        }
    }
}
int scope_session_poll(scope_session_t *s,uint32_t now)
{
    uint32_t rate=0U,sequence=0U;
    uint64_t base,first,max_index,absolute;
    scope_trigger_t trigger;
    scope_trigger_result_t hit;
    size_t skip,pre,block,drop,n;
    int r;
    if(!s||!s->running)return 0;
    block=scope_driver_frame_samples();n=s->config.record_samples;
    if(block==0U||block>SCOPE_MAX_RECORD_LENGTH)return capture_error(s);
    r=scope_driver_read_pair(s->incoming,s->incoming2,SCOPE_MAX_RECORD_LENGTH,&rate,&sequence);
    if(r<=0){if(r<0){s->last_error=r;scope_session_close(s);}return r;}
    if(rate==0U){s->last_error=-2;scope_session_close(s);return -2;}
    if((s->source_sequence && sequence!=s->source_sequence+1U)||
       (s->stream_rate_hz&&s->stream_rate_hz!=rate)){
        s->stream_count=0U;s->received=0U;s->next_trigger=0U;
    }
    s->source_sequence=sequence;s->stream_rate_hz=rate;
    if(s->stream_count+block>n*2U){
        drop=s->stream_count+block-n*2U;
        memmove(s->stream,s->stream+drop,(s->stream_count-drop)*sizeof(*s->stream));
        memmove(s->stream_mv,s->stream_mv+drop,(s->stream_count-drop)*sizeof(*s->stream_mv));
        if(s->io.enabled[1])memmove(s->stream2,s->stream2+drop,(s->stream_count-drop)*sizeof(*s->stream2));
        s->stream_count-=drop;
    }
    memcpy(s->stream+s->stream_count,s->incoming,block*sizeof(*s->incoming));
    if(s->io.enabled[1])memcpy(s->stream2+s->stream_count,s->incoming2,block*sizeof(*s->incoming2));
    if(!s->io.enabled[0])memcpy(s->stream+s->stream_count,s->incoming2,block*sizeof(*s->incoming2));
    {
        unsigned source=s->config.trigger_source;
        if(!s->io.enabled[source])source=s->io.enabled[0]?0U:1U;
        if(!scope_convert_adc(source?s->incoming2:s->incoming,block,
            source?&s->calibration2:&s->calibration,s->stream_mv+s->stream_count))return capture_error(s);
    }
    s->stream_count+=block;s->received+=block;
    base=s->received-s->stream_count;
    if(s->stream_count<n)return 0;
    pre=s->config.pre_samples;
    first=base+pre;
    if(s->next_trigger>first)first=s->next_trigger;
    max_index=s->received-n+pre;
    if(first<=max_index){
        skip=(size_t)(first-base-pre);
        trigger.level_mv=s->config.level_mv;trigger.edge=s->config.edge;
        trigger.pre_samples=pre;trigger.window_samples=n;
        /* 保留候选边沿前一个样本，预触发为0也不能丢弃边沿左邻点。 */
        if(skip>0U){skip--;trigger.pre_samples++;trigger.window_samples++;}
        if(s->config.hysteresis_mv>0.0f){
            hysteresis_trigger(s,(size_t)(first-base),(size_t)(max_index-base),&hit);
            skip=0U;trigger.window_samples=n;r=1;
        }else r=scope_find_trigger(s->stream_mv+skip,s->stream_count-skip,&trigger,&hit);
        if(!r)return capture_error(s);
        if(hit.found){
            absolute=base+skip+hit.index;
            s->next_trigger=absolute+1U+((uint64_t)rate*s->config.holdoff_ms)/1000U;
            r=capture(s,skip+hit.start+(trigger.window_samples-n),sequence,now,1);
            return r<0?capture_error(s):r;
        }
        s->next_trigger=max_index+1U;
    }
    /* Auto即使无边沿也持续扫屏，频率有效性不决定是否显示ADC数据。 */
    if(s->config.mode==SCOPE_MODE_AUTO&&(uint32_t)(now-s->wait_since_ms)>=40U)
    {
        r=capture(s,s->stream_count-n,sequence,now,0);
        return r<0?capture_error(s):r;
    }
    return 0;
}
int scope_session_dac(scope_session_t *s,const scope_dac_config_t *c,int enable)
{
    uint32_t count=32U,cycles=1U,n,k,i,rate=100U,index;
    unsigned harmonic;
    uint32_t minimum_points;
    double error,best=INFINITY;
    static uint16_t basis[SCOPE_DAC_MAX_SAMPLES]; /* 主循环独占，不占用嵌入式栈。 */
    scope_driver_status_t status;
    int r;
    if(!s||!s->opened)return -1;
    if(!enable){
        scope_driver_disable_dac();scope_driver_get_status(&status);
        s->dac_enabled=status.dac_enabled;s->dac_rate_hz=status.dac_rate_hz;
        return status.dac_enabled?-1:0;
    }
    harmonic=scope_dac_highest_harmonic(c);
    if(!harmonic||!isfinite(c->frequency_hz)||c->frequency_hz<1.0f||
       c->frequency_hz>31250.0f/(float)harmonic)return -1;
    minimum_points=32U*harmonic;
    if(c->waveform!=SCOPE_WAVE_DC){
        /* 频率误差优先，同误差选择每周期更多真实DAC点。 */
        for(index=0U;index<SCOPE_RATE_COUNT;index++){
            uint32_t candidate=scope_rate_at(index);
            if(candidate>SCOPE_DRIVER_DAC_MAX_RATE_HZ)break;
            for(n=minimum_points;n<=SCOPE_DAC_MAX_SAMPLES;n++)for(k=1U;k<=n/minimum_points;k++){
                error=fabs((double)candidate*(double)k/(double)n-(double)c->frequency_hz);
                if(error<best-1e-9||(fabs(error-best)<=1e-9&&n*cycles>count*k)){
                    best=error;count=n;cycles=k;rate=candidate;
                }
            }
        }
    }else {count=2U;cycles=0U;}
    if(!scope_make_dac(c,s->calibration.vref_mv,basis,count))return -1;
    for(i=0U;i<count;i++)s->dac_table[i]=basis[cycles?(i*cycles)%count:0U];
    r=scope_driver_set_dac(s->dac_table,count,rate);
    scope_driver_get_status(&status);
    s->dac_enabled=status.dac_enabled;s->dac_rate_hz=status.dac_rate_hz;
    if(r==0){s->dac_count=count;s->dac_cycles=cycles;s->dac_sample_rate_hz=rate;
        s->dac_frequency_hz=(float)status.dac_rate_hz*(float)cycles/(float)count;}
    s->last_error=r;return r;
}
static float raw_mean(const scope_session_t *s)
{
    uint64_t sum=0U;size_t i;
    for(i=0U;i<s->record_count;i++)sum+=s->raw[i];
    return (float)sum/(float)s->record_count;
}
int scope_session_calibrate_zero(scope_session_t *s)
{
    if(!s||!s->has_record||s->running)return -1;
    s->calibration.zero_code=raw_mean(s);return scope_session_refresh(s);
}
int scope_session_calibrate_gain(scope_session_t *s,float reference)
{
    float delta,gain;
    if(!s||!s->has_record||s->running||!isfinite(reference)||reference<=0.0f||
       reference>s->calibration.vref_mv)return -1;
    delta=(raw_mean(s)-s->calibration.zero_code)*s->calibration.vref_mv/65535.0f;
    if(delta<10.0f)return -1;
    gain=reference/delta;
    if(!isfinite(gain)||gain<0.1f||gain>10.0f)return -1;
    s->calibration.gain=gain;return scope_session_refresh(s);
}
int scope_session_calibrate_channel(scope_session_t *s,unsigned channel,int use_gain,float reference)
{
    scope_calibration_t *c;const uint16_t *raw;uint64_t sum=0U;size_t i;float mean,delta,gain;
    if(!s||channel>1U||!s->has_record||s->running||!s->record_io.enabled[channel]||
       !s->record_count)return -1;
    c=channel?&s->calibration2:&s->calibration;raw=channel?s->raw2:s->raw;
    for(i=0U;i<s->record_count;i++)sum+=raw[i];
    mean=(float)sum/(float)s->record_count;
    if(!use_gain)c->zero_code=mean;
    else {
        if(!isfinite(reference)||reference<=0.0f||reference>c->vref_mv)return -1;
        delta=(mean-c->zero_code)*c->vref_mv/65535.0f;
        if(delta<10.0f)return -1;
        gain=reference/delta;if(!isfinite(gain)||gain<0.1f||gain>10.0f)return -1;
        c->gain=gain;
    }
    return scope_session_refresh(s);
}
