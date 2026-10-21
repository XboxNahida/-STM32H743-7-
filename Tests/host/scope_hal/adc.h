#ifndef SCOPE_FAKE_HAL_H
#define SCOPE_FAKE_HAL_H
/* 真实驱动的宿主夹具：仅模拟本驱动访问的寄存器及 HAL 契约。 */
#include <stdint.h>
#include <stddef.h>
typedef enum { HAL_OK, HAL_ERROR, HAL_BUSY } HAL_StatusTypeDef;
typedef struct { uint32_t SMPR1, SMPR2, PCSEL, SQR1, SQR2, SQR3, SQR4, ISR, CR; } ADC_TypeDef;
typedef struct { uint32_t CCR; } ADC_Common_TypeDef;
typedef struct { uint32_t MODER, PUPDR; } GPIO_TypeDef;
typedef struct { uint32_t CFGR; } RCC_TypeDef;
typedef struct { uint32_t CR, SR, DHR12R1, DOR1; } DAC_TypeDef;
typedef struct { uint32_t running; } TIM_TypeDef;
typedef struct { uint32_t remaining, CR; } DMA_Stream_TypeDef;
typedef struct {
    uint32_t Request, Direction, PeriphInc, MemInc, PeriphDataAlignment;
    uint32_t MemDataAlignment, Mode, Priority, FIFOMode, FIFOThreshold, MemBurst, PeriphBurst;
} DMA_InitTypeDef;
typedef struct { DMA_Stream_TypeDef *Instance; DMA_InitTypeDef Init; void *Parent; } DMA_HandleTypeDef;
typedef struct { uint32_t ContinuousConvMode, ExternalTrigConv, ExternalTrigConvEdge, ConversionDataManagement, ClockPrescaler; } ADC_InitTypeDef;
typedef struct { ADC_TypeDef *Instance; ADC_InitTypeDef Init; DMA_HandleTypeDef *DMA_Handle; uint32_t State; } ADC_HandleTypeDef;
typedef struct { uint32_t Channel, Rank, SamplingTime, SingleDiff, OffsetNumber; } ADC_ChannelConfTypeDef;
typedef struct { uint32_t Mode,DualModeData,TwoSamplingDelay; } ADC_MultiModeTypeDef;
#define ADC_SOFTWARE_START 0U
#define ADC_EXTERNALTRIGCONVEDGE_NONE 0U
#define ADC_CONVERSIONDATA_DR 0U
#define ADC_MODE_INDEPENDENT 0U
#define ADC_DUALMODE_REGSIMULT 1U
#define ADC_DUALMODEDATAFORMAT_32_10_BITS 2U
#define ADC_TWOSAMPLINGDELAY_1CYCLE 0U
#define DMA_PDATAALIGN_WORD 2U
#define DMA_MDATAALIGN_WORD 2U
typedef struct { uint32_t Prescaler, CounterMode, Period, ClockDivision, AutoReloadPreload; } TIM_Base_InitTypeDef;
typedef struct { TIM_TypeDef *Instance; TIM_Base_InitTypeDef Init; } TIM_HandleTypeDef;
typedef struct { uint32_t MasterOutputTrigger, MasterSlaveMode; } TIM_MasterConfigTypeDef;
typedef struct { DAC_TypeDef *Instance; DMA_HandleTypeDef *DMA_Handle1,*DMA_Handle2; } DAC_HandleTypeDef;
#define DAC_CHANNEL_2 1U
#define DAC_IT_DMAUDR2 (1U<<16U)
#define DAC_FLAG_DMAUDR2 (1U<<16U)
#define DMA_REQUEST_DAC1_CH2 3U
void HAL_DACEx_DMAUnderrunCallbackCh2(DAC_HandleTypeDef *dac);
typedef struct { uint32_t DAC_SampleAndHold, DAC_Trigger, DAC_OutputBuffer, DAC_ConnectOnChipPeripheral, DAC_UserTrimming; } DAC_ChannelConfTypeDef;
typedef struct { uint32_t Pin, Mode, Pull; } GPIO_InitTypeDef;
typedef struct { uint32_t APB1CLKDivider; } RCC_ClkInitTypeDef;
extern ADC_TypeDef fake_adc;
extern ADC_TypeDef fake_adc2;
extern ADC_Common_TypeDef fake_adc_common;
extern uint32_t fake_revision;
extern GPIO_TypeDef fake_gpio;
extern RCC_TypeDef fake_rcc;
extern DAC_TypeDef fake_dac;
extern TIM_TypeDef fake_tim4, fake_tim7;
extern DMA_Stream_TypeDef fake_stream0, fake_stream3;
extern ADC_HandleTypeDef hadc1;
#define ADC1 (&fake_adc)
#define ADC2 (&fake_adc2)
#define ADC12_COMMON (&fake_adc_common)
#define ADC_CR_ADEN 1U
#define ADC_CR_ADCAL (1UL << 31)
#define ADC_CR_ADCALLIN (1UL << 16)
#define ADC_CR_LINCALRDYW1 (1UL << 22)
#define ADC_CR_LINCALRDYW6 (1UL << 27)
#define ADC_CR_ADVREGEN (1UL << 28)
#define ADC_CR_DEEPPWD (1UL << 29)
#define ADC_FLAG_EOC 2U
#define ADC_FLAG_EOS 4U
#define __HAL_ADC_CLEAR_FLAG(h,f) ((h)->Instance->ISR &= ~(f))
#define ADC_CCR_DUAL 0x1fU
#define ADC_CCR_CKMODE (3UL << 16)
#define ADC_CCR_PRESC (15UL << 18)
#define REV_ID_Y 0x1003U
#define REV_ID_V 0x2003U
#define HAL_GetREVID() fake_revision
#define ADC_CLOCK_SYNC_PCLK_DIV4 (3UL << 16)
#define ADC_CLOCK_ASYNC_DIV1 0x00U
#define ADC_CLOCK_ASYNC_DIV2 (1UL << 18)
#define ADC_CLOCK_ASYNC_DIV4 (2UL << 18)
#define ADC_CLOCK_ASYNC_DIV6 (3UL << 18)
#define ADC_CLOCK_ASYNC_DIV8 (4UL << 18)
#define ADC_CLOCK_ASYNC_DIV10 (5UL << 18)
#define ADC_CLOCK_ASYNC_DIV12 (6UL << 18)
#define ADC_CLOCK_ASYNC_DIV16 (7UL << 18)
#define ADC_CLOCK_ASYNC_DIV32 (8UL << 18)
#define ADC_CLOCK_ASYNC_DIV64 (9UL << 18)
#define ADC_CLOCK_ASYNC_DIV128 (10UL << 18)
#define ADC_CLOCK_ASYNC_DIV256 (11UL << 18)
#define GPIOA (&fake_gpio)
#define GPIOC (&fake_gpio)
#define __HAL_RCC_GPIOC_CLK_ENABLE() ((void)0)
#define RCC (&fake_rcc)
#define DAC1 (&fake_dac)
#define TIM4 (&fake_tim4)
#define TIM7 (&fake_tim7)
#define DMA1_Stream0 (&fake_stream0)
#define DMA1_Stream3 (&fake_stream3)
enum { DMA1_Stream0_IRQn, DMA1_Stream3_IRQn, TIM6_DAC_IRQn, FAKE_IRQ_COUNT };
#define HAL_ADC_STATE_REG_BUSY 1U
#define HAL_ADC_STATE_ERROR_INTERNAL 2U
#define RCC_CFGR_TIMPRE 1U
#define RCC_APB1_DIV1 1U
#define RCC_APB1_DIV2 2U
#define RCC_APB1_DIV4 4U
#define RCC_PERIPHCLK_ADC 1U
#define TIM_COUNTERMODE_UP 0U
#define TIM_CLOCKDIVISION_DIV1 0U
#define TIM_AUTORELOAD_PRELOAD_DISABLE 0U
#define TIM_TRGO_UPDATE 1U
#define TIM_MASTERSLAVEMODE_DISABLE 0U
#define TIM_FLAG_UPDATE 1U
#define DMA_PINC_DISABLE 0U
#define DMA_MINC_ENABLE 1U
#define DMA_PDATAALIGN_HALFWORD 1U
#define DMA_MDATAALIGN_HALFWORD 1U
#define DMA_CIRCULAR 1U
#define DMA_PRIORITY_HIGH 1U
#define DMA_PRIORITY_VERY_HIGH 2U
#define DMA_IT_HT 1U
#define DMA_IT_TC 2U
#define DMA_IT_TE 4U
#define DMA_IT_DME 8U
#define __HAL_DMA_DISABLE_IT(h,f) ((h)->Instance->CR &= ~(f))
#define DMA_FIFOMODE_DISABLE 0U
#define DMA_FIFOMODE_ENABLE 1U
#define DMA_FIFO_THRESHOLD_FULL 3U
#define DMA_MBURST_SINGLE 0U
#define DMA_PBURST_SINGLE 0U
#define DMA_REQUEST_ADC1 1U
#define DMA_REQUEST_DAC1_CH1 2U
#define DMA_PERIPH_TO_MEMORY 0U
#define DMA_MEMORY_TO_PERIPH 1U
#define DISABLE 0U
#define GPIO_PIN_4 (1U << 4)
#define GPIO_PIN_5 (1U << 5)
#define GPIO_MODE_ANALOG 3U
#define GPIO_NOPULL 0U
#define ADC_EXTERNALTRIG_T4_TRGO 9U
#define ADC_EXTERNALTRIGCONVEDGE_RISING 1U
#define ADC_CONVERSIONDATA_DMA_CIRCULAR 3U
#define ADC_CHANNEL_19 19U
#define ADC_CHANNEL_4 4U
#define ADC_REGULAR_RANK_1 1U
#define ADC_SAMPLETIME_16CYCLES_5 16U
#define ADC_SAMPLETIME_2CYCLES_5 2U
#define ADC_SAMPLETIME_8CYCLES_5 8U
#define ADC_SAMPLETIME_32CYCLES_5 32U
#define ADC_SAMPLETIME_64CYCLES_5 64U
#define ADC_SAMPLETIME_387CYCLES_5 387U
#define ADC_SAMPLETIME_810CYCLES_5 810U
#define ADC_SINGLE_ENDED 0U
#define ADC_OFFSET_NONE 0U
#define ADC_CALIB_OFFSET_LINEARITY 1U
#define ADC_FLAG_OVR 1U
#define DAC_CHANNEL_1 0U
#define DAC_IT_DMAUDR1 1U
#define DAC_FLAG_DMAUDR1 1U
#define DAC_CR_EN1 (1U << 1)
#define DAC_CR_TEN1 (1U << 2)
#define DAC_CR_TSEL1 (7U << 3)
#define DAC_CR_DMAEN1 (1U << 9)
#define DAC_SAMPLEANDHOLD_DISABLE 0U
#define DAC_TRIGGER_NONE 0U
#define DAC_TRIGGER_T7_TRGO (DAC_CR_TEN1 | (7U << 3))
#define DAC_OUTPUTBUFFER_ENABLE 0U
#define DAC_CHIPCONNECT_EXTERNAL 0U
#define DAC_TRIMMING_FACTORY 0U
#define DAC_ALIGN_12B_R 0U
#define __DMB() ((void)0)
#define __DSB() ((void)0)
uint32_t __get_PRIMASK(void);
uint32_t HAL_GetTick(void);
void __disable_irq(void);
void __enable_irq(void);
#define __HAL_RCC_GPIOA_CLK_ENABLE() ((void)0)
#define __HAL_RCC_DMA1_CLK_ENABLE() ((void)0)
#define __HAL_RCC_TIM4_CLK_ENABLE() ((void)0)
#define __HAL_RCC_TIM7_CLK_ENABLE() ((void)0)
#define __HAL_RCC_TIM4_CLK_DISABLE() ((void)0)
#define __HAL_RCC_TIM7_CLK_DISABLE() ((void)0)
#define __HAL_RCC_DAC12_CLK_ENABLE() ((void)0)
#define __HAL_TIM_CLEAR_FLAG(h, f) ((void)(h), (void)(f))
#define __HAL_TIM_DISABLE(h) ((h)->Instance->running = 0U)
#define __HAL_LINKDMA(h, field, dma) do { (h)->field = &(dma); (dma).Parent = (h); } while (0)
#define __HAL_DMA_GET_COUNTER(h) ((h)->Instance->remaining)
#define __HAL_DAC_DISABLE_IT(h, f) ((h)->Instance->CR &= ~(f))
/* W1C 在普通宿主内存中显式执行其硬件效果。 */
#define __HAL_DAC_CLEAR_FLAG(h, f) ((h)->Instance->SR &= ~(f))
#define HAL_RCC_GetPCLK1Freq() 100000000U
#define HAL_RCC_GetHCLKFreq() 200000000U
#define HAL_RCCEx_GetPeriphCLKFreq(c) ((void)(c), 64000000U)
#define HAL_ADC_GetState(h) ((h)->State)
#define HAL_ADC_Stop(h) ((h)->Instance->CR &= ~ADC_CR_ADEN, HAL_OK)
#define FAKE_OK1(a) ((void)(a), HAL_OK)
#define FAKE_OK2(a,b) ((void)(a), (void)(b), HAL_OK)
#define HAL_TIM_Base_Init(h) FAKE_OK1(h)
#define HAL_TIMEx_MasterConfigSynchronization(h,c) FAKE_OK2(h,c)
#define HAL_DMA_DeInit(h) FAKE_OK1(h)
#define HAL_DMA_IRQHandler(h) ((void)(h))
#define HAL_DAC_Init(h) FAKE_OK1(h)
#define HAL_DAC_DeInit(h) FAKE_OK1(h)
#define HAL_DAC_Stop(h,c) ((void)(c), (h)->Instance->CR &= ~DAC_CR_EN1, HAL_OK)
HAL_StatusTypeDef HAL_DAC_Stop_DMA(DAC_HandleTypeDef *dac, uint32_t channel);
/* 对齐真实HAL：先清UDR标志，再调用独立欠载回调。 */
#define HAL_DAC_IRQHandler(h) do { if (((h)->Instance->CR & DAC_IT_DMAUDR1) && \
    ((h)->Instance->SR & DAC_FLAG_DMAUDR1)) { \
    __HAL_DAC_CLEAR_FLAG((h),DAC_FLAG_DMAUDR1); \
    (h)->Instance->CR &= ~DAC_CR_DMAEN1; \
    HAL_DAC_DMAUnderrunCallbackCh1(h); } } while (0)
