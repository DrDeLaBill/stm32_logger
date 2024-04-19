/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Watchdogs.h"

#include "soul.h"
#include "sensor.h"

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
		DeviceInfo::record_loaded::get() &&
		!SettingsInterface::need_mb1_id_update::get()
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
		RecordStatus status = loadRecord();
		if (status != RECORD_ERROR) {
			DeviceInfo::record_loaded::set(1);
		} else {
			DeviceInfo::record_loaded::set(0);
		}
	}

	if (SettingsInterface::need_mb1_id_update::get()) {
		if (is_status(NEED_ENABLE_MODBUS1)) {
			return;
		}
		set_status(NEED_ENABLE_MODBUS1);
		sensors_enable();
		HAL_Delay(100); // TODO: remove?
		sensor_send_new_id(
			SettingsInterface::mb1_last_id::get(),
			SettingsInterface::mb1_new_id::get()
		);
		SettingsInterface::need_mb1_id_update::set(0);
		HAL_Delay(100); // TODO: remove?
		sensors_disable();
		reset_status(NEED_ENABLE_MODBUS1);
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
	RecordDB record(maxId);
	status = record.load(false);
	if (status != RECORD_OK) {
		return false;
	}
	for (unsigned i = 0; i < record_modbus1_sensors_count(&record.clust); i++) {
		modbus_sensor_t* sensor = get_record_modbus1_sensor(&record.record, i);
		DeviceInfo::modbus1_last_value::set(sensor->value, sensor->ID - 1);
	}
	for (unsigned i = 0; i < record_1wire_sensors_count(&record.clust); i++) {
		_1wire_sensor_t* sensor = get_record_1wire_sensor(&record.record, record_modbus1_sensors_count(&record.clust), i);
		DeviceInfo::_1wire_last_value::set(sensor->value, i);
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

RecordStatus InfoWatchdog::loadRecord()
{
	RecordStatus status = RECORD_OK;

	RecordDB record(DeviceInfo::current_id::get());

	status = record.loadNext();
	if (status == RECORD_NO_LOG) {
		DeviceInfo::current_id::set(record.record.id + 1);
		return status;
	}

	DeviceInfo::current_id::set(0);

	if (status != RECORD_OK) {
		return status;
	}

	DeviceInfo::current_id::set(record.record.id);
	DeviceInfo::current_mbodbus1_count::set(record.clust.modbus1_count);
	DeviceInfo::current_1wire_count::set(record.clust._1wire_count);
	RecordInterface::id::set(record.record.id);
	RecordInterface::time::set(record.record.time);
	for (unsigned i = 0; i < record_modbus1_sensors_count(&record.clust); i++) {
		modbus_sensor_t* sensor = get_record_modbus1_sensor(&record.record, i);
		RecordInterface::MODBUS1_ID::set(sensor->ID, i);
		RecordInterface::MODBUS1_value::set(sensor->value, i);
	}
	for (unsigned i = 0; i < record_1wire_sensors_count(&record.clust); i++) {
		_1wire_sensor_t* sensor = get_record_1wire_sensor(&record.record, record_modbus1_sensors_count(&record.clust), i);
		RecordInterface::_1WIRE_ADDR::set(sensor->ADDR, i);
		RecordInterface::_1WIRE_value::set(sensor->value, i);
	}

	return status;
}
