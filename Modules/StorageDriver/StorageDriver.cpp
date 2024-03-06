/* Copyright © 2023 Georgy E. All rights reserved. */

#include "StorageDriver.h"

#include "log.h"
#include "soul.h"
#include "w25qxx.h"
#include "bmacro.h"


#if STORAGE_DRIVER_USE_BUFFER

bool StorageDriver::hasBuffer = false;
uint8_t StorageDriver::bufferPage[Page::PAGE_SIZE] = {};
uint32_t StorageDriver::lastAddress = 0;

#endif


StorageStatus StorageDriver::read(uint32_t address, uint8_t *data, uint32_t len) {
	if (is_error(POWER_ERROR)) {

#if STORAGE_DRIVER_BEDUG
		printTagLog(TAG, "Error power", address);
#endif

		return STORAGE_ERROR;
	}
	flash_status_t status = FLASH_OK;

#if STORAGE_DRIVER_USE_BUFFER

	if (hasBuffer && lastAddress == address && len == Page::PAGE_SIZE) {
		memcpy(data, bufferPage, len);

#	if STORAGE_DRIVER_BEDUG
		printTagLog(TAG, "Copy %lu address start", address);
#	endif

	} else {

#endif

		status = flash_w25qxx_read(address, data, len);
#if STORAGE_DRIVER_BEDUG
		printTagLog(TAG, "Read %lu address start", address);
#endif

#if STORAGE_DRIVER_USE_BUFFER

	}

#endif
	BEDUG_ASSERT((status != FLASH_BUSY), "Storage is busy");
	if (status != FLASH_OK) {
    	set_error(MEMORY_ERROR);
	}
    if (status == FLASH_BUSY) {
        return STORAGE_BUSY;
    }
    if (status == FLASH_OOM) {
        return STORAGE_OOM;
    }
    if (status != FLASH_OK) {
        return STORAGE_ERROR;
    }

#if STORAGE_DRIVER_USE_BUFFER

    if (lastAddress != address && len == Page::PAGE_SIZE) {
    	memcpy(bufferPage, data, Page::PAGE_SIZE);
    	lastAddress = address;
    	hasBuffer = true;
    }

#endif

#if STORAGE_DRIVER_BEDUG
	printTagLog(TAG, "Read %lu address success", address);
#endif

	reset_error(MEMORY_ERROR);
    return STORAGE_OK;
}
;

StorageStatus StorageDriver::write(uint32_t address, uint8_t *data, uint32_t len) {
	if (is_error(POWER_ERROR)) {

#if STORAGE_DRIVER_BEDUG
		printTagLog(TAG, "Error power", address);
#endif

		return STORAGE_ERROR;
	}

#if STORAGE_DRIVER_BEDUG
	printTagLog(TAG, "Write %lu address start", address);
#endif

	flash_status_t status = flash_w25qxx_write(address, data, len);

#if STORAGE_DRIVER_USE_BUFFER

	if (lastAddress == address) {
		hasBuffer = false;
	}

#endif

	BEDUG_ASSERT((status != FLASH_BUSY), "Storage is busy");
	if (status != FLASH_OK) {
    	set_error(MEMORY_ERROR);
	}
    if (status == FLASH_BUSY) {
        return STORAGE_BUSY;
    }
    if (status == FLASH_OOM) {
        return STORAGE_OOM;
    }
    if (status != FLASH_OK) {
        return STORAGE_ERROR;
    }

#if STORAGE_DRIVER_BEDUG
	printTagLog(TAG, "Write %lu address success", address);
#endif

	reset_error(MEMORY_ERROR);
    return STORAGE_OK;
}
