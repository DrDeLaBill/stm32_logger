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

	// TODO: if device has MEMORY_ERROR: check memory every 1000 ms
}
