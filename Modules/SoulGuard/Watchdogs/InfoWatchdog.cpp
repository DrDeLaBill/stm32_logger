/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Watchdogs.h"

#include "soul.h"

#include "Record.h"
#include "deviceInfo.h"
#include "RecordInterface.h"


void InfoWatchdog::check()
{
	if (!is_status(NEED_LOAD_MAX_RECORD) &&
		!is_status(NEED_LOAD_MIN_RECORD) &&
		!is_status(NEED_LOAD_NEXT_RECORD)
	) {
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
		recordStatus = Record::getMinId(&maxId);
		if (recordStatus == RECORD_OK) {
			DeviceInfo::min_id::set(minId);
			reset_status(NEED_LOAD_MIN_RECORD);
		}
	}

	Record record(DeviceInfo::current_id::get());
	if (is_status(NEED_LOAD_NEXT_RECORD)) {
		DeviceInfo::record_loaded::set(0);
		recordStatus = record.loadNext();
		if (recordStatus == RECORD_OK) {
			RecordInterface::id::set(record.record.id);
			RecordInterface::time::set(record.record.id);
			for (unsigned i = 0; i < record.count(); i++) {
				RecordInterface::ID::set(record[i].ID);
				RecordInterface::value::set(record[i].value);
			}
			DeviceInfo::current_id::set(record.record.id);
			DeviceInfo::current_count::set(record.count());
			DeviceInfo::record_loaded::set(1);
			reset_status(NEED_LOAD_NEXT_RECORD);
		}
	}
}
