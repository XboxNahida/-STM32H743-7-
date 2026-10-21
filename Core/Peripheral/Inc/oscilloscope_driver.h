#ifndef OSCILLOSCOPE_DRIVER_H
#define OSCILLOSCOPE_DRIVER_H

#include <stdint.h>
#include "oscilloscope_io.h"

#define SCOPE_DRIVER_FRAME_SAMPLES 2048U
#define SCOPE_DRIVER_MAX_FRAME_SAMPLES 8192U
#define SCOPE_DRIVER_MIN_RATE_HZ 100U
#define SCOPE_DRIVER_MAX_RATE_HZ 2000000U
#define SCOPE_DRIVER_DAC_MAX_RATE_HZ 1000000U

typedef enum {
    SCOPE_ADC_STAGE_NONE, SCOPE_ADC_STAGE_RATE, SCOPE_ADC_STAGE_TIMER,
    SCOPE_ADC_STAGE_PLAN, SCOPE_ADC_STAGE_STOP, SCOPE_ADC_STAGE_INIT1,
    SCOPE_ADC_STAGE_CLOCK, SCOPE_ADC_STAGE_CHANNEL1, SCOPE_ADC_STAGE_INIT2,
    SCOPE_ADC_STAGE_CHANNEL2, SCOPE_ADC_STAGE_CAL2, SCOPE_ADC_STAGE_MULTI,
    SCOPE_ADC_STAGE_DMA, SCOPE_ADC_STAGE_START, SCOPE_ADC_STAGE_TRIGGER
} scope_adc_stage_t;

typedef struct
{
    uint32_t sample_rate_hz;
    uint32_t dac_rate_hz;
    uint32_t dma_sequence;
    uint32_t dropped_frames;
    uint32_t errors;
    uint32_t dac_underruns, dac_dma_errors, adc_errors;
    uint32_t dac_recoveries;
    uint32_t adc_clock_hz, chip_revision;
    uint16_t adc_sample_cycles_x2;
    uint8_t adc_asynchronous;
    int32_t last_error;
    uint8_t opened;
    uint8_t acquiring;
    uint8_t dac_enabled;
    uint8_t adc_stage;
    uint32_t adc_state[2],adc_cr[2]; /* 失败时快照；由界面在回滚前保留。 */
} scope_driver_status_t;

/* 仅主循环调用。打开时独占 ADC1/PA5，关闭时恢复原配置。 */
int scope_driver_open(void);
/* 停止采集后调用；PC4隔离失败不切换输入，DAC运行时拒绝引脚冲突。 */
int scope_driver_configure_io(const scope_io_config_t *config);
int scope_driver_start(uint32_t sample_rate_hz);
/* 可变记录采集：完整半块至少N点、最多2N且不超8192，16点对齐保障Cache行。 */
int scope_driver_start_capture(uint32_t sample_rate_hz,uint32_t record_samples);
uint32_t scope_driver_frame_samples(void);
void scope_driver_stop(void);
void scope_driver_close(void);
/* 1=完整新帧，0=暂时无帧，负数=错误。capacity 至少为 FRAME_SAMPLES。 */
int scope_driver_read(uint16_t *samples, uint32_t capacity,
                      uint32_t *sample_rate_hz, uint32_t *sequence);
int scope_driver_read_pair(uint16_t *ch1,uint16_t *ch2,uint32_t capacity,
                      uint32_t *sample_rate_hz,uint32_t *sequence);
/* 输入为12位 DAC 表；恒值表静态保持（实际点速率0），其它表复制后循环DMA。
 * 单次欠载主循环恢复，1秒内最多3次；DMA错误或恢复失败关闭输出。 */
int scope_driver_set_dac(const uint16_t *table, uint32_t count,
                         uint32_t sample_rate_hz);
void scope_driver_disable_dac(void);
/* 从共享 TIM6_DAC IRQ 调用；处理当前DAC1通道1/2欠载中断。 */
void scope_driver_dac_irq_handler(void);
void scope_driver_get_status(scope_driver_status_t *status);

#endif
