#include "oscilloscope_storage.h"
#include "oscilloscope_driver.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define SETTINGS_BYTES 304u
#define SETTINGS_V4_BYTES 208u
#define SETTINGS_V3_BYTES 148u
#define SETTINGS_V2_BYTES 144u
#define SETTINGS_V1_BYTES 136u
#define MAX_FILES 10000u
/* 主循环独占；避免在 Cortex-M 栈上分配截图整行。 */
static uint16_t rgb_row[SCOPE_BMP_MAX_WIDTH];
static unsigned char bgr_row[SCOPE_BMP_MAX_WIDTH * 3u + 3u];

static int range(float v, float lo, float hi)
{ return isfinite(v) && v >= lo && v <= hi; }

void scope_settings_defaults(scope_settings_t *s)
{
    if (s == NULL) return;
    memset(s, 0, sizeof(*s));
    s->sample_rate_hz = 100000.0f;
    s->trigger.level_mv = 1650.0f; s->trigger.pre_samples = 128;
    s->trigger.window_samples = SCOPE_RECORD_LENGTH;
    s->display.probe_factor = 1.0f;
    s->calibration.vref_mv = 3300.0f; s->calibration.gain = 1.0f;
    s->dac.frequency_hz = 1000.0f; s->dac.amplitude_mv = 1000.0f;
    s->dac.offset_mv = 1650.0f; s->dac.duty_percent = 50.0f;
    s->dac.harmonic_percent[0]=100.0f;
    s->time_per_div_s = 0.0001f; s->mv_per_div = 500.0f;
    s->average_count = 4; s->grid_enabled = 1;
    s->cursor_x[1] = 1024.0f; s->cursor_mv[1] = 3300.0f;
    s->dac_sample_rate_hz=200000U;
    s->io=(scope_io_config_t){{1U,0U},{SCOPE_INPUT_PA5,SCOPE_INPUT_PC4},SCOPE_OUTPUT_PA4};
    s->display2.probe_factor=1.0f;
    s->calibration2=s->calibration;
    s->mv_per_div2=500.0f;
    s->measurement_source=0U;s->fft_source=0U;
    s->reading_item[0]=0U;s->reading_item[1]=2U;
    s->reading_item[2]=3U;s->reading_item[3]=6U;
}

