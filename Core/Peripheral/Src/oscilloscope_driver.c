#include "oscilloscope_driver.h"
#include "oscilloscope_timing.h"
#include "oscilloscope_phy.h"
#include "adc.h"
#include "main.h"
#include "resource_manager.h"
#include "stm32h7xx_hal_dac.h"
#include <string.h>

#define SCOPE_OWNER 0x0310U
#define SCOPE_DMA_SAMPLES (SCOPE_DRIVER_MAX_FRAME_SAMPLES * 2U)
#define SCOPE_DAC_SAMPLES 1024U

/* 独立缓存行，scatter 明确放在 AXI SRAM，DMA1 不能访问 DTCM。 */
static uint16_t g_adc_buffer[SCOPE_DMA_SAMPLES * 2U]
    __attribute__((section(".scope_dma"), aligned(32)));
static uint16_t g_dac_buffer[SCOPE_DAC_SAMPLES]
    __attribute__((section(".scope_dma"), aligned(32)));
static DMA_HandleTypeDef g_adc_dma;
static DMA_HandleTypeDef g_dac_dma;
static TIM_HandleTypeDef g_adc_timer;
static TIM_HandleTypeDef g_dac_timer;
static DAC_HandleTypeDef g_dac;
static ADC_InitTypeDef g_saved_adc_init;
static DMA_HandleTypeDef *g_saved_adc_dma;
static uint32_t g_saved_smpr1, g_saved_smpr2, g_saved_pcsel;
static uint32_t g_saved_sqr[4];
static uint32_t g_saved_pa5_mode, g_saved_pa5_pull;
static uint32_t g_saved_pa4_mode,g_saved_pa4_pull,g_saved_pc4_mode,g_saved_pc4_pull;
static scope_io_config_t g_io={{1U,0U},{SCOPE_INPUT_PA5,SCOPE_INPUT_PC4},SCOPE_OUTPUT_PA4};
static uint8_t g_phy_used;
static ADC_HandleTypeDef g_adc2;
static uint8_t g_dual;
static uint8_t g_adc2_calibrated;
static volatile uint32_t g_completed_sequence;
static volatile uint32_t g_completed_half;
static volatile uint32_t g_irq_errors;
static volatile uint8_t g_irq_acquiring;
static volatile uint8_t g_dac_fault_latched;
static uint8_t g_dac_irq_restore_disabled;
static uint32_t g_dac_dma_count, g_dac_recovery_epoch;
static uint8_t g_dac_recovery_attempts;
static uint8_t g_restore_pending;
static uint8_t g_adc_needs_stop;
static uint32_t g_read_sequence;
static uint32_t g_frame_samples=SCOPE_DRIVER_FRAME_SAMPLES;
static scope_driver_status_t g_scope_status;
static uint32_t dac_channel(void){return DAC_CHANNEL_1;}
static uint32_t dac_it(void){return DAC_IT_DMAUDR1;}
static uint32_t dac_flag(void){return DAC_FLAG_DMAUDR1;}

static uint32_t scope_adc_prescaler(const scope_adc_plan_t *plan)
{
    static const uint32_t prescalers[] = {ADC_CLOCK_ASYNC_DIV1, ADC_CLOCK_ASYNC_DIV2,
        ADC_CLOCK_ASYNC_DIV4, ADC_CLOCK_ASYNC_DIV6, ADC_CLOCK_ASYNC_DIV8,
        ADC_CLOCK_ASYNC_DIV10, ADC_CLOCK_ASYNC_DIV12, ADC_CLOCK_ASYNC_DIV16,
        ADC_CLOCK_ASYNC_DIV32, ADC_CLOCK_ASYNC_DIV64, ADC_CLOCK_ASYNC_DIV128,
        ADC_CLOCK_ASYNC_DIV256};
    static const uint16_t divisors[] = {1U,2U,4U,6U,8U,10U,12U,16U,32U,64U,128U,256U};
    size_t i;
    if (!plan->asynchronous) return ADC_CLOCK_SYNC_PCLK_DIV4;
    for (i=0U;i<sizeof(divisors)/sizeof(divisors[0]);++i)
        if (plan->divisor==divisors[i]) return prescalers[i];
    return ADC_CLOCK_ASYNC_DIV256;
}

