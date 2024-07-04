/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Watchdogs.h"

#include <stdio.h>

#include "soul.h"
#include "internal_storage.h"

#include "CodeStopwatch.h"


void SDCardWatcher::check()
{
	utl::CodeStopwatch stopwatch("SDCw", GENERAL_TIMEOUT_MS);

	if (!is_error(SD_CARD_ERROR)) {
		return;
	}

	char filename[64];
	snprintf(filename, sizeof(filename), "%s" "%s", DIOSPIPath, "test.txt");

	char text[] = "test";

	UINT br;
	FRESULT res = intstor_write_file(filename, &text, sizeof(text), &br);
	if(res == FR_OK) {
		reset_error(SD_CARD_ERROR);
	}
}
