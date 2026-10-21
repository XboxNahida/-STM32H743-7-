#ifndef OSCILLOSCOPE_CORE_H
#define OSCILLOSCOPE_CORE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCOPE_RECORD_LENGTH 2048u
#define SCOPE_MIN_RECORD_LENGTH 64u
#define SCOPE_MAX_RECORD_LENGTH 8192u
#define SCOPE_CAPTURE_MAX_LENGTH (2u * SCOPE_MAX_RECORD_LENGTH)
#define SCOPE_FFT_MAX_LENGTH SCOPE_MAX_RECORD_LENGTH
#define SCOPE_DAC_MAX_CODE 4095u
#define SCOPE_DAC_MAX_SAMPLES 1024u
#define SCOPE_DAC_HARMONICS 8u

/* 单位：电压 mV，频率 Hz，时间 s。仅从主循环调用；FFT 使用静态工作区。 */
typedef struct {
    float vref_mv;
    float zero_code;
    float gain;
} scope_calibration_t;

typedef enum { SCOPE_COUPLING_DC, SCOPE_COUPLING_AC, SCOPE_COUPLING_GND } scope_coupling_t;
typedef struct {
    scope_coupling_t coupling;
    float probe_factor;
    int inverted;
} scope_display_t;

enum {
    SCOPE_VALID_MIN = 1u << 0,
    SCOPE_VALID_MAX = 1u << 1,
    SCOPE_VALID_MEAN = 1u << 2,
    SCOPE_VALID_RMS = 1u << 3,
    SCOPE_VALID_VPP = 1u << 4,
    SCOPE_VALID_FREQUENCY = 1u << 5,
    SCOPE_VALID_PERIOD = 1u << 6,
    SCOPE_VALID_DUTY = 1u << 7,
    SCOPE_VALID_RISE = 1u << 8,
    SCOPE_VALID_FALL = 1u << 9
};
typedef struct {
    uint32_t valid;
    float min_mv, max_mv, mean_mv, rms_mv, vpp_mv;
    float frequency_hz, period_s, duty_percent, rise_s, fall_s;
} scope_measurements_t;

typedef enum { SCOPE_EDGE_RISING, SCOPE_EDGE_FALLING } scope_edge_t;
typedef struct {
    float level_mv;
    scope_edge_t edge;
    size_t pre_samples;
    size_t window_samples;
} scope_trigger_t;
typedef struct {
    int found;
    size_t index;
    size_t start;
} scope_trigger_result_t;

typedef enum {
    SCOPE_WAVE_SINE, SCOPE_WAVE_SQUARE, SCOPE_WAVE_TRIANGLE,
    SCOPE_WAVE_SAW, SCOPE_WAVE_DC, SCOPE_WAVE_PULSE, SCOPE_WAVE_TRAPEZOID,
    SCOPE_WAVE_RECTIFIED, SCOPE_WAVE_EXPONENTIAL, SCOPE_WAVE_SINC, SCOPE_WAVE_HARMONIC
} scope_waveform_t;
typedef struct {
    scope_waveform_t waveform;
    float frequency_hz;
    float amplitude_mv; /* 峰峰值；直流必须为 0。 */
    float offset_mv;
    float duty_percent;
    float harmonic_percent[SCOPE_DAC_HARMONICS];
    float harmonic_phase_deg[SCOPE_DAC_HARMONICS];
} scope_dac_config_t;

/* 返回 1 成功 / 0 参数或数据无效；输出不因输入范围越界而静默钳位。
 * ADC 换算与触发查找允许两帧拼接（最多 SCOPE_CAPTURE_MAX_LENGTH）。
 * 显示变换、测量和 FFT 以 SCOPE_MAX_RECORD_LENGTH 为上限。 */
int scope_convert_adc(const uint16_t *raw, size_t count,
                      const scope_calibration_t *calibration, float *mv);
int scope_transform(const float *input, size_t count,
                    const scope_display_t *display, float *output);
/* 仅显示用：均匀选取原始点或局部保形Hermite插值，不改变测量样本。输出至少2点。 */
int scope_resample_view(const float *input,size_t total,size_t start,size_t count,
                        float *output,size_t points,int fitted);
/* 直流/不足周期等情况：基础量仍有效，无法测量的动态量 NaN 且无对应有效位。 */
int scope_measure(const float *mv, size_t count, float sample_rate_hz,
                  scope_measurements_t *result);
int scope_find_trigger(const float *mv, size_t count,
                       const scope_trigger_t *trigger, scope_trigger_result_t *result);
/* n 必须为 2 次幂且 4..8192，bins 容量至少 n/2+1，幅值单位 mV。 */
int scope_fft(const float *mv, size_t n, float *bins, size_t bin_capacity);
/* 完整周期表，12 位无符号码；停止/关闭输出由硬件驱动处理。 */
int scope_make_dac(const scope_dac_config_t *config, float vref_mv,
                   uint16_t *table, size_t count);
/* 非合成波返回1；合成波返回最高非零谐波次数，参数非法返回0。 */
unsigned scope_dac_highest_harmonic(const scope_dac_config_t *config);

#ifdef __cplusplus
}
#endif
#endif
