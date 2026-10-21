#include <stdio.h>
#include <string.h>
#include "scope_hal/adc.h"
#include "resource_manager.h"

/* 直接编译生产驱动；只将 HAL 和 DMA 并发交错替换为可控制的宿主边界。 */
static void *fake_copy(void *destination, const void *source, size_t count);
#define memcpy fake_copy
#include "../../Core/Peripheral/Src/oscilloscope_driver.c"
#undef memcpy

ADC_TypeDef fake_adc;
ADC_TypeDef fake_adc2;
ADC_Common_TypeDef fake_adc_common;
uint32_t fake_revision;
GPIO_TypeDef fake_gpio;
RCC_TypeDef fake_rcc;
DAC_TypeDef fake_dac;
TIM_TypeDef fake_tim4, fake_tim7;
DMA_Stream_TypeDef fake_stream0, fake_stream3;
ADC_HandleTypeDef hadc1;
static DMA_HandleTypeDef original_dma;
static unsigned failures, checks;
static int resource_held, resource_busy, dma_fail, channel_fail, calibration_fail;
static int adc_start_fail, adc_stop_failures, adc_init_failures;
static int stop_calls, init_calls, deinit_calls, releases, copy_interleave;
static int dac_starts, dac_start_fail, unsafe_dac_start, dac_static_starts, dac_value_fail;
static uint32_t irq_enabled[FAKE_IRQ_COUNT], irq_pending[FAKE_IRQ_COUNT];
static uint32_t fake_primask, deferred_adc_error, global_enable_calls;
static uint32_t configured_sampling;
static int phy_fail,phy_held;
static int slave_disable_missed;
static unsigned slave_calibrations, calibration_in_dual;
static uint8_t calibration_valid[2];
static uint32_t fake_tick;
static uint8_t dac_error_during_abort;
uint32_t HAL_GetTick(void) { return fake_tick; }
int scope_phy_acquire(void){if(phy_fail)return 0;phy_held=1;return 1;}
int scope_phy_release(void){phy_held=0;return 1;}
int scope_phy_check(void){return phy_held&&!phy_fail;}
#define CHECK(c) do { ++checks; if (!(c)) { ++failures; printf("FAIL line=%d: %s\n", __LINE__, #c); } } while (0)

