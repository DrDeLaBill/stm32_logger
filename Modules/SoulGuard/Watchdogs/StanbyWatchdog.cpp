/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Watchdogs.h"

#include "log.h"
#include "main.h"
#include "soul.h"
#include "clock.h"
#include "settings.h"
#include "hal_defs.h"

#include "Record.h"
#include "Measure.h"
#include "CodeStopwatch.h"


#define USE_WKUP_PA0       (true)
#define USE_WKUP_RTC_ALARM (true)


#if USE_WKUP_PA0
#	include "USBController.h"
#endif


#if USE_WKUP_RTC_ALARM
extern RTC_HandleTypeDef hrtc;
#endif


utl::Timer StandbyWatchdog::timer(MINUTE_MS);
bool StandbyWatchdog::alarmEnabled = false;
bool StandbyWatchdog::deviceLoaded = false;


#if USE_WKUP_RTC_ALARM
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef*)
{
	Measure::setNeeded();
	printTagLog(StandbyWatchdog::TAG, "The device has exited the standby mode by clock alarm");
}
#endif


void StandbyWatchdog::check()
{
	utl::CodeStopwatch stopwatch(TAG, GENERAL_TIMEOUT_MS);
	// TODO: load max record and compare time with current

#if USE_WKUP_RTC_ALARM
	if (needStandby()) {
		if (!alarmEnabled) {
			enableRTCAlarm();
		}
	}
#endif

#if USE_WKUP_PA0
	if (needStandby()) {
		printTagLog(TAG, "Initalizing the standby mode. The device turns off.");

		HAL_GPIO_DeInit(WKUP_GPIO_Port, WKUP_Pin);
		HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);

		HAL_PWR_EnterSTANDBYMode();
	}
//	if (has_errors()) { // TODO: timer
//		// TODO: rtc alarm start (60 minutes)
//		HAL_PWR_EnterSTANDBYMode();
//	}
#endif

	if (deviceLoaded) {
		return;
	}

	deviceLoaded = true;

	bool needAlarm = true;

#if USE_WKUP_PA0
	if (__HAL_PWR_GET_FLAG(PWR_FLAG_SB)) {
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
		needAlarm = false;
	}
	if (__HAL_PWR_GET_FLAG(PWR_FLAG_WU)) {
		printTagLog(TAG, "The device has exited the standby mode by WKUP pin");
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
		needAlarm = false;
	}
#endif


#if USE_WKUP_RTC_ALARM
	// TODO: if alarm has already started do not enableRtcAlarm() and do alarmEnabled = true
//	RTC_AlarmTypeDef sAlarm = {};
//	RTC_TimeTypeDef time = {};
//	if (HAL_RTC_GetAlarm(&hrtc, &sAlarm, RTC_ALARM_A, RTC_FORMAT_BIN) != HAL_OK) {
//		printTagLog(TAG, "Unable to get clock alarm");
//	}
//	if (!clock_get_rtc_time(&time)) {
//		printTagLog(TAG, "Unable to get current time");
//	}
//	sAlarm.Alarm
//	if (time.Hours > 0)


	if (needAlarm) {
		alarmEnabled = true;
		enableRTCAlarm();
	}
#endif

#if USE_WKUP_PA0
	HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN1);

	GPIO_InitTypeDef GPIO_InitStruct = {};
	GPIO_InitStruct.Pin  = WKUP_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(WKUP_GPIO_Port, &GPIO_InitStruct);
#endif
}

void StandbyWatchdog::enableRTCAlarm()
{
#if USE_WKUP_RTC_ALARM
	uint32_t seconds = (clock_get_timestamp() + settings.record_period / SECOND_MS);
	seconds = (clock_get_timestamp() + 30); // TODO: remove

	RTC_TimeTypeDef dumpTime = {};
	RTC_DateTypeDef dumpDate = {};
	clock_seconds_to_datetime(seconds, &dumpDate, &dumpTime);

	RTC_AlarmTypeDef sAlarm = {};

	sAlarm.AlarmTime.Hours          = dumpTime.Hours;
	sAlarm.AlarmTime.Minutes        = dumpTime.Minutes;
	sAlarm.AlarmTime.Seconds        = dumpTime.Seconds;
	sAlarm.AlarmTime.SubSeconds     = 0;
	sAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	sAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
	sAlarm.AlarmMask                = RTC_ALARMMASK_NONE;
	sAlarm.AlarmSubSecondMask       = RTC_ALARMSUBSECONDMASK_ALL;
	sAlarm.AlarmDateWeekDaySel      = RTC_ALARMDATEWEEKDAYSEL_DATE;
	sAlarm.AlarmDateWeekDay         = 1;
	sAlarm.Alarm                    = RTC_ALARM_A;

	if (HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN) != HAL_OK) {
		Error_Handler();
	}

	if (!clock_get_rtc_time(&dumpTime)) {
		printTagLog(TAG, "Unable to get current time");
	}

	printTagLog(
		TAG,
		"The alarm clock is set for %02u:%02u:%02u (current time %02u:%02u:%02u)",
		sAlarm.AlarmTime.Hours,
		sAlarm.AlarmTime.Minutes,
		sAlarm.AlarmTime.Seconds,
		dumpTime.Hours,
		dumpTime.Minutes,
		dumpTime.Seconds
	);
#endif
}

bool StandbyWatchdog::needStandby()
{
	if (is_status(WAIT_LOAD)) {
		return false;
	}

#if USE_WKUP_PA0
	if (USBController::connected()) {
		return false;
	}
#endif

	if (Measure::needed()) {
		return false;
	}

	return true;
}
