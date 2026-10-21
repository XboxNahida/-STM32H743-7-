#include "scope_reference_frequency.h"

#include <math.h>
#include <stddef.h>

#ifndef FFT_DDC_PI_F
#define FFT_DDC_PI_F 3.14159265358979323846f
#endif

#ifndef FFT_DDC_TWO_PI_F
#define FFT_DDC_TWO_PI_F 6.28318530717958647692f
#endif

#ifndef FFT_DDC_MIN_POWER
#define FFT_DDC_MIN_POWER 1.0e-18f
#endif

#define FFT_DDC_HARMONIC_SEARCH_BINS 2U
#define FFT_DDC_NARROWBAND_SWEEP_STEPS 33U
#define FFT_DDC_FUNDAMENTAL_RELATIVE_PEAK_POWER 2.0e-4f
#define FFT_DDC_HARMONIC_RELATIVE_PEAK_POWER 1.0e-7f
#define FFT_DDC_BLACKMAN_HARRIS_A0 0.35875f
#define FFT_DDC_BLACKMAN_HARRIS_A1 0.48829f
#define FFT_DDC_BLACKMAN_HARRIS_A2 0.14128f
#define FFT_DDC_BLACKMAN_HARRIS_A3 0.01168f

static fft_ddc_freq_ctx_t sweepCtx;
static float sweepRate, sweepCoarse, sweepBin;
static uint32_t sweepMask, sweepIndex;
static fft_ddc_freq_result_t *sweepResult;
static int sweepRunning;

static int finite_f32(float32_t x)
{
    return isfinite((double)x);
}

static int positive_finite_f32(float32_t x)
{
    return finite_f32(x) && (x > 0.0f);
}

static float32_t abs_f32(float32_t x)
{
    return (x < 0.0f) ? -x : x;
}

static float32_t clamp_f32(float32_t x, float32_t lo, float32_t hi)
{
    if (x < lo) {
        return lo;
    }
    if (x > hi) {
        return hi;
    }
    return x;
}

static float32_t mean_f32(const float32_t *x, uint32_t count)
{
    float32_t sum = 0.0f;
    uint32_t index;

    for (index = 0U; index < count; ++index) {
        sum += x[index];
    }
    return sum / (float32_t)count;
}

static float32_t rfft_bin_power_f32(const float32_t *fftOutput, uint32_t bin)
{
    float32_t real = fftOutput[2U * bin];
    float32_t imaginary = fftOutput[(2U * bin) + 1U];

    return (real * real) + (imaginary * imaginary);
}

static float32_t parabolic_peak_delta_f32(float32_t left, float32_t mid,
                                           float32_t right)
{
    float32_t denominator = left - (2.0f * mid) + right;
    float32_t delta;

    if (!finite_f32(denominator) || (abs_f32(denominator) < 1.0e-30f)) {
        return 0.0f;
    }
    delta = 0.5f * (left - right) / denominator;
    return clamp_f32(delta, -0.5f, 0.5f);
}

static void build_fft_input_f32(fft_ddc_freq_ctx_t *ctx,
                                const float32_t *sampleSignal,
                                float32_t dcMean)
{
    uint32_t index;
    uint32_t lastIndex = ctx->fftSize - 1U;

    for (index = 0U; index < ctx->fftSize; ++index) {
        float32_t sample = sampleSignal[index] - dcMean;

        if (ctx->useHannWindow != 0U) {
            float32_t window = 0.5f - 0.5f * scope_ref_cos_f32(
                FFT_DDC_TWO_PI_F * (float32_t)index / (float32_t)lastIndex);
            sample *= window;
        }
        ctx->fftInput[index] = sample;
    }
}

static fft_ddc_freq_status_t build_spectrum_f32(fft_ddc_freq_ctx_t *ctx,
                                                 const float32_t *sampleSignal,
                                                 float32_t *dcMean)
{
    if ((ctx == NULL) || (sampleSignal == NULL) || (dcMean == NULL) ||
        (ctx->fftInput == NULL) || (ctx->fftOutput == NULL)) {
        return FFT_DDC_FREQ_BAD_ARG;
    }

    *dcMean = mean_f32(sampleSignal, ctx->fftSize);
    build_fft_input_f32(ctx, sampleSignal, *dcMean);
    scope_ref_rfft_fast_f32(&ctx->rfft, ctx->fftInput, ctx->fftOutput, 0U);
    return FFT_DDC_FREQ_OK;
}

