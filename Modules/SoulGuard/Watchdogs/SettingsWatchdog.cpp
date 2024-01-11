/* Copyright © 2024 Georgy E. All rights reserved. */

#include "SettingsWatchdog.h"

#include <cstring>

#include "settings.h"
#include "SettingsDB.h"


void SettingsWatchdog::check()
{
	settings_t tmpSettings;
	SettingsDB settingsDB(reinterpret_cast<uint8_t*>(&tmpSettings), settings_size());
	SettingsDB::SettingsStatus status = SettingsDB::SETTINGS_OK;

	memcpy(settings_get(), reinterpret_cast<uint8_t*>(&tmpSettings), settings_size());
	if (is_settings_saved()) {
		status = settingsDB.load();
		set_settings_save_status(status != SettingsDB::SETTINGS_OK);
	} else if (is_settings_updated()) {
		memcpy(reinterpret_cast<uint8_t*>(&tmpSettings), settings_get(), settings_size());
		status = settingsDB.save();
		set_settings_update_status(status != SettingsDB::SETTINGS_OK);
	}

    if (!settings_check(reinterpret_cast<uint8_t*>(&tmpSettings))) {
		settings_reset();
		memcpy(reinterpret_cast<uint8_t*>(&tmpSettings), settings_get(), settings_size());
		status = settingsDB.save();
    }

	if (status != SettingsDB::SETTINGS_OK) {
		return;
	}

	if (memcmp(reinterpret_cast<uint8_t*>(&tmpSettings), settings_get(), settings_size())) {
		settings_set(reinterpret_cast<uint8_t*>(&tmpSettings));
		settings_show();
	}

	if (is_settings_initialized()) {
		return;
	}

	if (settingsDB.load() == SettingsDB::SETTINGS_OK) {
	    set_settings_initialized();
	}
}
