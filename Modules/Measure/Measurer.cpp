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
Record Measurer::m_record(0);


Measurer::Measurer(uint32_t delay)
{
#if MEASURER_BEDUG
	BEDUG_ASSERT(delay, "Measure delay should be longer");
#endif
	sensors_init(&response_packet_handler);
	this->delay = delay;
}

void Measurer::process()
{
	fsm.proccess();
}

void Measurer::_init_s::operator ()()
{
	if (is_settings_initialized()) {
		fsm.push_event(Measurer::ready_e{});
	}
}

utl::Timer Measurer::_idle_s::timer(HOUR_MS);
void Measurer::_idle_s::operator ()()
{
	if (!_idle_s::timer.wait()) {
		Measurer::m_record = Record(0, sensors_count());
#if MEASURER_BEDUG
		printTagLog(TAG, "state-_idle_s: event-timeout_e");
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

utl::Timer Measurer::_wait_s::timer(MODBUS_TIMEOUS_MS);
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

	if (!_wait_s::timer.wait()) {
	    m_record.set(sensIndex + 1, SENSOR_ERROR_VALUE);
		sensor_timeout();
#if MEASURER_BEDUG
		printTagLog(TAG, "state-_wait_s: event-timeout_e");
#endif
		fsm.push_event(Measurer::timeout_e{});
	}
}

utl::Timer Measurer::_save_s::timer(GENERAL_TIMEOUT_MS);
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

	if (!_save_s::timer.wait()) {
#if MEASURER_BEDUG
		printTagLog(TAG, "state-_save_s: event-timeout_e");
#endif
		fsm.push_event(Measurer::timeout_e{});
	}
}

void Measurer::init_sens_a::operator ()()
{
	fsm.clear_events();
	Measurer::sensIndex = 0;
	Measurer::errorsCount = 0;
	m_record = Record(0, sensors_count());
	HAL_GPIO_WritePin(POWER_L2_GPIO_Port, POWER_L2_Pin, GPIO_PIN_SET);
	if (!sensors_count()) {
#if MEASURER_BEDUG
		printTagLog(TAG, "action-reset_sens_a: event-no_sens_e");
#endif
		fsm.push_event(Measurer::no_sens_e{});
	}
}

void Measurer::wait_start_a::operator ()()
{
	fsm.clear_events();
	_wait_s::timer.start();
}

void Measurer::save_start_a::operator ()()
{
	fsm.clear_events();
#if MEASURER_BEDUG
	printTagLog(TAG, "Save new record begin");
#endif
	if (m_record.save() == RECORD_OK) {
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
	_save_s::timer.start();
}

void Measurer::idle_start_a::operator ()()
{
	fsm.clear_events();
	HAL_GPIO_WritePin(POWER_L2_GPIO_Port, POWER_L2_Pin, GPIO_PIN_RESET);
	_idle_s::timer = utl::Timer(Measurer::delay);
	_idle_s::timer.start();
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
	_idle_s::timer = utl::Timer(Measurer::delay);
	_idle_s::timer.start();
}

void Measurer::response_packet_handler(modbus_response_t* packet)
{
#if MEASURER_BEDUG
	BEDUG_ASSERT(packet, "Incorrect MODBUS response data");
#endif
	if (!packet) {
		return;
	}

    if (packet->status != MODBUS_NO_ERROR) {
#if MEASURER_BEDUG
        printTagLog(TAG, "ERROR: %02x", packet->status);
#endif
        fsm.push_event(error_e{});
        return;
    }

    m_record.set(sensIndex + 1, packet->response[0]);
    fsm.push_event(response_e{});

#if MEASURER_BEDUG
    printTagLog(TAG, "SUCCESS");
#endif
}
