#include "oscilloscope_session.h"
#include "oscilloscope_zoom.h"
#include "oscilloscope_driver.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static scope_session_t s;
static uint16_t source[SCOPE_MAX_RECORD_LENGTH];
static uint32_t feed_block=SCOPE_RECORD_LENGTH;
static uint32_t sequence;
static uint32_t feed_rate=100000U;
static int ready, stopped, closed, disabled, start_failure,restore_failure,read_failure,open_failure;
static uint8_t driver_opened;
static uint8_t driver_dac;
static uint32_t driver_dac_rate;
static int dac_failure;
int scope_driver_open(void) { driver_opened=1U;return open_failure?-3:(restore_failure?-10:0); }
int scope_driver_start(uint32_t rate) { (void)rate; return start_failure ? -8 : 0; }
int scope_driver_start_capture(uint32_t rate,uint32_t n) { (void)n;return scope_driver_start(rate); }
uint32_t scope_driver_frame_samples(void) { return feed_block; }
void scope_driver_stop(void) { stopped++; }
void scope_driver_close(void) { closed++;if(!restore_failure)driver_opened=0U; }
void scope_driver_disable_dac(void) { disabled++;driver_dac=0U;driver_dac_rate=0U; }
int scope_driver_set_dac(const uint16_t *p,uint32_t n,uint32_t r)
{ (void)p; (void)n;
  if(dac_failure){driver_dac=0U;driver_dac_rate=0U;return -8;}
  driver_dac=1U;driver_dac_rate=r;return 0; }
void scope_driver_get_status(scope_driver_status_t *status)
{ memset(status,0,sizeof(*status));status->opened=driver_opened;status->last_error=-10;
  status->dac_enabled=driver_dac;status->dac_rate_hz=driver_dac_rate;}
int scope_driver_read(uint16_t *p,uint32_t n,uint32_t *r,uint32_t *q)
{ assert(n>=feed_block); if(read_failure)return -7;if(!ready)return 0; memcpy(p,source,feed_block*sizeof(*source));
  *r=feed_rate; *q=++sequence; ready=0; return 1; }
int scope_driver_configure_io(const scope_io_config_t *c){return scope_io_valid(c)?0:-1;}
int scope_driver_read_pair(uint16_t *p,uint16_t *p2,uint32_t n,uint32_t *r,uint32_t *q)
{int result=scope_driver_read(p,n,r,q);size_t i;if(result>0)for(i=0;i<feed_block;i++)p2[i]=(uint16_t)(65535U-p[i]);return result;}
static void feed(uint32_t now, int edge)
{ size_t i; for(i=0;i<feed_block;i++)source[i]=(edge&&i>=700U)?50000U:10000U;
  ready=1; scope_session_poll(&s,now); }
