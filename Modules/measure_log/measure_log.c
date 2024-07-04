/* Copyright © 2024 Georgy E. All rights reserved. */

#include "measure_log.h"

#include <string.h>
#include <stdbool.h>

#include "soul.h"
#include "gutils.h"
#include "fsm_gc.h"
#include "record.h"
#include "sensor.h"
#include "settings.h"
#include "onewire_driver.h"
#include "modbus_rtu_master.h"


#define ERRORS_MAX                  (10)
#define MEASURE_VALUE_DELAY_MS      ((uint32_t)500)
#define MEASURE_WAIT_POWER_DELAY_MS ((uint32_t)10000)
#define ONEWIRE_CONVERSION_DELAY_MS ((uint32_t)20000)


void _response_packet_handler(modbus_response_t* packet);

void _meas_idle_s(void);
void _meas_wait_start_s(void);
void _meas_mb1_request_s(void);
void _meas_mb1_wait_s(void);
void _meas_1w_delay_s(void);
void _meas_1w_request_s(void);
void _meas_1w_wait_s(void);
void _meas_save_s(void);

void _wait_start_a(void);
void _init_mb1_sens_a(void);
void _wait_response_a(void);
void _start_delay_a(void);
void _iterate_mb1_sens_a(void);
void _count_error_a(void);
void _init_1w_sens_a(void);
void _iterate_1w_sens_a(void);
void _save_start_a(void);
void _idle_start_a(void);
void _register_error_a(void);


FSM_GC_CREATE(meas_fsm)

FSM_GC_CREATE_EVENT(meas_success_e)
FSM_GC_CREATE_EVENT(meas_timeout_e)
FSM_GC_CREATE_EVENT(meas_iterate_e)
FSM_GC_CREATE_EVENT(meas_end_e)
FSM_GC_CREATE_EVENT(meas_error_e)

FSM_GC_CREATE_STATE(meas_idle_s,        _meas_idle_s)
FSM_GC_CREATE_STATE(meas_wait_start_s,  _meas_wait_start_s)
FSM_GC_CREATE_STATE(meas_mb1_request_s, _meas_mb1_request_s)
FSM_GC_CREATE_STATE(meas_mb1_wait_s,    _meas_mb1_wait_s)
FSM_GC_CREATE_STATE(meas_1w_delay_s,    _meas_1w_delay_s)
FSM_GC_CREATE_STATE(meas_1w_request_s,  _meas_1w_request_s)
FSM_GC_CREATE_STATE(meas_1w_wait_s,     _meas_1w_wait_s)
FSM_GC_CREATE_STATE(meas_save_s,        _meas_save_s)

FSM_GC_CREATE_TABLE(
	meas_fsm_table,
    { &meas_idle_s,        &meas_success_e, &meas_wait_start_s },

    { &meas_wait_start_s,  &meas_timeout_e, &meas_mb1_request_s },

    { &meas_mb1_request_s, &meas_success_e, &meas_mb1_wait_s },
    { &meas_mb1_request_s, &meas_end_e,     &meas_1w_delay_s },
    { &meas_mb1_request_s, &meas_iterate_e, &meas_mb1_request_s },
    { &meas_mb1_wait_s,    &meas_iterate_e, &meas_mb1_request_s },
    { &meas_mb1_wait_s,    &meas_timeout_e, &meas_mb1_request_s },

    { &meas_1w_delay_s,    &meas_success_e, &meas_1w_request_s },
    { &meas_1w_delay_s,    &meas_timeout_e, &meas_save_s },

    { &meas_1w_request_s,  &meas_success_e, &meas_1w_wait_s },
    { &meas_1w_request_s,  &meas_end_e,     &meas_save_s },
    { &meas_1w_request_s,  &meas_iterate_e, &meas_1w_request_s },
    { &meas_1w_wait_s,     &meas_iterate_e, &meas_1w_request_s },
    { &meas_1w_wait_s,     &meas_timeout_e, &meas_1w_request_s },

    { &meas_save_s,        &meas_success_e, &meas_idle_s },
    { &meas_save_s,        &meas_timeout_e, &meas_save_s },
    { &meas_save_s,        &meas_error_e,   &meas_idle_s }
)



struct measure_info_t {
	util_old_timer_t timer;
	uint8_t sens_addr;
	uint8_t sens_idx;
	uint8_t errors;

	record_t record;
} meas_info;



