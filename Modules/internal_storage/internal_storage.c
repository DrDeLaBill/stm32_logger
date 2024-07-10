/*
 * internal_storage.c
 *
 *  Created on: 2 ���. 2022 �.
 *      Author: gauss
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stm32f4xx.h>
#include <fatfs.h>
//
#include "soul.h"
#include "glog.h"
#include "gutils.h"
#include "user_diskio_spi.h"
#include "internal_storage.h"


const char* STOR_MODULE_TAG = "STOR";


FRESULT _instor_mount();


FRESULT intstor_test()
{
	const char* filename = "test.txt";
	const char buf[] = "test";
	UINT size = strlen(buf);
	UINT bw = 0;

	FRESULT res;
	FRESULT out = FR_OK;

	res = f_mount(&DIOSPIFatFS, DIOSPIPath, 1);
	if (res != FR_OK) {
		printTagLog(STOR_MODULE_TAG, "f_mount() error=%i", res);
		out = res;
		goto do_unmount;
	}

	res = f_open(&DIOSPIFile, filename, FA_CREATE_ALWAYS|FA_WRITE);
	if (res != FR_OK) {
		printTagLog(STOR_MODULE_TAG, "f_open() error=%i", res);
		out = res;
		goto do_unmount;
	}

	res = f_write(&DIOSPIFile, (uint8_t*)buf, size, &bw);
	if (res != FR_OK) {
		printTagLog(STOR_MODULE_TAG, "f_write() error=%i", res);
		out = res;
		goto do_close;
	}

	res = f_close(&DIOSPIFile);
	if (res != FR_OK) {
		printTagLog(STOR_MODULE_TAG, "f_close() error=%i", res);
		out = res;
		goto do_unmount;
	}

	res = f_open(&DIOSPIFile, filename, FA_CREATE_ALWAYS|FA_WRITE);
	if (res != FR_OK) {
		printTagLog(STOR_MODULE_TAG, "f_open() error=%i", res);
		out = res;
		goto do_unmount;
	}

	UINT br = 0;
	char read_buf[sizeof(buf)] = {};
	res = f_read(&DIOSPIFile, (uint8_t*)read_buf, sizeof(read_buf), &br);
	if (res != FR_OK) {
		printTagLog(STOR_MODULE_TAG, "f_write() error=%i", res);
		out = res;
		goto do_close;
	}

do_close:
	res = f_close(&DIOSPIFile);
	if (res != FR_OK) {
		printTagLog(STOR_MODULE_TAG, "f_close() error=%i", res);
	}

do_unmount:
	if (res != FR_OK) {
		set_error(SD_CARD_ERROR);
	} else {
		reset_error(SD_CARD_ERROR);
	}

	res = f_mount(NULL, DIOSPIPath, 0);
	if (res != FR_OK) {
		printTagLog(STOR_MODULE_TAG, "f_mount(unmount) error=%i", res);
	}

	return out;
}

FRESULT intstor_read_file(const char* filename, void* buf, UINT size, UINT* br) {
	FRESULT res;
	FRESULT out = FR_OK;

	if(br) (*br) = 0;

	res = _instor_mount();
	if (res != FR_OK) {
		printTagLog(STOR_MODULE_TAG, "_instor_mount() error=%i", res);
		goto do_unmount;
	}

	res = f_open(&DIOSPIFile, filename, FA_OPEN_EXISTING|FA_READ);
	if (res != FR_OK) {
		printTagLog(STOR_MODULE_TAG, "f_open() error=%i\r", res);
		out = res;
		goto do_unmount;
	}

	res = f_read(&DIOSPIFile, (uint8_t*)buf, size, br);
	if (res != FR_OK) {
		printTagLog(STOR_MODULE_TAG, "f_write() error=%i", res);
		out = res;
		goto do_close;
	}

do_close:
	res = f_close(&DIOSPIFile);
	if (res != FR_OK) {
		printTagLog(STOR_MODULE_TAG, "f_close() error=%i", res);
	}

do_unmount:
	if (res != FR_OK) {
		set_error(SD_CARD_ERROR);
	} else {
		reset_error(SD_CARD_ERROR);
	}

	res = f_mount(NULL, DIOSPIPath, 0);
	if (res != FR_OK) {
		printTagLog(STOR_MODULE_TAG, "f_mount(unmount) error=%i", res);
	}

	return out;
}


FRESULT intstor_write_file(const char* filename, const void* buf, UINT size, UINT* bw) {
	FRESULT res;
	FRESULT out = FR_OK;

	if(bw) (*bw) = 0;

	res = _instor_mount();
	if (res != FR_OK) {
		printTagLog(STOR_MODULE_TAG, "_instor_mount() error=%i", res);
		out = res;
		goto do_unmount;
	}

	res = f_open(&DIOSPIFile, filename, FA_CREATE_ALWAYS|FA_WRITE);
	if (res != FR_OK) {
		printTagLog(STOR_MODULE_TAG, "f_open() error=%i", res);
		out = res;
		goto do_unmount;
	}

	res = f_write(&DIOSPIFile, (uint8_t*)buf, size, bw);
	if (res != FR_OK) {
		printTagLog(STOR_MODULE_TAG, "f_write() error=%i", res);
		out = res;
		goto do_close;
	}

do_close:
	res = f_close(&DIOSPIFile);
	if (res != FR_OK) {
		printTagLog(STOR_MODULE_TAG, "f_close() error=%i", res);
	}

do_unmount:
	if (res != FR_OK) {
		set_error(SD_CARD_ERROR);
	} else {
		reset_error(SD_CARD_ERROR);
	}

	res = f_mount(NULL, DIOSPIPath, 0);
	if (res != FR_OK) {
		printTagLog(STOR_MODULE_TAG, "f_mount(unmount) error=%i", res);
	}

	return out;
}


FRESULT intstor_append_file(const char* filename, const void* buf, UINT size, UINT* bw) {
	FRESULT res;
	FRESULT out = FR_OK;

	if(bw) (*bw) = 0;

	res = _instor_mount();
	if (res != FR_OK) {
		printTagLog(STOR_MODULE_TAG, "_instor_mount() error=%i", res);
		out = res;
		goto do_unmount;
	}

	res = f_open(&DIOSPIFile, filename, FA_OPEN_APPEND|FA_WRITE);
	if (res != FR_OK) {
		printTagLog(STOR_MODULE_TAG, "f_open() error=%i", res);
		out = res;
		goto do_unmount;
	}

	res = f_write(&DIOSPIFile, (uint8_t*)buf, size, bw);
	if (res != FR_OK) {
		printTagLog(STOR_MODULE_TAG, "f_write() error=%i", res);
		out = res;
		goto do_close;
	}

do_close:
	res = f_close(&DIOSPIFile);
	if (res != FR_OK) {
		printTagLog(STOR_MODULE_TAG, "f_close() error=%i", res);
	}

do_unmount:
	if (res != FR_OK) {
		set_error(SD_CARD_ERROR);
	} else {
		reset_error(SD_CARD_ERROR);
	}

	res = f_mount(NULL, DIOSPIPath, 0);
	if (res != FR_OK) {
		printTagLog(STOR_MODULE_TAG, "f_mount(unmount) error=%i", res);
	}

	return out;
}

FRESULT _instor_mount()
{
	FRESULT res = f_mount(&DIOSPIFatFS, DIOSPIPath, 1);
	if (res == FR_NO_FILESYSTEM) {
		printTagLog(STOR_MODULE_TAG, "f_mount() error=FR_NO_FILESYSTEM");
		BYTE work[_MAX_SS] = {0};
		res = f_mkfs(DIOSPIPath, FM_FAT, 0, work, sizeof work);
	} else if (res != FR_OK) {
		printTagLog(STOR_MODULE_TAG, "f_mount() error=%u", res);
		return res;
	}
	if (res != FR_OK) {
		printTagLog(STOR_MODULE_TAG, "f_mkfs() error=%i", res);
	}
	return res;
}