int scope_settings_valid(const scope_settings_t *s)
{
    const scope_dac_config_t *d;
    size_t i;unsigned harmonic=1U;
    if (s == NULL || sizeof(float) != 4u) return 0;
    for(i=0U;i<4U;++i)if(s->reading_item[i]>=10U||s->reading_source[i]>1U)return 0;
    d = &s->dac;
    for(i=0U;i<SCOPE_DAC_HARMONICS;++i){
        if(!range(d->harmonic_percent[i],0.0f,100.0f)||
           !range(d->harmonic_phase_deg[i],-360.0f,360.0f))return 0;
        if(d->waveform==SCOPE_WAVE_HARMONIC&&d->harmonic_percent[i]>0.0f)harmonic=(unsigned)i+1U;
    }
    if (s->render_mode>1U || s->measurement_mode>1U || s->dac_sample_rate_hz<100U ||
        s->dac_sample_rate_hz>1000000U ||
        !scope_io_valid(&s->io) ||
        !range(s->sample_rate_hz,(float)SCOPE_DRIVER_MIN_RATE_HZ,(float)scope_io_max_rate(&s->io)) ||
        s->measurement_source>1U || s->fft_source>2U || s->trigger_mode > 2u ||
        !range(s->trigger.level_mv, -1.0e9f, 1.0e9f) ||
        (s->trigger.edge != SCOPE_EDGE_RISING && s->trigger.edge != SCOPE_EDGE_FALLING) ||
        s->trigger.window_samples < SCOPE_MIN_RECORD_LENGTH || s->trigger.window_samples > SCOPE_MAX_RECORD_LENGTH ||
        s->trigger.pre_samples > s->trigger.window_samples*90U/100U || s->holdoff_ms > 60000u ||
        (unsigned)s->display.coupling > (unsigned)SCOPE_COUPLING_GND ||
        !range(s->display.probe_factor, 0.001f, 1000.0f) ||
        (s->display.inverted != 0 && s->display.inverted != 1) ||
        !range(s->calibration.vref_mv, 100.0f, 3600.0f) ||
        !range(s->calibration.zero_code, 0, 65535) ||
        !range(s->calibration.gain, 0.001f, 1000) ||
        (unsigned)s->display2.coupling>(unsigned)SCOPE_COUPLING_GND ||
        !range(s->display2.probe_factor,0.001f,1000.0f) ||
        (s->display2.inverted!=0&&s->display2.inverted!=1) ||
        !range(s->calibration2.vref_mv,100.0f,3600.0f) ||
        !range(s->calibration2.zero_code,0.0f,65535.0f) ||
        !range(s->calibration2.gain,0.001f,1000.0f) ||
        (unsigned)d->waveform > (unsigned)SCOPE_WAVE_HARMONIC ||
        !range(d->frequency_hz, 1.0f, 31250.0f/(float)harmonic) ||
        !range(d->amplitude_mv, 0, s->calibration.vref_mv) ||
        !range(d->offset_mv, 0, s->calibration.vref_mv) ||
        !range(d->duty_percent, 0.01f, 99.99f) ||
        (double)d->offset_mv - (double)d->amplitude_mv * 0.5 < 0 ||
        (double)d->offset_mv + (double)d->amplitude_mv * 0.5 > s->calibration.vref_mv ||
        (d->waveform == SCOPE_WAVE_DC && d->amplitude_mv != 0) ||
        !range(s->time_per_div_s, 1.0e-9f, 10) ||
        !range(s->mv_per_div, 1.0e-6f, 1.0e9f) ||
        !range(s->mv_per_div2,1.0e-6f,1.0e9f) ||
        !range(s->vertical_offset_mv, -1.0e9f, 1.0e9f) ||
        !range(s->vertical_offset2_mv,-1.0e9f,1.0e9f) ||
        s->acquisition_mode > 2 || s->average_count < 1 || s->average_count > 64 ||
        s->grid_enabled > 1 || s->persistence > 32 || s->cursor_enabled > 2 ||
        !range(s->cursor_x[0], 0, SCOPE_MAX_RECORD_LENGTH - 1) ||
        !range(s->cursor_x[1], 0, SCOPE_MAX_RECORD_LENGTH - 1) ||
        !range(s->cursor_mv[0], -1.0e9f, 1.0e9f) ||
        !range(s->cursor_mv[1], -1.0e9f, 1.0e9f)) return 0;
    return 1;
}

static void put32(unsigned char *p, uint32_t n)
{ p[0]=(unsigned char)n; p[1]=(unsigned char)(n>>8); p[2]=(unsigned char)(n>>16); p[3]=(unsigned char)(n>>24); }
static uint32_t get32(const unsigned char *p)
{ return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24); }
static uint32_t float_bits(float f) { uint32_t n; memcpy(&n, &f, 4); return n; }
static float bits_float(uint32_t n) { float f; memcpy(&f, &n, 4); return f; }
static uint32_t crc(const unsigned char *p, size_t n, uint32_t c)
{
    size_t i; unsigned bit;
    for (i=0; i<n; ++i) {
        c ^= p[i];
        for (bit=0; bit<8; ++bit) c=(c>>1) ^ (0xedb88320u & (0u-(c&1u)));
    }
    return c;
}
static uint32_t settings_crc(const unsigned char *b,size_t bytes)
{ return ~crc(b+16, bytes-16, crc(b, 12, 0xffffffffu)); }

