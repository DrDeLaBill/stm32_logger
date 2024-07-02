/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Measure.h"

#include <limits>
#include <cstring>

#include "glog.h"
#include "main.h"
#include "soul.h"
#include "gutils.h"
#include "sensor.h"
#include "bmacro.h"
#include "sensor.h"
#include "hal_defs.h"
#include "settings.h"
#include "onewire_driver.h"
#include "modbus_rtu_master.h"

#include "Record.h"


#define MEASURE_VALUE_DELAY_MS      ((uint32_t)500)
#define MEASURE_WAIT_POWER_DELAY_MS ((uint32_t)10000)
#define ONEWIRE_CONVERSION_DELAY_MS ((uint32_t)20000)


uint8_t Measure::sensAddress = 0;
uint8_t Measure::sensIdx     = 0;
uint8_t Measure::errorsCount = 0;

fsm::FiniteStateMachine<Measure::fsm_table> Measure::fsm;
utl::Timer Measure::timer(GENERAL_TIMEOUT_MS);
RecordDB Measure::record(0);


Measure::Measure()
{
	sensors_init(&response_packet_handler);
}

void Measure::process()
{
	fsm.proccess();
}

void Measure::_idle_s::operator ()()
{
	if (!is_status(SETTINGS_INITIALIZED)) {
		return;
	}

	if (!is_status(NEED_MEASURE)) {
		return;
	}

	if (is_status(NEED_ENABLE_SENSORS)) {
		return;
	}

	record = RecordDB(0);
	record_cluster_create(&record.clust);
	set_status(NEED_ENABLE_SENSORS);
	fsm.push_event(success_e{});
}

void Measure::_wait_start_s::operator ()()
{
	if (!timer.wait()) {
		fsm.push_event(timeout_e{});
	}
}

void Measure::_mb1_request_s::operator ()()
{
	if (Measure::sensAddress >= __arr_len(settings.modbus1_status) ||
		!sensors_count()
	) {
		fsm.push_event(end_e{});
	} else if (settings.modbus1_status[Measure::sensAddress] != SETTINGS_SENSOR_EMPTY) {
		sensor_request_value(sensAddress);
		fsm.push_event(success_e{});
	} else {
		fsm.push_event(iterate_e{});
	}
}

void Measure::_mb1_wait_s::operator ()()
{
	if (Measure::errorsCount >= ERRORS_MAX) {
		// TODO: send sensor error to stng_info
		fsm.push_event(iterate_e{});
		return;
	}

	if (!timer.wait()) {
		modbus_sensor_t measure{};
		measure.ID    = sensAddress + 1;
		measure.value = std::numeric_limits<int16_t>::max();
		set_record_modbus1_measure(
			&(record.record),
			sensIdx,
			&measure
		);
		sensor_timeout();
		fsm.push_event(timeout_e{});
	}
}

void Measure::__1w_delay_s::operator ()()
{
	if (!timer.wait()) {
		fsm.push_event(timeout_e{});
	}

	if (onewire_driver_ready()) {
		fsm.push_event(success_e{});
	}
}

void Measure::__1w_request_s::operator ()()
{
	if (sensAddress >= __arr_len(settings._1wire_address)) {
		fsm.push_event(end_e{});
	} else if (settings._1wire_address[sensAddress]) {
		onewire_driver_start_read(settings._1wire_address[sensAddress]);
		fsm.push_event(success_e{});
	} else {
		fsm.push_event(iterate_e{});
	}
}

void Measure::__1w_wait_s::operator ()()
{
	if (!timer.wait()) {
		_1wire_sensor_t measure{};
		measure.ADDR  = settings._1wire_address[sensAddress];
		measure.value = std::numeric_limits<int16_t>::max();
		set_record_1wire_measure(
			&(record.record),
			record_modbus1_sensors_count(&record.clust),
			sensIdx,
			&measure
		);
		onewire_driver_clear();
		fsm.push_event(timeout_e{});
	}

	bool needIterate = false;
	if (errorsCount >= ERRORS_MAX) {
		// TODO: send sensor error to stng_info
		needIterate = true;
	} else if (onewire_driver_has_response()) {
		_1wire_sensor_t measure{};
		measure.ADDR  = settings._1wire_address[sensAddress];
		measure.value = get_onewire_driver_value();
		set_record_1wire_measure(
			&(record.record),
			record_modbus1_sensors_count(&record.clust),
			sensIdx,
			&measure
		);
		needIterate = true;
	}

	if (needIterate) {
	    fsm.push_event(iterate_e{});
	}
}

void Measure::_save_s::operator ()()
{
	if (Measure::errorsCount >= ERRORS_MAX) {
		// TODO: send save error to errors list
		fsm.push_event(error_e{});
		return;
	}

	if (!timer.wait()) {
		fsm.push_event(timeout_e{});
	}
}