int main(void)
{
    scope_session_config_t cfg;
    scope_dac_config_t dac={SCOPE_WAVE_SINE,1000.0f,1000.0f,1650.0f,50.0f,{0},{0}};
    uint32_t captured;size_t i;
    scope_session_init(&s); assert(scope_session_open(&s)==0);
    assert(s.dac_sample_rate_hz==200000U);
    s.has_record=1U;s.actual_rate_hz=100000U;s.measurements.mean_mv=123.0f;
    assert(scope_session_refresh(&s)==0 && s.measurements.mean_mv==123.0f);
    assert(scope_session_measure(&s)==0 && s.measurements.mean_mv==0.0f);
    s.has_record=0U;
    cfg=s.config; cfg.mode=SCOPE_MODE_SINGLE; cfg.level_mv=1500.0f;
    assert(scope_session_configure(&s,&cfg)==0);
    assert(scope_session_run(&s,0U)==0);
    feed(10U,0); assert(!s.has_record && s.running);
    feed(1000U,0); assert(!s.has_record && s.running);
    feed(1010U,1); assert(!s.has_record && s.running); /* 等真实后触发样本 */
    feed(1015U,0); assert(s.has_record && !s.running && s.triggered);
    assert(s.raw[511U]<20000U && s.raw[512U]>40000U);
    feed(1020U,0); assert(s.raw[1000U]>40000U); /* 停止后冻结 */
    cfg.mode=SCOPE_MODE_NORMAL; assert(scope_session_configure(&s,&cfg)==0);
    assert(scope_session_run(&s,1100U)==0); feed(2000U,0);
    assert(s.raw[1000U]>40000U && s.running); /* 无触发保留原帧 */
    cfg.mode=SCOPE_MODE_AUTO; assert(scope_session_configure(&s,&cfg)==0);
    assert(scope_session_run(&s,2100U)==0); captured=s.record_sequence;
    feed(2139U,0);assert(s.record_sequence==captured);
    feed(2140U,0);assert(s.record_sequence==captured+1U);
    assert(s.raw[1000U]==10000U && !s.triggered);
    /* 没有触发/有效频率，仍发布真实低幅ADC变化；禁止合成噪声。 */
    for(i=0U;i<feed_block;i++)source[i]=(uint16_t)(10000U+i%7U);
    ready=1;assert(scope_session_poll(&s,2180U)==1);
    assert(s.raw[1000U]==source[1000U]&&!s.triggered&&s.running);
    assert(scope_session_measure(&s)==0&&s.measurements.vpp_mv>0.0f);
    for(i=0U;i<feed_block;i++)source[i]=10000U;
    ready=1;assert(scope_session_poll(&s,2220U)==1);
    assert(scope_session_measure(&s)==0&&!(s.measurements.valid&SCOPE_VALID_FREQUENCY));
    scope_session_stop(&s); assert(!s.running);
    assert(scope_session_calibrate_zero(&s)==0);
    assert(s.calibration.zero_code==10000.0f);
    assert(scope_session_calibrate_gain(&s,1000.0f)<0); /* 相同输入不可校增益 */
    cfg.sample_rate_hz=500000U;assert(scope_session_configure(&s,&cfg)==0);
    assert(s.actual_rate_hz==100000U); /* 改菜单不重解释冻结Fs */
    cfg.mode=SCOPE_MODE_NORMAL;assert(scope_session_configure(&s,&cfg)==0);
    assert(scope_session_run(&s,2450U)==0);feed_rate=500000U;feed(2460U,0);
    assert(s.actual_rate_hz==100000U&&s.raw[1000U]==10000U);
    feed(2470U,1);feed(2480U,0);assert(s.actual_rate_hz==500000U);
    scope_session_stop(&s);feed_rate=100000U;
    for(i=0;i<SCOPE_RECORD_LENGTH;i++)s.raw[i]=30000U;
    assert(scope_session_calibrate_gain(&s,1000.0f)==0);
    assert(fabsf(s.mv[0]-1000.0f)<0.01f);
    assert(scope_session_dac(&s,&dac,1)==0 && s.dac_enabled);
    assert(s.dac_count>=32U*s.dac_cycles&&fabsf(s.dac_frequency_hz-1000.0f)<0.01f);
    dac.frequency_hz=12345.0f;s.dac_sample_rate_hz=1000000U;
    assert(scope_session_dac(&s,&dac,1)==0);
    assert(s.dac_count>=32U*s.dac_cycles && fabsf(s.dac_frequency_hz-12345.0f)<10.0f);
    dac.frequency_hz=31250.0f;assert(scope_session_dac(&s,&dac,1)==0);
    assert(s.dac_rate_hz==1000000U&&s.dac_count>=32U*s.dac_cycles);
    dac.frequency_hz=31251.0f;assert(scope_session_dac(&s,&dac,1)<0);
    dac.frequency_hz=1.0f;s.dac_sample_rate_hz=256000U;
    assert(scope_session_dac(&s,&dac,1)==0&&fabsf(s.dac_frequency_hz-1.0f)<0.0001f);
    for(unsigned test_hz=1U;test_hz<=31250U;test_hz=test_hz*2U+7U){
        dac.frequency_hz=(float)test_hz;
        assert(scope_session_dac(&s,&dac,1)==0);
        assert(s.dac_count>=32U*s.dac_cycles&&s.dac_count<=SCOPE_DAC_MAX_SAMPLES);
        assert(fabsf(s.dac_frequency_hz-dac.frequency_hz)<dac.frequency_hz*0.01f);
        int fixed=0;for(unsigned r=0U;r<SCOPE_RATE_COUNT;r++)if(s.dac_rate_hz==scope_rate_at(r))fixed=1;
        assert(fixed);
    }
    dac.waveform=SCOPE_WAVE_HARMONIC;
    dac.harmonic_percent[0]=100.0f;dac.harmonic_percent[7]=20.0f;
    dac.frequency_hz=1000.0f;
    assert(scope_session_dac(&s,&dac,1)==0);
    assert(fabsf(s.dac_frequency_hz-1000.0f)<0.01f);
    assert(s.dac_count>=256U*s.dac_cycles);
    dac.frequency_hz=3906.25f;assert(scope_session_dac(&s,&dac,1)==0);
    dac.frequency_hz=3907.0f;assert(scope_session_dac(&s,&dac,1)<0);
    dac.waveform=SCOPE_WAVE_SINE;
    dac.frequency_hz=125001.0f;assert(scope_session_dac(&s,&dac,1)<0);
    dac.frequency_hz=1000.0f;
    dac_failure=1;assert(scope_session_dac(&s,&dac,1)==-8&&!s.dac_enabled&&s.dac_rate_hz==0U);
    dac_failure=0;
    dac.amplitude_mv=4000.0f;assert(scope_session_dac(&s,&dac,1)<0);
    assert(scope_session_dac(&s,NULL,0)==0 && !s.dac_enabled);
    s.calibration.zero_code=0.0f;s.calibration.gain=1.0f;
    cfg.mode=SCOPE_MODE_NORMAL;cfg.holdoff_ms=50U;cfg.sample_rate_hz=100000U;
    assert(scope_session_configure(&s,&cfg)==0);assert(scope_session_run(&s,2500U)==0);
    feed(2510U,1);feed(2520U,0);assert(s.triggered);
    captured=s.record_sequence;
    feed(2530U,1);feed(2540U,0);assert(s.record_sequence==captured); /* 5000点释抑 */
    feed(2550U,1);feed(2560U,0);assert(s.record_sequence>captured);
    captured=s.record_sequence;
    sequence+=4U;feed(2570U,0);assert(s.record_sequence==captured); /* 丢块不拼虚假边沿 */
    scope_session_stop(&s);
    /* 模拟驱动重启后DMA序号归零，冻结记录身份仍必须单调前进。 */
    captured=s.record_sequence;sequence=0U;
    cfg.mode=SCOPE_MODE_AUTO;cfg.holdoff_ms=0U;
    assert(scope_session_configure(&s,&cfg)==0);
    assert(scope_session_run(&s,2600U)==0);feed(2900U,0);
    assert(s.record_sequence>captured && s.source_sequence==1U);
    scope_session_stop(&s);
    start_failure=1; assert(scope_session_run(&s,3000U)<0 && !s.running);
    start_failure=0;assert(scope_session_run(&s,3010U)==0);
    read_failure=1;assert(scope_session_poll(&s,3020U)==-7 && !s.opened && !s.running);
    read_failure=0;assert(scope_session_open(&s)==0);
    restore_failure=1;scope_session_close(&s);assert(s.opened && s.last_error==-10);
    assert(scope_session_open(&s)==-10);restore_failure=0;
    scope_session_close(&s); assert(closed && disabled && stopped && !s.opened);
    open_failure=1;assert(scope_session_open(&s)==-3&&s.opened);
    scope_session_close(&s);assert(!s.opened&&!driver_opened);
    open_failure=0;assert(scope_session_open(&s)==0);
    cfg=s.config;cfg.mode=SCOPE_MODE_AUTO;cfg.record_samples=8192;cfg.pre_samples=2000;
    assert(scope_session_configure(&s,&cfg)==0);feed_block=2048;
    assert(scope_session_run(&s,4000)==0);
    feed(4300,0);feed(4301,0);feed(4302,0);assert(!s.has_record);
    feed(4303,0);assert(s.has_record&&s.record_count==8192&&s.raw[8191]==10000);
    scope_session_stop(&s);cfg.record_samples=64;cfg.pre_samples=16;
    assert(scope_session_configure(&s,&cfg)==0);
    assert(s.record_count==8192); /* 新配置不能改变旧冻结记录 */
    feed_block=16;assert(scope_session_run(&s,5000)==0);
    feed(5300,0);feed(5301,0);feed(5302,0);assert(s.record_count==8192);
    feed(5303,0);assert(s.record_count==64&&s.raw[63]==10000);
    /* 较小分块兼容：65点记录在两块64点之间保留待后窗边沿。 */
    scope_session_stop(&s);cfg.record_samples=65;cfg.pre_samples=20;cfg.mode=SCOPE_MODE_SINGLE;
    assert(scope_session_configure(&s,&cfg)==0);feed_block=64;
    assert(scope_session_run(&s,6000)==0);
    for(i=0;i<64;i++)source[i]=i<40?10000:50000;
    ready=1;assert(scope_session_poll(&s,6010)==0&&s.running);
    for(i=0;i<64;i++)source[i]=50000;
    ready=1;assert(scope_session_poll(&s,6020)==1&&!s.running);
    assert(s.record_count==65&&s.raw[19]==10000&&s.raw[20]==50000&&s.raw[64]==50000);
    scope_session_close(&s);assert(!s.has_record&&s.raw==NULL);
    scope_session_init(&s);assert(scope_session_open(&s)==0);
    s.io.enabled[1]=1U;cfg=s.config;cfg.mode=SCOPE_MODE_AUTO;cfg.record_samples=64U;cfg.pre_samples=16U;
    assert(scope_session_configure(&s,&cfg)==0);feed_block=16U;
    assert(scope_session_run(&s,7000U)==0);
    feed(7040U,0);feed(7041U,0);feed(7042U,0);feed(7043U,0);
    assert(s.has_record&&s.record_count==64U);
    for(i=0;i<64U;i++)assert(s.raw2[i]==65535U-s.raw[i]);
    assert(s.mv2[0]>s.mv[0]&&s.record_io.enabled[1]);
    assert(scope_session_calibrate_channel(&s,1U,0,0.0f)<0); /* 采集中禁止校准。 */
    scope_session_stop(&s);s.io.enabled[0]=0U;
    {float old_zero=s.calibration.zero_code;
     assert(scope_session_calibrate_channel(&s,1U,0,0.0f)==0);
     assert(s.calibration2.zero_code>0.0f&&s.calibration.zero_code==old_zero);
     s.calibration2.zero_code=0.0f;s.calibration2.gain=1.25f;}
    assert(scope_session_run(&s,7100U)==0);
    feed(7140U,0);feed(7141U,0);feed(7142U,0);feed(7143U,0);
    assert(!s.record_io.enabled[0]&&s.record_io.enabled[1]);
    assert(fabsf(s.mv[0]-s.mv2[0])<0.01f);
    assert(scope_session_calibrate_channel(&s,2U,0,0.0f)<0);
    scope_session_stop(&s);
    /* CH1为下降沿、CH2为上升沿；以CH2触发仍保留真实CH1记录。 */
    s.io.enabled[0]=1U;s.calibration.zero_code=0.0f;s.calibration.gain=1.0f;
    s.calibration2=s.calibration;cfg=s.config;cfg.mode=SCOPE_MODE_NORMAL;
    cfg.trigger_source=1U;cfg.hysteresis_mv=100.0f;cfg.level_mv=1650.0f;
    cfg.record_samples=64U;cfg.pre_samples=16U;feed_block=64U;
    assert(scope_session_configure(&s,&cfg)==0);assert(scope_session_run(&s,7200U)==0);
    for(i=0U;i<64U;i++)source[i]=i<20U?50000U:10000U;
    ready=1;assert(scope_session_poll(&s,7201U)==0);
    ready=1;assert(scope_session_poll(&s,7202U)==1&&s.triggered);
    assert(s.raw[15]>40000U&&s.raw[16]<20000U);
    assert(s.raw2[15]<20000U&&s.raw2[16]>40000U);
    /* 只在阈值附近抖动不能重复触发；必须先越过回滞下限。 */
    captured=s.record_sequence;
    for(i=0U;i<64U;i++)source[i]=(uint16_t)(32768U+(i%2U?20:-20));
    ready=1;(void)scope_session_poll(&s,7203U);
    ready=1;(void)scope_session_poll(&s,7204U);captured=s.record_sequence;
    ready=1;assert(scope_session_poll(&s,7205U)==0&&s.record_sequence==captured);
    /* 40ms界面轮询持续跳过DMA块，500点双路记录仍可从单个完整块更新。 */
    scope_session_stop(&s);cfg.mode=SCOPE_MODE_AUTO;cfg.record_samples=500U;
    cfg.pre_samples=100U;cfg.hysteresis_mv=0.0f;feed_block=992U;
    assert(scope_session_configure(&s,&cfg)==0);
    assert(scope_session_run(&s,7300U)==0);
    for(i=0U;i<992U;i++)source[i]=(uint16_t)(10000U+i);
    for(i=0U;i<4U;i++){
        captured=s.record_sequence;sequence+=7U;ready=1;
        assert(scope_session_poll(&s,7340U+(uint32_t)i*40U)==1);
        assert(s.running&&s.record_count==500U&&s.record_sequence!=captured);
        assert(s.raw[0]==10492U&&s.raw[499]==10991U);
        assert(s.raw2[0]==65535U-s.raw[0]&&s.raw2[499]==65535U-s.raw[499]);
    }
    /* 缩放所有64～500点边界：每次跳7块，完整半块仍连续发布双路。 */
    for(size_t length=64U;length<=8192U;length+=(length<512U?1U:16U)){
        uint32_t now=8000U+(uint32_t)length*100U;
        scope_session_stop(&s);cfg.record_samples=length;cfg.pre_samples=length/5U;
        feed_block=(uint32_t)((length+15U)&~15U);
        assert(scope_session_configure(&s,&cfg)==0);
        assert(scope_session_run(&s,now)==0);
        for(i=0U;i<feed_block;i++)source[i]=(uint16_t)(20000U+i);
        for(unsigned repeat=1U;repeat<=2U;repeat++){
            captured=s.record_sequence;sequence+=7U;ready=1;
            assert(scope_session_poll(&s,now+repeat*43U)==1);
            assert(s.running&&s.record_sequence>captured&&s.record_count==length);
            assert(s.raw[0]==20000U+feed_block-length&&s.raw[length-1U]==20000U+feed_block-1U);
            assert(s.raw2[0]==65535U-s.raw[0]&&s.raw2[length-1U]==65535U-s.raw[length-1U]);
        }
    }
    scope_session_close(&s);
    puts("PASS oscilloscope_session"); return 0;
}