static int scope_apply_adc_plan(uint32_t rate)
{
    static const uint32_t sampling[] = {ADC_SAMPLETIME_2CYCLES_5,
        ADC_SAMPLETIME_8CYCLES_5, ADC_SAMPLETIME_16CYCLES_5,
        ADC_SAMPLETIME_32CYCLES_5, ADC_SAMPLETIME_64CYCLES_5,
        ADC_SAMPLETIME_387CYCLES_5, ADC_SAMPLETIME_810CYCLES_5};
    static const uint16_t cycles[] = {5U,17U,33U,65U,129U,775U,1621U};
    scope_adc_plan_t plan;
    ADC_ChannelConfTypeDef channel = {0};
    ADC_MultiModeTypeDef multi={0};
    uint32_t revision = HAL_GetREVID();
    size_t i;
    g_scope_status.adc_stage=SCOPE_ADC_STAGE_PLAN;
    if (!scope_adc_plan_input(HAL_RCC_GetHCLKFreq(),
        HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_ADC),
        (uint8_t)(revision > REV_ID_Y), rate,
        (uint8_t)(scope_io_max_rate(&g_io)>1000000U), &plan)) return 0;
    g_scope_status.adc_stage=SCOPE_ADC_STAGE_STOP;
    if ((ADC2->CR & ADC_CR_ADEN) != 0U) return 0;
    if (HAL_ADC_Stop(&hadc1) != HAL_OK) return 0;
    /* RM0433 25.4.8：任何校准都必须在DUAL=0时进行。
       停采不会清除CCR的DUAL位，先退出上次的双路模式再重新配置。 */
    g_scope_status.adc_stage=SCOPE_ADC_STAGE_MULTI;
    multi.Mode=ADC_MODE_INDEPENDENT;
    if(HAL_ADCEx_MultiModeConfigChannel(&hadc1,&multi)!=HAL_OK)return 0;
    hadc1.Init.ClockPrescaler = scope_adc_prescaler(&plan);
    g_scope_status.adc_stage=SCOPE_ADC_STAGE_INIT1;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) return 0;
    /* HAL在common内还有ADC启用时可能跳过时钟写入，不能只看HAL_OK。 */
    g_scope_status.adc_stage=SCOPE_ADC_STAGE_CLOCK;
    if ((ADC12_COMMON->CCR & (ADC_CCR_CKMODE|ADC_CCR_PRESC)) !=
        hadc1.Init.ClockPrescaler) return 0;
    channel.Channel=g_io.input[g_io.enabled[0]?0:1]==SCOPE_INPUT_PC4?ADC_CHANNEL_4:ADC_CHANNEL_19;
    channel.Rank=ADC_REGULAR_RANK_1;
    channel.SingleDiff=ADC_SINGLE_ENDED; channel.OffsetNumber=ADC_OFFSET_NONE;
    for (i=0U;i<sizeof(cycles)/sizeof(cycles[0]);++i)
        if (cycles[i]==plan.sample_cycles_x2) {channel.SamplingTime=sampling[i];break;}
    g_scope_status.adc_stage=SCOPE_ADC_STAGE_CHANNEL1;
    if (i==sizeof(cycles)/sizeof(cycles[0]) ||
        HAL_ADC_ConfigChannel(&hadc1,&channel)!=HAL_OK) return 0;
    g_dual=(uint8_t)(g_io.enabled[0]&&g_io.enabled[1]);
    if(g_dual){
        g_adc2.Instance=ADC2;g_adc2.Init=hadc1.Init;
        g_adc2.Init.ExternalTrigConv=ADC_SOFTWARE_START;
        g_adc2.Init.ExternalTrigConvEdge=ADC_EXTERNALTRIGCONVEDGE_NONE;
        g_adc2.Init.ConversionDataManagement=ADC_CONVERSIONDATA_DR;
        g_scope_status.adc_stage=SCOPE_ADC_STAGE_INIT2;
        if(HAL_ADC_Init(&g_adc2)!=HAL_OK)return 0;
        channel.Channel=g_io.input[1]==SCOPE_INPUT_PC4?ADC_CHANNEL_4:ADC_CHANNEL_19;
        g_scope_status.adc_stage=SCOPE_ADC_STAGE_CHANNEL2;
        if(HAL_ADC_ConfigChannel(&g_adc2,&channel)!=HAL_OK)return 0;
        /* ADEN=0保留校准，Init也不关闭稳压器；只有DeInit/掉电后才需重做。
           避免拖动、缩放和改档时反复执行完整线性校准。 */
        if(!g_adc2_calibrated){
            g_scope_status.adc_stage=SCOPE_ADC_STAGE_CAL2;
            if(HAL_ADCEx_Calibration_Start(&g_adc2,ADC_CALIB_OFFSET_LINEARITY,ADC_SINGLE_ENDED)!=HAL_OK)return 0;
            g_adc2_calibrated=1U;
        }
        multi.Mode=ADC_DUALMODE_REGSIMULT;multi.DualModeData=ADC_DUALMODEDATAFORMAT_32_10_BITS;
        multi.TwoSamplingDelay=ADC_TWOSAMPLINGDELAY_1CYCLE;
    }else multi.Mode=ADC_MODE_INDEPENDENT;
    g_scope_status.adc_stage=SCOPE_ADC_STAGE_MULTI;
    if(HAL_ADCEx_MultiModeConfigChannel(&hadc1,&multi)!=HAL_OK)return 0;
    g_adc_dma.Init.PeriphDataAlignment=g_dual?DMA_PDATAALIGN_WORD:DMA_PDATAALIGN_HALFWORD;
    g_adc_dma.Init.MemDataAlignment=g_dual?DMA_MDATAALIGN_WORD:DMA_MDATAALIGN_HALFWORD;
    g_scope_status.adc_stage=SCOPE_ADC_STAGE_DMA;
    if(HAL_DMA_Init(&g_adc_dma)!=HAL_OK)return 0;
    g_scope_status.adc_clock_hz=plan.clock_hz;
    g_scope_status.adc_sample_cycles_x2=plan.sample_cycles_x2;
    g_scope_status.chip_revision=revision;
    g_scope_status.adc_asynchronous=plan.asynchronous;
    return 1;
}

