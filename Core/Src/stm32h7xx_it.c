/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32h7xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32h7xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "hander.h"
#include "oscilloscope_driver.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

void hardfault_debug(uint32_t *raw_sp, uint32_t exc_return);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern DMA2D_HandleTypeDef hdma2d;
extern LTDC_HandleTypeDef hltdc;
extern DMA_HandleTypeDef hdma_sai1_a;
extern DMA_HandleTypeDef hdma_sai1_b;
extern DMA_HandleTypeDef hdma_spdif_rx_dt;
extern SPDIFRX_HandleTypeDef hspdif1;
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim6;
extern UART_HandleTypeDef huart1;
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
   while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
__asm void HardFault_Handler(void)
{
  IMPORT  hardfault_debug
  TST     lr, #4
  ITE     EQ
  MRSEQ   r0, MSP
  MRSNE   r0, PSP
  MOV     r1, lr
  B       hardfault_debug
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */
	
  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */
	extern void lv_tick_inc(uint32_t tick_period);
	lv_tick_inc(1);
  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32H7xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32h7xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles DMA1 stream2 global interrupt.
  */
void DMA1_Stream2_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Stream2_IRQn 0 */

  /* USER CODE END DMA1_Stream2_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_spdif_rx_dt);
  /* USER CODE BEGIN DMA1_Stream2_IRQn 1 */

  /* USER CODE END DMA1_Stream2_IRQn 1 */
}

/**
  * @brief This function handles TIM1 update interrupt.
  */
void TIM1_UP_IRQHandler(void)
{
  /* USER CODE BEGIN TIM1_UP_IRQn 0 */

  /* USER CODE END TIM1_UP_IRQn 0 */
  HAL_TIM_IRQHandler(&htim1);
  /* USER CODE BEGIN TIM1_UP_IRQn 1 */

  /* USER CODE END TIM1_UP_IRQn 1 */
}

/**
  * @brief This function handles TIM1 capture compare interrupt.
  */
void TIM1_CC_IRQHandler(void)
{
  /* USER CODE BEGIN TIM1_CC_IRQn 0 */

  /* USER CODE END TIM1_CC_IRQn 0 */
  HAL_TIM_IRQHandler(&htim1);
  /* USER CODE BEGIN TIM1_CC_IRQn 1 */

  /* USER CODE END TIM1_CC_IRQn 1 */
}

/**
  * @brief This function handles TIM3 global interrupt.
  */
void TIM3_IRQHandler(void)
{
  /* USER CODE BEGIN TIM3_IRQn 0 */

  /* USER CODE END TIM3_IRQn 0 */
  HAL_TIM_IRQHandler(&htim3);
  /* USER CODE BEGIN TIM3_IRQn 1 */

  /* USER CODE END TIM3_IRQn 1 */
}

/**
  * @brief This function handles USART1 global interrupt.
  */
void USART1_IRQHandler(void)
{
  /* USER CODE BEGIN USART1_IRQn 0 */

  /* USER CODE END USART1_IRQn 0 */
  HAL_UART_IRQHandler(&huart1);
  /* USER CODE BEGIN USART1_IRQn 1 */

  /* USER CODE END USART1_IRQn 1 */
}

/**
  * @brief This function handles TIM6 global interrupt, DAC1_CH1 and DAC1_CH2 underrun error interrupts.
  */
void TIM6_DAC_IRQHandler(void)
{
  /* USER CODE BEGIN TIM6_DAC_IRQn 0 */
  scope_driver_dac_irq_handler();
  /* USER CODE END TIM6_DAC_IRQn 0 */
  HAL_TIM_IRQHandler(&htim6);
  /* USER CODE BEGIN TIM6_DAC_IRQn 1 */

  /* USER CODE END TIM6_DAC_IRQn 1 */
}

/**
  * @brief This function handles DMA2 stream3 global interrupt.
  */
void DMA2_Stream3_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Stream3_IRQn 0 */

  /* USER CODE END DMA2_Stream3_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_sai1_a);
  /* USER CODE BEGIN DMA2_Stream3_IRQn 1 */

  /* USER CODE END DMA2_Stream3_IRQn 1 */
}

/**
  * @brief This function handles DMA2 stream5 global interrupt.
  */
void DMA2_Stream5_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Stream5_IRQn 0 */

  /* USER CODE END DMA2_Stream5_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_sai1_b);
  /* USER CODE BEGIN DMA2_Stream5_IRQn 1 */

  /* USER CODE END DMA2_Stream5_IRQn 1 */
}

/**
  * @brief This function handles LTDC global interrupt.
  */
void LTDC_IRQHandler(void)
{
  /* USER CODE BEGIN LTDC_IRQn 0 */

  /* USER CODE END LTDC_IRQn 0 */
  HAL_LTDC_IRQHandler(&hltdc);
  /* USER CODE BEGIN LTDC_IRQn 1 */

  /* USER CODE END LTDC_IRQn 1 */
}

/**
  * @brief This function handles LTDC global error interrupt.
  */
void LTDC_ER_IRQHandler(void)
{
  /* USER CODE BEGIN LTDC_ER_IRQn 0 */

  /* USER CODE END LTDC_ER_IRQn 0 */
  HAL_LTDC_IRQHandler(&hltdc);
  /* USER CODE BEGIN LTDC_ER_IRQn 1 */

  /* USER CODE END LTDC_ER_IRQn 1 */
}

/**
  * @brief This function handles DMA2D global interrupt.
  */
