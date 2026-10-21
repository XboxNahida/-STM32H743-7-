/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    spdifrx.c
  * @brief   This file provides code for the configuration
  *          of the SPDIFRX instances.
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
#include "spdifrx.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

SPDIFRX_HandleTypeDef hspdif1;
DMA_HandleTypeDef hdma_spdif_rx_dt;

/* SPDIFRX1 init function */
void MX_SPDIFRX1_Init(void)
{

  /* USER CODE BEGIN SPDIFRX1_Init 0 */

  /* USER CODE END SPDIFRX1_Init 0 */

  /* USER CODE BEGIN SPDIFRX1_Init 1 */

  /* USER CODE END SPDIFRX1_Init 1 */
  hspdif1.Instance = SPDIFRX;
  hspdif1.Init.InputSelection = SPDIFRX_INPUT_IN1;
  hspdif1.Init.Retries = SPDIFRX_MAXRETRIES_15;
  hspdif1.Init.WaitForActivity = SPDIFRX_WAITFORACTIVITY_ON;
  hspdif1.Init.ChannelSelection = SPDIFRX_CHANNEL_A;
  hspdif1.Init.DataFormat = SPDIFRX_DATAFORMAT_LSB;
  hspdif1.Init.StereoMode = SPDIFRX_STEREOMODE_DISABLE;
  hspdif1.Init.PreambleTypeMask = SPDIFRX_PREAMBLETYPEMASK_OFF;
  hspdif1.Init.ChannelStatusMask = SPDIFRX_CHANNELSTATUS_OFF;
  hspdif1.Init.ValidityBitMask = SPDIFRX_VALIDITYMASK_OFF;
  hspdif1.Init.ParityErrorMask = SPDIFRX_PARITYERRORMASK_OFF;
  hspdif1.Init.SymbolClockGen = DISABLE;
  hspdif1.Init.BackupSymbolClockGen = DISABLE;
  if (HAL_SPDIFRX_Init(&hspdif1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPDIFRX1_Init 2 */

  /* CubeMX 6.17 does not expose this official overflow-alignment option. */
  hspdif1.Init.StereoMode = SPDIFRX_STEREOMODE_ENABLE;
  SET_BIT(hspdif1.Instance->CR, SPDIFRX_CR_RXSTEO);

  /* USER CODE END SPDIFRX1_Init 2 */

}

void HAL_SPDIFRX_MspInit(SPDIFRX_HandleTypeDef* spdifrxHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if(spdifrxHandle->Instance==SPDIFRX)
  {
  /* USER CODE BEGIN SPDIFRX_MspInit 0 */

  /* USER CODE END SPDIFRX_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPDIFRX;
    PeriphClkInitStruct.SpdifrxClockSelection = RCC_SPDIFRXCLKSOURCE_PLL;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    /* SPDIFRX clock enable */
    __HAL_RCC_SPDIFRX_CLK_ENABLE();

    __HAL_RCC_GPIOG_CLK_ENABLE();
    /**SPDIFRX1 GPIO Configuration
    PG12     ------> SPDIFRX1_IN1
    */
    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_SPDIF;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    /* SPDIFRX DMA Init */
    /* SPDIF_RX_DT Init */
    hdma_spdif_rx_dt.Instance = DMA1_Stream2;
    hdma_spdif_rx_dt.Init.Request = DMA_REQUEST_SPDIF_RX_DT;
    hdma_spdif_rx_dt.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_spdif_rx_dt.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spdif_rx_dt.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spdif_rx_dt.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_spdif_rx_dt.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_spdif_rx_dt.Init.Mode = DMA_CIRCULAR;
    hdma_spdif_rx_dt.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_spdif_rx_dt.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_spdif_rx_dt) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(spdifrxHandle,hdmaDrRx,hdma_spdif_rx_dt);

    /* SPDIFRX interrupt Init */
    HAL_NVIC_SetPriority(SPDIF_RX_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(SPDIF_RX_IRQn);
  /* USER CODE BEGIN SPDIFRX_MspInit 1 */

  /* USER CODE END SPDIFRX_MspInit 1 */
  }
}

void HAL_SPDIFRX_MspDeInit(SPDIFRX_HandleTypeDef* spdifrxHandle)
{

  if(spdifrxHandle->Instance==SPDIFRX)
  {
  /* USER CODE BEGIN SPDIFRX_MspDeInit 0 */

  /* USER CODE END SPDIFRX_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SPDIFRX_CLK_DISABLE();

    /**SPDIFRX1 GPIO Configuration
    PG12     ------> SPDIFRX1_IN1
    */
    HAL_GPIO_DeInit(GPIOG, GPIO_PIN_12);

    /* SPDIFRX DMA DeInit */
    HAL_DMA_DeInit(spdifrxHandle->hdmaDrRx);

    /* SPDIFRX interrupt Deinit */
    HAL_NVIC_DisableIRQ(SPDIF_RX_IRQn);
  /* USER CODE BEGIN SPDIFRX_MspDeInit 1 */

  /* USER CODE END SPDIFRX_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