static void scope_clear_irq_errors(uint32_t mask)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    g_irq_errors &= ~mask;
    if (primask == 0U) __enable_irq();
}

static int scope_error(int error)
{
    g_scope_status.last_error = error;
    g_scope_status.adc_state[0]=HAL_ADC_GetState(&hadc1);
    g_scope_status.adc_state[1]=HAL_ADC_GetState(&g_adc2);
    g_scope_status.adc_cr[0]=ADC1->CR;g_scope_status.adc_cr[1]=ADC2->CR;
    g_scope_status.errors++;
    return error;
}

static uint32_t scope_timer_clock(void)
{
    RCC_ClkInitTypeDef clocks;
    uint32_t latency;
    uint32_t pclk = HAL_RCC_GetPCLK1Freq();
    HAL_RCC_GetClockConfig(&clocks, &latency);
    if ((RCC->CFGR & RCC_CFGR_TIMPRE) == 0U)
        return clocks.APB1CLKDivider == RCC_APB1_DIV1 ? pclk : pclk * 2U;
    if (clocks.APB1CLKDivider == RCC_APB1_DIV1 ||
        clocks.APB1CLKDivider == RCC_APB1_DIV2 ||
        clocks.APB1CLKDivider == RCC_APB1_DIV4)
        return HAL_RCC_GetHCLKFreq();
    return pclk * 4U;
}

static int scope_configure_timer(TIM_HandleTypeDef *timer,
                                 TIM_TypeDef *instance, uint32_t rate,
                                 uint32_t maximum, uint32_t *actual)
{
    scope_timer_plan_t plan;
    TIM_MasterConfigTypeDef master = {0};
    if (!scope_timer_plan(scope_timer_clock(), rate, maximum, &plan)) return 0;
    timer->Instance = instance;
    timer->Init.Prescaler = plan.prescaler;
    timer->Init.CounterMode = TIM_COUNTERMODE_UP;
    timer->Init.Period = plan.period;
    timer->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    timer->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(timer) != HAL_OK) return 0;
    master.MasterOutputTrigger = TIM_TRGO_UPDATE;
    master.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(timer, &master) != HAL_OK) return 0;
    __HAL_TIM_CLEAR_FLAG(timer, TIM_FLAG_UPDATE);
    *actual = plan.actual_hz;
    return 1;
}

static int scope_configure_dma(DMA_HandleTypeDef *dma,
                               DMA_Stream_TypeDef *stream,
                               uint32_t request, uint32_t direction)
{
    dma->Instance = stream;
    dma->Init.Request = request;
    dma->Init.Direction = direction;
    dma->Init.PeriphInc = DMA_PINC_DISABLE;
    dma->Init.MemInc = DMA_MINC_ENABLE;
    dma->Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    dma->Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    dma->Init.Mode = DMA_CIRCULAR;
    dma->Init.Priority = direction == DMA_MEMORY_TO_PERIPH ?
                            DMA_PRIORITY_VERY_HIGH : DMA_PRIORITY_HIGH;
    dma->Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (direction == DMA_MEMORY_TO_PERIPH) {
        /* DAC预取8个半字，缓冲AXI访存短时延迟；单次传输兼容任意表长。 */
        dma->Init.FIFOMode = DMA_FIFOMODE_ENABLE;
        dma->Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
        dma->Init.MemBurst = DMA_MBURST_SINGLE;
        dma->Init.PeriphBurst = DMA_PBURST_SINGLE;
    }
    return HAL_DMA_Init(dma) == HAL_OK;
}

void scope_driver_stop(void)
{
    if (!g_scope_status.opened) return;
    if (g_adc_timer.Instance != NULL) (void)HAL_TIM_Base_Stop(&g_adc_timer);
    g_irq_acquiring = 0U;
    __DMB();
    if (g_adc_needs_stop)
    {
        HAL_StatusTypeDef stopped=g_dual?HAL_ADCEx_MultiModeStop_DMA(&hadc1):HAL_ADC_Stop_DMA(&hadc1);
        /* 此版HAL可能漏报从ADC禁用失败；主DMA停止后，用所属句柄补停并核对硬件。 */
        if(stopped==HAL_OK&&g_dual&&(ADC2->CR&ADC_CR_ADEN)!=0U)
            stopped=HAL_ADC_Stop(&g_adc2);
        if (stopped == HAL_OK && (ADC1->CR&ADC_CR_ADEN)==0U &&
            (!g_dual||(ADC2->CR&ADC_CR_ADEN)==0U))
            g_adc_needs_stop = 0U;
        else {g_scope_status.adc_stage=SCOPE_ADC_STAGE_STOP;(void)scope_error(-10);}
    }
    g_scope_status.acquiring = 0U;
    g_read_sequence = g_completed_sequence;
}

