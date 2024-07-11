/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Watchdogs.h"

#include <stdio.h>

#include "glog.h"
#include "soul.h"
#include "hal_defs.h"
#include "internal_storage.h"

#include "CodeStopwatch.h"


SDCardWatcher::SDCardWatcher(): errors(0) {}

void SDCardWatcher::check()
{
	utl::CodeStopwatch stopwatch(TAG, GENERAL_TIMEOUT_MS);

	if (!is_error(SD_CARD_ERROR)) {
		return;
	}

	printTagLog(TAG, "Try to reinit SD card");

	DSTATUS status = DIO_SPI_initialize(DIOSPIFatFS.drv);
	if (status != RES_OK) {
		set_error(SD_CARD_ERROR);
		printTagLog(TAG, "DIO_SPI_initialize() ERROR=%u", status);
	}

	char filename[64];
	snprintf(filename, sizeof(filename), "%s" "%s", DIOSPIPath, "test.txt");

	FRESULT res = intstor_test();
	if (res == FR_OK) {
		reset_error(SD_CARD_ERROR);
		errors = 0;
		printTagLog(TAG, "intstor_test() OK");
		return;
	} else {
		set_error(SD_CARD_ERROR);
		errors++;
		printTagLog(TAG, "intstor_test() ERROR=%u", res);
	}

	if (errors <= ERRORS_MAX) {
		return;
	}

	res = f_mount(&DIOSPIFatFS, DIOSPIPath, 1);
	if (res == FR_NO_FILESYSTEM) { // || res == FR_DISK_ERR) { // TODO
		printTagLog(TAG, "f_mount() error=%u (no file system or the physical drive cannot work)", res);
		BYTE work[_MAX_SS] = {0};
		res = f_mkfs(DIOSPIPath, FM_FAT, 0, work, sizeof work);
		if (res != FR_OK) {
			printTagLog(TAG, "f_mkfs() error=%u", res);
		} else {
			printTagLog(TAG, "make FAT OK");
		}
	} else if (res != FR_OK) {
		printTagLog(TAG, "f_mount() error=%u", res);
	}
	if (res == FR_OK) {
		printTagLog(TAG, "test mount OK");
	}
	res = f_mount(NULL, DIOSPIPath, 0);
	if (res != FR_OK) {
		printTagLog(TAG, "unmount error=%u", res);
	}
}
