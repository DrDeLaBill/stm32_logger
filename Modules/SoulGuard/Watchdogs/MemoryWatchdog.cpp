/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Watchdogs.h"

#include "main.h"
#include "soul.h"

#include "StorageDriver.h"
#include "CodeStopwatch.h"


utl::Timer MemoryWatchdog::timer(SECOND_MS);


void MemoryWatchdog::check()
{
	utl::CodeStopwatch stopwatch("MEM", GENERAL_TIMEOUT_MS);

	if (timer.wait()) {
		return;
	}

	if (StorageDriver::storageError()) {
		set_error(MEMORY_ERROR);
	} else {
		reset_error(MEMORY_ERROR);
	}

	timer.start();
}