static void build_harmonic_fft_input_f32(
    fft_ddc_freq_ctx_t *ctx,
    const float32_t *sampleSignal,
    float32_t dcMean)
{
    uint32_t index;
    float32_t phaseStep;
    float32_t stepCos1;
    float32_t stepSin1;
    float32_t stepCos2;
    float32_t stepSin2;
    float32_t stepCos3;
    float32_t stepSin3;
    float32_t phaseCos1 = 1.0f;
    float32_t phaseSin1 = 0.0f;
    float32_t phaseCos2 = 1.0f;
    float32_t phaseSin2 = 0.0f;
    float32_t phaseCos3 = 1.0f;
    float32_t phaseSin3 = 0.0f;

    phaseStep = FFT_DDC_TWO_PI_F / (float32_t)(ctx->fftSize - 1U);
    stepCos1 = scope_ref_cos_f32(phaseStep);
    stepSin1 = scope_ref_sin_f32(phaseStep);
    stepCos2 = scope_ref_cos_f32(2.0f * phaseStep);
    stepSin2 = scope_ref_sin_f32(2.0f * phaseStep);
    stepCos3 = scope_ref_cos_f32(3.0f * phaseStep);
    stepSin3 = scope_ref_sin_f32(3.0f * phaseStep);
    for (index = 0U; index < ctx->fftSize; ++index) {
        float32_t window = FFT_DDC_BLACKMAN_HARRIS_A0 -
                           (FFT_DDC_BLACKMAN_HARRIS_A1 * phaseCos1) +
                           (FFT_DDC_BLACKMAN_HARRIS_A2 * phaseCos2) -
                           (FFT_DDC_BLACKMAN_HARRIS_A3 * phaseCos3);
        float32_t nextCos;

        ctx->fftInput[index] = (sampleSignal[index] - dcMean) * window;

        nextCos = (phaseCos1 * stepCos1) - (phaseSin1 * stepSin1);
        phaseSin1 = (phaseSin1 * stepCos1) + (phaseCos1 * stepSin1);
        phaseCos1 = nextCos;
        nextCos = (phaseCos2 * stepCos2) - (phaseSin2 * stepSin2);
        phaseSin2 = (phaseSin2 * stepCos2) + (phaseCos2 * stepSin2);
        phaseCos2 = nextCos;
        nextCos = (phaseCos3 * stepCos3) - (phaseSin3 * stepSin3);
        phaseSin3 = (phaseSin3 * stepCos3) + (phaseCos3 * stepSin3);
        phaseCos3 = nextCos;
    }
}

static fft_ddc_freq_status_t build_harmonic_spectrum_f32(
    fft_ddc_freq_ctx_t *ctx,
    const float32_t *sampleSignal,
    float32_t *dcMean)
{
    if ((ctx == NULL) || (sampleSignal == NULL) || (dcMean == NULL) ||
        (ctx->fftInput == NULL) || (ctx->fftOutput == NULL)) {
        return FFT_DDC_FREQ_BAD_ARG;
    }

    *dcMean = mean_f32(sampleSignal, ctx->fftSize);
    build_harmonic_fft_input_f32(ctx, sampleSignal, *dcMean);
    scope_ref_rfft_fast_f32(&ctx->rfft, ctx->fftInput, ctx->fftOutput, 0U);
    return FFT_DDC_FREQ_OK;
}

