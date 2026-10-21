#include "oscilloscope_storage.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct { char name[128]; unsigned char data[524288]; size_t size; int used; } memory_file_t;
typedef struct { memory_file_t file[12]; int active, absent, short_write, close_fail, read_fail, scan_fail; size_t pos; } memory_fs_t;
static memory_fs_t fs;
static uint16_t raw[SCOPE_MAX_RECORD_LENGTH];
static uint16_t raw2[SCOPE_MAX_RECORD_LENGTH];
static int mem_open(void *ctx, const char *name, scope_file_mode_t mode)
{
    memory_fs_t *m = ctx; int i, empty = -1;
    if (m->absent) return SCOPE_IO_ERROR;
    for (i = 0; i < 12; ++i) {
        if (!m->file[i].used && empty < 0) empty = i;
        if (m->file[i].used && strcmp(m->file[i].name, name) == 0) {
            if (mode == SCOPE_FILE_CREATE_NEW) return SCOPE_IO_EXISTS;
            m->active = i; m->pos = 0; return SCOPE_IO_OK;
        }
    }
    if (mode == SCOPE_FILE_READ || empty < 0) return SCOPE_IO_ERROR;
    m->file[empty].used = 1; m->file[empty].size = 0;
    strcpy(m->file[empty].name, name); m->active = empty; m->pos = 0;
    return SCOPE_IO_OK;
}
static size_t mem_read(void *ctx, void *buffer, size_t bytes)
{
    memory_fs_t *m = ctx; memory_file_t *f = &m->file[m->active];
    if (m->read_fail) return (size_t)-1;
    if (bytes > f->size - m->pos) bytes = f->size - m->pos;
    memcpy(buffer, f->data + m->pos, bytes); m->pos += bytes; return bytes;
}
static size_t mem_write(void *ctx, const void *buffer, size_t bytes)
{
    memory_fs_t *m = ctx; memory_file_t *f = &m->file[m->active];
    if (m->short_write && bytes > 0) --bytes;
    assert(bytes <= sizeof(f->data) - m->pos);
    memcpy(f->data + m->pos, buffer, bytes); m->pos += bytes; f->size = m->pos; return bytes;
}
static int mem_close(void *ctx) { return !((memory_fs_t *)ctx)->close_fail; }
static int mem_remove(void *ctx, const char *name)
{
    memory_fs_t *m = ctx; int i;
    for (i = 0; i < 12; ++i) if (strcmp(m->file[i].name, name) == 0) {
        m->file[i].used = 0; return 1;
    }
    return 0;
}
static int mem_next(void *ctx, const char *dir, size_t index, char *name, size_t capacity)
{
    memory_fs_t *m = ctx; size_t n = 0; int i; (void)dir;
    if (m->scan_fail || m->absent) return -1;
    for (i = 0; i < 12; ++i) if (m->file[i].used && n++ == index) {
        const char *p = strrchr(m->file[i].name, '/'); p = p ? p + 1 : m->file[i].name;
        if (strlen(p) >= capacity) return -1;
        strcpy(name, p); return 1;
    }
    return 0;
}
static scope_storage_io_t io = { &fs, mem_open, mem_read, mem_write, mem_close, mem_remove, mem_next };
static void reset(void) { memset(&fs, 0, sizeof(fs)); }
static int used_count(void) { int i, n = 0; for (i = 0; i < 12; ++i) n += fs.file[i].used; return n; }
static uint32_t le32(const unsigned char *p)
{ return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }
static uint32_t crc32(const unsigned char *p, size_t n)
{
    uint32_t c = 0xffffffffu; size_t i; unsigned j;
    for (i = 0; i < n; ++i) { c ^= p[i]; for (j = 0; j < 8; ++j) c = (c >> 1) ^ (0xedb88320u & (0u - (c & 1u))); }
    return ~c;
}
static void put32(unsigned char *p, uint32_t n)
{ p[0]=(unsigned char)n; p[1]=(unsigned char)(n>>8); p[2]=(unsigned char)(n>>16); p[3]=(unsigned char)(n>>24); }
static void refresh_crc(memory_file_t *f)
{
    unsigned char b[384]; assert(f->size<=sizeof(b));
    memcpy(b,f->data,12); memcpy(b+12,f->data+16,f->size-16);
    put32(f->data+12,crc32(b,f->size-4));
}
static void test_settings(void)
{
    scope_settings_t s, out, old; char p[128], q[128]; unsigned char saved[384]; size_t len;
    reset(); scope_settings_defaults(&s); assert(scope_settings_valid(&s));
    assert(scope_storage_save_settings(&io, "scope", &s, p, sizeof(p)));
    len = fs.file[0].size; assert(len < sizeof(saved)); memcpy(saved, fs.file[0].data, len);
    assert(len==304U && le32(saved+4)==6U);
    assert(s.dac.harmonic_percent[0]==100.0f&&s.dac.harmonic_percent[7]==0.0f);
    assert(s.measurement_mode==0U && s.dac_sample_rate_hz==200000U);
    s.calibration.gain = 1.1f; s.trigger_mode = 2; s.dac.waveform = SCOPE_WAVE_TRIANGLE;
    s.io.enabled[0]=0U;s.io.enabled[1]=1U;s.sample_rate_hz=2000000.0f;
    s.calibration2.gain=1.25f;s.display2.inverted=1;s.mv_per_div2=250.0f;
    s.vertical_offset2_mv=1200.0f;s.measurement_source=1U;s.fft_source=2U;
    s.reading_item[0]=9U;s.reading_source[0]=1U;
    s.measurement_mode=1U;
    assert(scope_storage_save_settings(&io, "scope", &s, q, sizeof(q))); assert(strcmp(p,q));
    assert(fs.file[0].size == len && memcmp(saved,fs.file[0].data,len)==0);
    memset(&out,0,sizeof(out)); assert(scope_storage_load_latest_settings(&io,"scope",&out));
    assert(out.calibration.gain == 1.1f && out.trigger_mode == 2 && out.dac.waveform == SCOPE_WAVE_TRIANGLE);
    assert(out.io.enabled[0]==0U&&out.io.enabled[1]==1U&&out.sample_rate_hz==2000000.0f);
    assert(out.calibration2.gain==1.25f&&out.display2.inverted==1&&out.mv_per_div2==250.0f);
    assert(out.vertical_offset2_mv==1200.0f&&out.measurement_source==1U&&out.fft_source==2U);
    assert(out.reading_item[0]==9U&&out.reading_source[0]==1U);
    {scope_settings_t harmonic=s,restored;
     harmonic.dac.waveform=SCOPE_WAVE_HARMONIC;
     harmonic.dac.harmonic_percent[7]=25.0f;harmonic.dac.harmonic_phase_deg[7]=-90.0f;
     assert(scope_storage_save_settings(&io,"harmonics",&harmonic,p,sizeof(p)));
     assert(scope_storage_load_settings(&io,p,&restored));
     assert(restored.dac.waveform==SCOPE_WAVE_HARMONIC);
     assert(restored.dac.harmonic_percent[7]==25.0f&&restored.dac.harmonic_phase_deg[7]==-90.0f);
     harmonic.dac.frequency_hz=3907.0f;assert(!scope_settings_valid(&harmonic));
     harmonic.dac.frequency_hz=1000.0f;harmonic.dac.harmonic_phase_deg[7]=NAN;
     assert(!scope_settings_valid(&harmonic));
     harmonic.dac.harmonic_phase_deg[7]=0.0f;harmonic.dac.harmonic_percent[7]=101.0f;
     assert(!scope_settings_valid(&harmonic));
     assert(mem_remove(&fs,p));
     (void)snprintf(p,sizeof(p),"scope/SET_00000000.CFG");}
    {scope_settings_t invalid=s;invalid.reading_item[3]=10U;assert(!scope_settings_valid(&invalid));
     invalid=s;invalid.reading_source[2]=2U;assert(!scope_settings_valid(&invalid));}
    {scope_settings_t invalid=s;invalid.fft_source=3U;assert(!scope_settings_valid(&invalid));}
    assert(out.measurement_mode==0U);
    {unsigned char keep[304];float legacy=200000.0f;uint32_t bits;
     memcpy(keep,fs.file[1].data,sizeof(keep));memcpy(&bits,&legacy,sizeof(bits));
     fs.file[1].size=208U;put32(fs.file[1].data+4,4U);put32(fs.file[1].data+8,192U);
     put32(fs.file[1].data+72,bits);refresh_crc(&fs.file[1]);
     assert(scope_storage_load_settings(&io,q,&out)&&out.dac.frequency_hz==31250.0f);
     assert(out.dac.harmonic_percent[0]==100.0f&&out.dac.harmonic_percent[7]==0.0f);
     fs.file[1].size=304U;
     memcpy(fs.file[1].data,keep,sizeof(keep));}
    {unsigned char keep[208];memcpy(keep,fs.file[1].data,sizeof(keep));
     put32(fs.file[1].data+164,SCOPE_OUTPUT_PA5);refresh_crc(&fs.file[1]);
     assert(scope_storage_load_settings(&io,q,&out)&&out.io.output==SCOPE_OUTPUT_PA4);
     put32(fs.file[1].data+164,2U);refresh_crc(&fs.file[1]);
     assert(!scope_storage_load_settings(&io,q,&out));
     memcpy(fs.file[1].data,keep,sizeof(keep));}
    {unsigned char keep[208];memcpy(keep,fs.file[1].data,sizeof(keep));
     put32(fs.file[1].data+168,0x100U);refresh_crc(&fs.file[1]);
     assert(!scope_storage_load_settings(&io,q,&out));
     memcpy(fs.file[1].data,keep,sizeof(keep));}
    old = out; fs.file[1].data[20] ^= 1;
    assert(!scope_storage_load_settings(&io,q,&out)); assert(memcmp(&old,&out,sizeof(out))==0);
    assert(scope_storage_load_latest_settings(&io,"scope",&out)); assert(out.calibration.gain==1.0f);
    old = out; ++fs.file[0].size; assert(!scope_storage_load_settings(&io,p,&out));
    assert(memcmp(&old,&out,sizeof(out))==0); --fs.file[0].size;
    fs.file[0].data[4] = 99; assert(!scope_storage_load_settings(&io,p,&out));
    memcpy(fs.file[0].data,saved,len);
    /* 有效CRC但采样率NaN：全量字段校验必须拒绝。CRC覆盖头前12字节及负载。 */
    put32(fs.file[0].data+16,0x7fc00000u);
    { unsigned char c[384]; memcpy(c,fs.file[0].data,12); memcpy(c+12,fs.file[0].data+16,len-16);
      put32(fs.file[0].data+12,crc32(c,len-4)); }
    assert(!scope_storage_load_settings(&io,p,&out)); assert(memcmp(&old,&out,sizeof(out))==0);
    {scope_settings_t follow=s,restored;follow.cursor_enabled=2U;
     assert(scope_storage_save_settings(&io,"follow",&follow,p,sizeof(p)));
     assert(scope_storage_load_settings(&io,p,&restored)&&restored.cursor_enabled==2U);
     follow.cursor_enabled=3U;assert(!scope_settings_valid(&follow));}
    s.display.inverted = 2; assert(!scope_settings_valid(&s));
    scope_settings_defaults(&s); s.cursor_x[1] = 8192; assert(!scope_settings_valid(&s));
    scope_settings_defaults(&s); s.render_mode=2; assert(!scope_settings_valid(&s));
    scope_settings_defaults(&s);s.sample_rate_hz=1000001.0f;assert(!scope_settings_valid(&s));
    scope_settings_defaults(&s);s.io.enabled[1]=1U;s.io.input[1]=s.io.input[0];assert(!scope_settings_valid(&s));
    scope_settings_defaults(&s); s.dac.offset_mv=0; assert(!scope_settings_valid(&s));
    scope_settings_defaults(&s); s.dac.offset_mv=3300; s.dac.amplitude_mv=0.0001f;
    assert(!scope_settings_valid(&s)); /* 使用double边界，避免float舍入掩盖DAC越界。 */
}
static void test_v1_migration(void)
{
    scope_settings_t s,out;char p[128];memory_file_t *f;
    reset();scope_settings_defaults(&s);
    assert(scope_storage_save_settings(&io,"scope",&s,p,sizeof(p)));
    f=&fs.file[0];f->size=136U;put32(f->data+4,1U);put32(f->data+8,120U);refresh_crc(f);
    assert(scope_storage_load_settings(&io,p,&out));
    assert(out.measurement_mode==0U && out.dac_sample_rate_hz==200000U);
    assert(out.trigger.window_samples==2048U && out.render_mode==0U);
    f->size=144U;put32(f->data+4,2U);put32(f->data+8,128U);
    put32(f->data+136,1U);put32(f->data+140,1000000U);refresh_crc(f);
    assert(scope_storage_load_settings(&io,p,&out));
    assert(out.trigger.window_samples==2048U && out.measurement_mode==1U);
    f->size=144U;put32(f->data+4,2U);put32(f->data+8,128U);
    put32(f->data+136,0x100U);put32(f->data+140,256000U);refresh_crc(f);
    assert(!scope_storage_load_settings(&io,p,&out));
    put32(f->data+136,0U);put32(f->data+140,1000001U);refresh_crc(f);
    assert(!scope_storage_load_settings(&io,p,&out));
    f->size=148U;put32(f->data+4,3U);put32(f->data+8,132U);
    put32(f->data+140,256000U);refresh_crc(f);
    assert(scope_storage_load_settings(&io,p,&out));
    assert(out.io.enabled[0]==1U&&out.io.enabled[1]==0U&&out.io.input[1]==SCOPE_INPUT_PC4);
    f->size=208U;put32(f->data+4,4U);put32(f->data+8,192U);
    put32(f->data+200,1U);refresh_crc(f);
    assert(scope_storage_load_settings(&io,p,&out));
    assert(out.reading_source[0]==1U&&out.reading_item[0]==0U&&out.reading_item[3]==6U);
    reset();scope_settings_defaults(&s);s.cursor_enabled=1U;
    assert(scope_storage_save_settings(&io,"scope",&s,p,sizeof(p)));
    f=&fs.file[0];put32(f->data+4,5U);refresh_crc(f);
    assert(scope_storage_load_settings(&io,p,&out)&&out.cursor_enabled==1U);
    put32(f->data+116,2U);refresh_crc(f);
    assert(!scope_storage_load_settings(&io,p,&out));
}
static void test_csv(void)
{
    scope_calibration_t c = {3300.0f,0.0f,1.0f}; char p[128]; char *text; size_t i, rows=0;
    reset(); for(i=0;i<SCOPE_RECORD_LENGTH;++i) raw[i]=(uint16_t)(i*32); raw[2047]=65535;
    assert(scope_storage_save_csv(&io,"scope",raw,2048,100000,42,&c,p,sizeof(p)));
    text=(char *)fs.file[0].data; text[fs.file[0].size]=0;
    assert(strstr(text,"sample_rate_hz=100000") && strstr(text,"sequence=42") && strstr(text,"vref_mv=3300"));
    assert(strstr(text,"index,time_s,raw,mV\r\n0,0,0,0\r\n"));
    assert(strstr(text,"2047,0.02047,65535,3300\r\n"));
    for(i=0;i<fs.file[0].size;++i) if(text[i]=='\n') ++rows;
    assert(rows==2050); assert(!scope_storage_save_csv(&io,"scope",raw,8193,100000,42,&c,p,sizeof(p)));
    assert(scope_storage_save_csv_mode(&io,"scope",raw,2048,100000,43,&c,1U,42U,p,sizeof(p)));
    text=(char *)fs.file[1].data;text[fs.file[1].size]=0;
    assert(strstr(text,"measurement_mode=accurate,measurement_sequence=42"));
    assert(scope_storage_save_csv(&io,"scope",raw,8192,100,44,&c,p,sizeof(p)));
    text=(char *)fs.file[2].data;text[fs.file[2].size]=0;
    assert(strstr(text,"count=8192")&&strstr(text,"8191,81.91,"));
    assert(scope_storage_save_csv(&io,"scope",raw,64,100,45,&c,p,sizeof(p)));
    text=(char *)fs.file[3].data;text[fs.file[3].size]=0;
    rows=0;for(i=0;i<fs.file[3].size;i++)if(text[i]=='\n')rows++;
    assert(rows==66&&strstr(text,"count=64"));
    {
        scope_io_config_t channels={{1U,1U},{SCOPE_INPUT_PA5,SCOPE_INPUT_PC4},SCOPE_OUTPUT_PA4};
        scope_calibration_t c2={3300.0f,100.0f,1.5f};
        for(i=0U;i<64U;i++){raw[i]=(uint16_t)(1000U+i);raw2[i]=(uint16_t)(2000U+i);}
        reset();
        assert(scope_storage_save_csv_pair(&io,"scope",raw,raw2,64U,100000U,46U,&channels,
            &c,&c2,0U,0U,p,sizeof(p)));
        text=(char *)fs.file[0].data;text[fs.file[0].size]=0;
        assert(strstr(text,"CH1_pin=PA5,CH2_pin=PC4"));
        assert(strstr(text,"index,time_s,CH1_raw,CH1_mV,CH2_raw,CH2_mV"));
        assert(strstr(text,"0,0,1000,50.3547")&&strstr(text,",2000,143.511"));
        channels.enabled[0]=0U;
        assert(scope_storage_save_csv_pair(&io,"scope",NULL,raw2,64U,100000U,47U,&channels,
            NULL,&c2,0U,0U,p,sizeof(p)));
        text=(char *)fs.file[1].data;text[fs.file[1].size]=0;
        assert(strstr(text,"CH1_pin=off,CH2_pin=PC4")&&strstr(text,"0,0,,,2000,"));
    }
    reset();{scope_settings_t s,out;scope_settings_defaults(&s);
        s.trigger.window_samples=8192;s.render_mode=1U;
        assert(scope_storage_save_settings(&io,"scope",&s,p,sizeof(p)));
        assert(scope_storage_load_settings(&io,p,&out));
        assert(out.trigger.window_samples==8192&&out.render_mode==1U);
    }
}
static void test_setting_order(void)
{
    scope_settings_t s,out; char p[128],q[128],r[128];
    reset(); scope_settings_defaults(&s);
    assert(scope_storage_save_settings(&io,"scope",&s,p,sizeof(p)));
    assert(scope_storage_save_settings(&io,"scope",&s,q,sizeof(q)));
    assert(mem_remove(&fs,p)); s.trigger_mode=1;
    assert(scope_storage_save_settings(&io,"scope",&s,r,sizeof(r)));
    assert(strcmp(r,q)>0);
    assert(scope_storage_load_latest_settings(&io,"scope",&out)); assert(out.trigger_mode==1);
}
static void test_all_setting_fields(void)
{
    static const uint32_t bad[30]={
        0x7fc00000u,3,0x7fc00000u,0x100,8192,8193,60001,0x100,0x7fc00000u,0x100,
        0x7fc00000u,0x7fc00000u,0x7fc00000u,0x100,0x7fc00000u,0x7fc00000u,
        0x7fc00000u,0x7fc00000u,0x7fc00000u,0x7fc00000u,0x7fc00000u,3,0,2,33,3,
        0x7fc00000u,0x7fc00000u,0x7fc00000u,0x7fc00000u };
    scope_settings_t s,old; char p[128]; unsigned char original[384]; size_t i;
    reset(); scope_settings_defaults(&s); old=s;
    assert(scope_storage_save_settings(&io,"scope",&s,p,sizeof(p)));
    memcpy(original,fs.file[0].data,fs.file[0].size);
    for(i=0;i<30;++i) {
        memcpy(fs.file[0].data,original,fs.file[0].size);
        put32(fs.file[0].data+16+4*i,bad[i]); refresh_crc(&fs.file[0]);
        assert(!scope_storage_load_settings(&io,p,&s)); assert(memcmp(&old,&s,sizeof(s))==0);
    }
}
static int rgb_row(void *ctx, uint32_t y, uint16_t *row, size_t width)
{
    (void)ctx; assert(width==2); row[0]= y ? 0x001fu : 0xf800u; row[1]=0x07e0u; return 1;
}
static int failed_row(void *ctx,uint32_t y,uint16_t *row,size_t width)
{ (void)ctx;(void)y;(void)row;(void)width;return 0; }
static void test_bmp(void)
{
    char p[128]; unsigned char *d;
    reset(); assert(scope_storage_save_bmp(&io,"scope",2,2,rgb_row,NULL,p,sizeof(p)));
    d=fs.file[0].data; assert(d[0]=='B' && d[1]=='M' && le32(d+2)==70 && le32(d+10)==54);
    assert(le32(d+18)==2 && le32(d+22)==2 && d[28]==24);
    assert(d[54]==255 && d[55]==0 && d[56]==0 && d[57]==0 && d[58]==255 && d[59]==0);
    assert(d[60]==0 && d[61]==0 && d[62]==0 && d[63]==0 && d[64]==255);
    assert(!scope_storage_save_bmp(&io,"scope",1025,2,rgb_row,NULL,p,sizeof(p)));
    assert(!scope_storage_save_bmp(&io,"scope",2,2,failed_row,NULL,p,sizeof(p))); assert(used_count()==1);
    fs.short_write=1;
    assert(!scope_storage_save_bmp(&io,"scope",2,2,rgb_row,NULL,p,sizeof(p))); assert(used_count()==1);
}
static void test_failures(void)
{
    scope_settings_t s, old; char p[128]="unchanged";
    reset(); scope_settings_defaults(&s); fs.absent=1;
    assert(!scope_storage_save_settings(&io,"scope",&s,p,sizeof(p))); assert(strcmp(p,"unchanged")==0);
    old=s; assert(!scope_storage_load_latest_settings(&io,"scope",&s)); assert(memcmp(&old,&s,sizeof(s))==0);
    fs.absent=0; fs.short_write=1;
    assert(!scope_storage_save_settings(&io,"scope",&s,p,sizeof(p))); assert(used_count()==0);
    fs.short_write=0; fs.close_fail=1;
    assert(!scope_storage_save_settings(&io,"scope",&s,p,sizeof(p))); assert(used_count()==0);
    fs.close_fail=0; assert(!scope_storage_save_settings(&io,"scope",&s,p,3)); assert(used_count()==0);
    assert(scope_storage_save_settings(&io,"scope",&s,p,sizeof(p)));
    old=s; fs.read_fail=1; assert(!scope_storage_load_settings(&io,p,&s));
    assert(memcmp(&old,&s,sizeof(s))==0); fs.read_fail=0;
    fs.scan_fail=1; assert(!scope_storage_load_latest_settings(&io,"scope",&s));
    assert(memcmp(&old,&s,sizeof(s))==0);
    fs.scan_fail=0; fs.short_write=1;
    assert(!scope_storage_save_csv(&io,"scope",raw,2048,100000,1,&s.calibration,p,sizeof(p)));
    assert(used_count()==1);
}
int main(void)
{
    test_settings(); test_v1_migration();test_setting_order(); test_all_setting_fields(); test_csv(); test_bmp(); test_failures();
    puts("PASS oscilloscope_storage"); return 0;
}