void DMA2D_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2D_IRQn 0 */

  /* USER CODE END DMA2D_IRQn 0 */
  HAL_DMA2D_IRQHandler(&hdma2d);
  /* USER CODE BEGIN DMA2D_IRQn 1 */

  /* USER CODE END DMA2D_IRQn 1 */
}

/**
  * @brief This function handles SPDIFRX1 global interrupt.
  */
void SPDIF_RX_IRQHandler(void)
{
  /* USER CODE BEGIN SPDIF_RX_IRQn 0 */

  /* USER CODE END SPDIF_RX_IRQn 0 */
  HAL_SPDIFRX_IRQHandler(&hspdif1);
  /* USER CODE BEGIN SPDIF_RX_IRQn 1 */

  /* USER CODE END SPDIF_RX_IRQn 1 */
}

/* USER CODE BEGIN 1 */

#include <stdio.h>

#define HARDFAULT_RAW_WORD_COUNT 26U

static volatile uint32_t hardfault_raw_words[HARDFAULT_RAW_WORD_COUNT];

static uint8_t hardfault_frame_readable(const uint32_t *frame, uint32_t words)
{
    uint32_t start = (uint32_t)frame;
    uint32_t bytes = words * sizeof(uint32_t);
    uint32_t end;

    if((frame == NULL) || ((start & 3U) != 0U) || (bytes == 0U) ||
       (start > (0xFFFFFFFFUL - bytes))) return 0U;
    end = start + bytes;
    if((start >= 0x20000000UL) && (end <= 0x20020000UL)) return 1U;
    if((start >= 0x24000000UL) && (end <= 0x24080000UL)) return 1U;
    if((start >= 0x30000000UL) && (end <= 0x30048000UL)) return 1U;
    if((start >= 0x38000000UL) && (end <= 0x38010000UL)) return 1U;
    return 0U;
}

void hardfault_debug(uint32_t *raw_sp, uint32_t exc_return)
{
    volatile uint32_t *raw_words = hardfault_raw_words;
    const volatile uint32_t *core_frame = raw_words;
    uint32_t frame_words = 8U;
    uint32_t snapshot_words = 0U;
    uint32_t i;
    const char *frame_name = "basic";
    uint32_t cfsr = SCB->CFSR;
    uint32_t hfsr = SCB->HFSR;
    uint32_t mmfar = SCB->MMFAR;
    uint32_t bfar = SCB->BFAR;
    uint32_t dfsr = SCB->DFSR;
    uint32_t afsr = SCB->AFSR;
    uint32_t abfsr = SCB->ABFSR;
    uint32_t shcsr = SCB->SHCSR;
    uint32_t msp = __get_MSP();
    uint32_t psp = __get_PSP();
    uint32_t control = __get_CONTROL();
    uint32_t fpccr = FPU->FPCCR;

    if((exc_return & (1UL << 4)) == 0U) {
        frame_name = "extended-fpu";
        frame_words = HARDFAULT_RAW_WORD_COUNT;
    }

    for(i = 0U; i < HARDFAULT_RAW_WORD_COUNT; i++) {
        raw_words[i] = 0U;
    }
    if(hardfault_frame_readable(raw_sp, frame_words)) {
        snapshot_words = frame_words;
        for(i = 0U; i < snapshot_words; i++) {
            raw_words[i] = raw_sp[i];
        }
    }

    printf("\r\n======== HARDFAULT ========\r\n");
    printf("EXC_RETURN=0x%08lX RAW_SP=0x%08lX FRAME=%s\r\n",
           exc_return, (uint32_t)raw_sp, frame_name);
    printf("MSP=0x%08lX PSP=0x%08lX CONTROL=0x%08lX FPCCR=0x%08lX\r\n",
           msp, psp, control, fpccr);
    if(snapshot_words >= 8U) {
        printf("PC  = 0x%08lX\r\n", core_frame[6]);
        printf("LR  = 0x%08lX\r\n", core_frame[5]);
        printf("R0  = 0x%08lX\r\n", core_frame[0]);
        printf("R1  = 0x%08lX\r\n", core_frame[1]);
        printf("R2  = 0x%08lX\r\n", core_frame[2]);
        printf("R3  = 0x%08lX\r\n", core_frame[3]);
        printf("R12 = 0x%08lX\r\n", core_frame[4]);
        printf("xPSR= 0x%08lX\r\n", core_frame[7]);
        for(i = 0U; i < snapshot_words; i += 2U) {
            printf("STACK[%02lu]=0x%08lX STACK[%02lu]=0x%08lX\r\n",
                   i, raw_words[i], i + 1U, raw_words[i + 1U]);
        }
    }
    else {
        printf("frame valid=0\r\n");
    }
    printf("CFSR= 0x%08lX\r\n", cfsr);
    printf("HFSR= 0x%08lX\r\n", hfsr);
    printf("MMFAR=0x%08lX\r\n", mmfar);
    printf("BFAR = 0x%08lX\r\n", bfar);
    printf("DFSR=0x%08lX AFSR=0x%08lX ABFSR=0x%08lX SHCSR=0x%08lX\r\n",
           dfsr, afsr, abfsr, shcsr);
    printf("============================\r\n");

    /* LED0+LED1 alternating blink for fault indication */
    while (1) {
      led_poc(0, 1);  led_poc(1, 0);  for (volatile uint32_t d=0; d<24000000; d++) { __NOP(); }
      led_poc(0, 0);  led_poc(1, 1);  for (volatile uint32_t d=0; d<24000000; d++) { __NOP(); }
    }
}

/* USER CODE END 1 */
