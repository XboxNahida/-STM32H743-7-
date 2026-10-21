#include <stdint.h>
#include <stdio.h>
#include "oscilloscope_timing.h"
#include "oscilloscope_io.h"

static int failures;
#define CHECK(c) do { if (!(c)) { printf("FAIL line=%d: %s\n", __LINE__, #c); failures++; } } while (0)
int main(void)
{
    scope_timer_plan_t plan = {0};
    scope_adc_plan_t adc = {0};
    scope_io_config_t io={{1U,1U},{SCOPE_INPUT_PA5,SCOPE_INPUT_PC4},SCOPE_OUTPUT_PA4};
    uint64_t divisor;
    CHECK(scope_timer_plan(200000000U, 100000U, 500000U, &plan));
    divisor = ((uint64_t)plan.prescaler + 1U) * ((uint64_t)plan.period + 1U);
    CHECK(divisor == 2000U && plan.actual_hz == 100000U);
    CHECK(scope_timer_plan(200000000U, 100U, 500000U, &plan));
    CHECK(plan.prescaler > 0U && plan.actual_hz == 100U);
    CHECK(scope_timer_plan(200000000U, 44100U, 1000000U, &plan));
    CHECK(plan.actual_hz >= 44095U && plan.actual_hz <= 44105U);
    CHECK(!scope_timer_plan(0U, 100U, 500000U, &plan));
    CHECK(!scope_timer_plan(200000000U, 0U, 500000U, &plan));
    CHECK(!scope_timer_plan(200000000U, 500001U, 500000U, &plan));
    CHECK(!scope_timer_plan(100U, 101U, 1000U, &plan));
    CHECK(!scope_timer_plan(1000U, 100U, 1000U, NULL));
    CHECK(!scope_dma_half_readable(4096U, 2048U, 0U));
    CHECK(scope_dma_half_readable(2048U, 2048U, 0U));
    CHECK(scope_dma_half_readable(1U, 2048U, 0U));
    CHECK(!scope_dma_half_readable(0U, 2048U, 0U));
    CHECK(scope_dma_half_readable(4096U, 2048U, 1U));
    CHECK(!scope_dma_half_readable(2048U, 2048U, 1U));
    CHECK(!scope_dma_half_readable(4097U, 2048U, 1U));
    CHECK(!scope_dma_half_readable(4096U, 2048U, 2U));
    CHECK(!scope_dma_half_readable(UINT32_MAX, UINT32_MAX, 0U));
    CHECK(scope_adc_plan(200000000U,64000000U,1U,1000000U,&adc));
    CHECK(adc.clock_hz==25000000U && !adc.asynchronous && adc.divisor==4U);
    CHECK(adc.sample_cycles_x2==17U); /* PA5 slow channel 至少 8.5 cycles */
    CHECK(scope_adc_plan(200000000U,64000000U,0U,1000000U,&adc));
    CHECK(adc.clock_hz==32000000U && adc.asynchronous && adc.divisor==2U);
    CHECK(scope_adc_plan(200000000U,64000000U,1U,100000U,&adc));
    CHECK(adc.sample_cycles_x2==129U); /* 选尽量长的合法采样时间 */
    CHECK(!scope_adc_plan(200000000U,64000000U,1U,1000001U,&adc));
    CHECK(!scope_adc_plan(0U,0U,1U,1000000U,&adc));
    CHECK(!scope_adc_plan(20000000U,0U,1U,1000000U,&adc));
    CHECK(!scope_adc_plan(200000000U,64000000U,1U,0U,&adc));
    CHECK(!scope_adc_plan(200000000U,64000000U,1U,100U,NULL));
    CHECK(scope_io_valid(&io)&&scope_io_max_rate(&io)==1000000U);
    CHECK(!scope_io_dac_conflict(&io));io.output=SCOPE_OUTPUT_PA5;
    CHECK(scope_io_dac_conflict(&io)&&!scope_io_valid(&io));
    io.output=SCOPE_OUTPUT_PA4;io.enabled[0]=0U;
    CHECK(scope_io_max_rate(&io)==2000000U&&!scope_io_dac_conflict(&io));
    io.enabled[0]=1U;io.input[0]=SCOPE_INPUT_PC4;CHECK(!scope_io_valid(&io));
    io.input[0]=3U;CHECK(!scope_io_valid(&io));
    CHECK(scope_adc_plan_input(200000000U,64000000U,1U,2000000U,1U,&adc));
    CHECK(adc.clock_hz==25000000U&&adc.sample_cycles_x2==5U);
    CHECK(!scope_adc_plan_input(200000000U,64000000U,1U,2000000U,0U,&adc));
    CHECK(!scope_adc_plan_input(200000000U,64000000U,1U,2000001U,1U,&adc));
    printf("%s oscilloscope_timing failures=%d\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 1 : 0;
}
