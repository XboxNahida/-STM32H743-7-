#include "oscilloscope_core.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static unsigned failures;
static float signal_mv[SCOPE_RECORD_LENGTH];
static float processed[SCOPE_RECORD_LENGTH];
static float bins[SCOPE_RECORD_LENGTH / 2u + 1u];
static uint16_t table[SCOPE_RECORD_LENGTH];
static uint16_t capture_raw[2u * SCOPE_RECORD_LENGTH];
static float capture_mv[2u * SCOPE_RECORD_LENGTH];
#define CHECK(test) do { if (!(test)) { printf("FAIL %d: %s\n", __LINE__, #test); ++failures; } } while (0)
#define NEAR(actual, expected, tolerance) CHECK(fabs((double)(actual) - (double)(expected)) <= (tolerance))

static void test_measurement(void)
{
    size_t i;
    scope_measurements_t m;
    for (i = 0; i < 2000u; ++i)
        signal_mv[i] = 1500.0f + 1000.0f * sinf(6.28318530718f * (float)i / 100.0f);
    CHECK(scope_measure(signal_mv, 2000u, 100000.0f, &m));
    NEAR(m.min_mv, 500.0f, 0.1f);
    NEAR(m.max_mv, 2500.0f, 0.1f);
    NEAR(m.mean_mv, 1500.0f, 0.1f);
    NEAR(m.rms_mv, sqrt(2750000.0), 0.1f);
    NEAR(m.frequency_hz, 1000.0f, 0.5f);
    NEAR(m.period_s, 0.001f, 0.000001f);
    NEAR(m.duty_percent, 50.0f, 0.1f);
    CHECK((m.valid & (SCOPE_VALID_FREQUENCY | SCOPE_VALID_RISE | SCOPE_VALID_FALL)) ==
          (SCOPE_VALID_FREQUENCY | SCOPE_VALID_RISE | SCOPE_VALID_FALL));
    NEAR(m.rise_s, (asin(0.8) - asin(-0.8)) / 6283.18530718, 0.000001f);
    NEAR(m.fall_s, m.rise_s, 0.000001f);
    for (i = 0; i < 2000u; ++i) signal_mv[i] = 1234.0f;
    CHECK(scope_measure(signal_mv, 2000u, 100000.0f, &m));
    NEAR(m.rms_mv, 1234.0f, 0.01f);
    CHECK(!(m.valid & SCOPE_VALID_FREQUENCY));
    CHECK(isnan(m.frequency_hz) && isnan(m.duty_percent) && isnan(m.rise_s));
    CHECK(!scope_measure(signal_mv, 2000u, 0.0f, &m));
    CHECK(m.valid == 0u);
    signal_mv[30] = NAN;
    CHECK(!scope_measure(signal_mv, 2000u, 100000.0f, &m));
}

static void test_conversion(void)
{
    uint16_t raw[3] = {0u, 32768u, 65535u};
    scope_calibration_t cal = {3300.0f, 0.0f, 1.0f};
    scope_display_t view = {SCOPE_COUPLING_AC, 10.0f, 1};
    CHECK(scope_convert_adc(raw, 3u, &cal, signal_mv));
    NEAR(signal_mv[0], 0.0f, 0.001f);
    NEAR(signal_mv[1], 1650.025f, 0.01f);
    NEAR(signal_mv[2], 3300.0f, 0.001f);
    cal.zero_code = 100.0f; cal.gain = 1.02f;
    CHECK(scope_convert_adc(raw, 3u, &cal, signal_mv));
    NEAR(signal_mv[0], -100.0f * 3300.0f / 65535.0f * 1.02f, 0.001f);
    signal_mv[0] = 100.0f; signal_mv[1] = 200.0f; signal_mv[2] = 300.0f;
    CHECK(scope_transform(signal_mv, 3u, &view, processed));
    NEAR(processed[0], 1000.0f, 0.001f);
    NEAR(processed[2], -1000.0f, 0.001f);
    view.coupling = SCOPE_COUPLING_GND;
    CHECK(scope_transform(signal_mv, 3u, &view, processed));
    NEAR(processed[1], 0.0f, 0.001f);
    cal.gain = NAN;
    CHECK(!scope_convert_adc(raw, 3u, &cal, signal_mv));
}