/* 不写原生结构体，避免枚举大小、size_t、填充与主机ABI影响文件格式。 */
static void encode(unsigned char *b, const scope_settings_t *s)
{
    size_t i=16;
#define U(v) do { put32(b+i,(uint32_t)(v)); i+=4; } while(0)
#define F(v) U(float_bits(v))
    memcpy(b,"SCFG",4); put32(b+4,6); put32(b+8,SETTINGS_BYTES-16);
    F(s->sample_rate_hz); U(s->trigger_mode); F(s->trigger.level_mv); U(s->trigger.edge);
    U(s->trigger.pre_samples); U(s->trigger.window_samples); U(s->holdoff_ms);
    U(s->display.coupling); F(s->display.probe_factor); U(s->display.inverted);
    F(s->calibration.vref_mv); F(s->calibration.zero_code); F(s->calibration.gain);
    U(s->dac.waveform); F(s->dac.frequency_hz); F(s->dac.amplitude_mv);
    F(s->dac.offset_mv); F(s->dac.duty_percent); F(s->time_per_div_s);
    F(s->mv_per_div); F(s->vertical_offset_mv); U(s->acquisition_mode);
    U(s->average_count); U(s->grid_enabled); U(s->persistence); U(s->cursor_enabled);
    F(s->cursor_x[0]); F(s->cursor_x[1]); F(s->cursor_mv[0]); F(s->cursor_mv[1]);
    U(0U); U(s->dac_sample_rate_hz); U(s->render_mode); /* 准确模式只能由测量页主动开启。 */
    U(s->io.enabled[0]);U(s->io.enabled[1]);U(s->io.input[0]);U(s->io.input[1]);U(s->io.output);
    U(s->display2.coupling);F(s->display2.probe_factor);U(s->display2.inverted);
    F(s->calibration2.vref_mv);F(s->calibration2.zero_code);F(s->calibration2.gain);
    F(s->mv_per_div2);F(s->vertical_offset2_mv);U(s->measurement_source);U(s->fft_source);
    {size_t card;for(card=0U;card<4U;++card){U(s->reading_item[card]);U(s->reading_source[card]);}}
    {size_t h;for(h=0U;h<SCOPE_DAC_HARMONICS;++h){F(s->dac.harmonic_percent[h]);F(s->dac.harmonic_phase_deg[h]);}}
#undef F
#undef U
    put32(b+12,settings_crc(b,SETTINGS_BYTES));
}
static void decode(const unsigned char *b, scope_settings_t *s)
{
    size_t i=16;
#define U() (i+=4,get32(b+i-4))
#define F() bits_float(U())
    scope_settings_defaults(s);
    s->sample_rate_hz=F(); s->trigger_mode=U(); s->trigger.level_mv=F(); s->trigger.edge=(scope_edge_t)U();
    s->trigger.pre_samples=U(); s->trigger.window_samples=U(); s->holdoff_ms=U();
    s->display.coupling=(scope_coupling_t)U(); s->display.probe_factor=F(); s->display.inverted=(int)U();
    s->calibration.vref_mv=F(); s->calibration.zero_code=F(); s->calibration.gain=F();
    s->dac.waveform=(scope_waveform_t)U(); s->dac.frequency_hz=F(); s->dac.amplitude_mv=F();
    s->dac.offset_mv=F(); s->dac.duty_percent=F(); s->time_per_div_s=F();
    s->mv_per_div=F(); s->vertical_offset_mv=F(); s->acquisition_mode=U();
    s->average_count=U(); s->grid_enabled=U(); s->persistence=U(); s->cursor_enabled=U();
    s->cursor_x[0]=F(); s->cursor_x[1]=F(); s->cursor_mv[0]=F(); s->cursor_mv[1]=F();
    if(get32(b+4)>=2U){s->measurement_mode=U();s->dac_sample_rate_hz=U();}
    else {s->measurement_mode=0U;s->dac_sample_rate_hz=200000U;}
    if(get32(b+4)>=3U)s->render_mode=U();
    if(get32(b+4)>=4U){
        s->io.enabled[0]=U();s->io.enabled[1]=U();s->io.input[0]=U();s->io.input[1]=U();s->io.output=U();
        /* v4曾允许PA5输出；新固件仅PA4，恢复配置不自动开启DAC。 */
        if(s->io.output==SCOPE_OUTPUT_PA5)s->io.output=SCOPE_OUTPUT_PA4;
        s->display2.coupling=(scope_coupling_t)U();s->display2.probe_factor=F();s->display2.inverted=(int)U();
        s->calibration2.vref_mv=F();s->calibration2.zero_code=F();s->calibration2.gain=F();
        s->mv_per_div2=F();s->vertical_offset2_mv=F();s->measurement_source=U();s->fft_source=U();
    }
    {size_t card;for(card=0U;card<4U;++card){
        if(get32(b+4)>=5U){s->reading_item[card]=U();s->reading_source[card]=U();}
        else s->reading_source[card]=s->measurement_source;
    }}
    if(get32(b+4)>=5U){size_t h;for(h=0U;h<SCOPE_DAC_HARMONICS;++h){
        s->dac.harmonic_percent[h]=F();s->dac.harmonic_phase_deg[h]=F();
    }}
#undef F
#undef U
}

