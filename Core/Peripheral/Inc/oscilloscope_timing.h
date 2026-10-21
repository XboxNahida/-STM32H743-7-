#ifndef OSCILLOSCOPE_TIMING_H
#define OSCILLOSCOPE_TIMING_H
#include <stdint.h>
typedef struct
{
    uint16_t prescaler;
    uint16_t period;
    uint32_t actual_hz;
} scope_timer_plan_t;
typedef struct {
    uint32_t clock_hz;
    uint16_t divisor, sample_cycles_x2;
    uint8_t asynchronous;
} scope_adc_plan_t;
/* 16位单通道，保守限制ADC时钟36MHz，转换后至少留1周期余量。
 * revision_div2 对应HAL对Rev.V及以后版本施加的额外/2。 */
int scope_adc_plan(uint32_t hclk, uint32_t kernel, uint8_t revision_div2,
                    uint32_t rate, scope_adc_plan_t *plan);
int scope_adc_plan_input(uint32_t hclk,uint32_t kernel,uint8_t revision_div2,
                    uint32_t rate,uint8_t fast_only,scope_adc_plan_t *plan);
int scope_timer_plan(uint32_t timer_clock_hz, uint32_t requested_hz,
                      uint32_t maximum_hz, scope_timer_plan_t *plan);
/* DMA circular buffer contains two equally sized halves. */
int scope_dma_half_readable(uint32_t remaining, uint32_t half_samples,
                            uint32_t completed_half);
#endif