void measure_log_init()
{
	memset((uint8_t*)&meas_info, 0, sizeof(meas_info));

	sensors_init(&_response_packet_handler);

	fsm_gc_init(&meas_fsm, meas_fsm_table, __arr_len(meas_fsm_table));
}

void measure_log_proccess()
{
    fsm_gc_proccess(&meas_fsm);
}


void _meas_idle_s(void)
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

	memset((uint8_t*)&meas_info.record, 0, sizeof(meas_info.record));
	meas_info.record.mb1_count = settings_modbus1_count();
	meas_info.record._1w_count = settings_1wire_count();

	set_status(NEED_ENABLE_SENSORS);

	_wait_start_a();
	fsm_gc_push_event(&meas_fsm, &meas_success_e);
}

void _meas_wait_start_s(void)
{
	if (!util_old_timer_wait(&meas_info.timer)) {
		_init_mb1_sens_a();
		fsm_gc_push_event(&meas_fsm, &meas_timeout_e);
	}
}

void _meas_mb1_request_s(void)
{
	if (meas_info.sens_addr >= __arr_len(settings.modbus1_status) ||
		!sensors_count()
	) {
		_start_delay_a();
		fsm_gc_push_event(&meas_fsm, &meas_end_e);
	} else if (settings.modbus1_status[meas_info.sens_addr] != SETTINGS_SENSOR_EMPTY) {
		sensor_request_value(meas_info.sens_addr);

		_wait_response_a();
		fsm_gc_push_event(&meas_fsm, &meas_success_e);
	} else {
		_iterate_mb1_sens_a();
		fsm_gc_push_event(&meas_fsm, &meas_iterate_e);
	}
}

void _meas_mb1_wait_s(void)
{
	if (meas_info.errors >= ERRORS_MAX) {
		// TODO: send sensor error to stng_info
		_iterate_mb1_sens_a();
		fsm_gc_push_event(&meas_fsm, &meas_iterate_e);
		return;
	}

	if (!util_old_timer_wait(&meas_info.timer)) {
		meas_info.record.mb1_id[meas_info.sens_idx]
			= meas_info.sens_addr + 1;
		meas_info.record.mb1_value[meas_info.sens_idx]
		   = (int16_t)0xFFFF;

		sensor_timeout();

		_count_error_a();
		fsm_gc_push_event(&meas_fsm, &meas_timeout_e);
	}
}

void _meas_1w_delay_s(void)
{
	if (!util_old_timer_wait(&meas_info.timer)) {
		_save_start_a();
		fsm_gc_push_event(&meas_fsm, &meas_timeout_e);
	}

	if (onewire_driver_ready()) {
		_init_1w_sens_a();
		fsm_gc_push_event(&meas_fsm, &meas_success_e);
	}
}

void _meas_1w_request_s(void)
{
	if (meas_info.sens_addr >= __arr_len(settings._1wire_address)) {
		_save_start_a();
		fsm_gc_push_event(&meas_fsm, &meas_end_e);
	} else if (settings._1wire_address[meas_info.sens_addr]) {
		onewire_driver_start_read(settings._1wire_address[meas_info.sens_addr]);

		_wait_response_a();
		fsm_gc_push_event(&meas_fsm, &meas_success_e);
	} else {
		_iterate_1w_sens_a();
		fsm_gc_push_event(&meas_fsm, &meas_iterate_e);
	}
}

void _meas_1w_wait_s(void)
{
	if (!util_old_timer_wait(&meas_info.timer)) {
		meas_info.record._1w_id[meas_info.sens_idx]
            = settings._1wire_address[meas_info.sens_addr];
		meas_info.record._1w_value[meas_info.sens_idx]
		    = (int16_t)0xFFFF;

		onewire_driver_clear();

		_count_error_a();
		fsm_gc_push_event(&meas_fsm, &meas_timeout_e);
	}

	bool needIterate = false;
	if (meas_info.errors >= ERRORS_MAX) {
		// TODO: send sensor error to stng_info
		needIterate = true;
	} else if (onewire_driver_has_response()) {
		meas_info.record._1w_id[meas_info.sens_idx]
	        = settings._1wire_address[meas_info.sens_addr];
		meas_info.record._1w_value[meas_info.sens_idx]
		    = get_onewire_driver_value();

		needIterate = true;
	}

	if (needIterate) {
		_iterate_1w_sens_a();
		fsm_gc_push_event(&meas_fsm, &meas_iterate_e);
	}
}