static fft_ddc_freq_status_t estimate_residual_ddc_f32(
    const fft_ddc_freq_ctx_t *ctx,
    const float32_t *sampleRateHz,
    const float32_t *sampleSignal,
    float32_t dcMean,
    float32_t coarseFreqHz,
    float32_t binHz,
    float32_t *residualFreqHz)
{
    uint32_t index;
    uint32_t deltaCount = 0U;
    float32_t sampleRate = *sampleRateHz;
    float32_t lowpassCutHz = ctx->ddcLowpassCutHz;
    float32_t alpha;
    float32_t oscillatorReal = 1.0f;
    float32_t oscillatorImaginary = 0.0f;
    float32_t stepReal;
    float32_t stepImaginary;
    float32_t iLowpass = 0.0f;
    float32_t qLowpass = 0.0f;
    float32_t previousI = 0.0f;
    float32_t previousQ = 0.0f;
    float32_t rotationRealSum = 0.0f;
    float32_t rotationImaginarySum = 0.0f;
    uint32_t settleSamples = ctx->ddcSettleSamples;
    int hasPreviousIq = 0;

    if ((residualFreqHz == NULL) || !positive_finite_f32(coarseFreqHz)) {
        return FFT_DDC_FREQ_BAD_ARG;
    }
    if (!positive_finite_f32(lowpassCutHz)) {
        lowpassCutHz = binHz;
    }
    lowpassCutHz = clamp_f32(lowpassCutHz, 0.25f * binHz, 0.20f * sampleRate);
    alpha = 1.0f - expf(-FFT_DDC_TWO_PI_F * lowpassCutHz / sampleRate);
    alpha = clamp_f32(alpha, 0.0001f, 1.0f);

    if (settleSamples >= (ctx->fftSize - 2U)) {
        settleSamples = ctx->fftSize / 8U;
    }

    stepReal = scope_ref_cos_f32(-FFT_DDC_TWO_PI_F * coarseFreqHz / sampleRate);
    stepImaginary = scope_ref_sin_f32(-FFT_DDC_TWO_PI_F * coarseFreqHz / sampleRate);

    for (index = 0U; index < ctx->fftSize; ++index) {
        float32_t sample = sampleSignal[index] - dcMean;
        float32_t mixI = sample * oscillatorReal;
        float32_t mixQ = sample * oscillatorImaginary;
        float32_t nextReal;

        iLowpass += alpha * (mixI - iLowpass);
        qLowpass += alpha * (mixQ - qLowpass);

        if (index >= settleSamples) {
            float32_t iqPower = (iLowpass * iLowpass) +
                                (qLowpass * qLowpass);

            if (iqPower > FFT_DDC_MIN_POWER) {
                if (hasPreviousIq != 0) {
                    rotationRealSum += (iLowpass * previousI) +
                                       (qLowpass * previousQ);
                    rotationImaginarySum += (qLowpass * previousI) -
                                              (iLowpass * previousQ);
                    ++deltaCount;
                }
                previousI = iLowpass;
                previousQ = qLowpass;
                hasPreviousIq = 1;
            }
        }

        nextReal = (oscillatorReal * stepReal) -
                   (oscillatorImaginary * stepImaginary);
        oscillatorImaginary = (oscillatorReal * stepImaginary) +
                              (oscillatorImaginary * stepReal);
        oscillatorReal = nextReal;
    }

    if (deltaCount == 0U) {
        return FFT_DDC_FREQ_SIGNAL_TOO_SMALL;
    }

    {
        float32_t phaseDelta;

        if (scope_ref_atan2_f32(rotationImaginarySum, rotationRealSum,
                          &phaseDelta) != ARM_MATH_SUCCESS) {
            return FFT_DDC_FREQ_ATAN2_FAIL;
        }
        *residualFreqHz = phaseDelta * sampleRate / FFT_DDC_TWO_PI_F;
    }

    return finite_f32(*residualFreqHz) ? FFT_DDC_FREQ_OK : FFT_DDC_FREQ_BAD_ARG;
}

static float32_t ddc_tone_power_f32(const fft_ddc_freq_ctx_t *ctx,
                                    const float32_t *sampleRateHz,
                                    float32_t frequencyHz)
{
    uint32_t index;
    float32_t oscillatorReal = 1.0f;
    float32_t oscillatorImaginary = 0.0f;
    float32_t stepReal;
    float32_t stepImaginary;
    float32_t sumI = 0.0f;
    float32_t sumQ = 0.0f;

    stepReal = scope_ref_cos_f32(-FFT_DDC_TWO_PI_F * frequencyHz / *sampleRateHz);
    stepImaginary = scope_ref_sin_f32(-FFT_DDC_TWO_PI_F * frequencyHz /
                                *sampleRateHz);
    for (index = 0U; index < ctx->fftSize; ++index) {
        float32_t sample = ctx->fftInput[index];
        float32_t nextReal;

        sumI += sample * oscillatorReal;
        sumQ += sample * oscillatorImaginary;
        nextReal = (oscillatorReal * stepReal) -
                   (oscillatorImaginary * stepImaginary);
        oscillatorImaginary = (oscillatorReal * stepImaginary) +
                              (oscillatorImaginary * stepReal);
        oscillatorReal = nextReal;
    }
    return (sumI * sumI) + (sumQ * sumQ);
}

static float32_t harmonic_family_tone_power_f32(
    const fft_ddc_freq_ctx_t *ctx,
    const float32_t *sampleRateHz,
    float32_t fundamentalFrequencyHz,
    uint32_t harmonicMask)
{
    float32_t totalPower = 0.0f;
    uint32_t harmonicOrder;

    for (harmonicOrder = 1U; harmonicOrder <= 32U; ++harmonicOrder) {
        float32_t harmonicFrequencyHz;

        if ((harmonicMask & (1UL << (harmonicOrder - 1U))) == 0U) {
            continue;
        }
        harmonicFrequencyHz = fundamentalFrequencyHz *
                              (float32_t)harmonicOrder;
        if (harmonicFrequencyHz >= (0.5f * *sampleRateHz)) {
            break;
        }
        totalPower += ddc_tone_power_f32(ctx, sampleRateHz,
                                         harmonicFrequencyHz);
    }
    return totalPower;
}

