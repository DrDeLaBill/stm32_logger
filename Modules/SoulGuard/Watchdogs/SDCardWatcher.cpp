/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Watchdogs.h"

#include <stdio.h>

#include "glog.h"
#include "soul.h"
#include "hal_defs.h"
#include "internal_storage.h"

#include "CodeStopwatch.h"


void SDCardWatcher::check()
{
	utl::CodeStopwatch stopwatch(TAG, GENERAL_TIMEOUT_MS);

	if (!is_error(SD_CARD_ERROR)) {
		return;
	}

	DSTATUS ds_status = DIO_SPI_initialize(DIOSPIFatFS.drv);
	if (ds_status != RES_OK) {
		printTagLog(TAG, "Recall DIO_SPI_initialize ERROR=%u", ds_status);
	}

	char filename[64];
	snprintf(filename, sizeof(filename), "%s" "%s", DIOSPIPath, "test.txt");

	char text[] = "test";

	FRESULT res = intstor_test();
	if (res == FR_OK) {
		reset_error(SD_CARD_ERROR);
		printTagLog(TAG, "intstor_test OK");
		return;
	} else {
		printTagLog(TAG, "intstor_test ERROR=%u", res);
	}

	UINT br;
	res = intstor_write_file(filename, &text, strlen(text), &br);
	if (res == FR_OK) {
		reset_error(SD_CARD_ERROR);
		printTagLog(TAG, "Reset SD_CARD_ERROR OK");
	} else {
		printTagLog(TAG, "Reset SD_CARD_ERROR ERROR=%u", res);
	}
}
