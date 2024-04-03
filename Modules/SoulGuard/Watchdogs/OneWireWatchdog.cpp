/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Watchdogs.h"

#include "log.h"
#include "soul.h"
#include "settings.h"
#include "onewire_driver.h"

#include "deviceInfo.h"


fsm::FiniteStateMachine<OneWireWatcher::fsm_table> OneWireWatcher::fsm;
uint8_t OneWireWatcher::index = 0;


void OneWireWatcher::check()
{
	fsm.proccess();
}

void OneWireWatcher::_idle_s::operator()()
{
	if (is_status(NEED_REGISTRATE_1WIRE)) {
		memset(settings._1wire_address, 0, sizeof(settings._1wire_address));
		fsm.push_event(start_e{});
	}
}

void OneWireWatcher::_start_s::operator ()()
{
	if (HAL_GPIO_ReadPin(POWER_L2_GPIO_Port, POWER_L2_Pin)) {
		fsm.push_event(done_e{});
	}
}

void OneWireWatcher::_registrate_s::operator()()
{
	if (index >= __arr_len(settings._1wire_address)) {
		fsm.push_event(done_e{});
	}
	if (!is_status(NEED_REGISTRATE_1WIRE)) {
		fsm.push_event(done_e{});
	}
	if (!onewire_driver_ready()) {
		return;
	}
	fsm.push_event(next_e{});
	uint64_t address = get_onewire_driver_address();
	if (settings_1wire_sensor_exists(address)) {
		return;
	}
	printTagLog(
		TAG,
		"Address[%03u] 0x%08X%08X added",
		index,
		(unsigned)(get_onewire_driver_address() >> (sizeof(unsigned) * BITS_IN_BYTE)),
		(unsigned)(get_onewire_driver_address())
	);
	settings._1wire_address[index] = address;
	index++;
}

void OneWireWatcher::_end_s::operator()()
{
	set_status(NEED_SAVE_SETTINGS);
	onewire_driver_clear();

	fsm.push_event(done_e{});
}

void OneWireWatcher::done_a::operator ()()
{
	reset_status(NEED_ENABLE_1WIRE);
	DeviceInfo::need_registrate_1wire::set(0);
}

void OneWireWatcher::enable_a::operator ()()
{
	set_status(NEED_ENABLE_1WIRE);
}

void OneWireWatcher::start_search_a::operator ()()
{
	onewire_driver_start_search();
	index = 0;
}

void OneWireWatcher::next_search_a::operator ()()
{
	onewire_driver_next_search();
}

