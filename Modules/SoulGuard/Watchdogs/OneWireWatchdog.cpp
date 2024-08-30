/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Watchdogs.h"

#include "app.h"
#include "glog.h"
#include "soul.h"
#include "settings.h"
#include "onewire_driver.h"


fsm::FiniteStateMachine<OneWireWatcher::fsm_table> OneWireWatcher::fsm;
uint8_t OneWireWatcher::index = 0;

utl::Timer OneWireWatcher::timeoutTimer(TIMEOUT_MS);
utl::Timer OneWireWatcher::delayTimer(DELAY_MS);


void OneWireWatcher::check()
{
	fsm.proccess();
}

void OneWireWatcher::_idle_s::operator()()
{
	if (!onewire_driver_ready()) {
		return;
	}
	if (is_status(NEED_1WIRE)) {
		memset(settings._1wire_address, 0, sizeof(settings._1wire_address));
		index = 0;
		timeoutTimer.start();
		fsm.push_event(start_e{});
	}
}

void OneWireWatcher::_start_s::operator ()()
{
	if (!timeoutTimer.wait()) {
		fsm.push_event(timeout_e{});
	}

	if (HAL_GPIO_ReadPin(POWER_L2_GPIO_Port, POWER_L2_Pin)) {
		delayTimer.start();
		fsm.push_event(done_e{});
	}
}

void OneWireWatcher::_registrate_s::operator()()
{
	if (index >= __arr_len(settings._1wire_address) ||
		!app_info.need_registrate_1wire
	) {
		fsm.push_event(done_e{});
	}
	if (!timeoutTimer.wait()) {
		fsm.push_event(timeout_e{});
	}
	if (!delayTimer.wait()) {
		onewire_driver_clear();
		fsm.push_event(start_e{});
	}
	if (!onewire_driver_has_response()) {
		return;
	}

	fsm.push_event(next_e{});

	uint64_t address = get_onewire_driver_address();

	if (address && settings_1wire_sensor_exists(address)) {
		return;
	}
	timeoutTimer.start();

	settings._1wire_address[index] = address;
	index++;

	delayTimer.start();

	printTagLog(
		TAG,
		"Address[%03u] 0x%08X%08X added",
		index,
		(unsigned)(get_onewire_driver_address() >> (sizeof(unsigned) * BITS_IN_BYTE)),
		(unsigned)(get_onewire_driver_address())
	);
}

void OneWireWatcher::_end_s::operator()()
{
	set_status(NEED_SAVE_SETTINGS);
	onewire_driver_clear();

	fsm.push_event(done_e{});
}

void OneWireWatcher::timeout_a::operator ()()
{
	app_info.need_registrate_1wire = 0;

	set_status(NEED_LOAD_SETTINGS);

	onewire_driver_clear();
}

void OneWireWatcher::done_a::operator ()()
{
	app_info.need_registrate_1wire = 0;
}

void OneWireWatcher::enable_a::operator ()() {}

void OneWireWatcher::start_search_a::operator ()()
{
	onewire_driver_start_search();
	delayTimer.start();
}

void OneWireWatcher::next_search_a::operator ()()
{
	onewire_driver_next_search();
}

