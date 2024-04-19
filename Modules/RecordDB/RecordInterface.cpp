/* Copyright © 2024 Georgy E. All rights reserved. */

#include "RecordInterface.h"

#include "utils.h"
#include "bmacro.h"

#include "DeviceInfo.h"


record_t RecordInterface::record;

modbus_sensor_t emptyModbusSensor = {};
_1wire_sensor_t empty1WireSensor = {};


unsigned RecordInterface::__get_modbus1_index(unsigned index)
{
	uint8_t count = DeviceInfo::current_mbodbus1_count::get();
	if (index >= count) {
		return MODBUS_SENS_COUNT - 1;
	}
	return index;
}

unsigned RecordInterface::__get_1wire_index(unsigned index)
{
	uint8_t count = DeviceInfo::current_1wire_count::get();
	if (index >= count) {
		return MODBUS_SENS_COUNT - 1;
	}
	return index;
}


void RecordInterface::id::set(uint32_t value, unsigned)
{
	record.id = value;
}

uint32_t RecordInterface::id::get(unsigned)
{
    return record.id;
}


void RecordInterface::time::set(uint32_t value, unsigned)
{
	record.time = value;
}

uint32_t RecordInterface::time::get(unsigned)
{
    return record.time;
}


void RecordInterface::MODBUS1_ID::set(uint8_t value, unsigned index)
{
	uint8_t count = DeviceInfo::current_mbodbus1_count::get();
	if (index >= count) {
		return;
	}
	get_record_modbus1_sensor(&record, index)->ID = value;
}

uint8_t RecordInterface::MODBUS1_ID::get(unsigned index)
{
	uint8_t count = DeviceInfo::current_mbodbus1_count::get();
	if (index >= count) {
		return emptyModbusSensor.ID;
	}
    return get_record_modbus1_sensor(&record, index)->ID;
}

unsigned RecordInterface::MODBUS1_ID::index(unsigned index)
{
	return __get_modbus1_index(index);
}


void RecordInterface::MODBUS1_value::set(int16_t value, unsigned index)
{
	uint8_t count = DeviceInfo::current_mbodbus1_count::get();
	if (index >= count) {
		return;
	}
	get_record_modbus1_sensor(&record, index)->value = static_cast<int16_t>(value);
}

int16_t RecordInterface::MODBUS1_value::get(unsigned index)
{
	uint8_t count = DeviceInfo::current_mbodbus1_count::get();
	if (index >= count) {
		return emptyModbusSensor.value;
	}
    return get_record_modbus1_sensor(&record, index)->value;
}

unsigned RecordInterface::MODBUS1_value::index(unsigned index)
{
	return __get_modbus1_index(index);
}


void RecordInterface::_1WIRE_ADDR::set(uint64_t value, unsigned index)
{
	uint8_t count = DeviceInfo::current_1wire_count::get();
	if (index >= count) {
		return;
	}
	get_record_1wire_sensor(
		&record,
		DeviceInfo::current_mbodbus1_count::get(),
		index
	)->ADDR = value;
}

uint64_t RecordInterface::_1WIRE_ADDR::get(unsigned index)
{
	uint8_t count = DeviceInfo::current_1wire_count::get();
	if (index >= count) {
		return empty1WireSensor.ADDR;
	}
    return get_record_1wire_sensor(
		&record,
		DeviceInfo::current_mbodbus1_count::get(),
		index
	)->ADDR;
}

unsigned RecordInterface::_1WIRE_ADDR::index(unsigned index)
{
	return __get_1wire_index(index);
}


void RecordInterface::_1WIRE_value::set(int16_t value, unsigned index)
{
	uint8_t count = DeviceInfo::current_1wire_count::get();
	if (index >= count) {
		return;
	}
	get_record_1wire_sensor(
		&record,
		DeviceInfo::current_mbodbus1_count::get(),
		index
	)->value = value;
}

int16_t RecordInterface::_1WIRE_value::get(unsigned index)
{
	uint8_t count = DeviceInfo::current_1wire_count::get();
	if (index >= count) {
		return empty1WireSensor.value;
	}
    return get_record_1wire_sensor(
		&record,
		DeviceInfo::current_mbodbus1_count::get(),
		index
	)->value;
}

unsigned RecordInterface::_1WIRE_value::index(unsigned index)
{
	return __get_1wire_index(index);
}
