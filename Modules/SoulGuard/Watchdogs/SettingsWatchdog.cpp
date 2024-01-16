/* Copyright © 2024 Georgy E. All rights reserved. */

#include "SettingsWatchdog.h"

#include <cstring>

#include "log.h"
#include "settings.h"
#include "SettingsDB.h"


fsm::FiniteStateMachine<SettingsWatchdog::fsm_table> SettingsWatchdog::fsm;


SettingsWatchdog::SettingsWatchdog() { }

void SettingsWatchdog::check()
{
	fsm.proccess();
}

void SettingsWatchdog::state_init::operator ()() const
{
	SettingsDB settingsDB(reinterpret_cast<uint8_t*>(&settings), settings_size());
	SettingsDB::SettingsStatus status = settingsDB.load();
	if (status == SettingsDB::SETTINGS_OK) {
#if SETTINGS_WATCHDOG_BEDUG
		printPretty("state_init: event_loaded\n");
#endif
		SettingsWatchdog::fsm.push_event(SettingsWatchdog::event_loaded{});
	    set_settings_initialized();
		settings_show();
		return;
	}

	settings_reset(&settings);
	status = settingsDB.save();
	if (status == SettingsDB::SETTINGS_OK) {
#if SETTINGS_WATCHDOG_BEDUG
		printPretty("state_init: event_saved\n");
#endif
		SettingsWatchdog::fsm.push_event(SettingsWatchdog::event_saved{});
	    set_settings_initialized();
		settings_show();
	}
}

void SettingsWatchdog::state_idle::operator ()() const
{
	if (is_settings_updated()) {
#if SETTINGS_WATCHDOG_BEDUG
		printPretty("state_idle: event_updated\n");
#endif
		SettingsWatchdog::fsm.push_event(SettingsWatchdog::event_updated{});
	} else if (is_settings_saved()) {
#if SETTINGS_WATCHDOG_BEDUG
		printPretty("state_idle: event_saved\n");
#endif
		SettingsWatchdog::fsm.push_event(SettingsWatchdog::event_saved{});
	}
}

void SettingsWatchdog::state_save::operator ()() const
{
	SettingsDB settingsDB(reinterpret_cast<uint8_t*>(&settings), settings_size());
	SettingsDB::SettingsStatus status = settingsDB.save();
	if (status == SettingsDB::SETTINGS_OK) {
#if SETTINGS_WATCHDOG_BEDUG
		printPretty("state_save: event_saved\n");
#endif
		SettingsWatchdog::fsm.push_event(SettingsWatchdog::event_saved{});
		set_settings_update_status(false);
	}
}

void SettingsWatchdog::state_load::operator ()() const
{
	SettingsDB settingsDB(reinterpret_cast<uint8_t*>(&settings), settings_size());
	SettingsDB::SettingsStatus status = settingsDB.load();
	if (status == SettingsDB::SETTINGS_OK) {
#if SETTINGS_WATCHDOG_BEDUG
		printPretty("state_load: event_loaded\n");
#endif
		SettingsWatchdog::fsm.push_event(SettingsWatchdog::event_loaded{});
		set_settings_save_status(false);
		settings_show();
	}
}

void SettingsWatchdog::action_check::operator ()() const
{
	if (!settings_check(&settings)) {
#if SETTINGS_WATCHDOG_BEDUG
		printPretty("action_check: event_not_valid\n");
#endif
		SettingsWatchdog::fsm.push_event(SettingsWatchdog::event_not_valid{});
	}
}

void SettingsWatchdog::action_reset::operator ()() const
{
#if SETTINGS_WATCHDOG_BEDUG
		printPretty("action_reset: reset\n");
#endif
	settings_reset(&settings);
	set_settings_update_status(true);
}