static fft_ddc_freq_status_t estimate_narrowband_ddc_sweep_f32(
    const fft_ddc_freq_ctx_t *ctx,
    const float32_t *sampleRateHz,
    float32_t coarseFreqHz,
    float32_t binHz,
    uint32_t harmonicMask,
    fft_ddc_freq_result_t *result)
{
    float32_t startFrequency;
    float32_t stopFrequency;
    float32_t stepHz;
    static float32_t power[FFT_DDC_NARROWBAND_SWEEP_STEPS];
    uint32_t index;
    uint32_t bestIndex = 0U;

    /* 首次只保存任务；step重入原实现并将循环限制为一个候选。 */
    if (!sweepRunning) {
        sweepCtx=*ctx; sweepRate=*sampleRateHz; sweepCoarse=coarseFreqHz;
        sweepBin=binHz; sweepMask=harmonicMask; sweepResult=result;
        sweepIndex=0U;
        return (fft_ddc_freq_status_t)1;
    }

    if ((ctx == NULL) || (sampleRateHz == NULL) || (result == NULL) ||
        !positive_finite_f32(coarseFreqHz) ||
        !positive_finite_f32(binHz)) {
        return FFT_DDC_FREQ_BAD_ARG;
    }

    startFrequency = coarseFreqHz - binHz;
    stopFrequency = coarseFreqHz + binHz;
    if (startFrequency < ctx->minFreqHz) {
        startFrequency = ctx->minFreqHz;
    }
    if (stopFrequency > ctx->maxFreqHz) {
        stopFrequency = ctx->maxFreqHz;
    }
    if ((startFrequency >= stopFrequency) ||
        !positive_finite_f32(startFrequency) ||
        !positive_finite_f32(stopFrequency)) {
        return FFT_DDC_FREQ_NO_VALID_BIN;
    }

    stepHz = (stopFrequency - startFrequency) /
             (float32_t)(FFT_DDC_NARROWBAND_SWEEP_STEPS - 1U);
    for (index = sweepIndex; index <= sweepIndex; ++index) {
        float32_t frequency = startFrequency + ((float32_t)index * stepHz);

        power[index] = harmonic_family_tone_power_f32(
            ctx, sampleRateHz, frequency, harmonicMask);
    }
    ++sweepIndex;
    if(sweepIndex<FFT_DDC_NARROWBAND_SWEEP_STEPS) return (fft_ddc_freq_status_t)1;
    for(index=1U;index<FFT_DDC_NARROWBAND_SWEEP_STEPS;++index)
        if(power[index]>power[bestIndex]) bestIndex=index;
    if (power[bestIndex] <= FFT_DDC_MIN_POWER) {
        return FFT_DDC_FREQ_SIGNAL_TOO_SMALL;
    }

    result->freqHz = startFrequency + ((float32_t)bestIndex * stepHz);
    if ((bestIndex > 0U) &&
        (bestIndex < (FFT_DDC_NARROWBAND_SWEEP_STEPS - 1U))) {
        result->freqHz += parabolic_peak_delta_f32(power[bestIndex - 1U],
                                                   power[bestIndex],
                                                   power[bestIndex + 1U]) *
                          stepHz;
    }
    result->residualFreqHz = result->freqHz - coarseFreqHz;
    result->normFreqNyquist = 2.0f * result->freqHz / *sampleRateHz;
    return finite_f32(result->freqHz) ? FFT_DDC_FREQ_OK : FFT_DDC_FREQ_BAD_ARG;
}

int scope_reference_sweep_step(void)
{
    fft_ddc_freq_status_t status;
    sweepRunning=1;
    status=estimate_narrowband_ddc_sweep_f32(&sweepCtx,&sweepRate,
        sweepCoarse,sweepBin,sweepMask,sweepResult);
    sweepRunning=0;
    return (int)status;
}

int scope_reference_single_sweep(fft_ddc_freq_ctx_t *ctx,const float *rate,
    const float *signal,fft_ddc_freq_result_t *result)
{
    build_harmonic_fft_input_f32(ctx,signal,result->dcMean);
    return (int)estimate_narrowband_ddc_sweep_f32(ctx,rate,result->freqHz,
        result->binHz,1U,result);
}

