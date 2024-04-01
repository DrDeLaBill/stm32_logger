/* Copyright © 2024 Georgy E. All rights reserved. */

#include "RecordInterface.h"

#include "utils.h"
#include "bmacro.h"

#include "DeviceInfo.h"


record_t RecordInterface::record;


unsigned RecordInterface::__get_index(unsigned index)
{
	BEDUG_ASSERT(index < __arr_len(record.mb1_sens), "Unacceptable sensor index");
	if (index >= __arr_len(record.mb1_sens)) {
		return __arr_len(record.mb1_sens) - 1;
	}
	return index;
}


void RecordInterface::id::set(uint64_t value, unsigned)
{
	record.id = value;
}

uint64_t RecordInterface::id::get(unsigned)
{
    return record.id;
}


void RecordInterface::time::set(uint64_t value, unsigned)
{
	record.time = value;
}

uint64_t RecordInterface::time::get(unsigned)
{
    return record.time;
}


void RecordInterface::MODBUS1_ID::set(uint64_t value, unsigned index)
{
	record.mb1_sens[index].ID = value;
}

uint64_t RecordInterface::MODBUS1_ID::get(unsigned index)
{
    return record.mb1_sens[index].ID;
}

unsigned RecordInterface::MODBUS1_ID::index(unsigned index)
{
	return __get_index(index);
}


void RecordInterface::MODBUS1_value::set(uint64_t value, unsigned index)
{
	record.mb1_sens[index].value = value;
}

uint64_t RecordInterface::MODBUS1_value::get(unsigned index)
{
    return record.mb1_sens[index].value;
}

unsigned RecordInterface::MODBUS1_value::index(unsigned index)
{
	return __get_index(index);
}


void RecordInterface::_1WIRE_ADDR::set(uint64_t value, unsigned index)
{
	record.ow_sens[index].ADDR = value;
}

uint64_t RecordInterface::_1WIRE_ADDR::get(unsigned index)
{
    return record.ow_sens[index].ADDR;
}

unsigned RecordInterface::_1WIRE_ADDR::index(unsigned index)
{
	return __get_index(index);
}


void RecordInterface::_1WIRE_value::set(uint64_t value, unsigned index)
{
	record.ow_sens[index].value = value;
}

uint64_t RecordInterface::_1WIRE_value::get(unsigned index)
{
    return record.ow_sens[index].value;
}

unsigned RecordInterface::_1WIRE_value::index(unsigned index)
{
	return __get_index(index);
}
