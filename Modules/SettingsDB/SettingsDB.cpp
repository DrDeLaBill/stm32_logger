#include "SettingsDB.h"

#include <cstring>
#include <algorithm>

#include "log.h"
#include "bedug.h"
#include "w25qxx.h"
#include "StorageAT.h"
#include "StorageDriver.h"


SettingsDB::SettingsDB(uint8_t* settings, uint32_t size): size(size), settings(settings)
{ }

SettingsDB::SettingsStatus SettingsDB::load()
{
	StorageDriver storageDriver;
	StorageAT storage(
		flash_w25qxx_get_pages_count(),
		&storageDriver
	);
	uint32_t address = 0;
	StorageStatus status = STORAGE_OK;

    status = storage.find(FIND_MODE_EQUAL, &address, PREFIX, 1);
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        printTagLog(SettingsDB::TAG, "error load settings: storage find error=%02X", status);
#endif
        return SETTINGS_ERROR;
    }

    uint8_t tmpSettings[this->size] = {};
    status = storage.load(address, tmpSettings, this->size);
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        printTagLog(SettingsDB::TAG, "error load settings: storage load error=%02X address=%lu", status, address);
#endif
        return SETTINGS_ERROR;
    }

    memcpy(settings, tmpSettings, this->size);

#if SETTINGS_BEDUG
    printTagLog(SettingsDB::TAG, "settings loaded");
#endif

    return SETTINGS_OK;
}

SettingsDB::SettingsStatus SettingsDB::save()
{
	StorageDriver storageDriver;
	StorageAT storage(
		flash_w25qxx_get_pages_count(),
		&storageDriver
	);
	uint32_t address = 0;
	StorageStatus status = STORAGE_OK;

    status = storage.find(FIND_MODE_EQUAL, &address, PREFIX, 1);
    if (status == STORAGE_NOT_FOUND) {
        status = storage.find(FIND_MODE_EMPTY, &address);
    }

    if (status == STORAGE_NOT_FOUND) {
        // Search for any address
    	status = storage.find(FIND_MODE_NEXT, &address, "", 1);
    }

    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        printTagLog(SettingsDB::TAG, "error save settings: storage find error=%02X", status);
#endif
        return SETTINGS_ERROR;
    }

	status = storage.rewrite(address, PREFIX, 1, this->settings, this->size);
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        printTagLog(SettingsDB::TAG, "error save settings: storage save error=%02X address=%lu", status, address);
#endif
        return SETTINGS_ERROR;
    }

    if (this->load() == SETTINGS_OK) {
#if SETTINGS_BEDUG
    	printTagLog(SettingsDB::TAG, "settings saved (address=%lu)", address);
#endif
    	return SETTINGS_OK;
    }

    return SETTINGS_ERROR;
}