static float32_t local_peak_power_f32(const float32_t *fftOutput,
                                      uint32_t centerBin,
                                      uint32_t firstBin,
                                      uint32_t lastBin,
                                      uint32_t *peakBin)
{
    uint32_t begin = centerBin;
    uint32_t end = centerBin;
    uint32_t bin;
    float32_t bestPower = 0.0f;

    if (centerBin > FFT_DDC_HARMONIC_SEARCH_BINS) {
        begin = centerBin - FFT_DDC_HARMONIC_SEARCH_BINS;
    }
    if (begin < firstBin) {
        begin = firstBin;
    }
    if (centerBin <= (UINT32_MAX - FFT_DDC_HARMONIC_SEARCH_BINS)) {
        end = centerBin + FFT_DDC_HARMONIC_SEARCH_BINS;
    }
    if (end > lastBin) {
        end = lastBin;
    }

    for (bin = begin; bin <= end; ++bin) {
        float32_t power = rfft_bin_power_f32(fftOutput, bin);

        if (power > bestPower) {
            bestPower = power;
            if (peakBin != NULL) {
                *peakBin = bin;
            }
        }
    }
    return bestPower;
}

static int spectrum_bin_is_local_peak_f32(const float32_t *fftOutput,
                                          uint32_t bin,
                                          uint32_t firstBin,
                                          uint32_t lastBin)
{
    float32_t power;

    if ((fftOutput == NULL) || (bin <= firstBin) || (bin >= lastBin)) {
        return 0;
    }

    power = rfft_bin_power_f32(fftOutput, bin);
    return (power > rfft_bin_power_f32(fftOutput, bin - 1U)) &&
           (power > rfft_bin_power_f32(fftOutput, bin + 1U));
}

fft_ddc_freq_status_t fft_ddc_frequency_init_f32(
    fft_ddc_freq_ctx_t *ctx,
    uint32_t fftSize,
    float32_t *fftInput,
    float32_t *fftOutput)
{
    if ((ctx == NULL) || (fftInput == NULL) || (fftOutput == NULL) ||
        (fftSize < 32U) || (fftSize > UINT16_MAX)) {
        return FFT_DDC_FREQ_BAD_ARG;
    }
    if (scope_ref_rfft_fast_init_f32(&ctx->rfft, (uint16_t)fftSize) !=
        ARM_MATH_SUCCESS) {
        return FFT_DDC_FREQ_FFT_INIT_FAIL;
    }

    ctx->fftSize = fftSize;
    ctx->fftInput = fftInput;
    ctx->fftOutput = fftOutput;
    ctx->minFreqHz = 0.0f;
    ctx->maxFreqHz = 0.0f;
    ctx->ddcLowpassCutHz = 0.0f;
    ctx->ddcSettleSamples = fftSize / 8U;
    ctx->useHannWindow = 1U;
    return FFT_DDC_FREQ_OK;
}

