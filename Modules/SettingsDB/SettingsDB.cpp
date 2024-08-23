#include "SettingsDB.h"

#include <cstring>
#include <algorithm>

#include "glog.h"
#include "soul.h"
#include "gutils.h"
#include "clock.h"
#include "w25qxx.h"
#include "settings.h"

#include "StorageAT.h"
#include "StorageDriver.h"
#include "CodeStopwatch.h"


extern StorageAT* storage;


SettingsDB::SettingsDB(uint8_t* settings, uint32_t size): size(size), settings(settings) { }

SettingsStatus SettingsDB::load()
{
	uint32_t address1 = 0, address2 = 0;
	StorageStatus status = STORAGE_OK;
    settings_t tmpSettings1 = {};
    settings_t tmpSettings2 = {};

	bool needResaveFirst = false, needResaveSecond = false;
    status = storage->find(FIND_MODE_EQUAL, &address1, PREFIX, 1);
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        printTagLog(SettingsDB::TAG, "error load settings: try to find duplicate (error=%02X)", status);
#endif
        needResaveFirst = true;
    }
	if (!needResaveFirst) {
		status = storage->load(address1, reinterpret_cast<uint8_t*>(&tmpSettings1), this->size);
	}
	if (!needResaveFirst && status != STORAGE_OK) {
#if SETTINGS_BEDUG
		printTagLog(SettingsDB::TAG, "error load settings: storage load error=%02X address1=%08lX", status, address1);
#endif
		needResaveFirst = true;
	}

    status = storage->find(FIND_MODE_EQUAL, &address2, PREFIX, 2);
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        printTagLog(SettingsDB::TAG, "error load settings duplicate: storage find error=%02X", status);
#endif
        needResaveSecond = true;
    }
	if (!needResaveSecond) {
        status = storage->load(address2, reinterpret_cast<uint8_t*>(&tmpSettings2), this->size);
	}
	if (!needResaveSecond && status != STORAGE_OK) {
#if SETTINGS_BEDUG
		printTagLog(SettingsDB::TAG, "error load settings duplicate: storage load error=%02X adderss2=%08lX", status, address2);
#endif
		needResaveSecond = true;
	}
    if (needResaveSecond && needResaveFirst) {
    	status = STORAGE_NOT_FOUND;
    }
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        printTagLog(SettingsDB::TAG, "error load settings: storage load error=%02X address1=%08lX, adderss2=%08lX", status, address1, address2);
#endif
        return SETTINGS_ERROR;
    }

	if (!needResaveFirst) {
	    memcpy(this->settings, &tmpSettings1, this->size);
	} else if (!needResaveSecond) {
	    memcpy(this->settings, &tmpSettings2, this->size);
	}

#if SETTINGS_BEDUG
    printTagLog(SettingsDB::TAG, "settings loaded");
#endif

    if (needResaveFirst || needResaveSecond) {
    	set_status(NEED_SAVE_SETTINGS);
    }

    return SETTINGS_OK;
}

SettingsStatus SettingsDB::save()
{
	utl::CodeStopwatch stopwatch(TAG, GENERAL_TIMEOUT_MS);

	uint32_t address = 0;
	StorageStatus status = STORAGE_OK;

    status = storage->find(FIND_MODE_EQUAL, &address, PREFIX, 1);

    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        printTagLog(SettingsDB::TAG, "error save settings: storage find error, try to find duplicate (error=%02X)", status);
#endif
    	status = storage->find(FIND_MODE_EQUAL, &address, PREFIX, 2);
    }

    if (status == STORAGE_NOT_FOUND) {
#if SETTINGS_BEDUG
        printTagLog(SettingsDB::TAG, "error save settings: storage find duplicate error, try to find empty (error=%02X)", status);
#endif
        status = storage->find(FIND_MODE_EMPTY, &address);
    }

    if (status == STORAGE_NOT_FOUND) {
        // Search for any address
#if SETTINGS_BEDUG
        printTagLog(SettingsDB::TAG, "error save settings: storage find empty error, try to find any address (error=%02X)", status);
#endif
    	status = storage->find(FIND_MODE_NEXT, &address, "", 0);
    }

    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        printTagLog(SettingsDB::TAG, "error save settings: storage find error=%02X", status);
#endif
        return SETTINGS_ERROR;
    }

    // Save original settings
	status = storage->rewrite(address, PREFIX, 1, this->settings, this->size);
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        printTagLog(SettingsDB::TAG, "error save settings: storage save error=%02X address=%lu", status, address);
#endif
        return SETTINGS_ERROR;
    }

    // Save duplicate settings
	status = storage->find(FIND_MODE_EQUAL, &address, PREFIX, 2);
    if (status == STORAGE_NOT_FOUND) {
#if SETTINGS_BEDUG
        printTagLog(SettingsDB::TAG, "error save settings duplicate: storage find error, try to find empty (error=%02X)", status);
#endif
        status = storage->find(FIND_MODE_EMPTY, &address);
    }
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        printTagLog(SettingsDB::TAG, "error save settings duplicate: storage find empty error=%02X", status);
#endif
        return SETTINGS_ERROR;
    }

	status = storage->rewrite(address, PREFIX, 2, this->settings, this->size);
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        printTagLog(SettingsDB::TAG, "error save settings duplicate: storage save error=%02X address=%lu", status, address);
#endif
        return SETTINGS_ERROR;
    }

    if (this->load() == SETTINGS_OK) {
#if SETTINGS_BEDUG
    	printTagLog(SettingsDB::TAG, "settings saved successfully (address=%lu)", address);
#endif

    	return SETTINGS_OK;
    }

    return SETTINGS_ERROR;
}