void Measure::wait_start_a::operator ()()
{
	fsm.clear_events();
	timer.changeDelay(MEASURE_WAIT_POWER_DELAY_MS);
	timer.start();
}

void Measure::wait_response_a::operator ()()
{
	fsm.clear_events();
	timer.changeDelay(MEASURE_VALUE_DELAY_MS);
	timer.start();
}

void Measure::save_start_a::operator ()()
{
	set_status(LOADING);
	fsm.clear_events();
	if (record.save() == RECORD_OK) {
		fsm.push_event(success_e{});
	} else {
		fsm.push_event(timeout_e{});
	}

	Measure::errorsCount = 0;
	timer.changeDelay(GENERAL_TIMEOUT_MS);
	timer.start();
	reset_status(LOADING);
}

void Measure::idle_start_a::operator ()()
{
	fsm.clear_events();

	reset_status(NEED_ENABLE_SENSORS);
	reset_status(NEED_MEASURE);
}

void Measure::init_mb1_sens_a::operator ()()
{
	fsm.clear_events();
	errorsCount = 0;
	sensAddress = 0;
	sensIdx     = 0;

	while (sensAddress < __arr_len(settings.modbus1_status)) {
		if (settings.modbus1_status[sensAddress] != SETTINGS_SENSOR_EMPTY) {
			break;
		}
		sensAddress++;
	}
	if (sensAddress >= __arr_len(settings.modbus1_status) ||
		!sensors_count()
	) {
		fsm.push_event(end_e{});
	}
}

void Measure::iterate_mb1_sens_a::operator ()()
{
	fsm.clear_events();
	errorsCount = 0;
	while (sensAddress < __arr_len(settings.modbus1_status)) {
		if (settings.modbus1_status[++sensAddress] != SETTINGS_SENSOR_EMPTY) {
			sensIdx++;
			break;
		}
	}
	if (sensAddress >= __arr_len(settings.modbus1_status) ||
		!sensors_count()
	) {
		fsm.push_event(end_e{});
	}
}

void Measure::init_1w_sens_a::operator ()()
{
	fsm.clear_events();
	errorsCount = 0;
	sensAddress = 0;
	sensIdx     = 0;

	while (sensAddress < __arr_len(settings._1wire_address)) {
		if (settings._1wire_address[sensAddress]) {
			break;
		}
		sensAddress++;
	}
	if (sensAddress >= __arr_len(settings._1wire_address)) {
		fsm.push_event(end_e{});
	}
}

void Measure::iterate_1w_sens_a::operator ()()
{
	fsm.clear_events();
	errorsCount = 0;
	while (sensAddress < __arr_len(settings._1wire_address)) {
		if (settings._1wire_address[++sensAddress]) {
			sensIdx++;
			break;
		}
	}
	if (sensAddress >= __arr_len(settings._1wire_address)) {
		fsm.push_event(end_e{});
	}
}

void Measure::count_error_a::operator ()()
{
	fsm.clear_events();
	Measure::errorsCount++; // TODO: add new save try after error
}

void Measure::register_error_a::operator ()()
{
	// TODO: send measure error to errors list
	fsm.clear_events();

	reset_status(NEED_ENABLE_SENSORS);
	reset_status(NEED_MEASURE);
}

void Measure::start_delay_a::operator ()()
{
	onewire_driver_start_convert();
	timer.changeDelay(ONEWIRE_CONVERSION_DELAY_MS);
	timer.start();
}

void Measure::response_packet_handler(modbus_response_t* packet)
{
#if SENSOR_BEDUG
	BEDUG_ASSERT(packet, "Incorrect MODBUS response data");
#endif
	if (!packet) {
		return;
	}

    if (packet->status != MODBUS_NO_ERROR) {
#if SENSOR_BEDUG
        printTagLog(TAG, "ERROR: %02x", packet->status);
#endif
        fsm.push_event(error_e{});
        return;
    }

#if SENSOR_BEDUG
    printTagLog(TAG, "Response has been received successfully");
    printPretty("Status: %u\n", packet->status);
    printPretty("Slave ID: %u\n", packet->slave_id);
    printPretty("Command: %u\n", packet->command);
    printPretty("Data: ");
    for (unsigned  i = 0; i < __arr_len(packet->response); i++) {
    	gprint("%04X ", packet->response[i]);
    }
    gprint("\n");
#endif

	modbus_sensor_t measure{};
	measure.ID    = sensAddress + 1;
	measure.value = static_cast<int16_t>(packet->response[0]);
	set_record_modbus1_measure(
		&(record.record),
		sensIdx,
		&measure
	);
    fsm.push_event(iterate_e{});
}