static void test_trigger(void)
{
    const float data[12] = {0,0,0,10,10,0,0,0,10,10,10,10};
    scope_trigger_t t = {5.0f, SCOPE_EDGE_RISING, 4u, 8u};
    scope_trigger_result_t r;
    CHECK(scope_find_trigger(data, 12u, &t, &r));
    CHECK(r.found && r.index == 8u && r.start == 4u);
    t.pre_samples = 1u; t.window_samples = 12u;
    CHECK(scope_find_trigger(data, 12u, &t, &r));
    CHECK(!r.found);
    t.edge = SCOPE_EDGE_FALLING; t.pre_samples = 2u; t.window_samples = 6u;
    CHECK(scope_find_trigger(data, 12u, &t, &r));
    CHECK(r.found && r.index == 5u && r.start == 3u);
}

static void test_fft_dac(void)
{
    size_t i, peak = 1u;
    scope_dac_config_t c = {SCOPE_WAVE_SINE, 1000.0f, 2000.0f, 1650.0f, 50.0f,{0},{0}};
    for (i = 0; i < 1024u; ++i)
        signal_mv[i] = 300.0f + 1000.0f * sinf(6.28318530718f * 32.0f * (float)i / 1024.0f);
    CHECK(scope_fft(signal_mv, 1024u, bins, 1025u));
    for (i = 2u; i <= 512u; ++i) if (bins[i] > bins[peak]) peak = i;
    CHECK(peak == 32u);
    NEAR(bins[32], 1000.0f, 1.0f);
    NEAR(bins[0], 300.0f, 0.1f);
    CHECK(!scope_fft(signal_mv, 1000u, bins, 1025u));
    CHECK(scope_make_dac(&c, 3300.0f, table, 100u));
    NEAR(table[25] * 3300.0f / 4095.0f, 2650.0f, 0.5f);
    NEAR(table[75] * 3300.0f / 4095.0f, 650.0f, 0.5f);
    c.amplitude_mv = 3400.0f;
    CHECK(!scope_make_dac(&c, 3300.0f, table, 100u));
    c.amplitude_mv = 2000.0f; c.duty_percent = 0.0f;
    CHECK(!scope_make_dac(&c, 3300.0f, table, 100u));
    c.duty_percent = 25.0f; c.waveform = SCOPE_WAVE_SQUARE;
    CHECK(scope_make_dac(&c, 3300.0f, table, 100u));
    CHECK(table[24] > table[25]);
}

static void test_measurement_boundaries(void)
{
    size_t i;
    scope_measurements_t m;
    for (i = 0u; i < 2000u; ++i) signal_mv[i] = (i % 100u < 25u) ? 2500.0f : 500.0f;
    CHECK(scope_measure(signal_mv, 2000u, 100000.0f, &m));
    NEAR(m.frequency_hz, 1000.0f, 0.01f);
    NEAR(m.duty_percent, 25.0f, 0.01f);
    NEAR(m.mean_mv, 1000.0f, 0.01f);
    NEAR(m.rise_s, 0.000008f, 0.0000001f);
    NEAR(m.fall_s, 0.000008f, 0.0000001f);
    for (i = 0u; i < 2000u; ++i) {
        size_t phase = i % 100u;
        signal_mv[i] = 500.0f + 40.0f * (float)(phase < 50u ? phase : 100u - phase);
    }
    CHECK(scope_measure(signal_mv, 2000u, FLT_MIN, &m));
    CHECK(!(m.valid & SCOPE_VALID_PERIOD));
    CHECK(!(m.valid & SCOPE_VALID_RISE));
    CHECK(!(m.valid & SCOPE_VALID_FALL));
    CHECK(isnan(m.period_s) && isnan(m.rise_s) && isnan(m.fall_s));
    CHECK(scope_measure(signal_mv, 1u, 1000.0f, &m));
    CHECK(!(m.valid & (SCOPE_VALID_PERIOD | SCOPE_VALID_FREQUENCY | SCOPE_VALID_DUTY)));
    for (i = 0u; i < 100u; ++i) signal_mv[i] = i % 2u ? 1.0e-8f : 0.0f;
    CHECK(scope_measure(signal_mv, 100u, 1000.0f, &m));
    CHECK(!(m.valid & SCOPE_VALID_FREQUENCY));
    CHECK(!scope_measure(NULL, 10u, 1000.0f, &m));
    CHECK(!scope_measure(signal_mv, 0u, 1000.0f, &m));
    CHECK(!scope_measure(signal_mv, SCOPE_MAX_RECORD_LENGTH + 1u, 1000.0f, &m));
    CHECK(!scope_measure(signal_mv, 10u, INFINITY, &m));
    CHECK(!scope_measure(signal_mv, 10u, -1.0f, &m));
    CHECK(!scope_measure(signal_mv, 10u, 1000.0f, NULL));
}

