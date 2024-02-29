/* Copyright © 2023 Georgy E. All rights reserved. */

#include "StorageDriver.h"

#include "log.h"
#include "w25qxx.h"
#include "bmacro.h"


bool StorageDriver::hasBuffer = false;
uint8_t StorageDriver::bufferPage[Page::PAGE_SIZE] = {};
uint32_t StorageDriver::lastAddress = 0;


StorageStatus StorageDriver::read(uint32_t address, uint8_t *data, uint32_t len) {
	flash_status_t status = FLASH_OK;
	if (hasBuffer && lastAddress == address && len == Page::PAGE_SIZE) {
		memcpy(data, bufferPage, len);
#if STORAGE_DRIVER_BEDUG
		printTagLog(TAG, "Copy %lu address start", address);
#endif
	} else {
		status = flash_w25qxx_read(address, data, len);
#if STORAGE_DRIVER_BEDUG
		printTagLog(TAG, "Read %lu address start", address);
#endif
	}
	BEDUG_ASSERT((status != FLASH_BUSY), "Storage is busy");
    if (status == FLASH_BUSY) {
        return STORAGE_BUSY;
    }
    if (status == FLASH_OOM) {
        return STORAGE_OOM;
    }
    if (status != FLASH_OK) {
        return STORAGE_ERROR;
    }
    if (lastAddress != address && len == Page::PAGE_SIZE) {
    	memcpy(bufferPage, data, Page::PAGE_SIZE);
    	lastAddress = address;
    	hasBuffer = true;
    }
#if STORAGE_DRIVER_BEDUG
	printTagLog(TAG, "Read %lu address success", address);
#endif
    return STORAGE_OK;
}
;

StorageStatus StorageDriver::write(uint32_t address, uint8_t *data, uint32_t len) {
#if STORAGE_DRIVER_BEDUG
	printTagLog(TAG, "Write %lu address start", address);
#endif
	flash_status_t status = flash_w25qxx_write(address, data, len);
	hasBuffer = false;
	BEDUG_ASSERT((status != FLASH_BUSY), "Storage is busy");
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
    return STORAGE_OK;
}