fft_ddc_freq_status_t fft_ddc_frequency_estimate_f32(
    fft_ddc_freq_ctx_t *ctx,
    const float32_t *sampleRateHz,
    const float32_t *sampleSignal,
    fft_ddc_freq_result_t *result)
{
    uint32_t bin;
    uint32_t firstBin;
    uint32_t lastBin;
    uint32_t peakBin = 0U;
    uint32_t nyquistBin;
    float32_t sampleRate;
    float32_t binHz;
    float32_t dcMean;
    float32_t minFreq;
    float32_t maxFreq;
    float32_t peakPower = 0.0f;
    float32_t peakDelta = 0.0f;
    float32_t peakInterpBin;
    float32_t coarseFreqHz;
    float32_t residualFreqHz = 0.0f;
    fft_ddc_freq_status_t status;

    if ((ctx == NULL) || (sampleRateHz == NULL) || (sampleSignal == NULL) ||
        (result == NULL) || (ctx->fftInput == NULL) ||
        (ctx->fftOutput == NULL)) {
        return FFT_DDC_FREQ_BAD_ARG;
    }

    sampleRate = *sampleRateHz;
    if (!positive_finite_f32(sampleRate) || (ctx->fftSize < 32U)) {
        return FFT_DDC_FREQ_BAD_ARG;
    }

    binHz = sampleRate / (float32_t)ctx->fftSize;
    nyquistBin = ctx->fftSize / 2U;
    minFreq = ctx->minFreqHz;
    maxFreq = ctx->maxFreqHz;
    if (!finite_f32(minFreq) || (minFreq < 0.0f)) {
        minFreq = 0.0f;
    }
    if (!positive_finite_f32(maxFreq) || (maxFreq > (0.5f * sampleRate))) {
        maxFreq = 0.5f * sampleRate;
    }

    firstBin = (uint32_t)ceilf(minFreq / binHz);
    if (firstBin < 1U) {
        firstBin = 1U;
    }
    lastBin = (uint32_t)floorf(maxFreq / binHz);
    if (lastBin >= nyquistBin) {
        lastBin = nyquistBin - 1U;
    }
    if (firstBin > lastBin) {
        return FFT_DDC_FREQ_NO_VALID_BIN;
    }

    status = build_spectrum_f32(ctx, sampleSignal, &dcMean);
    if (status != FFT_DDC_FREQ_OK) {
        return status;
    }
    for (bin = firstBin; bin <= lastBin; ++bin) {
        float32_t power = rfft_bin_power_f32(ctx->fftOutput, bin);

        if (power > peakPower) {
            peakPower = power;
            peakBin = bin;
        }
    }
    if ((peakBin == 0U) || (peakPower <= FFT_DDC_MIN_POWER)) {
        return FFT_DDC_FREQ_SIGNAL_TOO_SMALL;
    }

    if ((peakBin > firstBin) && (peakBin < lastBin)) {
        float32_t left = rfft_bin_power_f32(ctx->fftOutput, peakBin - 1U);
        float32_t right = rfft_bin_power_f32(ctx->fftOutput, peakBin + 1U);

        peakDelta = parabolic_peak_delta_f32(left, peakPower, right);
    }
    peakInterpBin = (float32_t)peakBin + peakDelta;
    coarseFreqHz = peakInterpBin * binHz;

    status = estimate_residual_ddc_f32(ctx, sampleRateHz, sampleSignal,
                                       dcMean, coarseFreqHz, binHz,
                                       &residualFreqHz);
    if (status != FFT_DDC_FREQ_OK) {
        return status;
    }

    result->coarseFreqHz = coarseFreqHz;
    result->residualFreqHz = residualFreqHz;
    result->freqHz = clamp_f32(coarseFreqHz + residualFreqHz, 0.0f,
                               0.5f * sampleRate);
    result->binHz = binHz;
    result->peakInterpBin = peakInterpBin;
    result->peakBin = peakBin;
    result->peakPower = peakPower;
    result->dcMean = dcMean;
    result->normFreqNyquist = 2.0f * result->freqHz / sampleRate;
    return FFT_DDC_FREQ_OK;
}