static void test_fft_boundaries(void)
{
    size_t i;
    for (i = 0u; i < SCOPE_RECORD_LENGTH; ++i)
        signal_mv[i] = i % 2u ? -100.0f : 100.0f;
    CHECK(scope_fft(signal_mv, SCOPE_RECORD_LENGTH, bins, 1025u));
    NEAR(bins[1024], 100.0f, 0.01f);
    CHECK(!scope_fft(signal_mv, 0u, bins, 1025u));
    CHECK(!scope_fft(signal_mv, 2u, bins, 1025u));
    CHECK(!scope_fft(signal_mv, 4096u, bins, 1025u));
    CHECK(!scope_fft(signal_mv, 1024u, bins, 512u));
    CHECK(!scope_fft(NULL, 1024u, bins, 1025u));
    CHECK(!scope_fft(signal_mv, 1024u, NULL, 1025u));
    signal_mv[0] = INFINITY;
    CHECK(!scope_fft(signal_mv, 1024u, bins, 1025u));
    for (i = 0u; i < 4u; ++i) signal_mv[i] = 20.0f;
    CHECK(scope_fft(signal_mv, 4u, bins, 3u));
    NEAR(bins[0], 20.0f, 0.001f);
}

static void test_dac_boundaries(void)
{
    size_t i;
    scope_waveform_t wave;
    scope_dac_config_t c = {SCOPE_WAVE_SINE, 1000.0f, 3300.0f, 1650.0f, 50.0f,{0},{0}};
    for (wave = SCOPE_WAVE_SINE; wave <= SCOPE_WAVE_SAW; wave = (scope_waveform_t)(wave + 1)) {
        c.waveform = wave;
        CHECK(scope_make_dac(&c, 3300.0f, table, SCOPE_RECORD_LENGTH));
        for (i = 0u; i < SCOPE_RECORD_LENGTH; ++i) CHECK(table[i] <= 4095u);
    }
    c.waveform = SCOPE_WAVE_TRIANGLE;
    CHECK(scope_make_dac(&c, 3300.0f, table, 100u));
    CHECK(table[0] == 0u && table[50] == 4095u);
    c.waveform = SCOPE_WAVE_DC; c.amplitude_mv = 0.0f; c.offset_mv = 3300.0f;
    CHECK(scope_make_dac(&c, 3300.0f, table, 100u));
    CHECK(table[0] == 4095u && table[99] == 4095u);
    c.offset_mv = 0.0f;
    CHECK(scope_make_dac(&c, 3300.0f, table, 100u));
    CHECK(table[0] == 0u && table[99] == 0u);
    table[0] = 123u; c.amplitude_mv = 1.0f;
    CHECK(!scope_make_dac(&c, 3300.0f, table, 100u));
    CHECK(table[0] == 123u);
    c.amplitude_mv = 0.0f; c.frequency_hz = 0.0f;
    CHECK(!scope_make_dac(&c, 3300.0f, table, 100u));
    c.frequency_hz = NAN;
    CHECK(!scope_make_dac(&c, 3300.0f, table, 100u));
    c.frequency_hz = 1000.0f; c.offset_mv = -1.0f;
    CHECK(!scope_make_dac(&c, 3300.0f, table, 100u));
    c.offset_mv = 3301.0f;
    CHECK(!scope_make_dac(&c, 3300.0f, table, 100u));
    c.offset_mv = 1000.0f; c.duty_percent = 100.0f;
    CHECK(!scope_make_dac(&c, 3300.0f, table, 100u));
    c.duty_percent = 50.0f;
    c.waveform = (scope_waveform_t)-1;
    CHECK(!scope_make_dac(&c, 3300.0f, table, 100u));
    c.waveform = (scope_waveform_t)99;
    CHECK(!scope_make_dac(&c, 3300.0f, table, 100u));
    c.waveform = SCOPE_WAVE_DC;
    CHECK(!scope_make_dac(&c, 0.0f, table, 100u));
    CHECK(!scope_make_dac(&c, 3300.0f, table, 1u));
    CHECK(!scope_make_dac(&c, 3300.0f, table, SCOPE_RECORD_LENGTH + 1u));
    CHECK(!scope_make_dac(NULL, 3300.0f, table, 100u));
}

