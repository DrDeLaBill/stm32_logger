#include "deviceinfo.h"

#include "soul.h"
#include "clock.h"


uint16_t DeviceInfo::m_modbus1_value[MODBUS_SENS_COUNT] = {};
DeviceInfo::info_t DeviceInfo::info = {};


void DeviceInfo::time::set(uint64_t value, unsigned)
{
	RTC_TimeTypeDef dumpTime = {};
	RTC_DateTypeDef dumpDate = {};
	clock_seconds_to_datetime(value, &dumpDate, &dumpTime);
	clock_save_date(&dumpDate);
	clock_save_time(&dumpTime);
}

uint64_t DeviceInfo::time::get(unsigned)
{
	return clock_get_timestamp();
}


void DeviceInfo::min_id::set(uint64_t value, unsigned)
{
	info.min_id = value;
}

uint64_t DeviceInfo::min_id::get(unsigned)
{
    return info.min_id;
}


void DeviceInfo::max_id::set(uint64_t value, unsigned)
{
	info.max_id = value;
}

uint64_t DeviceInfo::max_id::get(unsigned)
{
    return info.max_id;
}


void DeviceInfo::current_id::set(uint64_t value, unsigned)
{
    info.current_id = value;
}

uint64_t DeviceInfo::current_id::get(unsigned)
{
    return info.current_id;
}


void DeviceInfo::current_mbodbus1_count::set(uint64_t value, unsigned)
{
    info.current_mbodbus1_count = value;
}

uint64_t DeviceInfo::current_mbodbus1_count::get(unsigned)
{
    return info.current_mbodbus1_count;
}


void DeviceInfo::current_1wire_count::set(uint64_t value, unsigned)
{
    info.current_1wire_count = value;
}

uint64_t DeviceInfo::current_1wire_count::get(unsigned)
{
    return info.current_1wire_count;
}


void DeviceInfo::record_loaded::set(uint64_t value, unsigned)
{
    info.record_loaded = value;
}

uint64_t DeviceInfo::record_loaded::get(unsigned)
{
    return info.record_loaded;
}


void DeviceInfo::need_registrate_1wire::set(uint64_t value, unsigned)
{
    if (value) {
    	set_status(NEED_REGISTRATE_1WIRE);
    } else {
    	reset_status(NEED_REGISTRATE_1WIRE);
    }
}

uint64_t DeviceInfo::need_registrate_1wire::get(unsigned)
{
    return is_status(NEED_REGISTRATE_1WIRE);
}


void DeviceInfo::modbus1_last_value::set(uint64_t value, unsigned index)
{
	m_modbus1_value[index] = value;
}

uint64_t DeviceInfo::modbus1_last_value::get(unsigned index)
{
    return m_modbus1_value[index];
}

unsigned DeviceInfo::modbus1_last_value::index(unsigned index)
{
	return settings_get_modbus1_index(index);
}
