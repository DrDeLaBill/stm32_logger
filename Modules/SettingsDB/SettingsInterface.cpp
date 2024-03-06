/* Copyright © 2024 Georgy E. All rights reserved. */

#include "SettingsInterface.h"

#include "log.h"
#include "clock.h"
#include "utils.h"
#include "settings.h"



unsigned __get_index(const unsigned index)
{
	unsigned counter = __arr_len(settings.modbus1_status) - 1;
	for (unsigned i = index; i < __arr_len(settings.modbus1_status); i++) {
		if (settings.modbus1_status[i] != SETTINGS_SENSOR_EMPTY) {
			counter = index;
			break;
		}
	}
	return counter;
}


// TODO: add values check for all parameters
void SettingsInterface::dv_type::set(uint32_t, unsigned)
{
//    settings.dv_type = value; // TODO
}

uint32_t SettingsInterface::dv_type::get(unsigned)
{
    return settings.dv_type;
}

void SettingsInterface::sw_id::set(uint32_t, unsigned)
{
//    settings.sw_id = value;
}

uint32_t SettingsInterface::sw_id::get(unsigned)
{
    return settings.sw_id;
}

void SettingsInterface::fw_id::set(uint32_t, unsigned)
{
//    settings.fw_id = value;
}

uint32_t SettingsInterface::fw_id::get(unsigned)
{
    return settings.fw_id;
}

void SettingsInterface::cf_id::set(uint32_t value, unsigned)
{
    settings.cf_id = value;
}

uint32_t SettingsInterface::cf_id::get(unsigned)
{
    return settings.cf_id;
}

void SettingsInterface::record_period::set(uint32_t value, unsigned)
{
    settings.record_period = value;
}

uint32_t SettingsInterface::record_period::get(unsigned)
{
    return settings.record_period;
}

void SettingsInterface::send_period::set(uint32_t value, unsigned)
{
    settings.send_period = value;
}

uint32_t SettingsInterface::send_period::get(unsigned)
{
    return settings.send_period;
}

void SettingsInterface::record_id::set(uint32_t value, unsigned)
{
	// TODO: check
    settings.record_id = value;
}

uint32_t SettingsInterface::record_id::get(unsigned)
{
    return settings.record_id;
}

void SettingsInterface::modbus1_status::set(uint32_t value, unsigned index)
{
    settings.modbus1_status[index] = value;
}

uint32_t SettingsInterface::modbus1_status::get(unsigned index)
{
    return settings.modbus1_status[index];
}

unsigned SettingsInterface::modbus1_status::index(unsigned index)
{
	return __get_index(index);
}

void SettingsInterface::modbus1_value_reg::set(uint32_t value, unsigned index)
{
    settings.modbus1_value_reg[index] = value;
}

uint32_t SettingsInterface::modbus1_value_reg::get(unsigned index)
{
    return settings.modbus1_value_reg[index];
}

unsigned SettingsInterface::modbus1_value_reg::index(unsigned index)
{
	return __get_index(index);
}

void SettingsInterface::modbus1_id_reg::set(uint32_t value, unsigned index)
{
    settings.modbus1_id_reg[index] = value;
}

uint32_t SettingsInterface::modbus1_id_reg::get(unsigned index)
{
    return settings.modbus1_id_reg[index];
}

unsigned SettingsInterface::modbus1_id_reg::index(unsigned index)
{
	return __get_index(index);
}

void SettingsInterface::time::set(uint32_t value, unsigned)
{
	RTC_TimeTypeDef dumpTime = {};
	RTC_DateTypeDef dumpDate = {};
	clock_seconds_to_datetime(value, &dumpDate, &dumpTime);
	clock_save_date(&dumpDate);
	clock_save_time(&dumpTime);
}

uint32_t SettingsInterface::time::get(unsigned)
{
	return clock_get_timestamp();
}
