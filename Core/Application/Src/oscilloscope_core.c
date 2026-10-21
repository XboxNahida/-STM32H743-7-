#include "oscilloscope_core.h"
#include "oscilloscope_workspace.h"
#include <float.h>
#include <math.h>

#define SCOPE_PI 3.14159265358979323846

static float display_slope(float left,float right)
{
    if(left==0.0f||right==0.0f||(left<0.0f)!=(right<0.0f))return 0.0f;
    return 2.0f*left*right/(left+right);
}
int scope_resample_view(const float *input,size_t total,size_t start,size_t count,
                        float *output,size_t points,int fitted)
{
    size_t j,i;float x,t,a,b,d,m0,m1,v;
    if(!input||!output||total>SCOPE_MAX_RECORD_LENGTH||start>=total||
       count<2U||count>total-start||points<2U||points>SCOPE_MAX_RECORD_LENGTH)return 0;
    for(j=0U;j<points;j++){
        x=(float)j*(float)(count-1U)/(float)(points-1U);i=(size_t)x;
        if(i>=count-1U){output[j]=input[start+count-1U];continue;}
        a=input[start+i];b=input[start+i+1U];t=x-(float)i;
        if(!fitted){output[j]=input[start+(size_t)(x+0.5f)];continue;}
        d=b-a;m0=i?display_slope(a-input[start+i-1U],d):d;
        m1=i+2U<count?display_slope(d,input[start+i+2U]-b):d;
        v=(2*t*t*t-3*t*t+1)*a+(t*t*t-2*t*t+t)*m0+
          (-2*t*t*t+3*t*t)*b+(t*t*t-t*t)*m1;
        /* 浮点舍入也不得使阶跃插值超出相邻样本范围。 */
        if(v<fminf(a,b))v=fminf(a,b);
        if(v>fmaxf(a,b))v=fmaxf(a,b);
        output[j]=v;
    }
    return 1;
}

static int valid_signal(const float *mv, size_t count)
{
    size_t i;
    if (mv == NULL || count == 0u || count > SCOPE_CAPTURE_MAX_LENGTH) return 0;
    for (i = 0u; i < count; ++i) if (!isfinite(mv[i])) return 0;
    return 1;
}

int scope_convert_adc(const uint16_t *raw, size_t count,
                      const scope_calibration_t *calibration, float *mv)
{
    size_t i;
    double value;
    if (raw == NULL || mv == NULL || calibration == NULL || count == 0u ||
        count > SCOPE_CAPTURE_MAX_LENGTH || !isfinite(calibration->vref_mv) ||
        calibration->vref_mv <= 0.0f || !isfinite(calibration->zero_code) ||
        calibration->zero_code < 0.0f || calibration->zero_code > 65535.0f ||
        !isfinite(calibration->gain) || calibration->gain <= 0.0f) return 0;
    for (i = 0u; i < count; ++i) {
        value = ((double)raw[i] - calibration->zero_code) *
                calibration->vref_mv / 65535.0 * calibration->gain;
        if (fabs(value) > FLT_MAX) return 0;
        mv[i] = (float)value;
    }
    return 1;
}

int scope_transform(const float *input, size_t count,
                    const scope_display_t *display, float *output)
{
    size_t i;
    double mean = 0.0, value;
    if (count > SCOPE_MAX_RECORD_LENGTH || !valid_signal(input, count) || display == NULL || output == NULL ||
        (unsigned)display->coupling > (unsigned)SCOPE_COUPLING_GND ||
        !isfinite(display->probe_factor) || display->probe_factor <= 0.0f) return 0;
    if (display->coupling == SCOPE_COUPLING_AC) {
        for (i = 0u; i < count; ++i) mean += input[i];
        mean /= (double)count;
    }
    for (i = 0u; i < count; ++i) {
        value = display->coupling == SCOPE_COUPLING_GND ? 0.0 :
                ((double)input[i] - mean) * display->probe_factor;
        if (display->inverted) value = -value;
        if (fabs(value) > FLT_MAX) return 0;
        output[i] = (float)value;
    }
    return 1;
}

static void invalidate_measurement(scope_measurements_t *m)
{
    m->valid = 0u;
    m->min_mv = NAN; m->max_mv = NAN; m->mean_mv = NAN;
    m->rms_mv = NAN; m->vpp_mv = NAN; m->frequency_hz = NAN;
    m->period_s = NAN; m->duty_percent = NAN; m->rise_s = NAN; m->fall_s = NAN;
}

static double crossing(size_t i, float before, float after, double threshold)
{
    return (double)(i - 1u) + (threshold - before) / ((double)after - before);
}

