/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef _CLOCK_H_
#define _CLOCK_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>

#include "hal_defs.h"


uint8_t  clock_get_year();
uint8_t  clock_get_month();
uint8_t  clock_get_date();
uint8_t  clock_get_hour();
uint8_t  clock_get_minute();
uint8_t  clock_get_second();
void     clock_save_time(RTC_TimeTypeDef* time);
void     clock_save_date(RTC_DateTypeDef* date);
bool     clock_get_rtc_time(RTC_TimeTypeDef* time);
bool     clock_get_rtc_date(RTC_DateTypeDef* date);
uint32_t clock_datetime_to_seconds(RTC_DateTypeDef* date, RTC_TimeTypeDef* time);
uint32_t clock_get_timestamp();


#ifdef __cplusplus
}
#endif


#endif