HAL_StatusTypeDef HAL_DAC_ConfigChannel(DAC_HandleTypeDef *dac, DAC_ChannelConfTypeDef *config, uint32_t channel);
HAL_StatusTypeDef HAL_DAC_SetValue(DAC_HandleTypeDef *dac, uint32_t channel, uint32_t alignment, uint32_t value);
HAL_StatusTypeDef HAL_DAC_Start(DAC_HandleTypeDef *dac, uint32_t channel);
#define SCB_CleanInvalidateDCache_by_Addr(p,n) ((void)(p), (void)(n))
#define SCB_InvalidateDCache_by_Addr(p,n) ((void)(p), (void)(n))
#define SCB_CleanDCache_by_Addr(p,n) ((void)(p), (void)(n))
void HAL_RCC_GetClockConfig(RCC_ClkInitTypeDef *clocks, uint32_t *latency);
void HAL_GPIO_Init(GPIO_TypeDef *gpio, const GPIO_InitTypeDef *config);
void HAL_NVIC_DisableIRQ(int irq);
void HAL_NVIC_EnableIRQ(int irq);
void HAL_NVIC_ClearPendingIRQ(int irq);
void HAL_NVIC_SetPriority(int irq, uint32_t priority, uint32_t subpriority);
uint32_t NVIC_GetEnableIRQ(int irq);
HAL_StatusTypeDef HAL_DMA_Init(DMA_HandleTypeDef *dma);
HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef *adc);
HAL_StatusTypeDef HAL_ADC_DeInit(ADC_HandleTypeDef *adc);
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef *adc, ADC_ChannelConfTypeDef *channel);
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *adc, uint32_t calibration, uint32_t mode);
HAL_StatusTypeDef HAL_ADC_Start_DMA(ADC_HandleTypeDef *adc, uint32_t *data, uint32_t count);
HAL_StatusTypeDef HAL_ADC_Stop_DMA(ADC_HandleTypeDef *adc);
HAL_StatusTypeDef HAL_ADCEx_MultiModeConfigChannel(ADC_HandleTypeDef *adc, ADC_MultiModeTypeDef *config);
HAL_StatusTypeDef HAL_ADCEx_MultiModeStart_DMA(ADC_HandleTypeDef *adc, const uint32_t *data, uint32_t count);
HAL_StatusTypeDef HAL_ADCEx_MultiModeStop_DMA(ADC_HandleTypeDef *adc);
HAL_StatusTypeDef HAL_TIM_Base_Start(TIM_HandleTypeDef *timer);
HAL_StatusTypeDef HAL_TIM_Base_Stop(TIM_HandleTypeDef *timer);
HAL_StatusTypeDef HAL_DAC_Start_DMA(DAC_HandleTypeDef *dac, uint32_t channel, uint32_t *data, uint32_t count, uint32_t alignment);
void HAL_DAC_ErrorCallbackCh1(DAC_HandleTypeDef *dac);
void HAL_DAC_DMAUnderrunCallbackCh1(DAC_HandleTypeDef *dac);
#endif
