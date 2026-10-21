/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    rtc.c
  * @brief   This file provides code for the configuration
  *          of the RTC instances.
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
#include "rtc.h"

/* USER CODE BEGIN 0 */

/* ȫ��RTCʱ����� */
volatile uint8_t g_rtc_hour  = 12;
volatile uint8_t g_rtc_min   = 0;
volatile uint8_t g_rtc_sec   = 0;
volatile uint8_t g_rtc_year  = 26;
volatile uint8_t g_rtc_month = 6;
volatile uint8_t g_rtc_day   = 10;
volatile uint8_t g_rtc_week  = 3;
volatile uint8_t g_alarm_hour = 255;
volatile uint8_t g_alarm_min  = 0;
alarm_entry_t g_alarms[MAX_ALARMS];


/* USER CODE END 0 */

RTC_HandleTypeDef hrtc;

/* RTC init function */
void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* force write prescalers (HAL skips when INITS=1) */
  if ((RTC->PRER & 0x7FFF) != ((127U << 16) | 255U)) {
    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;
    SET_BIT(RTC->ISR, RTC_ISR_INIT);
    while (!(RTC->ISR & RTC_ISR_INITF));
    RTC->PRER = (127U << 16) | 255U;
    CLEAR_BIT(RTC->ISR, RTC_ISR_INIT);
    RTC->WPR = 0xFF;
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) != 0x5050)
  {
    sTime.Hours   = 12;
    sTime.Minutes = 0;
    sTime.Seconds = 0;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_SET;
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
    {
      Error_Handler();
    }
    sDate.WeekDay = RTC_WEEKDAY_WEDNESDAY;
    sDate.Month   = RTC_MONTH_JUNE;
    sDate.Date    = 10;
    sDate.Year    = 26;
    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
    {
      Error_Handler();
    }
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, 0x5050);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, 0xFFFF);
    g_rtc_hour  = 12;
    g_rtc_min   = 0;
    g_rtc_sec   = 0;
    g_rtc_year  = 26;
    g_rtc_month = 6;
    g_rtc_day   = 10;
    g_rtc_week  = 3;
    g_alarm_hour = 255;
    for (int i = 0; i < MAX_ALARMS; i++) { g_alarms[i].hour = 255; g_alarms[i].min = 0; }
  } else {
    /* Restore alarm from backup registers */
    uint32_t val = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR2);
    if (val == 0xFFFF) {
      g_alarm_hour = 255;
    } else {
      g_alarm_hour = (uint8_t)(val & 0xFF);
      g_alarm_min  = (uint8_t)((val >> 8) & 0xFF);
    }
    rtc_update();
    for (int i = 0; i < MAX_ALARMS; i++) g_alarms[i].hour = 255;
    if (g_alarm_hour != 255) { g_alarms[0].hour = g_alarm_hour; g_alarms[0].min = g_alarm_min; }
  }
  /* USER CODE END Check_RTC_BKUP */

  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{

  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspInit 0 */

  /* USER CODE END RTC_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    /* RTC clock enable */
    __HAL_RCC_RTC_ENABLE();
  /* USER CODE BEGIN RTC_MspInit 1 */

  /* USER CODE END RTC_MspInit 1 */
  }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspDeInit 0 */

  /* USER CODE END RTC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_RTC_DISABLE();
  /* USER CODE BEGIN RTC_MspDeInit 1 */

  /* USER CODE END RTC_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

static const uint8_t table_week[12] = {0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5};

uint8_t rtc_set_time(uint8_t hour, uint8_t min, uint8_t sec)
{
    RTC_TimeTypeDef sTime = {0};
    sTime.Hours   = hour;
    sTime.Minutes = min;
    sTime.Seconds = sec;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_SET;
    uint8_t ret = (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) == HAL_OK) ? 0 : 1;
    if (ret == 0) {
        g_rtc_hour = hour;
        g_rtc_min  = min;
        g_rtc_sec  = sec;
    }
    return ret;
}

void rtc_get_time(uint8_t *hour, uint8_t *min, uint8_t *sec)
{
    RTC_TimeTypeDef sTime = {0};
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    *hour = sTime.Hours;
    *min  = sTime.Minutes;
    *sec  = sTime.Seconds;
}

uint8_t rtc_set_date(uint8_t year, uint8_t month, uint8_t day)
{
    RTC_DateTypeDef sDate = {0};
    uint8_t wk = rtc_get_week(2000 + year, month, day);
    sDate.WeekDay = wk;
    sDate.Month   = month;
    sDate.Date    = day;
    sDate.Year    = year;
    uint8_t ret = (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) == HAL_OK) ? 0 : 1;
    if (ret == 0) {
        g_rtc_year  = year;
        g_rtc_month = month;
        g_rtc_day   = day;
        g_rtc_week  = wk;
    }
    return ret;
}

void rtc_get_date(uint8_t *year, uint8_t *month, uint8_t *day, uint8_t *week)
{
    RTC_DateTypeDef sDate = {0};
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    *year  = sDate.Year;
    *month = sDate.Month;
    *day   = sDate.Date;
    *week  = sDate.WeekDay;
}

uint8_t rtc_get_week(uint16_t year, uint8_t month, uint8_t day)
{
    uint16_t temp2;
    uint8_t yearH, yearL;
    yearH = year / 100;
    yearL = year % 100;
    if (yearH > 19) yearL += 100;
    temp2 = yearL + yearL / 4;
    temp2 = temp2 % 7;
    temp2 = temp2 + day + table_week[month - 1];
    if (yearL % 4 == 0 && month < 3) temp2--;
    if (temp2 % 7 == 0) return 7;
    return (uint8_t)(temp2 % 7);
}

void rtc_set_alarma(uint8_t week, uint8_t hour, uint8_t min, uint8_t sec)
{
    RTC_AlarmTypeDef sAlarm = {0};
    sAlarm.AlarmTime.Hours          = hour;
    sAlarm.AlarmTime.Minutes        = min;
    sAlarm.AlarmTime.Seconds        = sec;
    sAlarm.AlarmTime.SubSeconds     = 0;
    sAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_SET;
    sAlarm.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY;
    sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
    sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_WEEKDAY;
    sAlarm.AlarmDateWeekDay = week;
    sAlarm.Alarm = RTC_ALARM_A;
    HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN);
}

void rtc_set_wakeup(uint8_t wksel, uint16_t cnt)
{
    HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, cnt, wksel);
}

void rtc_update(void)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    g_rtc_hour  = sTime.Hours;
    g_rtc_min   = sTime.Minutes;
    g_rtc_sec   = sTime.Seconds;
    g_rtc_year  = sDate.Year;
    g_rtc_month = sDate.Month;
    g_rtc_day   = sDate.Date;
    g_rtc_week  = sDate.WeekDay;
}
/* 多闹钟实现 */
static void alarm_sort(void)
{
    /* 按时间排序, 空槽(255)排到末尾 */
    for (int i = 0; i < MAX_ALARMS - 1; i++) {
        for (int j = i + 1; j < MAX_ALARMS; j++) {
            if (g_alarms[j].hour == 255) continue;
            if (g_alarms[i].hour == 255 ||
                g_alarms[j].hour * 60 + g_alarms[j].min <
                g_alarms[i].hour * 60 + g_alarms[i].min) {
                alarm_entry_t tmp = g_alarms[i];
                g_alarms[i] = g_alarms[j];
                g_alarms[j] = tmp;
            }
        }
    }
}

