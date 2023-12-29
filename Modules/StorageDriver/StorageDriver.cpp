/* Copyright © 2023 Georgy E. All rights reserved. */

#include "StorageDriver.h"

#include "w25qxx.h"


StorageStatus StorageDriver::read(uint32_t address, uint8_t *data, uint32_t len) {
    flash_status_t status = flash_w25qxx_read(address, data, len);
    if (status == FLASH_BUSY) {
        return STORAGE_BUSY;
    }
    if (status == FLASH_OOM) {
        return STORAGE_OOM;
    }
    if (status != FLASH_OK) {
        return STORAGE_ERROR;
    }
    return STORAGE_OK;
}
;

StorageStatus StorageDriver::write(uint32_t address, uint8_t *data, uint32_t len) {
	flash_status_t status = flash_w25qxx_write(address, data, len);
    if (status == FLASH_BUSY) {
        return STORAGE_BUSY;
    }
    if (status == FLASH_OOM) {
        return STORAGE_OOM;
    }
    if (status != FLASH_OK) {
        return STORAGE_ERROR;
    }
//    util_debug_hex_dump("MN", data, address, len);
    return STORAGE_OK;
}