void scope_driver_disable_dac(void)
{
    if (g_dac_timer.Instance != NULL) (void)HAL_TIM_Base_Stop(&g_dac_timer);
    if (g_dac.Instance != NULL)
    {
        __HAL_DAC_DISABLE_IT(&g_dac, dac_it());
        if (g_dac.DMA_Handle1 != NULL||g_dac.DMA_Handle2!=NULL)
            (void)HAL_DAC_Stop_DMA(&g_dac, dac_channel());
        (void)HAL_DAC_Stop(&g_dac, dac_channel());
        __HAL_DAC_CLEAR_FLAG(&g_dac, dac_flag());
        (void)HAL_DAC_DeInit(&g_dac);
        g_dac.Instance = NULL;
    }
    g_scope_status.dac_enabled = 0U;
    g_scope_status.dac_rate_hz = 0U;
    g_dac_fault_latched = 0U;
    g_dac_dma_count = 0U;
    g_dac_recovery_attempts = 0U;
    if (g_dac_irq_restore_disabled) {
        HAL_NVIC_DisableIRQ(TIM6_DAC_IRQn);
        g_dac_irq_restore_disabled = 0U;
    }
}

void scope_driver_close(void)
{
    int restored;
    if (!g_scope_status.opened) return;
    scope_driver_disable_dac();
    scope_driver_stop();
    if (g_adc_needs_stop)
    {
        g_restore_pending = 1U;
        return; /* 停止失败不覆盖句柄，不用运行中的ADC调用Init；close可重试。 */
    }
    HAL_NVIC_DisableIRQ(DMA1_Stream0_IRQn);
    HAL_NVIC_DisableIRQ(DMA1_Stream3_IRQn);
    HAL_NVIC_ClearPendingIRQ(DMA1_Stream0_IRQn);
    HAL_NVIC_ClearPendingIRQ(DMA1_Stream3_IRQn);
    if (g_adc_dma.Instance != NULL) (void)HAL_DMA_DeInit(&g_adc_dma);
    if (g_dac_dma.Instance != NULL) (void)HAL_DMA_DeInit(&g_dac_dma);
    if(g_adc2.Instance!=NULL){
        (void)HAL_ADC_Stop(&g_adc2);(void)HAL_ADC_DeInit(&g_adc2);
        g_adc2_calibrated=0U;
    }
    {
        ADC_MultiModeTypeDef independent={0};independent.Mode=ADC_MODE_INDEPENDENT;
        if(HAL_ADCEx_MultiModeConfigChannel(&hadc1,&independent)!=HAL_OK){
            g_restore_pending=1U;(void)scope_error(-10);return;
        }
    }
    hadc1.DMA_Handle = g_saved_adc_dma;
    hadc1.Init = g_saved_adc_init;
    if ((HAL_ADC_GetState(&hadc1) & HAL_ADC_STATE_ERROR_INTERNAL) != 0U)
        (void)HAL_ADC_DeInit(&hadc1);
    restored = HAL_ADC_Init(&hadc1) == HAL_OK;
    if (restored)
    {
        ADC1->SMPR1 = g_saved_smpr1;
        ADC1->SMPR2 = g_saved_smpr2;
        ADC1->PCSEL = g_saved_pcsel;
        ADC1->SQR1 = g_saved_sqr[0]; ADC1->SQR2 = g_saved_sqr[1];
        ADC1->SQR3 = g_saved_sqr[2]; ADC1->SQR4 = g_saved_sqr[3];
    }
    GPIOA->MODER = (GPIOA->MODER & ~(3UL << 10)) | g_saved_pa5_mode;
    GPIOA->PUPDR = (GPIOA->PUPDR & ~(3UL << 10)) | g_saved_pa5_pull;
    GPIOA->MODER=(GPIOA->MODER&~(3UL<<8))|g_saved_pa4_mode;
    GPIOA->PUPDR=(GPIOA->PUPDR&~(3UL<<8))|g_saved_pa4_pull;
    if(g_phy_used){
        GPIOC->MODER=(GPIOC->MODER&~(3UL<<8))|g_saved_pc4_mode;
        GPIOC->PUPDR=(GPIOC->PUPDR&~(3UL<<8))|g_saved_pc4_pull;
    }
    if(scope_phy_release())g_phy_used=0U;else restored=0;
    __HAL_RCC_TIM4_CLK_DISABLE();
    __HAL_RCC_TIM7_CLK_DISABLE();
    /* DMA1 与其他外设共用时钟，不能在这里关闭。 */
    if (restored)
    {
        g_restore_pending = 0U;
        g_scope_status.opened = 0U;
        (void)resource_release(RESOURCE_TIM2_PA5, SCOPE_OWNER);
    }
    else
    {
        g_restore_pending = 1U;
        (void)scope_error(-10); /* 保留快照和占用，close 可重试恢复。 */
    }
}

