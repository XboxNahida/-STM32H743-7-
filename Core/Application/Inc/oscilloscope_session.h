#ifndef OSCILLOSCOPE_SESSION_H
#define OSCILLOSCOPE_SESSION_H
#include "oscilloscope_core.h"
#include "oscilloscope_io.h"
typedef enum { SCOPE_MODE_AUTO, SCOPE_MODE_NORMAL, SCOPE_MODE_SINGLE } scope_mode_t;
typedef struct {
    uint32_t sample_rate_hz;
    scope_mode_t mode;
    scope_edge_t edge;
    float level_mv;
    uint32_t holdoff_ms;
    size_t pre_samples;
    size_t record_samples;
    uint8_t trigger_source;
    float hysteresis_mv; /* 原始ADC毫伏；0保持手动触发原有语义。 */
} scope_session_config_t;
/* 大结构仅静态分配；采集raw永远为真实原始记录，显示变换单独执行。 */
typedef struct {
    scope_session_config_t config;
    scope_calibration_t calibration;
    scope_calibration_t calibration2;
    scope_io_config_t io,record_io;
    uint16_t *raw,*stream,*incoming;
    uint16_t *raw2,*stream2,*incoming2;
    float *mv2;
    float *mv,*stream_mv;
    uint16_t dac_table[SCOPE_DAC_MAX_SAMPLES];
    scope_measurements_t measurements;
    uint32_t actual_rate_hz, stream_rate_hz, record_sequence, source_sequence;
    uint32_t wait_since_ms, dac_rate_hz, dac_count;
    uint32_t dac_sample_rate_hz,dac_cycles,measurement_sequence;
    float dac_frequency_hz;
    uint64_t received, next_trigger;
    size_t stream_count;
    size_t record_count; /* 冻结长度，与待采集配置独立。 */
    size_t record_trigger_index;
    int last_error;
    uint8_t opened, running, has_record, triggered, dac_enabled;
} scope_session_t;
void scope_session_init(scope_session_t *s);
int scope_session_open(scope_session_t *s);
int scope_session_configure(scope_session_t *s,const scope_session_config_t *config);
int scope_session_run(scope_session_t *s,uint32_t now_ms);
void scope_session_stop(scope_session_t *s);
void scope_session_close(scope_session_t *s);
/* 返回1=新冻结记录、0=等待、负值=驱动/计算错误。 */
int scope_session_poll(scope_session_t *s,uint32_t now_ms);
int scope_session_dac(scope_session_t *s,const scope_dac_config_t *config,int enable);
int scope_session_calibrate_zero(scope_session_t *s);
int scope_session_calibrate_gain(scope_session_t *s,float measured_reference_mv);
int scope_session_calibrate_channel(scope_session_t *s,unsigned channel,int gain,float reference_mv);
int scope_session_refresh(scope_session_t *s);
/* 独立低频调用；发布记录只做ADC换算，不执行重复测量。 */
int scope_session_measure(scope_session_t *s);
#endif
