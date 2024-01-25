/* Copyright © 2024 Georgy E. All rights reserved. */

#include "RTCWatchdog.h"

#include "main.h"
#include "clock.h"
#include "bmacro.h"

#include "CodeStopwatch.h"


void RTCWatchdog::check()
{
	utl::CodeStopwatch stopwatch("RTC", GENERAL_TIMEOUT_MS);

	RTC_DateTypeDef date = {};
	RTC_TimeTypeDef time = {};

	if (!clock_get_rtc_date(&date)) {
		BEDUG_ASSERT(false, "Unable to get current date");
		return;
	}

	if (!clock_get_rtc_time(&time)) {
		BEDUG_ASSERT(false, "Unable to get current time");
		return;
	}

	bool updateFlag = false;
	if (date.Date == 0 || date.Date > 31) {
		updateFlag = true;
		date.Date = 1;
	}
	if (date.Month == 0 || date.Month > 12) {
		updateFlag = true;
		date.Month = 1;
	}

	if (updateFlag) {
		clock_save_date(&date);
	}

	updateFlag = false;
	if (time.Seconds > 59) {
		updateFlag = true;
		time.Seconds = 0;
	}
	if (time.Minutes > 59) {
		updateFlag = true;
		time.Minutes = 0;
	}
	if (time.Hours > 23) {
		updateFlag = true;
		time.Hours = 0;
	}

	if (updateFlag) {
		clock_save_time(&time);
	}
}
