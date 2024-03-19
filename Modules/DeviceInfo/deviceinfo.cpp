#include "deviceinfo.h"

#include "soul.h"
#include "clock.h"


DeviceInfo::info_t DeviceInfo::info = {};


void DeviceInfo::time::set(uint32_t value, unsigned)
{
	RTC_TimeTypeDef dumpTime = {};
	RTC_DateTypeDef dumpDate = {};
	clock_seconds_to_datetime(value, &dumpDate, &dumpTime);
	clock_save_date(&dumpDate);
	clock_save_time(&dumpTime);
}

uint32_t DeviceInfo::time::get(unsigned)
{
	return clock_get_timestamp();
}


void DeviceInfo::min_id::set(uint32_t value, unsigned)
{
	info.min_id = value;
}

uint32_t DeviceInfo::min_id::get(unsigned)
{
    return info.min_id;
}


void DeviceInfo::max_id::set(uint32_t value, unsigned)
{
	info.max_id = value;
}

uint32_t DeviceInfo::max_id::get(unsigned)
{
    return info.max_id;
}


void DeviceInfo::current_id::set(uint32_t value, unsigned)
{
    info.current_id = value;
}

uint32_t DeviceInfo::current_id::get(unsigned)
{
    return info.current_id;
}


void DeviceInfo::current_count::set(uint32_t value, unsigned)
{
    info.current_count = value;
}

uint32_t DeviceInfo::current_count::get(unsigned)
{
    return info.current_count;
}


void DeviceInfo::record_loaded::set(uint32_t value, unsigned)
{
    info.record_loaded = value;
}

uint32_t DeviceInfo::record_loaded::get(unsigned)
{
    return info.record_loaded;
}
