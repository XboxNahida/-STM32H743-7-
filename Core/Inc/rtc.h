/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    rtc.h
  * @brief   This file contains all the function prototypes for
  *          the rtc.c file
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
#ifndef __RTC_H__
#define __RTC_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern RTC_HandleTypeDef hrtc;

/* USER CODE BEGIN Private defines */

/* 全局RTC时间变量，由rtc_update()刷新。 */
extern volatile uint8_t g_rtc_hour, g_rtc_min, g_rtc_sec;
extern volatile uint8_t g_rtc_year, g_rtc_month, g_rtc_day, g_rtc_week;
/* 多闹钟支持（最多8个） */
#define MAX_ALARMS 8
typedef struct {
    uint8_t hour;    /* 0-23, 255=empty */
    uint8_t min;     /* 0-59 */
} alarm_entry_t;

extern alarm_entry_t g_alarms[MAX_ALARMS];
extern volatile uint8_t g_alarm_hour, g_alarm_min;  /* 最近闹钟, 255=无闹钟 */
extern volatile uint8_t g_alarm_triggered;


/* USER CODE END Private defines */

void MX_RTC_Init(void);

/* USER CODE BEGIN Prototypes */
typedef void (*rtc_change_callback_t)(void);

uint8_t rtc_set_time(uint8_t hour, uint8_t min, uint8_t sec);
void    rtc_get_time(uint8_t *hour, uint8_t *min, uint8_t *sec);
uint8_t rtc_set_date(uint8_t year, uint8_t month, uint8_t day);
void    rtc_get_date(uint8_t *year, uint8_t *month, uint8_t *day, uint8_t *week);
uint8_t rtc_get_week(uint16_t year, uint8_t month, uint8_t day);
void    rtc_update(void);
void    rtc_tick(void);
uint8_t alarm_add(uint8_t hour, uint8_t min);    /* returns 0=ok, 1=full */
void    alarm_remove_nearest(void);
void    rtc_set_alarma(uint8_t week, uint8_t hour, uint8_t min, uint8_t sec);
void    rtc_set_wakeup(uint8_t wksel, uint16_t cnt);
uint8_t rtc_backup_was_valid(void);
void    rtc_set_change_callback(rtc_change_callback_t callback);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __RTC_H__ */

