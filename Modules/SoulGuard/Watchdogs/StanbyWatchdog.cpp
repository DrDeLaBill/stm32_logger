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
	printTagLog(StandbyWatchdog::TAG, "The clock alarm has been detected. Measure has requested.");
}
#endif


void StandbyWatchdog::check()
{
	utl::CodeStopwatch stopwatch(TAG, GENERAL_TIMEOUT_MS);

	if (!deviceLoaded) {
		init();
	}

#if USE_WKUP_RTC_ALARM
	if (checkAlarm()) {
		enableRTCAlarm();
	}
#endif

	if (needStandby()) {
		enterStandby();
	}
}

void StandbyWatchdog::enableRTCAlarm(uint32_t seconds)
{
	alarmEnabled = true;

#if USE_WKUP_RTC_ALARM

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
	sAlarm.AlarmDateWeekDay         = dumpDate.Date;
	sAlarm.Alarm                    = RTC_ALARM_A;

	if (HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN) != HAL_OK) {
		Error_Handler();
	}

	if (!clock_get_rtc_time(&dumpTime)) {
		printTagLog(TAG, "Unable to get current time");
	}
	if (!clock_get_rtc_date(&dumpDate)) {
		printTagLog(TAG, "Unable to get current date");
	}

	printTagLog(
		TAG,
		"The alarm clock has set for %02u day %02u:%02u:%02u (current %02u time %02u:%02u:%02u)",
		sAlarm.AlarmDateWeekDay,
		sAlarm.AlarmTime.Hours,
		sAlarm.AlarmTime.Minutes,
		sAlarm.AlarmTime.Seconds,
		dumpDate.Date,
		dumpTime.Hours,
		dumpTime.Minutes,
		dumpTime.Seconds
	);
#endif
}

void StandbyWatchdog::init()
{
	deviceLoaded = true;

	timer.start();

#if USE_WKUP_RTC_ALARM
	reloadAlarm();
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

void StandbyWatchdog::clearPWRFlags()
{
	PWR->CSR;
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
}

bool StandbyWatchdog::hasWokenUp()
{
	return __HAL_PWR_GET_FLAG(PWR_FLAG_SB) && __HAL_PWR_GET_FLAG(PWR_FLAG_WU);
}

bool StandbyWatchdog::needStandby()
{
	if (!USBController::connected() && !timer.wait() && has_errors()) {
		return true;
	}

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

bool StandbyWatchdog::checkAlarm()
{
#if !USE_WKUP_RTC_ALARM
	return false;
#endif

#if USE_WKUP_PA0
	if (USBController::connected() && hasWokenUp()) {
		clearPWRFlags();
		printTagLog(TAG, "The device has exited the standby mode by USB connection.");
	}
	if (hasWokenUp()) {
		clearPWRFlags();
		Measure::setNeeded();
		printTagLog(TAG, "The device has woken up. Measure has requested.");
		return true;
	}
#endif


#if USE_WKUP_RTC_ALARM
	RTC_AlarmTypeDef sAlarm = {};
	RTC_TimeTypeDef  time   = {};
	RTC_DateTypeDef  date   = {};
	if (HAL_RTC_GetAlarm(&hrtc, &sAlarm, RTC_ALARM_A, RTC_FORMAT_BIN) != HAL_OK) {
		printTagLog(TAG, "Unable to get clock alarm");
		return false;
	}
	if (!clock_get_rtc_time(&time)) {
		printTagLog(TAG, "Unable to get current time");
		return false;
	}
	if (!clock_get_rtc_date(&date)) {
		printTagLog(TAG, "Unable to get current date");
		return false;
	}

	if (sAlarm.AlarmDateWeekDaySel != RTC_ALARMDATEWEEKDAYSEL_DATE) {
		return true;
	}
	RTC_TimeTypeDef* atarmTime = &(sAlarm.AlarmTime);
	uint32_t currSeconds       = clock_datetime_to_seconds(&date, &time);
	date.Date = sAlarm.AlarmDateWeekDay;
	if (date.Date > sAlarm.AlarmDateWeekDay) {
		date.Month++;
	}
	if (date.Month > MONTHS_PER_YEAR) {
		date.Year++;
		date.Month = 1;
	}
	uint32_t alarmSeconds = clock_datetime_to_seconds(&date, atarmTime);
	uint32_t needSecods   = settings.record_period / SECOND_MS;
	if (currSeconds < alarmSeconds && (alarmSeconds - currSeconds) > needSecods) {
		return true;
	}
	if (alarmSeconds < currSeconds) {
		return true;
	}
#endif

	return false;
}

void StandbyWatchdog::reloadAlarm()
{
#if USE_WKUP_RTC_ALARM
	if (!(RTC->TAFCR & RTC_TAFCR_TAMPIE)) {
		printTagLog(TAG, "The device has loaded. Try to init clock alarm at the first time.");
		alarmFirstInit();
		return;
	}
	printTagLog(TAG, "The device has reloaded. Try to reinit clock alarm.");
	RTC_AlarmTypeDef sAlarm  = {};
	HAL_StatusTypeDef status = HAL_RTC_GetAlarm(&hrtc, &sAlarm, RTC_ALARM_A, RTC_FORMAT_BIN);
	if (status != HAL_OK) {
		printTagLog(TAG, "Unable to get clock alarm");
		clearPWRFlags();
		enableRTCAlarm();
		return;
	}
	status = HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN);
	if (status != HAL_OK) {
		printTagLog(TAG, "Unable to restart clock alarm");
		clearPWRFlags();
		enableRTCAlarm();
		return;
	}
#endif
}

void StandbyWatchdog::enterStandby()
{
	printTagLog(TAG, "Initalizing the standby mode. The device turns off.");

#if USE_WKUP_RTC_ALARM
	if (!alarmEnabled) {
		enableRTCAlarm();
	}
#endif

#if USE_WKUP_PA0
	HAL_GPIO_DeInit(WKUP_GPIO_Port, WKUP_Pin);

	RTC->TAFCR &= ~(RTC_TAFCR_TAMPIE);
	RTC->ISR   &= ~(RTC_ISR_TAMP1F);
	RTC->ISR   &= ~(RTC_ISR_TSF);
	PWR->CR    &= ~(PWR_CR_CWUF); asm("nop"); asm("nop");
	RTC->TAFCR |= RTC_TAFCR_TAMPIE;

	HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);
#endif

	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

	HAL_PWR_EnterSTANDBYMode();
}

void StandbyWatchdog::alarmFirstInit()
{
	Measure::setNeeded();
	clearPWRFlags();

	uint32_t needSeconds = sleepTimeSec();
	uint32_t lastSeconds = 0;
	if (Record::getLastTime(&lastSeconds) != RECORD_OK) {
		enableRTCAlarm();
		return;
	}

	uint32_t currSeconds = clock_get_timestamp();
	if (lastSeconds < currSeconds && lastSeconds + needSeconds > currSeconds) {
		needSeconds = currSeconds - lastSeconds;
	}

	enableRTCAlarm(needSeconds);
}

uint32_t StandbyWatchdog::sleepTimeSec()
{
	uint32_t seconds = (clock_get_timestamp() + settings.record_period / SECOND_MS);
	return seconds;
}