int scope_measure(const float *mv, size_t count, float sample_rate_hz,
                  scope_measurements_t *m)
{
    size_t i;
    unsigned rises = 0u, falls = 0u, periods = 0u, duties = 0u;
    double minimum, maximum, sum = 0.0, sumsq = 0.0, span, low, mid, high;
    double r10 = NAN, r50 = NAN, f90 = NAN, f50 = NAN;
    double last_rise = NAN, last_fall = NAN;
    double rise_sum = 0.0, fall_sum = 0.0, period_sum = 0.0;
    double high_sum = 0.0, duty_period_sum = 0.0, time;
    if (m == NULL) return 0;
    invalidate_measurement(m);
    if (count > SCOPE_MAX_RECORD_LENGTH || !valid_signal(mv, count) || !isfinite(sample_rate_hz) || sample_rate_hz <= 0.0f) return 0;
    minimum = maximum = mv[0];
    for (i = 0u; i < count; ++i) {
        if (mv[i] < minimum) minimum = mv[i];
        if (mv[i] > maximum) maximum = mv[i];
        sum += mv[i]; sumsq += (double)mv[i] * mv[i];
    }
    span = maximum - minimum;
    if (span > FLT_MAX) return 0;
    m->min_mv = (float)minimum; m->max_mv = (float)maximum;
    m->mean_mv = (float)(sum / (double)count);
    m->rms_mv = (float)sqrt(sumsq / (double)count); m->vpp_mv = (float)span;
    m->valid = SCOPE_VALID_MIN | SCOPE_VALID_MAX | SCOPE_VALID_MEAN | SCOPE_VALID_RMS | SCOPE_VALID_VPP;
    if (span <= 0.000001) return 1;
    low = minimum + 0.1 * span; mid = minimum + 0.5 * span; high = minimum + 0.9 * span;
    for (i = 1u; i < count; ++i) {
        if (mv[i] > mv[i - 1u]) {
            if (mv[i - 1u] <= low && mv[i] > low) {
                r10 = crossing(i, mv[i - 1u], mv[i], low); r50 = NAN;
            }
            if (mv[i - 1u] <= mid && mv[i] > mid) r50 = crossing(i, mv[i - 1u], mv[i], mid);
            if (mv[i - 1u] < high && mv[i] >= high && isfinite(r10) && isfinite(r50)) {
                time = crossing(i, mv[i - 1u], mv[i], high);
                rise_sum += time - r10; ++rises;
                if (isfinite(last_rise) && r50 > last_rise) {
                    period_sum += r50 - last_rise; ++periods;
                    if (isfinite(last_fall) && last_fall > last_rise && last_fall < r50) {
                        high_sum += last_fall - last_rise;
                        duty_period_sum += r50 - last_rise; ++duties;
                    }
                }
                last_rise = r50; r10 = NAN; r50 = NAN;
                f90 = NAN; f50 = NAN;
            }
        } else if (mv[i] < mv[i - 1u]) {
            if (mv[i - 1u] >= high && mv[i] < high) {
                f90 = crossing(i, mv[i - 1u], mv[i], high); f50 = NAN;
            }
            if (mv[i - 1u] >= mid && mv[i] < mid) f50 = crossing(i, mv[i - 1u], mv[i], mid);
            if (mv[i - 1u] > low && mv[i] <= low && isfinite(f90) && isfinite(f50)) {
                time = crossing(i, mv[i - 1u], mv[i], low);
                fall_sum += time - f90; ++falls;
                last_fall = f50; f90 = NAN; f50 = NAN;
                r10 = NAN; r50 = NAN;
            }
        }
    }
    if (rises != 0u) {
        time = rise_sum / rises / sample_rate_hz;
        if (time <= FLT_MAX && (float)time > 0.0f) {
            m->rise_s = (float)time; m->valid |= SCOPE_VALID_RISE;
        }
    }
    if (falls != 0u) {
        time = fall_sum / falls / sample_rate_hz;
        if (time <= FLT_MAX && (float)time > 0.0f) {
            m->fall_s = (float)time; m->valid |= SCOPE_VALID_FALL;
        }
    }
    if (periods != 0u && period_sum > 0.0) {
        double period_s = period_sum / periods / sample_rate_hz;
        double frequency_hz = 1.0 / period_s;
        if (period_s <= FLT_MAX && frequency_hz <= FLT_MAX && (float)period_s > 0.0f) {
            m->period_s = (float)period_s; m->frequency_hz = (float)frequency_hz;
            m->valid |= SCOPE_VALID_PERIOD | SCOPE_VALID_FREQUENCY;
        }
    }
    if (duties != 0u && duty_period_sum > 0.0) {
        m->duty_percent = (float)(100.0 * high_sum / duty_period_sum); m->valid |= SCOPE_VALID_DUTY;
    }
    return 1;
}

