/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Watchdogs.h"

#include "system.h"
#include "hal_defs.h"

#include "CodeStopwatch.h"


void PowerWatchdog::check()
{
	utl::CodeStopwatch stopwatch("PWRw", WATCHDOG_TIMEOUT_MS);

	if (!is_status(WORKING)) {
		reset_error(POWER_ERROR);
		return;
	}

	uint16_t voltage = (uint16_t)get_system_power();

	if (STM_MIN_VOLTAGEx10 <= voltage && voltage <= STM_MAX_VOLTAGEx10) {
		reset_error(POWER_ERROR);
	} else {
		set_error(POWER_ERROR);
	}
}
