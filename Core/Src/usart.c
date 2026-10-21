/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
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
#include "usart.h"

/* USER CODE BEGIN 0 */
// ====== 外部引用显示缓冲区 ======
#include <string.h>
#include <stdio.h>
/* UART_DISPLAY_BUF_SIZE defined in main.h */

extern char uart_display_buf[];
extern uint16_t uart_display_len;

///重定向c库函数printf到串口USARTx，同时显示到屏幕上
int fputc(int ch, FILE *f)
{
    // 1. 发送到串口（主机端能看到）
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);
    
    // 2. 添加到屏幕显示缓冲区
    if(uart_display_len >= UART_DISPLAY_BUF_SIZE - 20) {
        // 缓冲区满，清空
        memset(uart_display_buf, 0, UART_DISPLAY_BUF_SIZE);
        uart_display_len = 0;
    }
    
    // 过滤回车符
    if(ch == '\r') {
        return ch;
    }
    
    // 如果是换行符，直接添加
    if(ch == '\n') {
        // 行首添加 [TX] 标记
        if(uart_display_len == 0 || 
           uart_display_buf[uart_display_len - 1] == '\n') {
            uart_display_len += snprintf(uart_display_buf + uart_display_len, 
                                        UART_DISPLAY_BUF_SIZE - uart_display_len, 
                                        "[TX] ");
        }
        uart_display_buf[uart_display_len++] = '\n';
    } else {
        // 普通字符：如果是新行开头，添加 [TX] 标记
        if(uart_display_len == 0 || 
           uart_display_buf[uart_display_len - 1] == '\n') {
            uart_display_len += snprintf(uart_display_buf + uart_display_len, 
                                        UART_DISPLAY_BUF_SIZE - uart_display_len, 
                                        "[TX] ");
        }
        uart_display_buf[uart_display_len++] = ch;
    }
    
    uart_display_buf[uart_display_len] = '\0';
    
    return ch;
}
 
///重定向c库函数scanf到串口USARTx，重写向后可使用scanf、getchar等函数
int fgetc(FILE *f)
{	
	int ch;
	/* 等待串口输入数据 */
	while(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE) == RESET);
	__HAL_UART_CLEAR_OREFLAG(&huart1);
	HAL_UART_Receive(&huart1, (uint8_t *)&ch, 1, 0xFFFF);
	return (ch);
}
/* USER CODE END 0 */

UART_HandleTypeDef huart1;

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART1;
    PeriphClkInitStruct.Usart16ClockSelection = RCC_USART16CLKSOURCE_D2PCLK2;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, 10, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */

  /* USER CODE END USART1_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

