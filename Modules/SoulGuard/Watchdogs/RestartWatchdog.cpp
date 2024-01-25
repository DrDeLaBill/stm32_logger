/* Copyright © 2024 Georgy E. All rights reserved. */

#include "RestartWatchdog.h"

#include "main.h"

#include "CodeStopwatch.h"


void RestartWatchdog::check()
{
	utl::CodeStopwatch stopwatch("IWDG", GENERAL_TIMEOUT_MS);
	// TODO: IWDG, NVIC_SysReset and other restarts detect and reset
//	BEDUG_ASSERT(false, "INTERNAL ERROR HAS BEEN OCCURRED (HARD FAULT)");
}