int scope_driver_open(void)
{
    ADC_ChannelConfTypeDef channel = {0};
    GPIO_InitTypeDef gpio = {0};
    if (g_restore_pending) return scope_error(-10);
    if (g_scope_status.opened) return 0;
    if (resource_acquire(RESOURCE_TIM2_PA5, SCOPE_OWNER) != RESOURCE_OK)
        return scope_error(-2);
    if ((HAL_ADC_GetState(&hadc1) & HAL_ADC_STATE_REG_BUSY) != 0U)
    {
        (void)resource_release(RESOURCE_TIM2_PA5, SCOPE_OWNER);
        return scope_error(-2);
    }
    if ((ADC2->CR & ADC_CR_ADEN) != 0U) {
        (void)resource_release(RESOURCE_TIM2_PA5, SCOPE_OWNER);
        return scope_error(-2);
    }
    memset(&g_scope_status, 0, sizeof(g_scope_status));
    memset(&g_adc_dma, 0, sizeof(g_adc_dma));
    memset(&g_dac_dma, 0, sizeof(g_dac_dma));
    memset(&g_adc2,0,sizeof(g_adc2));g_dual=0U;g_adc2_calibrated=0U;g_phy_used=0U;
    g_saved_adc_init = hadc1.Init;
    g_saved_adc_dma = hadc1.DMA_Handle;
    g_saved_smpr1 = ADC1->SMPR1; g_saved_smpr2 = ADC1->SMPR2;
    g_saved_pcsel = ADC1->PCSEL;
    g_saved_sqr[0] = ADC1->SQR1; g_saved_sqr[1] = ADC1->SQR2;
    g_saved_sqr[2] = ADC1->SQR3; g_saved_sqr[3] = ADC1->SQR4;
    g_saved_pa5_mode = GPIOA->MODER & (3UL << 10);
    g_saved_pa5_pull = GPIOA->PUPDR & (3UL << 10);
    g_saved_pa4_mode=GPIOA->MODER&(3UL<<8);g_saved_pa4_pull=GPIOA->PUPDR&(3UL<<8);
    __HAL_RCC_GPIOC_CLK_ENABLE();
    g_saved_pc4_mode=GPIOC->MODER&(3UL<<8);g_saved_pc4_pull=GPIOC->PUPDR&(3UL<<8);
    g_io=(scope_io_config_t){{1U,0U},{SCOPE_INPUT_PA5,SCOPE_INPUT_PC4},SCOPE_OUTPUT_PA4};
    g_scope_status.opened = 1U;
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_TIM4_CLK_ENABLE();
    __HAL_RCC_TIM7_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_5;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);
    if (!scope_configure_dma(&g_adc_dma, DMA1_Stream0, DMA_REQUEST_ADC1,
                              DMA_PERIPH_TO_MEMORY)) goto fail;
    __HAL_LINKDMA(&hadc1, DMA_Handle, g_adc_dma);
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIG_T4_TRGO;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    hadc1.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) goto fail;
    channel.Channel = ADC_CHANNEL_19;
    channel.Rank = ADC_REGULAR_RANK_1;
    channel.SamplingTime = ADC_SAMPLETIME_16CYCLES_5;
    channel.SingleDiff = ADC_SINGLE_ENDED;
    channel.OffsetNumber = ADC_OFFSET_NONE;
    if (HAL_ADC_ConfigChannel(&hadc1, &channel) != HAL_OK) goto fail;
    if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET_LINEARITY,
                                    ADC_SINGLE_ENDED) != HAL_OK) goto fail;
    HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 6U, 0U);
    HAL_NVIC_ClearPendingIRQ(DMA1_Stream0_IRQn);
    HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
    HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 6U, 0U);
    HAL_NVIC_ClearPendingIRQ(DMA1_Stream3_IRQn);
    HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
    return 0;
fail:
    scope_driver_close();
    return scope_error(-3);
}

