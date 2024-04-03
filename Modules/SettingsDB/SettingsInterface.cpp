/* Copyright © 2024 Georgy E. All rights reserved. */

#include "SettingsInterface.h"

#include "log.h"
#include "utils.h"
#include "settings.h"


// TODO: add values check for all parameters
void SettingsInterface::dv_type::set(uint64_t, unsigned)
{
//    settings.dv_type = value; // TODO
}

uint64_t SettingsInterface::dv_type::get(unsigned)
{
    return settings.dv_type;
}

void SettingsInterface::sw_id::set(uint64_t, unsigned)
{
//    settings.sw_id = value;
}

uint64_t SettingsInterface::sw_id::get(unsigned)
{
    return settings.sw_id;
}

void SettingsInterface::fw_id::set(uint64_t, unsigned)
{
//    settings.fw_id = value;
}

uint64_t SettingsInterface::fw_id::get(unsigned)
{
    return settings.fw_id;
}

void SettingsInterface::cf_id::set(uint64_t value, unsigned)
{
    settings.cf_id = value;
}

uint64_t SettingsInterface::cf_id::get(unsigned)
{
    return settings.cf_id;
}

void SettingsInterface::record_period::set(uint64_t value, unsigned)
{
    settings.record_period = value;
}

uint64_t SettingsInterface::record_period::get(unsigned)
{
    return settings.record_period;
}

void SettingsInterface::send_period::set(uint64_t value, unsigned)
{
    settings.send_period = value;
}

uint64_t SettingsInterface::send_period::get(unsigned)
{
    return settings.send_period;
}

void SettingsInterface::record_id::set(uint64_t value, unsigned)
{
	// TODO: check
    settings.record_id = value;
}

uint64_t SettingsInterface::record_id::get(unsigned)
{
    return settings.record_id;
}

void SettingsInterface::modbus1_status::set(uint64_t value, unsigned index)
{
    settings.modbus1_status[index] = value;
}

uint64_t SettingsInterface::modbus1_status::get(unsigned index)
{
    return settings.modbus1_status[index];
}

unsigned SettingsInterface::modbus1_status::index(unsigned index)
{
	return settings_get_modbus1_index(index);
}

void SettingsInterface::modbus1_value_reg::set(uint64_t value, unsigned index)
{
    settings.modbus1_value_reg[index] = value;
}

uint64_t SettingsInterface::modbus1_value_reg::get(unsigned index)
{
    return settings.modbus1_value_reg[index];
}

unsigned SettingsInterface::modbus1_value_reg::index(unsigned index)
{
	return settings_get_modbus1_index(index);
}

void SettingsInterface::modbus1_id_reg::set(uint64_t value, unsigned index)
{
    settings.modbus1_id_reg[index] = value;
}

uint64_t SettingsInterface::modbus1_id_reg::get(unsigned index)
{
    return settings.modbus1_id_reg[index];
}

unsigned SettingsInterface::modbus1_id_reg::index(unsigned index)
{
	return settings_get_modbus1_index(index);
}

void SettingsInterface::_1wire_address::set(uint64_t value, unsigned index)
{
	settings._1wire_address[index] = value;
}

uint64_t SettingsInterface::_1wire_address::get(unsigned index)
{
	return settings._1wire_address[index];
}

unsigned SettingsInterface::_1wire_address::index(unsigned index)
{
	if (index >= __arr_len(settings._1wire_address)) {
		return __arr_len(settings._1wire_address) - 1;
	}
	return index;
}