static void alarm_update_nearest(void)
{
    alarm_sort();
    if (g_alarms[0].hour != 255) {
        g_alarm_hour = g_alarms[0].hour;
        g_alarm_min  = g_alarms[0].min;
    } else {
        g_alarm_hour = 255;
        g_alarm_min  = 0;
    }
    /* backup: save nearest to RTC_BKP_DR2 */
    uint32_t val = (g_alarm_hour == 255) ? 0xFFFF :
                   ((uint32_t)g_alarm_min << 8) | g_alarm_hour;
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, val);
}

uint8_t alarm_add(uint8_t hour, uint8_t min)
{
    /* check for duplicate */
    for (int i = 0; i < MAX_ALARMS; i++) {
        if (g_alarms[i].hour == hour && g_alarms[i].min == min)
            return 0;  /* already exists, ok */
    }
    /* find empty slot */
    for (int i = 0; i < MAX_ALARMS; i++) {
        if (g_alarms[i].hour == 255) {
            g_alarms[i].hour = hour;
            g_alarms[i].min  = min;
            alarm_update_nearest();
            return 0;
        }
    }
    return 1;  /* full */
}

void alarm_remove_nearest(void)
{
    alarm_sort();
    if (g_alarms[0].hour != 255) {
        g_alarms[0].hour = 255;
        g_alarms[0].min  = 0;
        alarm_update_nearest();
    }
}

void rtc_tick(void)
{
    g_rtc_sec++;
    if (g_rtc_sec >= 60) {
        g_rtc_sec = 0;
        g_rtc_min++;
        if (g_rtc_min >= 60) {
            g_rtc_min = 0;
            g_rtc_hour++;
            if (g_rtc_hour >= 24) {
                g_rtc_hour = 0;
                /* day rollover */
                static const uint8_t days_in_month[] = {31,28,31,30,31,30,31,31,30,31,30,31};
                uint8_t dim = days_in_month[g_rtc_month - 1];
                if (g_rtc_month == 2) {
                    uint16_t yr = 2000 + g_rtc_year;
                    if ((yr % 4 == 0 && yr % 100 != 0) || (yr % 400 == 0)) dim = 29;
                }
                g_rtc_day++;
                if (g_rtc_day > dim) {
                    g_rtc_day = 1;
                    g_rtc_month++;
                    if (g_rtc_month > 12) {
                        g_rtc_month = 1;
                        g_rtc_year++;
                    }
                }
                g_rtc_week = rtc_get_week(2000 + g_rtc_year, g_rtc_month, g_rtc_day);
            }
        }
    }
}

/* USER CODE END 1 */