static void test_validation_and_noninteger_period(void)
{
    size_t i;
    uint16_t raw[2] = {100u, 200u};
    scope_calibration_t cal = {3300.0f, 0.0f, 1.0f};
    scope_display_t view = {SCOPE_COUPLING_DC, 1.0f, 0};
    scope_measurements_t m;
    scope_trigger_t t = {5.0f, SCOPE_EDGE_RISING, 0u, 2u};
    scope_trigger_result_t r;
    for (i = 0u; i < SCOPE_RECORD_LENGTH; ++i)
        signal_mv[i] = 1400.0f + 800.0f * sinf(6.28318530718f * 1234.5f * (float)i / 100000.0f);
    CHECK(scope_measure(signal_mv, SCOPE_RECORD_LENGTH, 100000.0f, &m));
    NEAR(m.frequency_hz, 1234.5f, 0.15f);
    NEAR(m.duty_percent, 50.0f, 0.05f);
    CHECK(!scope_convert_adc(NULL, 2u, &cal, processed));
    CHECK(!scope_convert_adc(raw, 0u, &cal, processed));
    CHECK(!scope_convert_adc(raw, SCOPE_CAPTURE_MAX_LENGTH + 1u, &cal, processed));
    cal.zero_code = -1.0f;
    CHECK(!scope_convert_adc(raw, 2u, &cal, processed));
    cal.zero_code = 0.0f; cal.gain = 0.0f;
    CHECK(!scope_convert_adc(raw, 2u, &cal, processed));
    cal.gain = 1.0f; cal.vref_mv = INFINITY;
    CHECK(!scope_convert_adc(raw, 2u, &cal, processed));
    signal_mv[0] = 100.0f; signal_mv[1] = 300.0f;
    CHECK(scope_transform(signal_mv, 2u, &view, processed));
    NEAR(processed[0], 100.0f, 0.001f);
    view.coupling = SCOPE_COUPLING_AC;
    CHECK(scope_transform(signal_mv, 2u, &view, signal_mv));
    NEAR(signal_mv[0], -100.0f, 0.001f);
    NEAR(signal_mv[1], 100.0f, 0.001f);
    view.probe_factor = 0.0f;
    CHECK(!scope_transform(signal_mv, 2u, &view, processed));
    view.probe_factor = NAN;
    CHECK(!scope_transform(signal_mv, 2u, &view, processed));
    view.probe_factor = 1.0f; view.coupling = (scope_coupling_t)-1;
    CHECK(!scope_transform(signal_mv, 2u, &view, processed));
    view.coupling = (scope_coupling_t)99;
    CHECK(!scope_transform(signal_mv, 2u, &view, processed));
    signal_mv[0] = 0.0f; signal_mv[1] = 10.0f;
    CHECK(scope_find_trigger(signal_mv, 2u, &t, &r));
    CHECK(!r.found); /* 触发在最后点，后窗不足。 */
    t.pre_samples = 1u;
    CHECK(scope_find_trigger(signal_mv, 2u, &t, &r));
    CHECK(r.found && r.index == 1u && r.start == 0u);
    t.pre_samples = 2u;
    CHECK(!scope_find_trigger(signal_mv, 2u, &t, &r));
    CHECK(!r.found);
    t.pre_samples = 0u; t.level_mv = NAN;
    CHECK(!scope_find_trigger(signal_mv, 2u, &t, &r));
    t.level_mv = 5.0f; t.edge = (scope_edge_t)99;
    CHECK(!scope_find_trigger(signal_mv, 2u, &t, &r));
    t.edge = SCOPE_EDGE_RISING;
    t.edge = (scope_edge_t)-1;
    CHECK(!scope_find_trigger(signal_mv, 2u, &t, &r));
    t.edge = SCOPE_EDGE_RISING;
    signal_mv[1] = NAN;
    CHECK(!scope_find_trigger(signal_mv, 2u, &t, &r));
}

