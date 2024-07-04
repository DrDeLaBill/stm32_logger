/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Watchdogs.h"

#include <random>

#include "main.h"
#include "soul.h"
#include "w25qxx.h"
#include "system.h"
#include "hal_defs.h"

#include "CodeStopwatch.h"


#define ERRORS_MAX (5)


MemoryWatchdog::MemoryWatchdog():
	errorTimer(TIMEOUT_MS), timer(SECOND_MS), errors(0), timerStarted(false)
{
	set_error(MEMORY_INIT_ERROR);
}

void MemoryWatchdog::check()
{
	utl::CodeStopwatch stopwatch("MEMw", GENERAL_TIMEOUT_MS);

	if (is_error(MEMORY_INIT_ERROR)) {
		flash_w25qxx_init() == FLASH_OK ? reset_error(MEMORY_INIT_ERROR) : set_error(MEMORY_INIT_ERROR);
		return;
	}

	if (timer.wait()) {
		return;
	}
	timer.start();

	uint8_t data = 0;
	flash_status_t status = FLASH_OK;
	if (is_status(MEMORY_READ_FAULT) ||
		is_status(MEMORY_WRITE_FAULT) ||
		is_error(MEMORY_ERROR)
	) {
		uint32_t address = static_cast<uint32_t>(rand()) % (flash_w25qxx_get_pages_count() * FLASH_W25_PAGE_SIZE);

		status = flash_w25qxx_read(address, &data, sizeof(data));
		if (status == FLASH_OK) {
			reset_status(MEMORY_READ_FAULT);
			status = flash_w25qxx_write(address, &data, sizeof(data));
		} else {
			errors++;
		}
		if (status == FLASH_OK) {
			reset_status(MEMORY_WRITE_FAULT);
			timerStarted = false;
			errors = 0;
		} else {
			errors++;
		}
	}

	(errors > ERRORS_MAX) ? set_error(MEMORY_ERROR) : reset_error(MEMORY_ERROR);

	if (!timerStarted && is_error(MEMORY_ERROR)) {
		timerStarted = true;
		errorTimer.start();
	}

	if (timerStarted && !errorTimer.wait()) {
		system_error_handler(MEMORY_ERROR, nullptr);
	}
}