void _meas_save_s(void)
{
	if (!util_old_timer_wait(&meas_info.timer)) {
		// TODO: send save error to errors list
		_register_error_a();
		fsm_gc_push_event(&meas_fsm, &meas_error_e);
		return;
	}

	if (!util_old_timer_wait(&meas_info.timer)) {
		_count_error_a();
		fsm_gc_push_event(&meas_fsm, &meas_timeout_e);
	}
}

void _wait_start_a(void)
{
	fsm_gc_clear(&meas_fsm);

	util_old_timer_start(&meas_info.timer, MEASURE_WAIT_POWER_DELAY_MS);
}

void _init_mb1_sens_a(void)
{
	fsm_gc_clear(&meas_fsm);

	meas_info.errors    = 0;
	meas_info.sens_addr = 0;
	meas_info.sens_idx  = 0;

	while (meas_info.sens_addr < __arr_len(settings.modbus1_status)) {
		if (settings.modbus1_status[meas_info.sens_addr] != SETTINGS_SENSOR_EMPTY) {
			break;
		}
		meas_info.sens_addr++;
	}
}

void _wait_response_a(void)
{
	fsm_gc_clear(&meas_fsm);

	util_old_timer_start(&meas_info.timer, MEASURE_VALUE_DELAY_MS);
}

void _start_delay_a(void)
{
	onewire_driver_start_convert();

	util_old_timer_start(&meas_info.timer, ONEWIRE_CONVERSION_DELAY_MS);
}

void _iterate_mb1_sens_a(void)
{
	fsm_gc_clear(&meas_fsm);

	meas_info.errors = 0;
	while (meas_info.sens_addr < __arr_len(settings.modbus1_status)) {
		if (settings.modbus1_status[++meas_info.sens_addr] != SETTINGS_SENSOR_EMPTY) {
			meas_info.sens_idx++;
			break;
		}
	}
}

void _count_error_a(void)
{
	fsm_gc_clear(&meas_fsm);

	meas_info.errors++; // TODO: add new save try after error
}

void _init_1w_sens_a(void)
{
	fsm_gc_clear(&meas_fsm);

	meas_info.errors    = 0;
	meas_info.sens_addr = 0;
	meas_info.sens_idx  = 0;

	while (meas_info.sens_addr < __arr_len(settings._1wire_address)) {
		if (settings._1wire_address[meas_info.sens_addr]) {
			break;
		}
		meas_info.sens_addr++;
	}
}

void _iterate_1w_sens_a(void)
{
	fsm_gc_clear(&meas_fsm);

	meas_info.errors = 0;

	while (meas_info.sens_addr < __arr_len(settings._1wire_address)) {
		if (settings._1wire_address[++meas_info.sens_addr]) {
			meas_info.sens_idx++;
			break;
		}
	}
}

void _save_start_a(void)
{
	fsm_gc_clear(&meas_fsm);

	set_status(LOADING);

	if (record_save(&meas_info.record) == RECORD_OK) {
		_idle_start_a();
		fsm_gc_push_event(&meas_fsm, &meas_success_e);
	} else {
		_count_error_a();
		fsm_gc_push_event(&meas_fsm, &meas_timeout_e);
	}

	meas_info.errors = 0;

	util_old_timer_start(&meas_info.timer, GENERAL_TIMEOUT_MS);

	reset_status(LOADING);
}

void _idle_start_a(void)
{
	fsm_gc_clear(&meas_fsm);

	reset_status(NEED_ENABLE_SENSORS);
	reset_status(NEED_MEASURE);
}

void _register_error_a(void)
{
	fsm_gc_clear(&meas_fsm);

	reset_status(NEED_ENABLE_SENSORS);
	reset_status(NEED_MEASURE);
}

void _response_packet_handler(modbus_response_t* packet)
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
        _count_error_a();
		fsm_gc_push_event(&meas_fsm, &meas_timeout_e);
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

	meas_info.record.mb1_id[meas_info.sens_idx]
		= meas_info.sens_addr + 1;
	meas_info.record.mb1_value[meas_info.sens_idx]
	   = (int16_t)(packet->response[0]);

	_iterate_mb1_sens_a();
	fsm_gc_push_event(&meas_fsm, &meas_iterate_e);
}