int scope_find_trigger(const float *mv, size_t count,
                       const scope_trigger_t *trigger, scope_trigger_result_t *result)
{
    size_t i;
    if (result == NULL) return 0;
    result->found = 0; result->index = 0u; result->start = 0u;
    if (!valid_signal(mv, count) || trigger == NULL || !isfinite(trigger->level_mv) ||
        (unsigned)trigger->edge > (unsigned)SCOPE_EDGE_FALLING ||
        trigger->window_samples == 0u || trigger->window_samples > count ||
        trigger->pre_samples >= trigger->window_samples) return 0;
    for (i = 1u; i < count; ++i) {
        int edge = trigger->edge == SCOPE_EDGE_RISING ?
                   (mv[i - 1u] < trigger->level_mv && mv[i] >= trigger->level_mv) :
                   (mv[i - 1u] > trigger->level_mv && mv[i] <= trigger->level_mv);
        if (edge && i >= trigger->pre_samples &&
            i - trigger->pre_samples <= count - trigger->window_samples) {
            result->found = 1; result->index = i; result->start = i - trigger->pre_samples;
            break;
        }
    }
    return 1;
}

int scope_fft(const float *mv, size_t n, float *bins, size_t bin_capacity)
{
    scope_workspace_t *w=scope_workspace_get();
    float *fft_real,*fft_imag;
    size_t i, j = 0u, bit, length, base, k;
    double coherent = 0.0, window, angle, wr, wi, tr, ti, magnitude;
    float swap;
    if (w == NULL || n < 4u || n > SCOPE_FFT_MAX_LENGTH || (n & (n - 1u)) != 0u ||
        bins == NULL || bin_capacity < n / 2u + 1u || !valid_signal(mv, n)) return 0;
    fft_real=w->fft_real;fft_imag=w->fft_imag;
    for (i = 0u; i < n; ++i) {
        window = 0.5 * (1.0 - cos(2.0 * SCOPE_PI * (double)i / (double)(n - 1u)));
        coherent += window / (double)n;
        fft_real[i] = (float)((double)mv[i] * window / (double)n);
        fft_imag[i] = 0.0f;
    }
    for (i = 1u; i < n; ++i) {
        bit = n >> 1u;
        while ((j & bit) != 0u) { j ^= bit; bit >>= 1u; }
        j ^= bit;
        if (i < j) {
            swap = fft_real[i]; fft_real[i] = fft_real[j]; fft_real[j] = swap;
            swap = fft_imag[i]; fft_imag[i] = fft_imag[j]; fft_imag[j] = swap;
        }
    }
    for (length = 2u; length <= n; length <<= 1u) {
        for (k = 0u; k < length / 2u; ++k) {
            angle = -2.0 * SCOPE_PI * (double)k / (double)length;
            wr = cos(angle); wi = sin(angle);
            for (base = 0u; base < n; base += length) {
                i = base + k; j = i + length / 2u;
                tr = wr * fft_real[j] - wi * fft_imag[j];
                ti = wr * fft_imag[j] + wi * fft_real[j];
                fft_real[j] = (float)(fft_real[i] - tr);
                fft_imag[j] = (float)(fft_imag[i] - ti);
                fft_real[i] = (float)(fft_real[i] + tr);
                fft_imag[i] = (float)(fft_imag[i] + ti);
            }
        }
    }
    for (i = 0u; i <= n / 2u; ++i) {
        magnitude = sqrt((double)fft_real[i] * fft_real[i] + (double)fft_imag[i] * fft_imag[i]) / coherent;
        if (i != 0u && i != n / 2u) magnitude *= 2.0;
        if (!isfinite(magnitude) || magnitude > FLT_MAX) return 0;
        bins[i] = (float)magnitude;
    }
    return 1;
}

