/* 片上ADC通用准确测量：原始幅值/均值/RMS不施加前端经验补偿。 */
#include "oscilloscope_accurate.h"
#include "scope_reference_frequency.h"
#include "oscilloscope_workspace.h"
#include <math.h>
#include <string.h>
static float *samples,*fftIn,*fftOut;
static size_t analysis_count;
static fft_ddc_freq_ctx_t ctx;
static fft_ddc_freq_result_t frequency;
static scope_measurements_t measurement;
static scope_accurate_info_t detail;
static float rate;
static int active,ready;
/* 窗主瓣以外存在明显非谐波能量时，不把复合信号标成可靠基波。 */
static int coherent_family(float base) {
    unsigned b,h; double all=0, family=0;
    float bin=rate/(float)analysis_count;
    for(b=1;b<analysis_count/2;b++) {
        double p=(double)fftOut[2*b]*fftOut[2*b]+(double)fftOut[2*b+1]*fftOut[2*b+1];
        int belongs=0;
        all+=p;
        for(h=1;h<=8;h++)
            if(fabsf((float)b-base*(float)h/bin)<=3.0f) belongs=1;
        if(belongs) family+=p;
    }
    return all>0 && family/all>0.80;
}
static void finish(int status) {
    measurement.valid &= ~(uint32_t)(SCOPE_VALID_FREQUENCY|SCOPE_VALID_PERIOD);
    measurement.frequency_hz=NAN; measurement.period_s=NAN;
    if(status==0 && isfinite(frequency.freqHz) && frequency.freqHz>=3.0f*rate/(float)analysis_count && frequency.freqHz<0.49f*rate) {
        measurement.frequency_hz=frequency.freqHz;
        measurement.period_s=1.0f/frequency.freqHz;
        measurement.valid|=SCOPE_VALID_FREQUENCY|SCOPE_VALID_PERIOD;
        detail.reason=SCOPE_ACCURATE_OK;
    } else detail.reason=SCOPE_ACCURATE_TOO_SHORT;
    ready=1; active=0;
}
void scope_accurate_cancel(void) { active=0; ready=0; }
int scope_accurate_begin(const float *mv,size_t n,float fs) {
    scope_workspace_t *w=scope_workspace_get();
    size_t i;
    int status;
    scope_accurate_cancel();
    if(w==NULL || mv==NULL || n<SCOPE_MIN_RECORD_LENGTH || n>SCOPE_MAX_RECORD_LENGTH || !isfinite(fs) || fs<=0) return 0;
    for(i=0;i<n;i++) if(!isfinite(mv[i])) return 0;
    samples=w->accurate_samples;fftIn=w->accurate_in;fftOut=w->accurate_out;
    memcpy(samples,mv,n*sizeof(*samples)); rate=fs;
    if(!scope_measure(samples,n,fs,&measurement)) return 0;
    /* 动态边沿值采用已有核心；频率替换为参考估计。 */
    analysis_count=1U;while(analysis_count*2U<=n)analysis_count*=2U;
    detail.steps=0; detail.reason=SCOPE_ACCURATE_OK;detail.analysis_samples=analysis_count;
    if(measurement.vpp_mv<0.01f) {
        finish(-1); detail.reason=SCOPE_ACCURATE_DC; return 1;
    }
    if(!(measurement.valid&SCOPE_VALID_FREQUENCY)) { finish(-1); return 1; }
    if(fft_ddc_frequency_init_f32(&ctx,(uint32_t)analysis_count,fftIn,fftOut)!=FFT_DDC_FREQ_OK) return 0;
    ctx.minFreqHz=3.0f*fs/(float)analysis_count; ctx.maxFreqHz=0.49f*fs;
    ctx.ddcLowpassCutHz=0.25f*fs/(float)analysis_count;
    ctx.ddcSettleSamples=(uint32_t)(analysis_count*3u/4u);
    status=(int)fft_ddc_frequency_estimate_harmonic_fundamental_f32(
        &ctx,&rate,samples,ctx.minFreqHz,ctx.maxFreqHz,8,&frequency);
    if(status==1) {
        if(!coherent_family(frequency.coarseFreqHz)) {
            finish(-1); detail.reason=SCOPE_ACCURATE_AMBIGUOUS;
        } else active=1;
    }
    else {
        status=(int)fft_ddc_frequency_estimate_f32(&ctx,&rate,samples,&frequency);
        if(status==0) {
            if(!coherent_family(frequency.freqHz)) {
                finish(-1); detail.reason=SCOPE_ACCURATE_AMBIGUOUS; return 1;
            }
            status=scope_reference_single_sweep(&ctx,&rate,samples,&frequency);
            if(status==1) active=1; else finish(status);
        } else finish(status);
    }
    return 1;
}
int scope_accurate_step(void) {
    int status;
    if(!active) return 1;
    ++detail.steps;
    status=scope_reference_sweep_step();
    if(status!=1) finish(status);
    return ready;
}
int scope_accurate_result(scope_measurements_t *out,scope_accurate_info_t *info) {
    if(!ready || out==NULL) return 0;
    *out=measurement; if(info!=NULL) *info=detail; return 1;
}