uint32_t scope_driver_frame_samples(void) { return g_frame_samples; }
int scope_driver_configure_io(const scope_io_config_t *c)
{
    GPIO_InitTypeDef pin={0};
    if(!g_scope_status.opened||g_restore_pending||g_adc_needs_stop||
       g_scope_status.acquiring||!scope_io_valid(c)||
       (g_scope_status.dac_enabled&&(scope_io_dac_conflict(c)||c->output!=g_io.output)))
        return scope_error(-12);
    if((c->enabled[0]&&c->input[0]==SCOPE_INPUT_PC4)||
       (c->enabled[1]&&c->input[1]==SCOPE_INPUT_PC4)){
        if(!scope_phy_acquire())return scope_error(-13);
        g_phy_used=1U;pin.Pin=GPIO_PIN_4;pin.Mode=GPIO_MODE_ANALOG;pin.Pull=GPIO_NOPULL;
        HAL_GPIO_Init(GPIOC,&pin);
    }
    g_io=*c;return 0;
}
static int scope_start_block(uint32_t rate,uint32_t block)
{
    uint32_t actual;
    if (!g_scope_status.opened || g_restore_pending) return scope_error(-1);
    g_scope_status.adc_stage=SCOPE_ADC_STAGE_RATE;
    if (rate < SCOPE_DRIVER_MIN_RATE_HZ || rate > scope_io_max_rate(&g_io))
        return scope_error(-4);
    scope_driver_stop();
    if(g_adc_needs_stop)return scope_error(-10);
    if(g_phy_used&&!scope_phy_check())return scope_error(-13);
    if ((HAL_ADC_GetState(&hadc1) & HAL_ADC_STATE_REG_BUSY) != 0U)
        return scope_error(-10);
    g_scope_status.adc_stage=SCOPE_ADC_STAGE_TIMER;
    if (!scope_configure_timer(&g_adc_timer, TIM4, rate,
                               SCOPE_DRIVER_MAX_RATE_HZ, &actual))
        return scope_error(-4);
    if (!scope_apply_adc_plan(actual)) return scope_error(-4);
    g_frame_samples=block;
    scope_clear_irq_errors(1U);
    g_completed_sequence = 0U;
    g_completed_half = 0U;
    g_read_sequence = 0U;
    SCB_CleanInvalidateDCache_by_Addr((uint32_t *)g_adc_buffer,
                                     (int32_t)sizeof(g_adc_buffer));
    __DSB();
    /* 双路HAL只清主ADC标志；从ADC旧OVR不能污染新一轮采集。 */
    if(g_dual)__HAL_ADC_CLEAR_FLAG(&g_adc2,ADC_FLAG_EOC|ADC_FLAG_EOS|ADC_FLAG_OVR);
    g_scope_status.acquiring = 1U;
    g_adc_needs_stop = 1U;
    g_irq_acquiring = 1U;
    __DMB();
    g_scope_status.adc_stage=SCOPE_ADC_STAGE_START;
    if ((g_dual?HAL_ADCEx_MultiModeStart_DMA(&hadc1,(uint32_t *)g_adc_buffer,2U*g_frame_samples):
         HAL_ADC_Start_DMA(&hadc1,(uint32_t *)g_adc_buffer,2U*g_frame_samples)) != HAL_OK)
    {
        /* HAL即使DMA启动失败也可能已经arm ADC，必须执行完整停止。 */
        scope_driver_stop();
        return scope_error(-5);
    }
    g_scope_status.adc_stage=SCOPE_ADC_STAGE_TRIGGER;
    if (HAL_TIM_Base_Start(&g_adc_timer) != HAL_OK)
    {
        scope_driver_stop();
        return scope_error(-5);
    }
    g_scope_status.sample_rate_hz = actual;
    g_scope_status.last_error = 0;
    g_scope_status.adc_stage=SCOPE_ADC_STAGE_NONE;
    return 0;
}

int scope_driver_start(uint32_t rate)
{ return scope_start_block(rate,SCOPE_DRIVER_FRAME_SAMPLES); }
int scope_driver_start_capture(uint32_t rate,uint32_t count)
{
    uint32_t block,limit,minimum;
    if(count<64U||count>SCOPE_DRIVER_MAX_FRAME_SAMPLES)return scope_error(-4);
    block=(rate/40U)&~15U;if(block<16U)block=16U;
    /* 每个完整DMA半块至少含一条记录，丢块后也能独立更新；
       触发余量最多使用会话的2N容量，仍保持16对样本/缓存行对齐。 */
    minimum=(count+15U)&~15U;
    limit=(count*2U)&~15U;
    if(limit>SCOPE_DRIVER_MAX_FRAME_SAMPLES)limit=SCOPE_DRIVER_MAX_FRAME_SAMPLES;
    if(block<minimum)block=minimum;
    if(block>limit)block=limit;
    return scope_start_block(rate,block);
}

int scope_driver_read(uint16_t *samples, uint32_t capacity,
                      uint32_t *rate, uint32_t *sequence)
{return scope_driver_read_pair(samples,NULL,capacity,rate,sequence);}
int scope_driver_read_pair(uint16_t *samples,uint16_t *second,uint32_t capacity,
                      uint32_t *rate,uint32_t *sequence)
{
    uint32_t before, half, after;
    uint32_t distance,i;uint16_t *source;
    if ((g_io.enabled[0]&&samples==NULL)||(g_io.enabled[1]&&second==NULL)||rate == NULL || sequence == NULL ||
        capacity < g_frame_samples) return scope_error(-6);
    if (!g_scope_status.opened || !g_scope_status.acquiring) return 0;
    if ((g_irq_errors & 1U) != 0U || (ADC1->ISR & ADC_FLAG_OVR) != 0U||
        (g_dual&&(ADC2->ISR&ADC_FLAG_OVR)!=0U))
    {
        g_scope_status.adc_errors++;
        scope_driver_stop();
        return scope_error(-7);
    }
    before = g_completed_sequence;
    __DMB();
    half = g_completed_half;
    if (before == g_read_sequence) return 0;
    if (!scope_dma_half_readable(__HAL_DMA_GET_COUNTER(&g_adc_dma),
                                 g_frame_samples, half)) return 0;
    source=&g_adc_buffer[half*g_frame_samples*(g_dual?2U:1U)];
    SCB_InvalidateDCache_by_Addr((uint32_t *)source,
        (int32_t)(g_frame_samples*sizeof(uint16_t)*(g_dual?2U:1U)));
    __DSB();
    if(g_dual){for(i=0U;i<g_frame_samples;i++){samples[i]=source[i*2U];second[i]=source[i*2U+1U];}}
    else memcpy(g_io.enabled[0]?samples:second,source,g_frame_samples*sizeof(uint16_t));
    __DMB();
    after = g_completed_sequence;
    if (after != before || !scope_dma_half_readable(
            __HAL_DMA_GET_COUNTER(&g_adc_dma), g_frame_samples, half))
    {
        g_scope_status.dropped_frames++;
        return 0; /* 复制期间 DMA 进入当前半区，丢弃，禁止发布混合帧。 */
    }
    distance = before - g_read_sequence;
    if (distance > 1U) g_scope_status.dropped_frames += distance - 1U;
    g_read_sequence = before;
    g_scope_status.dma_sequence = before;
    *rate = g_scope_status.sample_rate_hz;
    *sequence = before;
    return 1;
}

