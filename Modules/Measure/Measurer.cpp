/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Measurer.h"

#include <cstring>

#include "log.h"
#include "main.h"
#include "utils.h"
#include "sensor.h"
#include "bmacro.h"
#include "sensor.h"
#include "hal_defs.h"
#include "settings.h"
#include "modbus_rtu_master.h"

#include "Record.h"


uint32_t Measurer::delay      = 0;
uint32_t Measurer::sensIndex  = 0;
uint8_t Measurer::errorsCount = 0;

fsm::FiniteStateMachine<Measurer::fsm_table> Measurer::fsm;
utl::Timer Measurer::timer(GENERAL_TIMEOUT_MS);
Record Measurer::record(0);


Measurer::Measurer(uint32_t delay)
{
#if MEASURER_BEDUG
	BEDUG_ASSERT(delay, "Measure delay should be larger than 0");
#endif
	sensors_init(&response_packet_handler);
	this->delay = delay;
}

void Measurer::process()
{
	if (!this->delay) {
		return;
	}
	fsm.proccess();
}

uint32_t Measurer::getDelay()
{
	return delay;
}

void Measurer::setDelay(uint32_t delay)
{
	this->delay = delay;
}

void Measurer::_init_s::operator ()()
{
	if (is_settings_initialized()) {
		fsm.push_event(Measurer::ready_e{});
	}
}

void Measurer::_idle_s::operator ()()
{
	if (!timer.wait()) {
		Measurer::record = Record(0, sensors_count());
#if MEASURER_BEDUG
		printTagLog(TAG, "state-_idle_s: event-timeout_e");
#endif
		fsm.push_event(Measurer::timeout_e{});
	}
}

void Measurer::_delay_s::operator ()()
{
	if (!timer.wait()) {
#if MEASURER_BEDUG
		printTagLog(TAG, "state-_delay_s: event-timeout_e");
#endif
		fsm.push_event(Measurer::timeout_e{});
	}
}

void Measurer::_request_s::operator ()()
{
	if (settings.modbus1_status[Measurer::sensIndex] != SETTINGS_SENSOR_EMPTY) {
		sensor_request_value(Measurer::sensIndex);
#if MEASURER_BEDUG
		printTagLog(TAG, "state-_request_s: event-sended_e");
#endif
		fsm.push_event(Measurer::sended_e{});
	} else {
#if MEASURER_BEDUG
		printTagLog(TAG, "state-_request_s: event-skip_e");
#endif
		fsm.push_event(Measurer::skip_e{});
	}
}

void Measurer::_wait_s::operator ()()
{
	if (Measurer::errorsCount >= ERRORS_MAX) {
		// TODO: send sensor error to stng_info
#if MEASURER_BEDUG
		printTagLog(TAG, "state-_wait_s: event-error_e");
#endif
		fsm.push_event(Measurer::error_e{});
		return;
	}

	if (!timer.wait()) {
	    record.set(sensIndex + 1, SENSOR_ERROR_VALUE);
		sensor_timeout();
#if MEASURER_BEDUG
		printTagLog(TAG, "state-_wait_s: event-timeout_e");
#endif
		fsm.push_event(Measurer::timeout_e{});
	}
}

void Measurer::_save_s::operator ()()
{
	if (Measurer::errorsCount >= ERRORS_MAX) {
		// TODO: send save error to errors list
#if MEASURER_BEDUG
		printTagLog(TAG, "state-_save_s: event-error_e");
#endif
		fsm.push_event(Measurer::error_e{});
		return;
	}

	if (!timer.wait()) {
#if MEASURER_BEDUG
		printTagLog(TAG, "state-_save_s: event-timeout_e");
#endif
		fsm.push_event(Measurer::timeout_e{});
	}
}

void Measurer::none_a::operator ()() { }

void Measurer::init_sens_a::operator ()()
{
	fsm.clear_events();
	Measurer::sensIndex = 0;
	Measurer::errorsCount = 0;
	record = Record(0, sensors_count());
	HAL_GPIO_WritePin(POWER_L2_GPIO_Port, POWER_L2_Pin, GPIO_PIN_SET);
	if (!sensors_count()) {
#if MEASURER_BEDUG
		printTagLog(TAG, "action-reset_sens_a: event-no_sens_e");
#endif
		fsm.push_event(Measurer::no_sens_e{});
	}
	timer.changeDelay(_delay_s::TIMEOUT_MS);
	timer.start();
}

void Measurer::wait_start_a::operator ()()
{
	fsm.clear_events();
	timer.changeDelay(GENERAL_TIMEOUT_MS);
	timer.start();
}

void Measurer::save_start_a::operator ()()
{
	fsm.clear_events();
#if MEASURER_BEDUG
	printTagLog(TAG, "Save new record begin");
#endif
	if (record.save() == RECORD_OK) {
#if MEASURER_BEDUG
		printTagLog(TAG, "action-save_start_a: event-saved_e");
#endif
		fsm.push_event(Measurer::saved_e{});
	} else {
#if MEASURER_BEDUG
		printTagLog(TAG, "action-save_start_a: event-error_e");
#endif
		fsm.push_event(Measurer::timeout_e{});
	}

	Measurer::errorsCount = 0;
	timer.changeDelay(GENERAL_TIMEOUT_MS);
	timer.start();
}

void Measurer::idle_start_a::operator ()()
{
	fsm.clear_events();
	HAL_GPIO_WritePin(POWER_L2_GPIO_Port, POWER_L2_Pin, GPIO_PIN_RESET);
	timer.changeDelay(Measurer::delay);
	timer.start();
}

void Measurer::iterate_sens_a::operator ()()
{
	fsm.clear_events();
	Measurer::errorsCount = 0;
	if (++Measurer::sensIndex >= __arr_len(settings.modbus1_status)) {
#if MEASURER_BEDUG
		printTagLog(TAG, "action-iterate_sens_a: event-sens_end_e");
#endif
		fsm.push_event(Measurer::sens_end_e{});
	}
	if (!sensors_count()) {
#if MEASURER_BEDUG
		printTagLog(TAG, "action-iterate_sens_a: event-no_sens_e");
#endif
		fsm.push_event(Measurer::no_sens_e{});
	}
}

void Measurer::count_error_a::operator ()()
{
	fsm.clear_events();
	Measurer::errorsCount++; // TODO: add new save try after error
}

void Measurer::register_error_a::operator ()()
{
	// TODO: send measure error to errors list
	fsm.clear_events();
	timer.changeDelay(Measurer::delay);
	timer.start();
}

void Measurer::response_packet_handler(modbus_response_t* packet)
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

    record.set(sensIndex + 1, packet->response[0]);
    fsm.push_event(response_e{});
}