int resource_acquire(resource_id_t id, resource_owner_t owner)
{
    CHECK(id == RESOURCE_TIM2_PA5 && owner == SCOPE_OWNER);
    if (resource_busy || resource_held) return RESOURCE_BUSY;
    resource_held = 1; return RESOURCE_OK;
}
int resource_release(resource_id_t id, resource_owner_t owner)
{
    CHECK(id == RESOURCE_TIM2_PA5 && owner == SCOPE_OWNER && resource_held);
    resource_held = 0; ++releases; return RESOURCE_OK;
}
void HAL_RCC_GetClockConfig(RCC_ClkInitTypeDef *clocks, uint32_t *latency)
{ clocks->APB1CLKDivider = RCC_APB1_DIV2; *latency = 0U; }
void HAL_GPIO_Init(GPIO_TypeDef *gpio, const GPIO_InitTypeDef *config)
{
    unsigned shift = config->Pin == GPIO_PIN_5 ? 10U : 8U;
    gpio->MODER = (gpio->MODER & ~(3UL << shift)) | (config->Mode << shift);
    gpio->PUPDR = (gpio->PUPDR & ~(3UL << shift)) | (config->Pull << shift);
}
void HAL_NVIC_DisableIRQ(int irq) { irq_enabled[irq] = 0U; }
void HAL_NVIC_EnableIRQ(int irq) { irq_enabled[irq] = 1U; }
void HAL_NVIC_ClearPendingIRQ(int irq) { irq_pending[irq] = 0U; }
void HAL_NVIC_SetPriority(int irq, uint32_t priority, uint32_t subpriority)
{ (void)irq; (void)priority; (void)subpriority; }
uint32_t NVIC_GetEnableIRQ(int irq) { return irq_enabled[irq]; }
uint32_t __get_PRIMASK(void) { return fake_primask; }
void __disable_irq(void) { fake_primask = 1U; }
void __enable_irq(void)
{
    fake_primask = 0U; ++global_enable_calls;
    if (deferred_adc_error) {
        deferred_adc_error = 0U;
        HAL_ADC_ErrorCallback(&hadc1);
    }
}
HAL_StatusTypeDef HAL_DMA_Init(DMA_HandleTypeDef *dma)
{ (void)dma; return dma_fail ? HAL_ERROR : HAL_OK; }
HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef *adc)
{
    ++init_calls;
    if (adc_init_failures > 0 || (adc->State & (HAL_ADC_STATE_REG_BUSY | HAL_ADC_STATE_ERROR_INTERNAL))) {
        if (adc_init_failures > 0) --adc_init_failures;
        adc->State |= HAL_ADC_STATE_ERROR_INTERNAL;
        return HAL_ERROR;
    }
    /* 对齐HAL：重配时钟不清DUAL，退出深度掉电不会恢复旧校准。 */
    fake_adc_common.CCR=(fake_adc_common.CCR&~(ADC_CCR_CKMODE|ADC_CCR_PRESC))|
        adc->Init.ClockPrescaler;
    adc->Instance->CR=(adc->Instance->CR&~ADC_CR_DEEPPWD)|ADC_CR_ADVREGEN;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_ADC_DeInit(ADC_HandleTypeDef *adc)
{
    ++deinit_calls;adc->State=0U;
    calibration_valid[adc->Instance==ADC2?1U:0U]=0U;
    adc->Instance->CR=(adc->Instance->CR&~ADC_CR_ADVREGEN)|ADC_CR_DEEPPWD;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef *adc, ADC_ChannelConfTypeDef *channel)
{
    CHECK(channel->Channel == ADC_CHANNEL_19||channel->Channel==ADC_CHANNEL_4);
    configured_sampling=channel->SamplingTime;
    adc->Instance->SMPR2 = 16U;
    return channel_fail ? HAL_ERROR : HAL_OK;
}
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *adc, uint32_t calibration, uint32_t mode)
{
    (void)mode;
    if(adc->Instance==ADC2)++slave_calibrations;
    /* RM0433 25.4.8：校准时DUAL必须为0。违规时模拟实机超时后的状态。 */
    if((fake_adc_common.CCR&ADC_CCR_DUAL)!=ADC_MODE_INDEPENDENT){
        ++calibration_in_dual;
        adc->Instance->CR|=ADC_CR_ADCAL|ADC_CR_ADCALLIN;
        adc->State|=HAL_ADC_STATE_ERROR_INTERNAL;
        return HAL_ERROR;
    }
    if(calibration_fail){adc->State|=HAL_ADC_STATE_ERROR_INTERNAL;return HAL_ERROR;}
    CHECK((adc->Instance->CR&(ADC_CR_ADVREGEN|ADC_CR_DEEPPWD))==ADC_CR_ADVREGEN);
    calibration_valid[adc->Instance==ADC2?1U:0U]=1U;
    if(calibration==ADC_CALIB_OFFSET_LINEARITY)
        adc->Instance->CR|=(ADC_CR_LINCALRDYW6*2U-ADC_CR_LINCALRDYW1)|ADC_CR_ADCALLIN;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_ADCEx_MultiModeConfigChannel(ADC_HandleTypeDef *adc, ADC_MultiModeTypeDef *config)
{
    CHECK(adc==&hadc1);
    if((ADC1->CR|ADC2->CR)&ADC_CR_ADEN)return HAL_ERROR;
    fake_adc_common.CCR=(fake_adc_common.CCR&~ADC_CCR_DUAL)|config->Mode;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_ADC_Start_DMA(ADC_HandleTypeDef *adc, uint32_t *data, uint32_t count)
{
    CHECK(data == (uint32_t *)g_adc_buffer && count == 2U*scope_driver_frame_samples());
    CHECK(calibration_valid[0]&&(!g_dual||calibration_valid[1]));
    /* 本工程 HAL 即使 DMA_Start_IT 失败，仍置 REG_BUSY 并设置 ADSTART。 */
    adc->State |= HAL_ADC_STATE_REG_BUSY;
    adc->Instance->CR |= ADC_CR_ADEN;
    return adc_start_fail ? HAL_ERROR : HAL_OK;
}
HAL_StatusTypeDef HAL_ADC_Stop_DMA(ADC_HandleTypeDef *adc)
{
    ++stop_calls;
    CHECK(adc->DMA_Handle == &g_adc_dma);
    if (adc_stop_failures > 0) { --adc_stop_failures; return HAL_ERROR; }
    adc->State &= ~HAL_ADC_STATE_REG_BUSY;
    adc->Instance->CR &= ~ADC_CR_ADEN;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_ADCEx_MultiModeStart_DMA(ADC_HandleTypeDef *adc, const uint32_t *data, uint32_t count)
{
    /* 真实HAL只清主ADC标志，且只更新主句柄State。 */
    __HAL_ADC_CLEAR_FLAG(adc,ADC_FLAG_EOC|ADC_FLAG_EOS|ADC_FLAG_OVR);
    ADC2->CR |= ADC_CR_ADEN;
    return HAL_ADC_Start_DMA(adc,(uint32_t *)data,count);
}
HAL_StatusTypeDef HAL_ADCEx_MultiModeStop_DMA(ADC_HandleTypeDef *adc)
{
    HAL_StatusTypeDef result=HAL_ADC_Stop_DMA(adc);
    /* 本工程HAL的ADC_Disable(slave)失败分支可能仍返回HAL_OK。 */
    if(result==HAL_OK&&!slave_disable_missed)ADC2->CR &= ~ADC_CR_ADEN;
    return result;
}
HAL_StatusTypeDef HAL_TIM_Base_Start(TIM_HandleTypeDef *timer)
{ timer->Instance->running = 1U; return HAL_OK; }
HAL_StatusTypeDef HAL_TIM_Base_Stop(TIM_HandleTypeDef *timer)
{ timer->Instance->running = 0U; return HAL_OK; }
HAL_StatusTypeDef HAL_DAC_ConfigChannel(DAC_HandleTypeDef *dac, DAC_ChannelConfTypeDef *config, uint32_t channel)
{
    CHECK(channel == DAC_CHANNEL_1);
    dac->Instance->CR = (dac->Instance->CR & ~(DAC_CR_TEN1|DAC_CR_TSEL1)) | config->DAC_Trigger;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_DAC_SetValue(DAC_HandleTypeDef *dac, uint32_t channel, uint32_t alignment, uint32_t value)
{
    (void)channel; (void)alignment;
    if (dac_value_fail) return HAL_ERROR;
    dac->Instance->DHR12R1 = value;
    if ((dac->Instance->CR & (DAC_CR_EN1|DAC_CR_TEN1)) == DAC_CR_EN1)
        dac->Instance->DOR1 = value;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_DAC_Start(DAC_HandleTypeDef *dac, uint32_t channel)
{
    (void)channel; ++dac_static_starts;
    if (dac_start_fail) return HAL_ERROR;
    dac->Instance->CR |= DAC_CR_EN1;
    if (!(dac->Instance->CR & DAC_CR_TEN1)) dac->Instance->DOR1 = dac->Instance->DHR12R1;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_DAC_Stop_DMA(DAC_HandleTypeDef *dac, uint32_t channel)
{
    (void)channel;
    if(dac_error_during_abort){dac_error_during_abort=0U;HAL_DAC_ErrorCallbackCh1(dac);}
    dac->Instance->CR &= ~(DAC_CR_EN1|DAC_CR_DMAEN1|DAC_IT_DMAUDR1);
    return HAL_OK;
}
HAL_StatusTypeDef HAL_DAC_Start_DMA(DAC_HandleTypeDef *dac, uint32_t channel, uint32_t *data, uint32_t count, uint32_t alignment)
{
    (void)channel; (void)data; (void)count; (void)alignment; ++dac_starts;
    if (irq_enabled[TIM6_DAC_IRQn] || (dac->Instance->SR & DAC_FLAG_DMAUDR1)) ++unsafe_dac_start;
    dac->Instance->CR |= DAC_IT_DMAUDR1|DAC_CR_DMAEN1|DAC_CR_EN1;
    (dac->DMA_Handle1?dac->DMA_Handle1:dac->DMA_Handle2)->Instance->CR = DMA_IT_HT|DMA_IT_TC|DMA_IT_TE|DMA_IT_DME;
    return dac_start_fail ? HAL_ERROR : HAL_OK;
}
static void *fake_copy(void *destination, const void *source, size_t count)
{
    void *result = memcpy(destination, source, count);
    if (source == g_adc_buffer && copy_interleave != 0) {
        fake_stream0.remaining = 2U*scope_driver_frame_samples();
        if (copy_interleave == 2) HAL_ADC_ConvCpltCallback(&hadc1);
        copy_interleave = 0;
    }
    return result;
}
static void reset_fixture(void)
{
    memset(&g_scope_status, 0, sizeof(g_scope_status));
    memset(&g_adc_dma, 0, sizeof(g_adc_dma)); memset(&g_dac_dma, 0, sizeof(g_dac_dma));
    memset(&g_adc_timer, 0, sizeof(g_adc_timer)); memset(&g_dac_timer, 0, sizeof(g_dac_timer));
    memset(&g_dac, 0, sizeof(g_dac)); memset(&fake_adc, 0, sizeof(fake_adc));
    memset(&fake_dac, 0, sizeof(fake_dac)); memset(&hadc1, 0, sizeof(hadc1));
    memset(irq_enabled, 0, sizeof(irq_enabled)); memset(irq_pending, 0, sizeof(irq_pending));
    g_restore_pending = g_adc_needs_stop = g_irq_acquiring = 0U;
    g_completed_sequence = g_completed_half = g_read_sequence = g_irq_errors = 0U;
    resource_held = resource_busy = dma_fail = channel_fail = calibration_fail = 0;
    adc_start_fail = adc_stop_failures = adc_init_failures = 0;
    slave_disable_missed=0;
    slave_calibrations=calibration_in_dual=0U;
    calibration_valid[0]=calibration_valid[1]=0U;
    stop_calls = init_calls = deinit_calls = releases = copy_interleave = 0;
    dac_starts = dac_start_fail = unsafe_dac_start = dac_static_starts = dac_value_fail = 0;
    fake_primask = deferred_adc_error = global_enable_calls = 0U;
    fake_tick=0U;
    dac_error_during_abort=0U;
    fake_revision=REV_ID_V;fake_adc2.CR=0U;fake_adc_common.CCR=0U;configured_sampling=0U;
    fake_rcc.CFGR = 0U; fake_tim4.running = fake_tim7.running = 0U;
    fake_stream0.remaining = fake_stream3.remaining = 0U;
    hadc1.Instance = ADC1; hadc1.DMA_Handle = &original_dma;
    hadc1.Init.ExternalTrigConv = 123U;
    hadc1.Init.ClockPrescaler=ADC_CLOCK_ASYNC_DIV2;
    fake_adc.SMPR2 = 810U; fake_adc.SQR1 = 19U; fake_adc.PCSEL = 1U << 19;
    fake_gpio.MODER = 0U; fake_gpio.PUPDR = 1U << 10;
}
static void check_restored(void)
{
    CHECK(!g_scope_status.opened && !resource_held && !g_restore_pending);
    CHECK(hadc1.DMA_Handle == &original_dma && hadc1.Init.ExternalTrigConv == 123U);
    CHECK(fake_adc.SMPR2 == 810U && fake_adc.SQR1 == 19U && fake_adc.PCSEL == (1U << 19));
    CHECK((fake_gpio.MODER & (3U << 10)) == 0U);
    CHECK((fake_gpio.PUPDR & (3U << 10)) == (1U << 10));
}
static void test_open_failure_and_busy(void)
{
    reset_fixture(); resource_busy = 1;
    CHECK(scope_driver_open() == -2 && !g_scope_status.opened && releases == 0);
    reset_fixture(); hadc1.State = HAL_ADC_STATE_REG_BUSY;
    CHECK(scope_driver_open() == -2 && !resource_held && releases == 1 && init_calls == 0);
    reset_fixture(); dma_fail = 1;
    CHECK(scope_driver_open() == -3); check_restored();
    reset_fixture(); channel_fail = 1;
    CHECK(scope_driver_open() == -3); check_restored();
    reset_fixture(); calibration_fail = 1;
    CHECK(scope_driver_open() == -3); check_restored();
    reset_fixture(); adc_init_failures = 1;
    CHECK(scope_driver_open() == -3 && deinit_calls == 1); check_restored();
}
static void test_adc_failure_cleanup(void)
{
    reset_fixture(); CHECK(scope_driver_open() == 0); adc_start_fail = 1;
    CHECK(scope_driver_start(100000U) == -5 && stop_calls == 1);
    CHECK(!(hadc1.State & HAL_ADC_STATE_REG_BUSY) && !g_adc_needs_stop && !g_scope_status.acquiring);
    scope_driver_close(); check_restored();
    reset_fixture(); CHECK(scope_driver_open() == 0); CHECK(scope_driver_start(100000U) == 0);
    adc_stop_failures = 1; scope_driver_close();
    CHECK(g_restore_pending && resource_held && g_scope_status.opened && g_adc_needs_stop);
    CHECK(hadc1.DMA_Handle == &g_adc_dma && init_calls == 2); /* open+采样时序重配置 */
    CHECK(scope_driver_open() == -10 && scope_driver_start(100000U) == -1);
    scope_driver_close(); CHECK(stop_calls == 2); check_restored();
    reset_fixture(); CHECK(scope_driver_open() == 0); adc_start_fail = 1; adc_stop_failures = 2;
    CHECK(scope_driver_start(100000U) == -5 && g_adc_needs_stop);
    scope_driver_close(); CHECK(g_restore_pending && stop_calls == 2);
    scope_driver_close(); CHECK(stop_calls == 3); check_restored();
    reset_fixture(); CHECK(scope_driver_open() == 0); adc_init_failures = 1;
    scope_driver_close(); CHECK(g_restore_pending && resource_held);
    scope_driver_close(); CHECK(deinit_calls == 1); check_restored();
}
static void test_dac_restart_and_shared_irq(void)
{
    const uint16_t table[] = {0U, 2048U, 4095U, 2048U};
    static uint16_t full_table[1024];
    scope_driver_status_t status;
    reset_fixture(); CHECK(scope_driver_open() == 0);
    full_table[1023]=4095U;
    CHECK(scope_driver_set_dac(full_table,1024U,1000000U)==0);
    CHECK(g_dac_buffer[1023]==4095U&&sizeof(g_dac_buffer)==2048U);
    CHECK(scope_driver_set_dac(full_table,1025U,1000000U)<0);
    scope_driver_disable_dac();dac_starts=0;
    irq_enabled[TIM6_DAC_IRQn] = irq_pending[TIM6_DAC_IRQn] = 1U;
    fake_dac.SR = DAC_FLAG_DMAUDR1; g_irq_errors = 2U;
    CHECK(scope_driver_set_dac(table, 4U, 10000U) == 0);
    CHECK(unsafe_dac_start == 0 && dac_starts == 1 && !(g_irq_errors & 2U));
    CHECK(irq_enabled[TIM6_DAC_IRQn] && irq_pending[TIM6_DAC_IRQn]);
    CHECK(!(fake_dac.SR & DAC_FLAG_DMAUDR1) && (fake_dac.CR & DAC_IT_DMAUDR1));
    g_irq_errors = 3U;
    CHECK(scope_driver_set_dac(table, 4U, 10000U) == 0 && g_irq_errors == 1U);
    scope_driver_get_status(&status);
    CHECK(status.dac_enabled); /* ADC 错误不能误报为 DAC 故障。 */
    g_irq_errors = 0U;
    fake_dac.SR = DAC_FLAG_DMAUDR1; scope_driver_dac_irq_handler();
    CHECK(!fake_tim7.running);
    scope_driver_get_status(&status);
    CHECK(status.dac_enabled && fake_tim7.running && !(fake_dac.SR & DAC_FLAG_DMAUDR1));
    CHECK(status.dac_underruns==1U && status.dac_dma_errors==0U);
    CHECK(scope_driver_set_dac(table, 4U, 10000U) == 0 && unsafe_dac_start == 0);
    /* TIM7已启动但主循环尚未来得及发布enabled时，UDR仍必须被处理。 */
    g_scope_status.dac_enabled=0U;fake_dac.SR=DAC_FLAG_DMAUDR1;
    scope_driver_dac_irq_handler();
    CHECK(!(fake_dac.SR&DAC_FLAG_DMAUDR1) && !fake_tim7.running);
    g_scope_status.dac_enabled=1U;scope_driver_get_status(&status);
    irq_enabled[TIM6_DAC_IRQn] = 0U;
    CHECK(scope_driver_set_dac(table, 4U, 10000U) == 0);
    CHECK(irq_enabled[TIM6_DAC_IRQn] && irq_pending[TIM6_DAC_IRQn]);
    scope_driver_disable_dac();
    CHECK(!irq_enabled[TIM6_DAC_IRQn] && irq_pending[TIM6_DAC_IRQn]);
    dac_start_fail = 1;
    CHECK(scope_driver_set_dac(table, 4U, 10000U) == -9 && !g_scope_status.dac_enabled);
    CHECK(!irq_enabled[TIM6_DAC_IRQn] && irq_pending[TIM6_DAC_IRQn]);
    scope_driver_close(); check_restored();
}
static void test_dc_holds_without_dma_or_trigger(void)
{
    const uint16_t wave[] = {0U,2048U,4095U,2048U};
    uint16_t dc[] = {2048U,2048U};
    scope_io_config_t io = {{1U,1U},{SCOPE_INPUT_PA5,SCOPE_INPUT_PC4},SCOPE_OUTPUT_PA4};
    scope_driver_status_t status;
    unsigned i;
    reset_fixture(); CHECK(scope_driver_open() == 0);
    CHECK(scope_driver_set_dac(wave,4U,1000000U) == 0 && fake_tim7.running);
    irq_enabled[TIM6_DAC_IRQn] = 0U;
    /* 直流不能依赖DMA可用性，也不能继承前一波形的欠载标志。 */
    dma_fail=1; fake_dac.SR=DAC_FLAG_DMAUDR1;
    CHECK(scope_driver_set_dac(dc,2U,100U) == 0);
    scope_driver_get_status(&status);
    CHECK(status.dac_enabled && status.dac_rate_hz==0U && status.dac_underruns==0U);
    CHECK(fake_dac.DOR1==2048U && (fake_dac.CR&DAC_CR_EN1));
    CHECK(!(fake_dac.CR&(DAC_CR_TEN1|DAC_CR_DMAEN1|DAC_IT_DMAUDR1)));
    CHECK(!fake_tim7.running && !irq_enabled[TIM6_DAC_IRQn]);
    CHECK(g_dac.DMA_Handle1==NULL && dac_starts==1 && dac_static_starts==1);
    dma_fail=0;
    CHECK(scope_driver_configure_io(&io)==0);
    for(i=0U;i<16U;i++){
        CHECK(scope_driver_start_capture(i%2U?1000000U:100000U,512U)==0);
        scope_driver_get_status(&status);
        CHECK(status.acquiring && status.dac_enabled && fake_dac.DOR1==2048U);
        CHECK(!fake_tim7.running && dac_starts==1 && dac_static_starts==1);
        scope_driver_stop();
    }
    CHECK(scope_driver_set_dac(wave,4U,1000000U)==0 && fake_tim7.running);
    CHECK((fake_dac.CR&(DAC_CR_TEN1|DAC_CR_DMAEN1))==(DAC_CR_TEN1|DAC_CR_DMAEN1));
    dc[0]=dc[1]=4095U;
    CHECK(scope_driver_set_dac(dc,2U,100U)==0 && fake_dac.DOR1==4095U);
    dc[0]=dc[1]=0U;
    CHECK(scope_driver_set_dac(dc,2U,100U)==0 && fake_dac.DOR1==0U);
    scope_driver_get_status(&status); CHECK(status.dac_enabled);
    scope_driver_disable_dac(); scope_driver_get_status(&status);
    CHECK(!status.dac_enabled && !(fake_dac.CR&DAC_CR_EN1));
    dac_start_fail=1;
    CHECK(scope_driver_set_dac(dc,2U,100U)==-9 && !g_scope_status.dac_enabled);
    dac_start_fail=0; dac_value_fail=1;
    CHECK(scope_driver_set_dac(dc,2U,100U)==-9 && !g_scope_status.dac_enabled);
    dac_value_fail=0;
    CHECK(scope_driver_set_dac(dc,2U,100U)==0);
    scope_driver_close(); CHECK(!(fake_dac.CR&DAC_CR_EN1)); check_restored();
}
static void test_dac_underrun_recovery_is_bounded(void)
{
    const uint16_t wave[]={0U,2048U,4095U,2048U};
    scope_driver_status_t status;
    unsigned i;
    reset_fixture();CHECK(scope_driver_open()==0);
    CHECK(scope_driver_start(1000000U)==0);
    CHECK(scope_driver_set_dac(wave,4U,1000000U)==0);
    CHECK(g_dac_dma.Init.FIFOMode==DMA_FIFOMODE_ENABLE);
    CHECK(g_dac_dma.Init.FIFOThreshold==DMA_FIFO_THRESHOLD_FULL);
    CHECK(g_dac_dma.Init.MemBurst==DMA_MBURST_SINGLE && g_dac_dma.Init.PeriphBurst==DMA_PBURST_SINGLE);
    CHECK(g_adc_dma.Init.FIFOMode==DMA_FIFOMODE_DISABLE);
    for(i=1U;i<=8U;i++){
        fake_tick=i*2000U;
        fake_dac.SR=DAC_FLAG_DMAUDR1;scope_driver_dac_irq_handler();
        CHECK(!fake_tim7.running && !(fake_dac.CR&DAC_CR_DMAEN1));
        scope_driver_get_status(&status);
        CHECK(status.dac_enabled && status.acquiring && fake_tim7.running);
        CHECK(status.dac_underruns==i && status.dac_dma_errors==0U);
        CHECK(status.dac_recoveries==i);
        CHECK(fake_dac.CR&DAC_CR_DMAEN1);
        CHECK(dac_starts==(int)i+1 && unsafe_dac_start==0);
        CHECK(memcmp(wave,g_dac_buffer,sizeof(wave))==0);
        CHECK(!(fake_stream3.CR&(DMA_IT_HT|DMA_IT_TC)));
    }
    /* 1秒内最多恢复3次；持续故障不能让主循环反复启动而卡住。 */
    fake_tick+=2000U;
    for(i=0U;i<4U;i++){
        fake_dac.SR=DAC_FLAG_DMAUDR1;scope_driver_dac_irq_handler();
        scope_driver_get_status(&status);
        CHECK(status.dac_enabled==(i<3U));
    }
    CHECK(status.last_error==-9 && status.acquiring && status.dac_underruns==12U);
    {
        int attempts=dac_starts;
        scope_driver_get_status(&status); CHECK(dac_starts==attempts);
    }
    CHECK(scope_driver_set_dac(wave,4U,1000000U)==0);
    fake_dac.SR=DAC_FLAG_DMAUDR1;scope_driver_dac_irq_handler();dac_start_fail=1;
    scope_driver_get_status(&status);
    CHECK(!status.dac_enabled && status.acquiring && status.last_error==-9);
    dac_start_fail=0;
    CHECK(scope_driver_set_dac(wave,4U,1000000U)==0);
    fake_dac.SR=DAC_FLAG_DMAUDR1;scope_driver_dac_irq_handler();
    dac_error_during_abort=1U;
    {
        int attempts=dac_starts;
        scope_driver_get_status(&status);
        CHECK(!status.dac_enabled && status.dac_dma_errors==1U && dac_starts==attempts);
    }
    CHECK(scope_driver_set_dac(wave,4U,1000000U)==0);
    HAL_DAC_ErrorCallbackCh1(&g_dac);
    scope_driver_get_status(&status);
    CHECK(!status.dac_enabled && status.dac_dma_errors==2U && status.acquiring);
    scope_driver_close();check_restored();
}
static void test_frame_publication(void)
{
    uint16_t output[SCOPE_DRIVER_FRAME_SAMPLES];
    uint32_t rate = 999U, sequence = 999U, i;
    reset_fixture(); CHECK(scope_driver_open() == 0); CHECK(scope_driver_start(100000U) == 0);
    for (i = 0U; i < SCOPE_DMA_SAMPLES; ++i) g_adc_buffer[i] = (uint16_t)i;
    CHECK(scope_driver_read(output, SCOPE_DRIVER_FRAME_SAMPLES, &rate, &sequence) == 0);
    HAL_ADC_ConvHalfCpltCallback(&hadc1); fake_stream0.remaining = 3000U;
    CHECK(scope_driver_read(output, SCOPE_DRIVER_FRAME_SAMPLES, &rate, &sequence) == 0);
    CHECK(rate == 999U && sequence == 999U);
    fake_stream0.remaining = 1000U;
    CHECK(scope_driver_read(output, SCOPE_DRIVER_FRAME_SAMPLES, &rate, &sequence) == 1);
    CHECK(rate == 100000U && sequence == 1U && output[0] == 0U && output[2047] == 2047U);
    CHECK(scope_driver_read(output, SCOPE_DRIVER_FRAME_SAMPLES, &rate, &sequence) == 0);
    HAL_ADC_ConvCpltCallback(&hadc1); fake_stream0.remaining = 3500U;
    CHECK(scope_driver_read(output, SCOPE_DRIVER_FRAME_SAMPLES, &rate, &sequence) == 1);
    CHECK(sequence == 2U && output[0] == 2048U && output[2047] == 4095U);
    HAL_ADC_ConvHalfCpltCallback(&hadc1); fake_stream0.remaining = 1000U; copy_interleave = 1;
    CHECK(scope_driver_read(output, SCOPE_DRIVER_FRAME_SAMPLES, &rate, &sequence) == 0);
    CHECK(sequence == 2U && g_read_sequence == 2U && g_scope_status.dropped_frames == 1U);
    fake_stream0.remaining = 1000U; copy_interleave = 2;
    CHECK(scope_driver_read(output, SCOPE_DRIVER_FRAME_SAMPLES, &rate, &sequence) == 0);
    CHECK(sequence == 2U && g_read_sequence == 2U && g_scope_status.dropped_frames == 2U);
    CHECK(scope_driver_read(output, SCOPE_DRIVER_FRAME_SAMPLES, &rate, &sequence) == 1);
    CHECK(sequence == 4U && g_scope_status.dropped_frames == 3U);
    HAL_ADC_ErrorCallback(&hadc1);
    CHECK(scope_driver_read(output, SCOPE_DRIVER_FRAME_SAMPLES, &rate, &sequence) == -7);
    CHECK(!g_scope_status.acquiring && !fake_tim4.running);
    scope_driver_close(); check_restored();
}
static void test_irq_error_clear_critical_section(void)
{
    reset_fixture(); CHECK(scope_driver_open() == 0); CHECK(scope_driver_start(100000U) == 0);
    g_irq_errors = 2U; deferred_adc_error = 1U;
    scope_clear_irq_errors(2U);
    CHECK(g_irq_errors == 1U && fake_primask == 0U && deferred_adc_error == 0U);
    /* 调用者已屏蔽中断时，不得提前开中断；等待中的错误在原调用者恢复时到达。 */
    fake_primask = 1U; g_irq_errors = 3U; deferred_adc_error = 1U;
    global_enable_calls = 0U;
    scope_clear_irq_errors(2U);
    CHECK(fake_primask == 1U && global_enable_calls == 0U && deferred_adc_error == 1U && g_irq_errors == 1U);
    __enable_irq(); CHECK(g_irq_errors == 1U && !deferred_adc_error);
    scope_driver_stop(); g_irq_errors = 3U;
    CHECK(scope_driver_start(100000U) == 0 && g_irq_errors == 2U);
    scope_driver_close(); check_restored();
}
static void test_high_rate_and_dac_isolation(void)
{
    uint16_t table[]={0U,2048U,4095U,2048U};
    uint16_t output[SCOPE_DRIVER_FRAME_SAMPLES];
    scope_driver_status_t status;
    uint32_t rate=0U,sequence=0U;
    reset_fixture();CHECK(scope_driver_open()==0);
    CHECK(scope_driver_start(1000000U)==0);
    CHECK(hadc1.Init.ClockPrescaler==ADC_CLOCK_SYNC_PCLK_DIV4);
    CHECK(configured_sampling==ADC_SAMPLETIME_8CYCLES_5);
    CHECK(scope_driver_set_dac(table,4U,1000000U)==0);
    CHECK(g_dac_dma.Init.Priority>g_adc_dma.Init.Priority);
    CHECK(!(fake_stream3.CR&(DMA_IT_HT|DMA_IT_TC)));
    CHECK((fake_stream3.CR&(DMA_IT_TE|DMA_IT_DME))==(DMA_IT_TE|DMA_IT_DME));
    HAL_DAC_ErrorCallbackCh1(&g_dac);
    CHECK(!fake_tim7.running);
    HAL_ADC_ConvHalfCpltCallback(&hadc1);fake_stream0.remaining=1000U;
    CHECK(scope_driver_read(output,SCOPE_DRIVER_FRAME_SAMPLES,&rate,&sequence)==1);
    scope_driver_get_status(&status);
    CHECK(status.acquiring && !status.dac_enabled && status.dac_dma_errors==1U);
    CHECK(scope_driver_set_dac(table,4U,1000000U)==0);
    fake_dac.SR=DAC_FLAG_DMAUDR1;
    scope_driver_get_status(&status);
    CHECK(status.dac_underruns==1U && status.dac_dma_errors==1U && status.acquiring);
    scope_driver_get_status(&status);CHECK(status.dac_underruns==1U);
    scope_driver_close();check_restored();
    reset_fixture();fake_revision=REV_ID_Y;CHECK(scope_driver_open()==0);
    CHECK(scope_driver_start(1000000U)==0);
    CHECK(hadc1.Init.ClockPrescaler==ADC_CLOCK_ASYNC_DIV2);
    scope_driver_close();check_restored();
    reset_fixture();fake_adc2.CR=ADC_CR_ADEN;
    CHECK(scope_driver_open()==-2 && !resource_held);
}
static void test_io_selection(void)
{
    scope_io_config_t io={{1U,0U},{SCOPE_INPUT_PC4,SCOPE_INPUT_PA5},SCOPE_OUTPUT_PA4};
    reset_fixture();CHECK(scope_driver_open()==0);phy_fail=1;
    CHECK(scope_driver_configure_io(&io)!=0&&!phy_held);
    phy_fail=0;CHECK(scope_driver_configure_io(&io)==0&&phy_held);
    CHECK(scope_driver_start(2000000U)==0);
    CHECK(configured_sampling==ADC_SAMPLETIME_2CYCLES_5);
    scope_driver_stop();io.enabled[1]=1U;
    CHECK(scope_driver_configure_io(&io)==0);
    CHECK(scope_driver_start(2000000U)!=0);
    CHECK(scope_driver_start(1000000U)==0);
    {
        static uint16_t first[2048],second[2048];uint32_t rate=0U,seq=0U,i;
        for(i=0U;i<2048U;i++){g_adc_buffer[i*2U]=(uint16_t)i;
            g_adc_buffer[i*2U+1U]=(uint16_t)(65000U-i);}
        HAL_ADC_ConvHalfCpltCallback(&hadc1);fake_stream0.remaining=1000U;
        CHECK(scope_driver_read_pair(first,second,2048U,&rate,&seq)==1);
        CHECK(first[123]==123U&&second[123]==64877U&&seq==1U);
        CHECK(g_adc_dma.Init.PeriphDataAlignment==DMA_PDATAALIGN_WORD);
        CHECK(g_adc_dma.Init.MemDataAlignment==DMA_MDATAALIGN_WORD);
    }
    scope_driver_stop();
    io.input[1]=SCOPE_INPUT_PC4;CHECK(scope_driver_configure_io(&io)!=0);
    scope_driver_close();CHECK(!phy_held);check_restored();
    reset_fixture();CHECK(scope_driver_open()==0);
    io=(scope_io_config_t){{1U,0U},{SCOPE_INPUT_PA5,SCOPE_INPUT_PC4},SCOPE_OUTPUT_PA5};
    CHECK(scope_driver_configure_io(&io)!=0);
    io.output=SCOPE_OUTPUT_PA4;CHECK(scope_driver_configure_io(&io)==0);
    {uint16_t table[]={0U,4095U};
        CHECK(scope_driver_set_dac(table,2U,1000000U)==0);
        CHECK(g_dac_dma.Init.Request==DMA_REQUEST_DAC1_CH1);
        HAL_DAC_DMAUnderrunCallbackCh1(&g_dac);CHECK(!fake_tim7.running);
        scope_driver_get_status(&g_scope_status);CHECK(g_scope_status.dac_enabled&&fake_tim7.running);
    }
    scope_driver_close();check_restored();
}
static void test_dual_restart(void)
{
    scope_io_config_t io={{1U,1U},{SCOPE_INPUT_PA5,SCOPE_INPUT_PC4},SCOPE_OUTPUT_PA4};
    static uint16_t first[8192],second[8192];
    uint32_t rate=0U,sequence=0U;unsigned i;
    reset_fixture();CHECK(scope_driver_open()==0);
    CHECK(scope_driver_configure_io(&io)==0);
    for(i=0U;i<3U;i++){
        /* 上次从ADC的OVR不应让新一轮启动立刻失败。 */
        ADC2->ISR=ADC_FLAG_OVR;
        CHECK(scope_driver_start_capture(85000U,500U)==0);
        HAL_ADC_ConvHalfCpltCallback(&hadc1);fake_stream0.remaining=500U;
        CHECK(scope_driver_read_pair(first,second,8192U,&rate,&sequence)==1);
        slave_disable_missed=1;scope_driver_stop();slave_disable_missed=0;
        CHECK((ADC2->CR&ADC_CR_ADEN)==0U&&!g_adc_needs_stop);
        io.enabled[1]=0U;CHECK(scope_driver_configure_io(&io)==0);
        CHECK(scope_driver_start_capture(85000U,500U)==0);scope_driver_stop();
        io.enabled[1]=1U;CHECK(scope_driver_configure_io(&io)==0);
    }
    scope_driver_close();check_restored();
}
static void test_adc_failure_stage(void)
{
    scope_io_config_t io={{1U,1U},{SCOPE_INPUT_PA5,SCOPE_INPUT_PC4},SCOPE_OUTPUT_PA4};
    scope_driver_status_t status;
    reset_fixture();CHECK(scope_driver_open()==0);CHECK(scope_driver_configure_io(&io)==0);
    calibration_fail=1;CHECK(scope_driver_start_capture(85000U,500U)==-4);
    scope_driver_get_status(&status);
    CHECK(status.adc_stage==SCOPE_ADC_STAGE_CAL2);
    CHECK(status.adc_cr[1]==ADC2->CR);
    calibration_fail=0;scope_driver_close();check_restored();
}
static void test_dual_calibration_lifetime_with_dac(void)
{
    scope_io_config_t io={{1U,1U},{SCOPE_INPUT_PC4,SCOPE_INPUT_PA5},SCOPE_OUTPUT_PA4};
    const uint16_t table[]={0U,2048U,4095U,2048U};
    static uint16_t first[8192],second[8192];
    uint32_t rate=0U,sequence=0U;unsigned i;
    reset_fixture();CHECK(scope_driver_open()==0);
    CHECK(scope_driver_set_dac(table,4U,10000U)==0);
    CHECK(scope_driver_configure_io(&io)==0);
    for(i=0U;i<8U;++i){
        /* 连续双路重启，覆盖拖动松手和采样档位改变，DAC保持输出。 */
        CHECK(scope_driver_start_capture(i%2U?1000000U:500000U,2512U)==0);
        CHECK(g_scope_status.acquiring&&fake_tim4.running);
        CHECK(g_scope_status.dac_enabled&&fake_tim7.running&&dac_starts==1);
        CHECK((ADC2->CR&ADC_CR_ADCAL)==0U);
        HAL_ADC_ConvHalfCpltCallback(&hadc1);
        fake_stream0.remaining=scope_driver_frame_samples();
        CHECK(scope_driver_read_pair(first,second,8192U,&rate,&sequence)==1);
    }
    CHECK(calibration_in_dual==0U&&slave_calibrations==1U);
    scope_driver_stop();io.enabled[1]=0U;CHECK(scope_driver_configure_io(&io)==0);
    CHECK(scope_driver_start_capture(2000000U,512U)==0);
    scope_driver_stop();io.enabled[1]=1U;CHECK(scope_driver_configure_io(&io)==0);
    CHECK(scope_driver_start_capture(1000000U,512U)==0);
    CHECK(slave_calibrations==1U&&fake_tim7.running&&dac_starts==1);
    scope_driver_close();check_restored();
    CHECK(!calibration_valid[1]&&(ADC2->CR&ADC_CR_DEEPPWD));
    CHECK(scope_driver_open()==0);CHECK(scope_driver_configure_io(&io)==0);
    CHECK(scope_driver_start_capture(1000000U,512U)==0);
    CHECK(slave_calibrations==2U&&calibration_in_dual==0U);
    CHECK(calibration_valid[1]&&!(ADC2->CR&ADC_CR_DEEPPWD));
    scope_driver_close();check_restored();
}
static void test_zoom_record_lengths(void)
{
    static const uint32_t rates[]={100U,1000U,85000U,1000000U};
    uint32_t n;size_t r;
    reset_fixture();CHECK(scope_driver_open()==0);
    for(r=0U;r<sizeof(rates)/sizeof(rates[0]);r++)for(n=64U;n<=500U;n++){
        CHECK(scope_driver_start_capture(rates[r],n)==0);
        CHECK(scope_driver_frame_samples()>=n&&scope_driver_frame_samples()<=2U*n);
        CHECK((scope_driver_frame_samples()%16U)==0U);
    }
    scope_driver_close();check_restored();
}
static void test_fixed_rate_full_windows(void)
{
    static const uint32_t rates[]={100U,200U,500U,1000U,2000U,5000U,10000U,
        20000U,50000U,100000U,200000U,500000U,1000000U,2000000U};
    scope_io_config_t io={{1U,0U},{SCOPE_INPUT_PC4,SCOPE_INPUT_PA5},SCOPE_OUTPUT_PA4};
    uint32_t n;size_t r;unsigned dual;
    reset_fixture();CHECK(scope_driver_open()==0);
    for(dual=0U;dual<2U;dual++){
        scope_driver_stop();io.enabled[1]=(uint8_t)dual;
        CHECK(scope_driver_configure_io(&io)==0);
        for(r=0U;r<sizeof(rates)/sizeof(rates[0]);r++){
            if(dual&&rates[r]>1000000U)continue;
            for(n=512U;n<=8192U;n+=16U){
                CHECK(scope_driver_start_capture(rates[r],n)==0);
                CHECK(scope_driver_frame_samples()>=n&&scope_driver_frame_samples()<=8192U);
                CHECK(scope_driver_frame_samples()%16U==0U);
            }
        }
    }
    scope_driver_close();check_restored();
}
int main(void)
{
    test_open_failure_and_busy(); test_adc_failure_cleanup();
    test_dac_restart_and_shared_irq(); test_frame_publication();
    test_dc_holds_without_dma_or_trigger();
    test_dac_underrun_recovery_is_bounded();
    test_irq_error_clear_critical_section();
    test_high_rate_and_dac_isolation();
    test_io_selection();
    test_dual_restart();
    test_adc_failure_stage();
    test_dual_calibration_lifetime_with_dac();
    test_zoom_record_lengths();
    test_fixed_rate_full_windows();
    {
        static uint16_t output[8192];uint32_t r=0,q=0;
        reset_fixture();CHECK(scope_driver_open()==0);
        CHECK(scope_driver_start_capture(100U,64U)==0);
        CHECK(scope_driver_frame_samples()>=64U);
        g_adc_buffer[15]=4321;HAL_ADC_ConvHalfCpltCallback(&hadc1);fake_stream0.remaining=32U;
        CHECK(scope_driver_read(output,8192,&r,&q)==1 && output[15]==4321);
        CHECK(scope_driver_start_capture(1000000U,65U)==0);
        CHECK(scope_driver_frame_samples()>=65U&&scope_driver_frame_samples()<=130U);
        CHECK(scope_driver_start_capture(85000U,500U)==0);
        CHECK(scope_driver_frame_samples()>=500U&&scope_driver_frame_samples()<=1000U);
        CHECK(scope_driver_start_capture(1000000U,8192U)==0);
        CHECK(scope_driver_frame_samples()==8192U);
        CHECK(scope_driver_start_capture(100U,63U)==-4);
        CHECK(scope_driver_start_capture(100U,8193U)==-4);
        g_adc_buffer[8191]=12345;HAL_ADC_ConvHalfCpltCallback(&hadc1);fake_stream0.remaining=4000U;
        CHECK(scope_driver_read(output,8191,&r,&q)==-6);
        CHECK(scope_driver_read(output,8192,&r,&q)==1 && output[8191]==12345);
        scope_driver_close();check_restored();
    }
    printf("%s oscilloscope_driver checks=%u failures=%u\n", failures ? "FAIL" : "PASS", checks, failures);
    return failures ? 1 : 0;
}