fft_ddc_freq_status_t fft_ddc_frequency_estimate_harmonic_fundamental_f32(
    fft_ddc_freq_ctx_t *ctx,
    const float32_t *sampleRateHz,
    const float32_t *sampleSignal,
    float32_t minFundamentalHz,
    float32_t maxFundamentalHz,
    uint32_t maxHarmonicOrder,
    fft_ddc_freq_result_t *result)
{
    uint32_t candidateBin;
    uint32_t firstBin;
    uint32_t lastBin;
    uint32_t nyquistBin;
    uint32_t bestFundamentalBin = 0U;
    uint32_t bestHarmonicMask = 0U;
    float32_t sampleRate;
    float32_t binHz;
    float32_t dcMean;
    float32_t savedMinFreq;
    float32_t savedMaxFreq;
    float32_t globalPeakPower = 0.0f;
    float32_t minimumFundamentalPeakPower;
    float32_t minimumHarmonicPeakPower;
    float32_t bestScore = 0.0f;
    float32_t peakPower;
    float32_t peakDelta = 0.0f;
    float32_t coarseFreqHz;
    fft_ddc_freq_status_t status;

    if ((ctx == NULL) || (sampleRateHz == NULL) || (sampleSignal == NULL) ||
        (result == NULL) || !positive_finite_f32(minFundamentalHz) ||
        !positive_finite_f32(maxFundamentalHz) ||
        (minFundamentalHz >= maxFundamentalHz) ||
        (maxHarmonicOrder < 2U)) {
        return FFT_DDC_FREQ_BAD_ARG;
    }

    sampleRate = *sampleRateHz;
    if (!positive_finite_f32(sampleRate) ||
        (minFundamentalHz >= (0.5f * sampleRate))) {
        return FFT_DDC_FREQ_NO_VALID_BIN;
    }
    if (maxFundamentalHz > (0.5f * sampleRate)) {
        maxFundamentalHz = 0.5f * sampleRate;
    }

    binHz = sampleRate / (float32_t)ctx->fftSize;
    nyquistBin = ctx->fftSize / 2U;
    firstBin = (uint32_t)floorf(minFundamentalHz / binHz);
    if (firstBin < 1U) {
        firstBin = 1U;
    }
    lastBin = (uint32_t)ceilf(maxFundamentalHz / binHz);
    if (lastBin >= nyquistBin) {
        lastBin = nyquistBin - 1U;
    }
    if (firstBin > lastBin) {
        return FFT_DDC_FREQ_NO_VALID_BIN;
    }

    status = build_harmonic_spectrum_f32(ctx, sampleSignal, &dcMean);
    if (status != FFT_DDC_FREQ_OK) {
        return status;
    }

    for (candidateBin = 1U; candidateBin < nyquistBin; ++candidateBin) {
        float32_t power = rfft_bin_power_f32(ctx->fftOutput, candidateBin);

        if (power > globalPeakPower) {
            globalPeakPower = power;
        }
    }
    if (globalPeakPower <= FFT_DDC_MIN_POWER) {
        return FFT_DDC_FREQ_SIGNAL_TOO_SMALL;
    }
    minimumFundamentalPeakPower = globalPeakPower *
                                  FFT_DDC_FUNDAMENTAL_RELATIVE_PEAK_POWER;
    minimumHarmonicPeakPower = globalPeakPower *
                                FFT_DDC_HARMONIC_RELATIVE_PEAK_POWER;

    for (candidateBin = firstBin; candidateBin <= lastBin; ++candidateBin) {
        uint32_t harmonicOrder;
        uint32_t harmonicCount = 0U;
        uint32_t harmonicMask = 1U;
        float32_t fundamentalPower;
        float32_t harmonicPowerSum = 0.0f;
        float32_t score;

        if (spectrum_bin_is_local_peak_f32(ctx->fftOutput, candidateBin,
                                           1U, nyquistBin - 1U) == 0) {
            continue;
        }
        fundamentalPower = rfft_bin_power_f32(ctx->fftOutput, candidateBin);
        if (fundamentalPower < minimumFundamentalPeakPower) {
            continue;
        }
        for (harmonicOrder = 2U; harmonicOrder <= maxHarmonicOrder;
             ++harmonicOrder) {
            uint32_t harmonicBin;
            uint32_t harmonicPeakBin = 0U;
            float32_t harmonicPower;

            if (candidateBin > ((nyquistBin - 1U) / harmonicOrder)) {
                break;
            }
            harmonicBin = candidateBin * harmonicOrder;
            harmonicPower = local_peak_power_f32(
                ctx->fftOutput, harmonicBin, 1U, nyquistBin - 1U,
                &harmonicPeakBin);
            if ((harmonicPeakBin != 0U) &&
                (spectrum_bin_is_local_peak_f32(ctx->fftOutput,
                                                harmonicPeakBin, 1U,
                                                nyquistBin - 1U) != 0) &&
                (harmonicPower >= minimumHarmonicPeakPower)) {
                harmonicPowerSum += harmonicPower;
                ++harmonicCount;
                if (harmonicOrder <= 32U) {
                    harmonicMask |= 1UL << (harmonicOrder - 1U);
                }
            }
        }
        score = fundamentalPower + harmonicPowerSum;
        if ((harmonicCount != 0U) && (score > bestScore)) {
            /* Score the complete observable harmonic family. This prevents a
             * strong 2f component from outranking its weaker true base. */
            bestFundamentalBin = candidateBin;
            bestHarmonicMask = harmonicMask;
            bestScore = score;
        }
    }

    if (bestFundamentalBin == 0U) {
        return FFT_DDC_FREQ_NO_VALID_BIN;
    }

    savedMinFreq = ctx->minFreqHz;
    savedMaxFreq = ctx->maxFreqHz;
    ctx->minFreqHz = (bestFundamentalBin > FFT_DDC_HARMONIC_SEARCH_BINS) ?
        ((float32_t)(bestFundamentalBin - FFT_DDC_HARMONIC_SEARCH_BINS) * binHz) :
        binHz;
    ctx->maxFreqHz = (float32_t)(bestFundamentalBin +
                                  FFT_DDC_HARMONIC_SEARCH_BINS) * binHz;
    peakPower = rfft_bin_power_f32(ctx->fftOutput, bestFundamentalBin);
    if ((bestFundamentalBin > firstBin) &&
        (bestFundamentalBin < lastBin)) {
        peakDelta = parabolic_peak_delta_f32(
            rfft_bin_power_f32(ctx->fftOutput, bestFundamentalBin - 1U),
            peakPower,
            rfft_bin_power_f32(ctx->fftOutput, bestFundamentalBin + 1U));
    }
    result->peakBin = bestFundamentalBin;
    result->peakPower = peakPower;
    result->peakInterpBin = (float32_t)bestFundamentalBin + peakDelta;
    result->binHz = binHz;
    result->dcMean = dcMean;
    coarseFreqHz = result->peakInterpBin * binHz;
    result->coarseFreqHz = coarseFreqHz;
    /* CMSIS fast RFFT uses fftInput as in-place working memory. Rebuild the
     * windowed time-domain samples before the narrow-band projection sweep. */
    build_harmonic_fft_input_f32(ctx, sampleSignal, dcMean);
    status = estimate_narrowband_ddc_sweep_f32(
        ctx, sampleRateHz, coarseFreqHz, binHz, bestHarmonicMask, result);
    ctx->minFreqHz = savedMinFreq;
    ctx->maxFreqHz = savedMaxFreq;
    if (status == FFT_DDC_FREQ_OK) {
        result->freqHz = clamp_f32(result->freqHz, minFundamentalHz,
                                   maxFundamentalHz);
        result->residualFreqHz = result->freqHz - result->coarseFreqHz;
        result->normFreqNyquist = 2.0f * result->freqHz / sampleRate;
    }
    return status;
}

