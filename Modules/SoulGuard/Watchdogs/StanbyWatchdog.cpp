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


#define STANDBY_W_BEDUG    (true)

#define USE_WKUP_PA0       (true)
#define USE_WKUP_RTC_ALARM (true)


#if USE_WKUP_PA0
#	include "USBController.h"
#endif


#if USE_WKUP_RTC_ALARM
extern RTC_HandleTypeDef hrtc;
#endif


fsm::FiniteStateMachine<StandbyWatchdog::fsm_table> StandbyWatchdog::fsm;
utl::Timer StandbyWatchdog::timer(MINUTE_MS);


void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef*)
{
	set_status(NEED_MEASURE);
#if USE_WKUP_RTC_ALARM
	StandbyWatchdog::alarm();
#	if STANDBY_W_BEDUG
	printTagLog(StandbyWatchdog::TAG, "The clock alarm has been detected. Measure has requested.");
#	endif
#endif
}

void StandbyWatchdog::check()
{
	utl::CodeStopwatch stopwatch(TAG, WATCHDOG_TIMEOUT_MS);

	fsm.proccess();
}

void StandbyWatchdog::alarm()
{
	fsm.push_event(alarm_e{});
}

bool StandbyWatchdog::isAlarmReady()
{
#if USE_WKUP_RTC_ALARM
	RTC_AlarmTypeDef sAlarm = {};
	RTC_TimeTypeDef  time   = {};
	RTC_DateTypeDef  date   = {};
	if (HAL_RTC_GetAlarm(&hrtc, &sAlarm, RTC_ALARM_A, RTC_FORMAT_BIN) != HAL_OK) {
#	if STANDBY_W_BEDUG
		printTagLog(TAG, "Unable to get clock alarm");
#	endif
		return false;
	}
	if (!clock_get_rtc_time(&time)) {
#	if STANDBY_W_BEDUG
		printTagLog(TAG, "Unable to get current time");
#	endif
		return false;
	}
	if (!clock_get_rtc_date(&date)) {
#	if STANDBY_W_BEDUG
		printTagLog(TAG, "Unable to get current date");
#	endif
		return false;
	}

	if (sAlarm.AlarmDateWeekDaySel != RTC_ALARMDATEWEEKDAYSEL_DATE) {
#	if STANDBY_W_BEDUG
		printTagLog(TAG, "Incorrect alarm settings loaded.");
#	endif
		return false;
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
	uint32_t needSecods   = settings.record_period > 0 ? settings.record_period / SECOND_MS : DELTA_SEC;
	if (currSeconds < alarmSeconds && (alarmSeconds - currSeconds) > needSecods) {
#	if STANDBY_W_BEDUG
		printTagLog(TAG, "The current alarm time is too long (%lu sec > %lu sec).", (alarmSeconds - currSeconds), needSecods);
#	endif
		return false;
	}
	if (alarmSeconds < currSeconds) {
#	if STANDBY_W_BEDUG
		printTagLog(TAG, "The alarm time has already passed.");
#	endif
		return false;
	}
#endif

	return true;
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

void StandbyWatchdog::startRTCAlarm(uint32_t seconds)
{
	if (seconds == 0) {
#if STANDBY_W_BEDUG
		printTagLog(TAG, "The standby time must be higher than 0");
		seconds = clock_get_timestamp() + DELTA_SEC;
#endif
	}

#if USE_WKUP_RTC_ALARM

	RTC_TimeTypeDef dumpTime = {};
	RTC_DateTypeDef dumpDate = {};
	RTC_TimeTypeDef currTime = {};
	RTC_DateTypeDef currDate = {};
	if (!clock_get_rtc_time(&currTime)) {
#	if STANDBY_W_BEDUG
		printTagLog(TAG, "Unable to get current time");
#	endif
	}
	if (!clock_get_rtc_date(&currDate)) {
#	if STANDBY_W_BEDUG
		printTagLog(TAG, "Unable to get current date");
#	endif
	}

	clock_seconds_to_datetime(seconds, &dumpDate, &dumpTime);

	if (dumpDate.Date    == currDate.Date &&
		dumpTime.Hours   == currTime.Hours &&
		dumpTime.Minutes == currTime.Minutes &&
		dumpTime.Seconds == currTime.Seconds
	) {
#	if STANDBY_W_BEDUG
		printTagLog(TAG, "Error - setting the same alarm time of the current time (adding the delta = %lu sec)", DELTA_SEC);
#	endif
		seconds += DELTA_SEC;
		clock_seconds_to_datetime(seconds, &dumpDate, &dumpTime);
	}

	if (seconds < clock_get_timestamp()) {
#	if STANDBY_W_BEDUG
		printTagLog(TAG, "The alarm time has already pass (adding the delta = %lu sec)", DELTA_SEC);
#	endif
		seconds += DELTA_SEC;
		clock_seconds_to_datetime(seconds, &dumpDate, &dumpTime);
	}


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

#	if STANDBY_W_BEDUG
	printTagLog(
		TAG,
		"The alarm clock has set for %02u day %02u:%02u:%02u (current %02u day time %02u:%02u:%02u)",
		sAlarm.AlarmDateWeekDay,
		sAlarm.AlarmTime.Hours,
		sAlarm.AlarmTime.Minutes,
		sAlarm.AlarmTime.Seconds,
		currDate.Date,
		currTime.Hours,
		currTime.Minutes,
		currTime.Seconds
	);
#	endif
#endif
}

bool StandbyWatchdog::needEnterStandby()
{
	if (!USBController::connected() && !timer.wait() && has_errors()) {
		return true;
	}

	if (is_status(NEED_STANDBY)) {
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

	if (is_status(NEED_MEASURE)) {
		return false;
	}

	return true;
}

uint32_t StandbyWatchdog::sleepTimeSec()
{
	return clock_get_timestamp() + (settings.record_period / SECOND_MS);
}


void StandbyWatchdog::_init_s::operator()()
{
	if (!is_status(WAIT_LOAD)) {
		fsm.push_event(loaded_e{});
	} else if (is_status(NEED_STANDBY)) {
		fsm.push_event(need_standby_e{});
	}
}

void StandbyWatchdog::_idle_s::operator ()()
{
#if USE_WKUP_RTC_ALARM
//	if (needStartAlarm()) { // TODO
//		fsm.push_event(need_start_alarm_e{});
//	}
#endif
	if (needEnterStandby()) {
		fsm.push_event(need_standby_e{});
	}
}

void StandbyWatchdog::_start_s::operator ()()
{
	fsm.push_event(started_e{});
}

void StandbyWatchdog::check_last_alarm_a::operator ()()
{
#if USE_WKUP_PA0
	HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN1);

	GPIO_InitTypeDef GPIO_InitStruct = {};
	GPIO_InitStruct.Pin  = WKUP_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(WKUP_GPIO_Port, &GPIO_InitStruct);
#endif

	timer.start();

	if (USBController::connected() && hasWokenUp()) {
		clearPWRFlags();
#if STANDBY_W_BEDUG
		printTagLog(TAG, "The device has exited the standby mode by USB connection.");
#endif
	}

#if USE_WKUP_PA0
	if (hasWokenUp()) {
		set_status(NEED_MEASURE);
#	if STANDBY_W_BEDUG
		printTagLog(TAG, "The device has woken up. Measure has requested.");
#	endif
		fsm.push_event(need_start_alarm_e{});
		return;
	}
#endif

#if STANDBY_W_BEDUG
	printTagLog(TAG, "The device has reloaded. Try to reinit clock alarm.");
#endif

	RTC_AlarmTypeDef sAlarm  = {};
	HAL_StatusTypeDef status = HAL_RTC_GetAlarm(&hrtc, &sAlarm, RTC_ALARM_A, RTC_FORMAT_BIN);
	if (status != HAL_OK) {
#if STANDBY_W_BEDUG
		printTagLog(TAG, "Unable to get clock alarm");
#endif
		fsm.push_event(need_start_alarm_e{});
		return;
	}

#if USE_WKUP_RTC_ALARM
	RTC_DateTypeDef  date = {};
	RTC_TimeTypeDef  time = {};
	if (!clock_get_rtc_time(&time)) {
#	if STANDBY_W_BEDUG
		printTagLog(TAG, "Unable to get current time");
#	endif
	}
	if (!clock_get_rtc_date(&date)) {
#	if STANDBY_W_BEDUG
		printTagLog(TAG, "Unable to get current date");
#	endif
	}
#endif

#if STANDBY_W_BEDUG
	printTagLog(
		TAG,
		"The reloaded alarm clock: %02u day %02u:%02u:%02u (current %02u day time %02u:%02u:%02u)",
		sAlarm.AlarmDateWeekDay,
		sAlarm.AlarmTime.Hours,
		sAlarm.AlarmTime.Minutes,
		sAlarm.AlarmTime.Seconds,
		date.Date,
		time.Hours,
		time.Minutes,
		time.Seconds
	);
#endif

#if USE_WKUP_RTC_ALARM
	if (!isAlarmReady()) {
		fsm.push_event(need_start_alarm_e{});
		return;
	}
#endif

	fsm.push_event(need_restart_alarm_e{});
}

void StandbyWatchdog::restart_alarm_a::operator ()()
{
	fsm.clear_events();

	RTC_AlarmTypeDef sAlarm  = {};
	HAL_StatusTypeDef status = HAL_RTC_GetAlarm(&hrtc, &sAlarm, RTC_ALARM_A, RTC_FORMAT_BIN);
	if (status != HAL_OK) {
#if STANDBY_W_BEDUG
		printTagLog(TAG, "Unable to get clock alarm");
#endif
		fsm.push_event(need_start_alarm_e{});
		return;
	}
	status = HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN);
	if (status != HAL_OK) {
#if STANDBY_W_BEDUG
		printTagLog(TAG, "Unable to restart clock alarm");
#endif
		fsm.push_event(need_start_alarm_e{});
		return;
	}
}

void StandbyWatchdog::start_alarm_a::operator ()()
{
	fsm.clear_events();

	set_status(NEED_MEASURE);

	clearPWRFlags();

	uint32_t needSeconds = sleepTimeSec();
//	uint32_t lastSeconds = 0; // TODO
//	if (Record::getLastTime(&lastSeconds) != RECORD_OK) {
//		startRTCAlarm(needSeconds);
//		return;
//	}
//
//	uint32_t currSeconds = clock_get_timestamp();
//	if (lastSeconds < currSeconds && lastSeconds + needSeconds > currSeconds) {
//		needSeconds = currSeconds - lastSeconds;
//	}

	startRTCAlarm(needSeconds);
}

void StandbyWatchdog::check_alarm_a::operator ()()
{
	if (!isAlarmReady()) {
		fsm.push_event(need_start_alarm_e{});
	}
}

void StandbyWatchdog::enter_standby_a::operator ()()
{
#if STANDBY_W_BEDUG
	printTagLog(TAG, "Initalizing the standby mode. The device turns off.");
#endif

#if USE_WKUP_RTC_ALARM
	if (!isAlarmReady()) {
		startRTCAlarm();
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