static int join_path(char *path, const char *dir, const char *name)
{
    int n;
    if (dir==NULL || dir[0]=='\0') return 0;
    n=snprintf(path,SCOPE_STORAGE_PATH_MAX,"%s/%s",dir,name);
    return n>0 && (size_t)n<SCOPE_STORAGE_PATH_MAX;
}
static int setting_number(const char *name, uint32_t *number)
{
    size_t j; uint32_t n=0;
    if (strlen(name)!=16 || memcmp(name,"SET_",4) || strcmp(name+12,".CFG")) return 0;
    for (j=4;j<12;++j) {
        if (name[j]<'0' || name[j]>'9') return 0;
        n=n*10u+(uint32_t)(name[j]-'0');
    }
    *number=n; return 1;
}
static int next_setting_number(const scope_storage_io_t *io, const char *dir, uint32_t *next)
{
    size_t i; char name[32]; uint32_t n; int r;
    *next=0;
    if (!io->next_file || !dir) return 0;
    for (i=0;i<MAX_FILES;++i) {
        memset(name,0,sizeof(name)); r=io->next_file(io->context,dir,i,name,sizeof(name));
        if (r==0) return 1;
        if (r<0) return 0;
        if (name[sizeof(name)-1]!=0 || !setting_number(name,&n)) continue;
        if (n==99999999u) return 0;
        if (n>=*next) *next=n+1u;
    }
    return 0;
}
static int create(const scope_storage_io_t *io, const char *dir, const char *prefix,
                  const char *ext, char *work, char *path, size_t capacity)
{
    uint32_t n,start=0,attempt; char name[32]; int r;
    if (io==NULL || !io->open || !io->write || !io->close || !io->remove || !path) return 0;
    if (strcmp(prefix,"SET")==0 && !next_setting_number(io,dir,&start)) return 0;
    for (attempt=0,n=start; attempt<MAX_FILES && n<=99999999u; ++attempt,++n) {
        (void)snprintf(name,sizeof(name),"%s_%08lu.%s",prefix,(unsigned long)n,ext);
        if (!join_path(work,dir,name) || strlen(work)>=capacity) return 0;
        r=io->open(io->context,work,SCOPE_FILE_CREATE_NEW);
        if (r==SCOPE_IO_OK) return 1;
        if (r!=SCOPE_IO_EXISTS) return 0;
    }
    return 0;
}
static int finish(const scope_storage_io_t *io, const char *work, char *path, int ok)
{
    int closed=io->close(io->context);
    if (!ok || !closed) { (void)io->remove(io->context,work); return 0; }
    memcpy(path,work,strlen(work)+1U); return 1;
}
static int write_bytes(const scope_storage_io_t *io, const void *p, size_t n)
{ return io->write(io->context,p,n)==n; }
int scope_storage_save_settings(const scope_storage_io_t *io, const char *dir,
                                const scope_settings_t *s, char *path, size_t capacity)
{
    unsigned char b[SETTINGS_BYTES]; char work[SCOPE_STORAGE_PATH_MAX];
    if (!scope_settings_valid(s) || !create(io,dir,"SET","CFG",work,path,capacity)) return 0;
    encode(b,s); return finish(io,work,path,write_bytes(io,b,sizeof(b)));
}
int scope_storage_load_settings(const scope_storage_io_t *io, const char *path, scope_settings_t *s)
{
    unsigned char b[SETTINGS_BYTES], extra; scope_settings_t next; int ok, closed;
    size_t bytes;
    if (!io || !io->open || !io->read || !io->close || !path || !s || sizeof(float)!=4) return 0;
    if (io->open(io->context,path,SCOPE_FILE_READ)!=SCOPE_IO_OK) return 0;
    ok=io->read(io->context,b,16U)==16U;
    bytes=ok&&get32(b+4)==1U?SETTINGS_V1_BYTES:
          (ok&&get32(b+4)==2U?SETTINGS_V2_BYTES:
          (ok&&get32(b+4)==3U?SETTINGS_V3_BYTES:
          (ok&&get32(b+4)==4U?SETTINGS_V4_BYTES:SETTINGS_BYTES)));
    if(ok)ok=io->read(io->context,b+16,bytes-16)==bytes-16;
    if (ok) ok=io->read(io->context,&extra,1)==0;
    closed=io->close(io->context);
    if (!ok || !closed || memcmp(b,"SCFG",4) || get32(b+4)<1U || get32(b+4)>6U ||
        get32(b+8)!=bytes-16 || get32(b+12)!=settings_crc(b,bytes)) return 0;
    /* 在转换至可能采用小尺寸的 ARMCC 枚举前检查原始32位值，禁止截断绕过校验。 */
    if (get32(b+28)>1 || get32(b+44)>2 || get32(b+52)>1 ||
        get32(b+68)>(get32(b+4)>=5U?(uint32_t)SCOPE_WAVE_HARMONIC:4U)) return 0;
    if(get32(b+4)>=4U&&(get32(b+168)>2U||get32(b+176)>1U))return 0;
    if(get32(b+116)>(get32(b+4)>=6U?2U:1U))return 0;
    decode(b,&next);
    /* 旧文件曾允许低质量高频和手填点速率；限制频率但绝不自动开启输出。 */
    if(!range(next.dac.frequency_hz,0.001f,1000000.0f))return 0;
    if(get32(b+4)<5U)next.dac.frequency_hz=fmaxf(1.0f,fminf(31250.0f,next.dac.frequency_hz));
    if(next.dac_sample_rate_hz<100U||next.dac_sample_rate_hz>1000000U)return 0;
    next.dac_sample_rate_hz=200000U; /* 启用时按频率重新选择。 */
    if(get32(b+4)<3U){
        if(next.trigger.window_samples<2U||next.trigger.window_samples>SCOPE_RECORD_LENGTH||
           next.trigger.pre_samples>=next.trigger.window_samples)return 0;
        next.trigger.window_samples=SCOPE_RECORD_LENGTH;
    }
    if (!scope_settings_valid(&next)) return 0;
    *s=next; return 1;
}
int scope_storage_load_latest_settings(const scope_storage_io_t *io, const char *dir, scope_settings_t *s)
{
    size_t i; char name[32], path[SCOPE_STORAGE_PATH_MAX]; uint32_t seq,best=0;
    scope_settings_t next, chosen; int found=0,r;
    if (!io || !io->next_file || !dir || !s) return 0;
    for (i=0;i<MAX_FILES;++i) {
        memset(name,0,sizeof(name)); r=io->next_file(io->context,dir,i,name,sizeof(name));
        if (r<0) return 0;
        if (r==0) { if (found) *s=chosen; return found; }
        if (name[sizeof(name)-1]!=0 || !setting_number(name,&seq) ||
            (found && seq<=best) || !join_path(path,dir,name)) continue;
        if (scope_storage_load_settings(io,path,&next)) { chosen=next; best=seq; found=1; }
    }
    return 0; /* 超限或扫描失败，不把不完整扫描的候选作为最新值。 */
}
int scope_storage_save_csv(const scope_storage_io_t *io, const char *dir, const uint16_t *raw,
                           size_t count, float rate, uint32_t sequence, const scope_calibration_t *c,
                           char *path, size_t capacity)
{
    return scope_storage_save_csv_mode(io,dir,raw,count,rate,sequence,c,0U,sequence,path,capacity);
}
int scope_storage_save_csv_mode(const scope_storage_io_t *io,const char *dir,const uint16_t *raw,
                           size_t count,float rate,uint32_t sequence,const scope_calibration_t *c,
                           uint32_t mode,uint32_t measurement_sequence,char *path,size_t capacity)
{
    char work[SCOPE_STORAGE_PATH_MAX], line[384]; size_t i; int n,ok=1; double mv;
    if (mode>1U || !raw || count<SCOPE_MIN_RECORD_LENGTH || count>SCOPE_MAX_RECORD_LENGTH || !range(rate,1,10000000) || !c ||
        !range(c->vref_mv,100,3600) || !range(c->zero_code,0,65535) || !range(c->gain,0.001f,1000)) return 0;
    if (!create(io,dir,"CAP","CSV",work,path,capacity)) return 0;
    n=snprintf(line,sizeof(line),"# sample_rate_hz=%.9g,sequence=%lu,count=%lu,vref_mv=%.9g,zero_code=%.9g,gain=%.9g,measurement_mode=%s,measurement_sequence=%lu\r\nindex,time_s,raw,mV\r\n",
               (double)rate,(unsigned long)sequence,(unsigned long)count,(double)c->vref_mv,(double)c->zero_code,(double)c->gain,
               mode?"accurate":"smooth",(unsigned long)measurement_sequence);
    ok=n>0 && (size_t)n<sizeof(line) && write_bytes(io,line,(size_t)n);
    for (i=0;ok && i<count;++i) {
        mv=((double)raw[i]-c->zero_code)*c->vref_mv/65535.0*c->gain;
        n=snprintf(line,sizeof(line),"%lu,%.9g,%u,%.9g\r\n",(unsigned long)i,(double)i/rate,(unsigned)raw[i],mv);
        ok=n>0 && (size_t)n<sizeof(line) && write_bytes(io,line,(size_t)n);
    }
    return finish(io,work,path,ok);
}
int scope_storage_save_csv_pair(const scope_storage_io_t *io,const char *dir,
                           const uint16_t *raw1,const uint16_t *raw2,size_t count,
                           float rate,uint32_t sequence,const scope_io_config_t *channels,
                           const scope_calibration_t *c1,const scope_calibration_t *c2,
                           uint32_t mode,uint32_t measurement_sequence,char *path,size_t capacity)
{
    char work[SCOPE_STORAGE_PATH_MAX],line[384];size_t i;int n,ok=1;double mv1,mv2;
    if(mode>1U||!scope_io_valid(channels)||
       (channels->enabled[0]&&!raw1)||(channels->enabled[1]&&!raw2)||
       count<SCOPE_MIN_RECORD_LENGTH||count>SCOPE_MAX_RECORD_LENGTH||!range(rate,1,10000000)||
       (channels->enabled[0]&&(!c1||!range(c1->vref_mv,100,3600)||
          !range(c1->zero_code,0,65535)||!range(c1->gain,0.001f,1000)))||
       (channels->enabled[1]&&(!c2||!range(c2->vref_mv,100,3600)||
          !range(c2->zero_code,0,65535)||!range(c2->gain,0.001f,1000))))return 0;
    if(!create(io,dir,"CAP","CSV",work,path,capacity))return 0;
    n=snprintf(line,sizeof(line),"# sample_rate_hz=%.9g,sequence=%lu,count=%lu,CH1_pin=%s,CH2_pin=%s,measurement_mode=%s,measurement_sequence=%lu\r\nindex,time_s,CH1_raw,CH1_mV,CH2_raw,CH2_mV\r\n",
        (double)rate,(unsigned long)sequence,(unsigned long)count,
        channels->enabled[0]?(channels->input[0]==SCOPE_INPUT_PA5?"PA5":"PC4"):"off",
        channels->enabled[1]?(channels->input[1]==SCOPE_INPUT_PA5?"PA5":"PC4"):"off",
        mode?"accurate":"smooth",(unsigned long)measurement_sequence);
    ok=n>0&&(size_t)n<sizeof(line)&&write_bytes(io,line,(size_t)n);
    for(i=0;ok&&i<count;i++){
        mv1=channels->enabled[0]?((double)raw1[i]-c1->zero_code)*c1->vref_mv/65535.0*c1->gain:0.0;
        mv2=channels->enabled[1]?((double)raw2[i]-c2->zero_code)*c2->vref_mv/65535.0*c2->gain:0.0;
        if(channels->enabled[0]&&channels->enabled[1])
            n=snprintf(line,sizeof(line),"%lu,%.9g,%u,%.9g,%u,%.9g\r\n",(unsigned long)i,(double)i/rate,
                (unsigned)raw1[i],mv1,(unsigned)raw2[i],mv2);
        else if(channels->enabled[0])
            n=snprintf(line,sizeof(line),"%lu,%.9g,%u,%.9g,,\r\n",(unsigned long)i,(double)i/rate,(unsigned)raw1[i],mv1);
        else
            n=snprintf(line,sizeof(line),"%lu,%.9g,,,%u,%.9g\r\n",(unsigned long)i,(double)i/rate,(unsigned)raw2[i],mv2);
        ok=n>0&&(size_t)n<sizeof(line)&&write_bytes(io,line,(size_t)n);
    }
    return finish(io,work,path,ok);
}
int scope_storage_save_bmp(const scope_storage_io_t *io, const char *dir, uint32_t width,
                           uint32_t height, scope_rgb565_row_fn read_row, void *ctx,
                           char *path, size_t capacity)
{
    char work[SCOPE_STORAGE_PATH_MAX]; unsigned char header[54]; uint32_t stride,y,x; int ok=1;
    if (!width || width>SCOPE_BMP_MAX_WIDTH || !height || height>16384 || !read_row) return 0;
    stride=(width*3u+3u)&~3u;
    if (!create(io,dir,"IMG","BMP",work,path,capacity)) return 0;
    memset(header,0,sizeof(header)); header[0]='B'; header[1]='M';
    put32(header+2,54u+stride*height); put32(header+10,54); put32(header+14,40);
    put32(header+18,width); put32(header+22,height); header[26]=1; header[28]=24;
    put32(header+34,stride*height); ok=write_bytes(io,header,sizeof(header));
    for (y=height;ok && y>0;--y) {
        if (!read_row(ctx,y-1,rgb_row,width)) { ok=0; break; }
        memset(bgr_row,0,stride);
        for (x=0;x<width;++x) {
            uint16_t p=rgb_row[x]; unsigned r=(p>>11)&31u,g=(p>>5)&63u,b=p&31u;
            bgr_row[x*3]=(unsigned char)((b<<3)|(b>>2));
            bgr_row[x*3+1]=(unsigned char)((g<<2)|(g>>4));
            bgr_row[x*3+2]=(unsigned char)((r<<3)|(r>>2));
        }
        ok=write_bytes(io,bgr_row,stride);
    }
    return finish(io,work,path,ok);
}