void fft_ddc_frequency_tracker_init(
    fft_ddc_frequency_tracker_t *tracker)
{
    if (tracker == NULL) {
        return;
    }

    tracker->acceptedFreqHz = 0.0f;
    tracker->pendingFreqHz = 0.0f;
    tracker->pendingCount = 0U;
    tracker->initialized = 0U;
}

int fft_ddc_frequency_tracker_update_f32(
    fft_ddc_frequency_tracker_t *tracker,
    float32_t candidateFreqHz,
    float32_t relativeJumpThreshold,
    uint32_t confirmFrames,
    float32_t *selectedFreqHz)
{
    float32_t relativeDifference;

    if ((tracker == NULL) || (selectedFreqHz == NULL)) {
        return 0;
    }
    if (!positive_finite_f32(candidateFreqHz) ||
        !finite_f32(relativeJumpThreshold) ||
        (relativeJumpThreshold < 0.0f) || (confirmFrames == 0U)) {
        tracker->pendingFreqHz = 0.0f;
        tracker->pendingCount = 0U;
        *selectedFreqHz = (tracker->initialized != 0U) ?
            tracker->acceptedFreqHz : 0.0f;
        return 0;
    }

    if (tracker->initialized == 0U) {
        tracker->acceptedFreqHz = candidateFreqHz;
        tracker->pendingFreqHz = 0.0f;
        tracker->pendingCount = 0U;
        tracker->initialized = 1U;
        *selectedFreqHz = candidateFreqHz;
        return 1;
    }

    relativeDifference = abs_f32(candidateFreqHz -
                                 tracker->acceptedFreqHz) /
                         tracker->acceptedFreqHz;
    if (relativeDifference <= relativeJumpThreshold) {
        tracker->acceptedFreqHz = candidateFreqHz;
        tracker->pendingFreqHz = 0.0f;
        tracker->pendingCount = 0U;
        *selectedFreqHz = candidateFreqHz;
        return 1;
    }

    if ((tracker->pendingCount == 0U) ||
        ((abs_f32(candidateFreqHz - tracker->pendingFreqHz) /
          tracker->pendingFreqHz) > relativeJumpThreshold)) {
        tracker->pendingFreqHz = candidateFreqHz;
        tracker->pendingCount = 1U;
    } else {
        tracker->pendingFreqHz = candidateFreqHz;
        ++tracker->pendingCount;
    }

    if (tracker->pendingCount >= confirmFrames) {
        tracker->acceptedFreqHz = candidateFreqHz;
        tracker->pendingFreqHz = 0.0f;
        tracker->pendingCount = 0U;
        *selectedFreqHz = candidateFreqHz;
        return 1;
    }

    *selectedFreqHz = tracker->acceptedFreqHz;
    return 0;
}

float32_t fft_ddc_frequency_smooth_f32(float32_t newFreqHz,
                                       float32_t alpha,
                                       float32_t *stateFreqHz)
{
    float32_t clampedAlpha;

    if ((stateFreqHz == NULL) || !finite_f32(newFreqHz)) {
        return 0.0f;
    }
    clampedAlpha = clamp_f32(alpha, 0.0f, 1.0f);
    *stateFreqHz += clampedAlpha * (newFreqHz - *stateFreqHz);
    return *stateFreqHz;
}