static HAL_StatusTypeDef scope_start_dac_dma(uint32_t count)
{
    uint32_t irq_enabled = NVIC_GetEnableIRQ(TIM6_DAC_IRQn);
    HAL_StatusTypeDef started;
    /* HAL短暂开启UDR中断；共享TIM6 IRQ暂遮断，保留pending及原使能状态。 */
    HAL_NVIC_DisableIRQ(TIM6_DAC_IRQn);
    started = HAL_DAC_Start_DMA(&g_dac, dac_channel(), (uint32_t *)g_dac_buffer,
                                count, DAC_ALIGN_12B_R);
    if (irq_enabled != 0U || started == HAL_OK) HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
    if (started == HAL_OK) {
        /* 静态循环表无需CPU补数；错误中断仍保留。 */
        __HAL_DMA_DISABLE_IT(&g_dac_dma, DMA_IT_HT|DMA_IT_TC);
    }
    return started;
}

int scope_driver_set_dac(const uint16_t *table, uint32_t count,
                         uint32_t rate)
{
    DAC_ChannelConfTypeDef channel = {0};
    GPIO_InitTypeDef gpio = {0};
    uint32_t i, actual;
    uint8_t constant = 1U;
    if (!g_scope_status.opened || g_restore_pending || scope_io_dac_conflict(&g_io)||table == NULL || count < 2U ||
        count > SCOPE_DAC_SAMPLES || rate == 0U ||
        rate > SCOPE_DRIVER_DAC_MAX_RATE_HZ) return scope_error(-8);
    for (i = 0U; i < count; ++i) {
        if (table[i] > 4095U) return scope_error(-8);
        if (table[i] != table[0]) constant = 0U;
    }
    scope_driver_disable_dac();
    __HAL_RCC_DAC12_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_4;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);
    memset(&g_dac, 0, sizeof(g_dac));
    g_dac.Instance = DAC1;
    if (HAL_DAC_Init(&g_dac) != HAL_OK) goto fail_dac;
    if (!constant && !scope_configure_dma(&g_dac_dma, DMA1_Stream3,
                              DMA_REQUEST_DAC1_CH1,
                              DMA_MEMORY_TO_PERIPH)) goto fail_dac;
    if (!constant) { __HAL_LINKDMA(&g_dac,DMA_Handle1,g_dac_dma); }
    channel.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
    channel.DAC_Trigger = constant ? DAC_TRIGGER_NONE : DAC_TRIGGER_T7_TRGO;
    channel.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    channel.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_EXTERNAL;
    channel.DAC_UserTrimming = DAC_TRIMMING_FACTORY;
    if (HAL_DAC_ConfigChannel(&g_dac, &channel, dac_channel()) != HAL_OK)
        goto fail_dac;
    scope_clear_irq_errors(6U);
    __HAL_DAC_CLEAR_FLAG(&g_dac, dac_flag());
    if (constant) {
        /* 直流由输出寄存器持续保持，不启动TIM7/DMA，不存在补数欠载。 */
        if (HAL_DAC_SetValue(&g_dac, dac_channel(), DAC_ALIGN_12B_R, table[0]) != HAL_OK ||
            HAL_DAC_Start(&g_dac, dac_channel()) != HAL_OK) goto fail_dac;
        g_scope_status.dac_enabled = 1U;
        g_scope_status.dac_rate_hz = 0U;
        g_scope_status.last_error = 0;
        return 0;
    }
    if (!scope_configure_timer(&g_dac_timer, TIM7, rate,
                               SCOPE_DRIVER_DAC_MAX_RATE_HZ, &actual)) goto fail_dac;
    memcpy(g_dac_buffer, table, count * sizeof(uint16_t));
    SCB_CleanDCache_by_Addr((uint32_t *)g_dac_buffer,
                            (int32_t)((count * sizeof(uint16_t) + 31U) & ~31U));
    __DSB();
    if (HAL_DAC_SetValue(&g_dac, dac_channel(), DAC_ALIGN_12B_R, table[0]) != HAL_OK)
        goto fail_dac;
    scope_clear_irq_errors(6U);
    __HAL_DAC_CLEAR_FLAG(&g_dac, dac_flag());
    g_dac_irq_restore_disabled = (uint8_t)(NVIC_GetEnableIRQ(TIM6_DAC_IRQn) == 0U);
    if (scope_start_dac_dma(count) != HAL_OK) goto fail_dac;
    if (HAL_TIM_Base_Start(&g_dac_timer) != HAL_OK) goto fail_dac;
    g_dac_dma_count = count;
    g_dac_recovery_epoch = HAL_GetTick();
    g_scope_status.dac_enabled = 1U;
    g_scope_status.dac_rate_hz = actual;
    g_scope_status.last_error = 0;
    return 0;