unsigned scope_dac_highest_harmonic(const scope_dac_config_t *config)
{
    unsigned i,highest=1U;
    if(!config||(unsigned)config->waveform>(unsigned)SCOPE_WAVE_HARMONIC)return 0U;
    if(config->waveform!=SCOPE_WAVE_HARMONIC)return 1U;
    for(i=0U;i<SCOPE_DAC_HARMONICS;++i){
        if(!isfinite(config->harmonic_percent[i])||config->harmonic_percent[i]<0.0f||
           config->harmonic_percent[i]>100.0f||!isfinite(config->harmonic_phase_deg[i])||
           config->harmonic_phase_deg[i]<-360.0f||config->harmonic_phase_deg[i]>360.0f)return 0U;
        if(config->harmonic_percent[i]>0.0f)highest=i+1U;
    }
    return highest;
}
static double dac_function(const scope_dac_config_t *config,double phase)
{
    double duty=(double)config->duty_percent/100.0,edge,value=0.0;unsigned i;
    switch(config->waveform){
    case SCOPE_WAVE_SINE:return sin(2.0*SCOPE_PI*phase);
    case SCOPE_WAVE_SQUARE:return phase<duty?1.0:-1.0;
    case SCOPE_WAVE_TRIANGLE:return phase<duty?-1.0+2.0*phase/duty:
        1.0-2.0*(phase-duty)/(1.0-duty);
    case SCOPE_WAVE_SAW:return 2.0*phase-1.0;
    case SCOPE_WAVE_DC:return 0.0;
    case SCOPE_WAVE_PULSE:return phase<duty?2.0*sin(SCOPE_PI*phase/duty)-1.0:-1.0;
    case SCOPE_WAVE_TRAPEZOID:
        edge=fmin(duty,1.0-duty)*0.5;
        if(phase<edge)return -1.0+2.0*phase/edge;
        if(phase<duty)return 1.0;
        if(phase<duty+edge)return 1.0-2.0*(phase-duty)/edge;
        return -1.0;
    case SCOPE_WAVE_RECTIFIED:return 2.0*sin(SCOPE_PI*phase)-1.0;
    case SCOPE_WAVE_EXPONENTIAL:
        return phase<duty?-1.0+2.0*(1.0-exp(-5.0*phase/duty))/(1.0-exp(-5.0)):
            1.0-2.0*(1.0-exp(-5.0*(phase-duty)/(1.0-duty)))/(1.0-exp(-5.0));
    case SCOPE_WAVE_SINC:
        phase=8.0*SCOPE_PI*(phase-0.5);return fabs(phase)<1e-12?1.0:sin(phase)/phase;
    case SCOPE_WAVE_HARMONIC:
        for(i=0U;i<SCOPE_DAC_HARMONICS;++i)if(config->harmonic_percent[i]>0.0f)
            value+=(double)config->harmonic_percent[i]*0.01*
                sin(2.0*SCOPE_PI*(double)(i+1U)*phase+(double)config->harmonic_phase_deg[i]*SCOPE_PI/180.0);
        return value;
    default:return 0.0;
    }
}
int scope_make_dac(const scope_dac_config_t *config, float vref_mv,
                   uint16_t *table, size_t count)
{
    size_t i;
    double phase,normalized,voltage,minimum=0.0,maximum=0.0;
    if (config == NULL || table == NULL || count < 2u || count > SCOPE_RECORD_LENGTH ||
        !scope_dac_highest_harmonic(config) ||
        !isfinite(vref_mv) || vref_mv <= 0.0f ||
        !isfinite(config->frequency_hz) || config->frequency_hz <= 0.0f ||
        !isfinite(config->amplitude_mv) || config->amplitude_mv < 0.0f ||
        !isfinite(config->offset_mv) || !isfinite(config->duty_percent) ||
        config->duty_percent <= 0.0f || config->duty_percent >= 100.0f ||
        (double)config->offset_mv - 0.5 * (double)config->amplitude_mv < 0.0 ||
        (double)config->offset_mv + 0.5 * (double)config->amplitude_mv > (double)vref_mv ||
        (config->waveform == SCOPE_WAVE_DC && config->amplitude_mv != 0.0f)) return 0;
    /* 合成/sinc按整周期真实极值归一化，总峰峰值受面板幅值控制。 */
    if(config->waveform==SCOPE_WAVE_HARMONIC||config->waveform==SCOPE_WAVE_SINC){
        minimum=maximum=dac_function(config,0.0);
        for(i=1U;i<count;++i){
            normalized=dac_function(config,(double)i/(double)count);
            if(normalized<minimum)minimum=normalized;
            if(normalized>maximum)maximum=normalized;
        }
    }
    for (i = 0u; i < count; ++i) {
        phase = (double)i / (double)count;
        normalized=dac_function(config,phase);
        if(config->waveform==SCOPE_WAVE_HARMONIC||config->waveform==SCOPE_WAVE_SINC)
            normalized=maximum-minimum>1e-12?2.0*(normalized-minimum)/(maximum-minimum)-1.0:0.0;
        normalized=fmax(-1.0,fmin(1.0,normalized));
        voltage = (double)config->offset_mv + 0.5 * (double)config->amplitude_mv * normalized;
        table[i] = (uint16_t)floor(voltage / vref_mv * SCOPE_DAC_MAX_CODE + 0.5);
    }
    return 1;
}
