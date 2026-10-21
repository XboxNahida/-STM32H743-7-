#include "oscilloscope_accurate.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static float x[SCOPE_MAX_RECORD_LENGTH];
static scope_measurements_t m;
static scope_accurate_info_t info;
static void run(float fs) {
    unsigned steps=0;
    assert(scope_accurate_begin(x,SCOPE_RECORD_LENGTH,fs));
    while(!scope_accurate_step()) assert(++steps<40);
    assert(scope_accurate_result(&m,&info));
}
int main(void) {
    setbuf(stdout,NULL);
    unsigned i; float fs=2000000.0f, f=17321.7f;
    for(i=0;i<SCOPE_RECORD_LENGTH;i++) x[i]=1650+700*sinf(6.28318530718f*f*(float)i/fs+0.4f);
    run(fs); printf("sine %.6f error %.6f Hz\n",m.frequency_hz,m.frequency_hz-f);
    assert(m.valid&SCOPE_VALID_FREQUENCY); assert(fabsf(m.frequency_hz-f)<2.0f);
    for(i=0;i<SCOPE_RECORD_LENGTH;i++) x[i]=1650+100*sinf(6.28318530718f*f*(float)i/fs)+700*sinf(12.56637061436f*f*(float)i/fs);
    run(fs); printf("harmonic %.6f error %.6f Hz\n",m.frequency_hz,m.frequency_hz-f);
    assert(m.valid&SCOPE_VALID_FREQUENCY); assert(fabsf(m.frequency_hz-f)<2.0f);
    for(i=0;i<SCOPE_RECORD_LENGTH;i++) x[i]=1234;
    run(fs); assert(!(m.valid&SCOPE_VALID_FREQUENCY)); assert(m.mean_mv==1234);
    assert(info.reason==SCOPE_ACCURATE_DC);
    for(i=0;i<SCOPE_RECORD_LENGTH;i++) x[i]=1650+700*sinf(6.28318530718f*100*(float)i/fs);
    run(fs); assert(!(m.valid&SCOPE_VALID_FREQUENCY));
    assert(info.reason==SCOPE_ACCURATE_TOO_SHORT);
    for(i=0;i<SCOPE_RECORD_LENGTH;i++) x[i]=1650+700*sinf(6.28318530718f*f*(float)i/fs)+700*sinf(6.28318530718f*41239*(float)i/fs);
    run(fs); assert(!(m.valid&SCOPE_VALID_FREQUENCY)); assert(info.reason==SCOPE_ACCURATE_AMBIGUOUS);
    for(i=0;i<SCOPE_RECORD_LENGTH;i++) x[i]=1650+700*(sinf(6.28318530718f*f*(float)i/fs)>0?1.0f:-1.0f);
    run(fs); printf("square %.6f error %.6f Hz\n",m.frequency_hz,m.frequency_hz-f);
    assert(m.valid&SCOPE_VALID_FREQUENCY); assert(fabsf(m.frequency_hz-f)<10);
    {
        unsigned seed=1;
        for(i=0;i<SCOPE_RECORD_LENGTH;i++) {
            seed=1664525u*seed+1013904223u;
            x[i]=1650+700*sinf(6.28318530718f*f*(float)i/fs)+(float)((int)(seed>>16)-32768)*0.001f;
        }
        run(fs); printf("noise %.6f error %.6f Hz\n",m.frequency_hz,m.frequency_hz-f);
        assert(m.valid&SCOPE_VALID_FREQUENCY); assert(fabsf(m.frequency_hz-f)<5);
    }
    fs=1000000.0f;
    for(i=0;i<SCOPE_RECORD_LENGTH;i++) x[i]=1650+700*sinf(6.28318530718f*f*(float)i/fs);
    run(fs);printf("sine 1M %.6f error %.6f Hz\n",m.frequency_hz,m.frequency_hz-f);
    assert((m.valid&SCOPE_VALID_FREQUENCY) && fabsf(m.frequency_hz-f)<2.0f);
    x[40]=NAN; assert(!scope_accurate_begin(x,SCOPE_RECORD_LENGTH,fs));
    assert(!scope_accurate_begin(x,SCOPE_RECORD_LENGTH,0));
    scope_accurate_cancel(); assert(!scope_accurate_result(&m,&info));
    {
        const size_t counts[]={512,3000,8192};size_t c,n;unsigned steps;
        for(c=0;c<3;c++) {
            n=counts[c];fs=c==0?100.0f:1000000.0f;f=c==0?1.0f:200000.0f;
            for(i=0;i<n;i++)x[i]=1650+700*sinf(6.28318530718f*f*(float)i/fs);
            assert(scope_accurate_begin(x,n,fs));steps=0;
            while(!scope_accurate_step())assert(++steps<40);
            assert(scope_accurate_result(&m,&info));
            printf("variable n=%lu f=%.6f analysis=%lu\n",(unsigned long)n,m.frequency_hz,(unsigned long)info.analysis_samples);
            assert(m.valid&SCOPE_VALID_FREQUENCY);
            assert(fabsf(m.frequency_hz-f)<(f==1.0f?0.01f:20.0f));
            assert(info.analysis_samples==(c==0?512U:(c==1?2048U:8192U)));
        }
        for(i=0;i<3000;i++)x[i]=i<2048?1000.0f:2000.0f;
        assert(scope_accurate_begin(x,3000,100));
        while(!scope_accurate_step()){}
        assert(scope_accurate_result(&m,&info));
        assert(fabsf(m.mean_mv-(1000.0f+952000.0f/3000.0f))<0.001f);
    }
    puts("accurate PASS"); return 0;
}
