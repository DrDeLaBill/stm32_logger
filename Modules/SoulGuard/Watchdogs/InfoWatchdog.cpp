/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Watchdogs.h"

#include "soul.h"

#include "Record.h"
#include "deviceInfo.h"
#include "CodeStopwatch.h"
#include "RecordInterface.h"


void InfoWatchdog::check()
{
	utl::CodeStopwatch stopwatch("INFw", WATCHDOG_TIMEOUT_MS);

	if (!is_status(NEED_LOAD_MAX_RECORD) &&
		!is_status(NEED_LOAD_MIN_RECORD) &&
		!is_status(NEED_LOAD_NEXT_RECORD)
	) {
#if RECORD_ENABLE_CACHE
		Record::updateCache(DeviceInfo::current_id::get());
#endif
		return;
	}

	RecordStatus recordStatus = RECORD_OK;

	uint32_t maxId = DeviceInfo::max_id::get();
	if (is_status(NEED_LOAD_MAX_RECORD)) {
		recordStatus = Record::getMaxId(&maxId);
		if (recordStatus == RECORD_OK) {
			DeviceInfo::max_id::set(maxId);
			reset_status(NEED_LOAD_MAX_RECORD);
		}
	}

	uint32_t minId = DeviceInfo::min_id::get();
	if (is_status(NEED_LOAD_MIN_RECORD)) {
		recordStatus = Record::getMinId(&minId);
		if (recordStatus == RECORD_OK) {
			DeviceInfo::min_id::set(minId);
			reset_status(NEED_LOAD_MIN_RECORD);
		}
	}

	Record record(DeviceInfo::current_id::get());
	if (is_status(NEED_LOAD_NEXT_RECORD)) {
		DeviceInfo::record_loaded::set(0);
		recordStatus = record.loadNext();
		if (recordStatus == RECORD_NO_LOG) {
			DeviceInfo::current_id::set(0);
			reset_status(NEED_LOAD_NEXT_RECORD);
		}
		if (recordStatus == RECORD_OK) {
			RecordInterface::id::set(record.record.id);
			RecordInterface::time::set(record.record.time);
			for (unsigned i = 0; i < record.count(); i++) {
				RecordInterface::ID::set(record[i].ID, i);
				RecordInterface::value::set(record[i].value, i);
			}
			DeviceInfo::current_id::set(record.record.id);
			DeviceInfo::current_count::set(record.count());
			DeviceInfo::record_loaded::set(1);
			reset_status(NEED_LOAD_NEXT_RECORD);
		}
	}
}
