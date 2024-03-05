/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Measure.h"

#include <cstring>

#include "log.h"
#include "main.h"
#include "soul.h"
#include "utils.h"
#include "sensor.h"
#include "bmacro.h"
#include "sensor.h"
#include "hal_defs.h"
#include "settings.h"
#include "modbus_rtu_master.h"

#include "Record.h"
#include "USBController.h"


uint32_t Measure::sensIndex  = 0;
uint8_t Measure::errorsCount = 0;

fsm::FiniteStateMachine<Measure::fsm_table> Measure::fsm;
utl::Timer Measure::timer(GENERAL_TIMEOUT_MS);
Record Measure::record(0);


Measure::Measure()
{
#if MEASURER_BEDUG
	BEDUG_ASSERT(delay, "Measure delay should be larger than 0");
#endif
	sensors_init(&response_packet_handler);
}

void Measure::process()
{
	fsm.proccess();
}

void Measure::_init_s::operator ()()
{
	if (is_settings_initialized()) {
		fsm.push_event(Measure::ready_e{});
	}
}

void Measure::_idle_s::operator ()()
{
	if (is_status(NEED_MEASURE)) {
		fsm.push_event(need_measure_e{});
	}
}

void Measure::_request_s::operator ()()
{
	if (settings.modbus1_status[Measure::sensIndex] != SETTINGS_SENSOR_EMPTY) {
		sensor_request_value(Measure::sensIndex);
#if MEASURER_BEDUG
		printTagLog(TAG, "state-_request_s: event-sended_e");
#endif
		fsm.push_event(Measure::sended_e{});
	} else {
#if MEASURER_BEDUG
		printTagLog(TAG, "state-_request_s: event-skip_e");
#endif
		fsm.push_event(Measure::skip_e{});
	}
}

void Measure::_wait_s::operator ()()
{
	if (Measure::errorsCount >= ERRORS_MAX) {
		// TODO: send sensor error to stng_info
#if MEASURER_BEDUG
		printTagLog(TAG, "state-_wait_s: event-error_e");
#endif
		fsm.push_event(Measure::error_e{});
		return;
	}

	if (!timer.wait()) {
	    record.set(sensIndex + 1, SENSOR_ERROR_VALUE);
		sensor_timeout();
#if MEASURER_BEDUG
		printTagLog(TAG, "state-_wait_s: event-timeout_e");
#endif
		fsm.push_event(Measure::timeout_e{});
	}
}

void Measure::_save_s::operator ()()
{
	if (Measure::errorsCount >= ERRORS_MAX) {
		// TODO: send save error to errors list
#if MEASURER_BEDUG
		printTagLog(TAG, "state-_save_s: event-error_e");
#endif
		fsm.push_event(Measure::error_e{});
		return;
	}

	if (!timer.wait()) {
#if MEASURER_BEDUG
		printTagLog(TAG, "state-_save_s: event-timeout_e");
#endif
		fsm.push_event(Measure::timeout_e{});
	}
}

void Measure::none_a::operator ()() { }

void Measure::init_sens_a::operator ()()
{
	fsm.clear_events();
	Measure::sensIndex = 0;
	Measure::errorsCount = 0;
	record = Record(0, sensors_count());
	if (!USBController::connected()) {
		HAL_GPIO_WritePin(STEPUP_5V_ON_GPIO_Port, STEPUP_5V_ON_Pin, GPIO_PIN_SET);
	}
	HAL_GPIO_WritePin(POWER_L2_GPIO_Port, POWER_L2_Pin, GPIO_PIN_SET);
	if (!sensors_count()) {
#if MEASURER_BEDUG
		printTagLog(TAG, "action-reset_sens_a: event-no_sens_e");
#endif
		fsm.push_event(Measure::no_sens_e{});
	}
}

void Measure::wait_start_a::operator ()()
{
	fsm.clear_events();
	timer.changeDelay(GENERAL_TIMEOUT_MS);
	timer.start();
}

void Measure::save_start_a::operator ()()
{
	set_status(WAIT_LOAD);
	fsm.clear_events();
#if MEASURER_BEDUG
	printTagLog(TAG, "Save new record begin");
#endif
	if (record.save() == RECORD_OK) {
#if MEASURER_BEDUG
		printTagLog(TAG, "action-save_start_a: event-saved_e");
#endif
		fsm.push_event(Measure::saved_e{});
	} else {
#if MEASURER_BEDUG
		printTagLog(TAG, "action-save_start_a: event-error_e");
#endif
		fsm.push_event(Measure::timeout_e{});
	}

	Measure::errorsCount = 0;
	timer.changeDelay(GENERAL_TIMEOUT_MS);
	timer.start();
	reset_status(WAIT_LOAD);
}

void Measure::idle_start_a::operator ()()
{
	fsm.clear_events();
	HAL_GPIO_WritePin(POWER_L2_GPIO_Port, POWER_L2_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(STEPUP_5V_ON_GPIO_Port, STEPUP_5V_ON_Pin, GPIO_PIN_RESET);

	reset_status(NEED_MEASURE);
}

void Measure::iterate_sens_a::operator ()()
{
	fsm.clear_events();
	Measure::errorsCount = 0;
	if (++Measure::sensIndex >= __arr_len(settings.modbus1_status)) {
#if MEASURER_BEDUG
		printTagLog(TAG, "action-iterate_sens_a: event-sens_end_e");
#endif
		fsm.push_event(Measure::sens_end_e{});
	}
	if (!sensors_count()) {
#if MEASURER_BEDUG
		printTagLog(TAG, "action-iterate_sens_a: event-no_sens_e");
#endif
		fsm.push_event(Measure::no_sens_e{});
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

	reset_status(NEED_MEASURE);
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

    record.set(sensIndex + 1, packet->response[0]);
    fsm.push_event(response_e{});
}
