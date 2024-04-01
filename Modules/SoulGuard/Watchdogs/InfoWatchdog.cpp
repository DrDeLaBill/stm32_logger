/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Watchdogs.h"

#include "soul.h"

#include "RecordDB.h"
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
		RecordDB::updateCache(DeviceInfo::current_id::get());
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

	status = RecordDB::getMaxId(&maxId);
	if (status != RECORD_OK) {
		return false;
	}
	DeviceInfo::max_id::set(maxId);

	if (maxId == 0) {
		return true;
	}
	RecordDB record(maxId - 1);
	status = record.loadNext();
	if (status != RECORD_OK) {
		return false;
	}
	for (unsigned i = 0; i < record_modbus1_sensors_count(&record.clust); i++) {
		modbus_sensor_t* sensor = get_record_modbus1_sensor(&record.clust, i, record.record->mb1_sens[i].ID - 1);
		DeviceInfo::modbus1_value::set(sensor->value, sensor->ID - 1);
	}

	return true;
}

bool InfoWatchdog::loadMinRecord()
{
	uint32_t minId = DeviceInfo::min_id::get();

	RecordStatus status = RecordDB::getMinId(&minId);
	if (status == RECORD_OK) {
		DeviceInfo::min_id::set(minId);
		return true;
	}

	return false;
}

bool InfoWatchdog::loadRecord()
{
	RecordStatus status = RECORD_OK;

	RecordDB record(DeviceInfo::current_id::get());

	status = record.loadNext();
	if (status == RECORD_NO_LOG) {
		return false;
	}

	DeviceInfo::current_id::set(0);

	if (status != RECORD_OK) {
		return false;
	}

	RecordInterface::id::set(record.record->id);
	RecordInterface::time::set(record.record->time);
	for (unsigned i = 0; i < record_modbus1_sensors_count(&record.clust); i++) {
		RecordInterface::MODBUS1_ID::set(record.record->mb1_sens[i].ID, i);
		RecordInterface::MODBUS1_value::set(record.record->mb1_sens[i].value, i);
	}
	for (unsigned i = 0; i < record_1wire_sensors_count(&record.clust); i++) {
		RecordInterface::_1WIRE_ADDR::set(record.record->ow_sens[i].ADDR, i);
		RecordInterface::_1WIRE_value::set(record.record->ow_sens[i].value, i);
	}
	DeviceInfo::current_id::set(record.record->id);
	DeviceInfo::current_mbodbus1_count::set(record.clust.modbus1_count);
	DeviceInfo::current_1wire_count::set(record.clust._1wire_count);

	return true;
}
