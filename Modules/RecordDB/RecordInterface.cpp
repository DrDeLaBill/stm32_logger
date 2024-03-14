/* Copyright © 2024 Georgy E. All rights reserved. */

#include "RecordInterface.h"

#include "utils.h"
#include "bmacro.h"

#include "DeviceInfo.h"


record_t RecordInterface::record;


unsigned RecordInterface::__get_index(unsigned index)
{
	BEDUG_ASSERT(index < __arr_len(record.sens), "Unacceptable sensor index");
	if (index >= __arr_len(record.sens) ||
		index >= DeviceInfo::current_count::get()
	) {
		return __arr_len(record.sens) - 1;
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


void RecordInterface::ID::set(uint32_t value, unsigned index)
{
	record.sens[index].ID = value;
}

uint32_t RecordInterface::ID::get(unsigned index)
{
    return record.sens[index].ID;
}

unsigned RecordInterface::ID::index(unsigned index)
{
	return __get_index(index);
}


void RecordInterface::value::set(uint32_t value, unsigned index)
{
	record.sens[index].value = value;
}

uint32_t RecordInterface::value::get(unsigned index)
{
    return record.sens[index].value;
}

unsigned RecordInterface::value::index(unsigned index)
{
	return __get_index(index);
}
