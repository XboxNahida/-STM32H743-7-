#ifndef FFT_DDC_FREQUENCY_H
#define FFT_DDC_FREQUENCY_H

#include <stdint.h>

#include "scope_reference_dsp.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PPC分步窄带扫描：准备返回1；每次step最多一个候选的八次2048点投影。 */
int scope_reference_sweep_step(void);

typedef enum {
    FFT_DDC_FREQ_OK = 0,
    FFT_DDC_FREQ_BAD_ARG = -1,
    FFT_DDC_FREQ_FFT_INIT_FAIL = -2,
    FFT_DDC_FREQ_NO_VALID_BIN = -3,
    FFT_DDC_FREQ_SIGNAL_TOO_SMALL = -4,
    FFT_DDC_FREQ_ATAN2_FAIL = -5
} fft_ddc_freq_status_t;

typedef struct {
    arm_rfft_fast_instance_f32 rfft;
    uint32_t fftSize;
    float32_t *fftInput;
    float32_t *fftOutput;
    float32_t minFreqHz;
    float32_t maxFreqHz;
    float32_t ddcLowpassCutHz;
    uint32_t ddcSettleSamples;
    uint8_t useHannWindow;
} fft_ddc_freq_ctx_t;

typedef struct {
    float32_t freqHz;
    float32_t coarseFreqHz;
    float32_t residualFreqHz;
    float32_t binHz;
    float32_t peakInterpBin;
    uint32_t peakBin;
    float32_t peakPower;
    float32_t dcMean;
    float32_t normFreqNyquist;
} fft_ddc_freq_result_t;
int scope_reference_single_sweep(fft_ddc_freq_ctx_t *ctx,const float *rate,
    const float *signal,fft_ddc_freq_result_t *result);

typedef struct {
    float32_t acceptedFreqHz;
    float32_t pendingFreqHz;
    uint32_t pendingCount;
    uint8_t initialized;
} fft_ddc_frequency_tracker_t;

fft_ddc_freq_status_t fft_ddc_frequency_init_f32(
    fft_ddc_freq_ctx_t *ctx,
    uint32_t fftSize,
    float32_t *fftInput,
    float32_t *fftOutput);

fft_ddc_freq_status_t fft_ddc_frequency_estimate_f32(
    fft_ddc_freq_ctx_t *ctx,
    const float32_t *sampleRateHz,
    const float32_t *sampleSignal,
    fft_ddc_freq_result_t *result);

/*
 * Select the base frequency from a harmonic family before the narrow-band DDC
 * sweep. A valid candidate requires local FFT peaks at the fundamental and at
 * least one integer multiple; the largest individual FFT peak is not assumed
 * to be the fundamental.
 */
fft_ddc_freq_status_t fft_ddc_frequency_estimate_harmonic_fundamental_f32(
    fft_ddc_freq_ctx_t *ctx,
    const float32_t *sampleRateHz,
    const float32_t *sampleSignal,
    float32_t minFundamentalHz,
    float32_t maxFundamentalHz,
    uint32_t maxHarmonicOrder,
    fft_ddc_freq_result_t *result);

void fft_ddc_frequency_tracker_init(
    fft_ddc_frequency_tracker_t *tracker);

int fft_ddc_frequency_tracker_update_f32(
    fft_ddc_frequency_tracker_t *tracker,
    float32_t candidateFreqHz,
    float32_t relativeJumpThreshold,
    uint32_t confirmFrames,
    float32_t *selectedFreqHz);

float32_t fft_ddc_frequency_smooth_f32(
    float32_t newFreqHz,
    float32_t alpha,
    float32_t *stateFreqHz);

#ifdef __cplusplus
}
#endif

#endif