static void test_trigger_across_capture_boundary(void)
{
    size_t i;
    scope_calibration_t cal = {3300.0f, 0.0f, 1.0f};
    scope_trigger_t t = {1650.0f, SCOPE_EDGE_RISING, 1024u, SCOPE_RECORD_LENGTH};
    scope_trigger_result_t r;
    scope_measurements_t m;
    for (i = 0u; i < 2u * SCOPE_RECORD_LENGTH; ++i)
        capture_raw[i] = i < 2000u ? 0u : 65535u;
    CHECK(scope_convert_adc(capture_raw, 2u * SCOPE_RECORD_LENGTH, &cal, capture_mv));
    NEAR(capture_mv[1999], 0.0f, 0.001f);
    NEAR(capture_mv[2000], 3300.0f, 0.001f);
    CHECK(scope_find_trigger(capture_mv, 2u * SCOPE_RECORD_LENGTH, &t, &r));
    CHECK(r.found && r.index == 2000u && r.start == 976u);
    CHECK(!scope_convert_adc(capture_raw, SCOPE_CAPTURE_MAX_LENGTH + 1u, &cal, capture_mv));
    CHECK(!scope_find_trigger(capture_mv, SCOPE_CAPTURE_MAX_LENGTH + 1u, &t, &r));
    CHECK(!scope_measure(capture_mv, SCOPE_MAX_RECORD_LENGTH+1u, 100000.0f, &m));
}

int main(void)
{
    {
        scope_dac_config_t function_wave={0};uint16_t samples[1024];unsigned wave;
        function_wave.frequency_hz=1000.0f;function_wave.amplitude_mv=2000.0f;
        function_wave.offset_mv=1650.0f;function_wave.duty_percent=30.0f;
        for(wave=5U;wave<=9U;++wave){
            function_wave.waveform=(scope_waveform_t)wave;
            CHECK(scope_make_dac(&function_wave,3300.0f,samples,1024U));
            {size_t j;uint16_t lo=4095U,hi=0U;
             for(j=0U;j<1024U;++j){CHECK(samples[j]<=4095U);
                if(samples[j]<lo)lo=samples[j];
                if(samples[j]>hi)hi=samples[j];}
             CHECK((unsigned)(hi-lo)>2400U);}
        }
        function_wave.waveform=SCOPE_WAVE_HARMONIC;
        function_wave.harmonic_percent[0]=100.0f;function_wave.harmonic_percent[2]=33.0f;
        function_wave.harmonic_phase_deg[2]=90.0f;
        CHECK(scope_dac_highest_harmonic(&function_wave)==3U);
        CHECK(scope_make_dac(&function_wave,3300.0f,samples,1024U));
        {size_t j;double re1=0.0,im1=0.0,re3=0.0,im3=0.0;
         for(j=0U;j<1024U;++j){
             double phase=2.0*3.141592653589793*(double)j/1024.0;
             re1+=samples[j]*cos(phase);im1+=samples[j]*sin(phase);
             re3+=samples[j]*cos(3.0*phase);im3+=samples[j]*sin(3.0*phase);
         }
         NEAR(hypot(re3,im3)/hypot(re1,im1),0.33,0.002);}
        function_wave.harmonic_percent[2]=0.0f;function_wave.harmonic_phase_deg[0]=90.0f;
        CHECK(scope_make_dac(&function_wave,3300.0f,samples,1024U));
        CHECK(samples[0]>3200U&&samples[512U]<900U);
        function_wave.harmonic_phase_deg[0]=NAN;
        CHECK(!scope_make_dac(&function_wave,3300.0f,samples,1024U));
    }
    {
        float input[]={0,0,10,10,0},out[41];size_t i;
        CHECK(scope_resample_view(input,5,0,5,out,41,1));
        for(i=0;i<41;i++)CHECK(out[i]>=0.0f&&out[i]<=10.0f);
        NEAR(out[0],0,0.001f);NEAR(out[40],0,0.001f);
        CHECK(scope_resample_view(input,5,1,3,out,3,0));
        NEAR(out[0],0,0.001f);NEAR(out[1],10,0.001f);NEAR(out[2],10,0.001f);
        CHECK(!scope_resample_view(input,5,4,2,out,3,1));
        CHECK(!scope_resample_view(input,5,0,5,out,1,1));
    }
    test_measurement(); test_conversion(); test_trigger(); test_fft_dac();
    test_measurement_boundaries(); test_fft_boundaries(); test_dac_boundaries();
    test_validation_and_noninteger_period();
    test_trigger_across_capture_boundary();
    if (failures != 0u) { printf("%u failures\n", failures); return 1; }
    puts("PASS oscilloscope_core");
    return 0;
}
