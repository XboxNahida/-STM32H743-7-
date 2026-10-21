#include "oscilloscope_timing.h"
#include <stddef.h>
#include <stdint.h>

int scope_adc_plan(uint32_t hclk, uint32_t kernel, uint8_t revision_div2,
                    uint32_t rate, scope_adc_plan_t *plan)
{ return scope_adc_plan_input(hclk,kernel,revision_div2,rate,0U,plan); }

int scope_adc_plan_input(uint32_t hclk,uint32_t kernel,uint8_t revision_div2,
                    uint32_t rate,uint8_t fast_only,scope_adc_plan_t *plan)
{
    static const uint16_t cycles[] = {1621U,775U,129U,65U,33U,17U,5U};
    static const uint16_t divisors[] = {1U,2U,4U,6U,8U,10U,12U,16U,
                                        32U,64U,128U,256U};
    scope_adc_plan_t next = {0};
    uint32_t extra = revision_div2 ? 2U : 1U;
    size_t i;
    if (plan == NULL || rate < 100U || rate > (fast_only?2000000U:1000000U)) return 0;
    next.divisor = 4U;
    next.clock_hz = hclk / (4U * extra);
    /* 同步源源于HSE/PLL，优先保持测频时基；旧版超限时改用合法异步源。 */
    if (next.clock_hz == 0U || next.clock_hz > 36000000U) {
        next.asynchronous = 1U;
        next.clock_hz = 0U;
        for (i=0U; i<sizeof(divisors)/sizeof(divisors[0]); ++i) {
            uint32_t clock = kernel / ((uint32_t)divisors[i] * extra);
            if (clock != 0U && clock <= 36000000U) {
                next.divisor = divisors[i]; next.clock_hz = clock; break;
            }
        }
    }
    if (next.clock_hz == 0U) return 0;
    /* PA5/ADC1_INP19 是 slow channel，不能使用 2.5 cycles 直通通道配置。 */
    for (i=0U; i<sizeof(cycles)/sizeof(cycles[0])-(fast_only?0U:1U); ++i) {
        /* 16位处理8.5周期；半周期整数表示，额外1周期避免边界触发。 */
        if ((uint64_t)rate * (cycles[i]+19U) <= (uint64_t)next.clock_hz*2U) {
            next.sample_cycles_x2 = cycles[i]; *plan = next; return 1;
        }
    }
    return 0;
}

int scope_timer_plan(uint32_t clock, uint32_t requested,
                      uint32_t maximum, scope_timer_plan_t *plan)
{
    uint64_t ticks;
    uint64_t prescale;
    uint64_t period;
    if (plan == NULL || clock == 0U || requested == 0U ||
        requested > maximum || requested > clock)
        return 0;
    ticks = ((uint64_t)clock + requested / 2U) / requested;
    prescale = (ticks + 65535U) / 65536U;
    if (prescale == 0U || prescale > 65536U) return 0;
    period = ((uint64_t)clock + (uint64_t)requested * prescale / 2U) /
             ((uint64_t)requested * prescale);
    if (period == 0U || period > 65536U) return 0;
    plan->prescaler = (uint16_t)(prescale - 1U);
    plan->period = (uint16_t)(period - 1U);
    plan->actual_hz = (uint32_t)((uint64_t)clock / (prescale * period));
    return plan->actual_hz <= maximum && plan->actual_hz != 0U;
}

int scope_dma_half_readable(uint32_t remaining, uint32_t half,
                            uint32_t completed)
{
    if (half == 0U || half > UINT32_MAX / 2U || completed > 1U ||
        remaining == 0U || remaining > half * 2U)
        return 0;
    return completed == 0U ? remaining <= half : remaining > half;
}