fail_dac:
    scope_driver_disable_dac();
    return scope_error(-9);
}

void scope_driver_get_status(scope_driver_status_t *status)
{
    if (g_scope_status.dac_enabled &&
        (g_dac_fault_latched != 0U || (g_irq_errors & 6U) != 0U ||
         (DAC1->SR & dac_flag()) != 0U))
    {
        uint8_t recovered = 0U;
        if ((g_irq_errors & 4U) != 0U ||
            (DAC1->SR & dac_flag()) != 0U) g_scope_status.dac_underruns++;
        if ((g_irq_errors & 2U) != 0U) g_scope_status.dac_dma_errors++;
        if ((g_irq_errors & 2U) == 0U && g_dac_dma_count != 0U) {
            uint32_t now = HAL_GetTick();
            if ((uint32_t)(now-g_dac_recovery_epoch) >= 1000U) {
                g_dac_recovery_epoch = now;
                g_dac_recovery_attempts = 0U;
            }
            if (g_dac_recovery_attempts < 3U) {
                ++g_dac_recovery_attempts;
                /* HAL欠载时已清DMAEN，必须停表、Abort后重新装载，不能只清SR。
                   所有HAL操作仅在主循环执行，中断仍只锁存并停触发。 */
                (void)HAL_TIM_Base_Stop(&g_dac_timer);
                __HAL_DAC_DISABLE_IT(&g_dac, dac_it());
                if (HAL_DAC_Stop_DMA(&g_dac, dac_channel()) == HAL_OK) {
                    /* Abort期间DMA错误IRQ仍可抢占；只清欠载，不能擦掉新错误。 */
                    scope_clear_irq_errors(4U);
                    g_dac_fault_latched = 0U;
                    __HAL_DAC_CLEAR_FLAG(&g_dac, dac_flag());
                    if ((g_irq_errors & 2U) != 0U) ++g_scope_status.dac_dma_errors;
                    else if (HAL_DAC_SetValue(&g_dac, dac_channel(), DAC_ALIGN_12B_R, g_dac_buffer[0]) == HAL_OK &&
                        scope_start_dac_dma(g_dac_dma_count) == HAL_OK &&
                        HAL_TIM_Base_Start(&g_dac_timer) == HAL_OK) {
                        ++g_scope_status.dac_recoveries;
                        recovered = 1U;
                    }
                }
            }
        }
        if (!recovered) {
            scope_driver_disable_dac();
            scope_clear_irq_errors(6U);
            (void)scope_error(-9);
        }
    }
    if (status != NULL) *status = g_scope_status;
}

void scope_driver_dac_irq_handler(void)
{
    /* HAL启动DMA后中断即有效，不依赖稍后发布的主循环状态。 */
    if (g_dac.Instance != NULL)
        HAL_DAC_IRQHandler(&g_dac);
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *adc)
{
    if (adc != &hadc1 || !g_irq_acquiring) return;
    g_completed_half = 0U;
    __DMB();
    g_completed_sequence++;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *adc)
{
    if (adc != &hadc1 || !g_irq_acquiring) return;
    g_completed_half = 1U;
    __DMB();
    g_completed_sequence++;
}

void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *adc)
{
    if (adc == &hadc1 && g_irq_acquiring) g_irq_errors |= 1U;
}

void HAL_DAC_ErrorCallbackCh1(DAC_HandleTypeDef *dac)
{
    if (dac == &g_dac) {
        g_irq_errors |= 2U;
        g_dac_fault_latched = 1U;
        /* 中断上下文只做有界寄存器操作，立即停止TIM7触发。 */
        if (g_dac_timer.Instance != NULL) __HAL_TIM_DISABLE(&g_dac_timer);
    }
}

void HAL_DAC_DMAUnderrunCallbackCh1(DAC_HandleTypeDef *dac)
{
    if (dac == &g_dac) {
        /* HAL调用前已清除SR，必须独立锁存欠载原因。 */
        g_irq_errors |= 4U;
        g_dac_fault_latched = 1U;
        __HAL_DAC_DISABLE_IT(dac, dac_it());
        if (g_dac_timer.Instance != NULL) __HAL_TIM_DISABLE(&g_dac_timer);
    }
}

void HAL_DACEx_ErrorCallbackCh2(DAC_HandleTypeDef *dac){HAL_DAC_ErrorCallbackCh1(dac);}
void HAL_DACEx_DMAUnderrunCallbackCh2(DAC_HandleTypeDef *dac){HAL_DAC_DMAUnderrunCallbackCh1(dac);}

void DMA1_Stream0_IRQHandler(void)
{
    if (g_scope_status.opened) HAL_DMA_IRQHandler(&g_adc_dma);
}

void DMA1_Stream3_IRQHandler(void)
{
    if (g_scope_status.opened && g_dac.Instance != NULL)
        HAL_DMA_IRQHandler(&g_dac_dma);
}
