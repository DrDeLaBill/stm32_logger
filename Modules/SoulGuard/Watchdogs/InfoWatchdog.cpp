/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Watchdogs.h"

#include "soul.h"

#include "Record.h"
#include "deviceInfo.h"
#include "CodeStopwatch.h"
#include "RecordInterface.h"
#include "SettingsInterface.h"


void InfoWatchdog::check()
{
	utl::CodeStopwatch stopwatch("INFw", WATCHDOG_TIMEOUT_MS);

	if (!is_status(NEED_LOAD_MAX_RECORD) &&
		!is_status(NEED_LOAD_MIN_RECORD) &&
		DeviceInfo::record_loaded::get()
	) {
#if RECORD_ENABLE_CACHE
		if (DeviceInfo::current_id::get() >= DeviceInfo::max_id::get()) {
			DeviceInfo::current_id::set(0);
		}
		Record::updateCache(DeviceInfo::current_id::get());
#endif
		return;
	}

	if (is_status(NEED_LOAD_MAX_RECORD)) {
		if (loadMaxRecord()) {
			reset_status(NEED_LOAD_MAX_RECORD);
		} else {
			set_status(NEED_LOAD_MAX_RECORD);
		}
	}

	if (is_status(NEED_LOAD_MIN_RECORD)) {
		if (loadMinRecord()) {
			reset_status(NEED_LOAD_MIN_RECORD);
		} else {
			set_status(NEED_LOAD_MIN_RECORD);
		}
	}

	if (!DeviceInfo::record_loaded::get()) {
		if (loadRecord()) {
			DeviceInfo::record_loaded::set(1);
		} else {
			DeviceInfo::record_loaded::set(0);
		}
	}
}

bool InfoWatchdog::loadMaxRecord()
{
	uint32_t maxId = DeviceInfo::max_id::get();

	RecordStatus status = RECORD_OK;

	status = Record::getMaxId(&maxId);
	if (status != RECORD_OK) {
		return false;
	}
	DeviceInfo::max_id::set(maxId);

	if (maxId == 0) {
		return true;
	}
	Record record(maxId - 1);
	status = record.loadNext();
	if (status != RECORD_OK) {
		return false;
	}
	for (unsigned i = 0; i < record.count(); i++) {
		DeviceInfo::modbus1_value::set(record[i].value, record[i].ID - 1);
	}

	return true;
}

bool InfoWatchdog::loadMinRecord()
{
	uint32_t minId = DeviceInfo::min_id::get();

	RecordStatus status = Record::getMinId(&minId);
	if (status == RECORD_OK) {
		DeviceInfo::min_id::set(minId);
		return true;
	}

	return false;
}

bool InfoWatchdog::loadRecord()
{
	RecordStatus status = RECORD_OK;

	Record record(DeviceInfo::current_id::get());

	status = record.loadNext();
	if (status == RECORD_NO_LOG) {
		return false;
	}

	DeviceInfo::current_id::set(0);

	if (status != RECORD_OK) {
		return false;
	}

	RecordInterface::id::set(record.record.id);
	RecordInterface::time::set(record.record.time);
	for (unsigned i = 0; i < record.count(); i++) {
		RecordInterface::ID::set(record[i].ID, i);
		RecordInterface::value::set(record[i].value, i);
	}
	DeviceInfo::current_id::set(record.record.id);
	DeviceInfo::current_count::set(record.count());

	return true;
}
