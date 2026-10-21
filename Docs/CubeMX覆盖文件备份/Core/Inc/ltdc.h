/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    ltdc.h
  * @brief   This file contains all the function prototypes for
  *          the ltdc.c file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LTDC_H__
#define __LTDC_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern LTDC_HandleTypeDef hltdc;

/* USER CODE BEGIN Private defines */
extern uint32_t *g_ltdc_framebuf[2];

/* USER CODE END Private defines */

void MX_LTDC_Init(void);

/* USER CODE BEGIN Prototypes */
void ltdc_switch_to_video_fb(void);
uint32_t ltdc_get_back_video_fb(void);
void ltdc_video_fb_swap(void);
void ltdc_restore_lvgl_fb(void);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __LTDC_H__ */

