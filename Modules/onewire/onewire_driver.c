/* Copyright © 2024 Georgy E. All rights reserved. */

#include "onewire_driver.h"

#include "log.h"
#include "utils.h"
#include "hal_defs.h"

#include "onewire_prootocl.h"


#define _1WIRE_DELAY_MS             ((uint32_t)100)
#define _1WIRE_ADDRESS_BIT_SIZE     (sizeof(uint64_t) * BITS_IN_BYTE)

#define _1WIRE_SEARCH_UNDEFINED_BIT (0x00)
#define _1WIRE_SEARCH_ZERO_BIT      (0x01)
#define _1WIRE_SEARCH_UNIT_BIT      (0x02)
#define _1WIRE_SEARCH_EMPTY_BIT     (0x03)

#define _1WIRE_SEARCH_ROM           ((uint8_t)0xF0)
#define _1WIRE_READ_ROM             ((uint8_t)0x33)



typedef struct _driver_state_t {
	void     (*fsm) (void);
	bool     need_search;
	bool     need_value;
	bool     ready;

	bool     tree_found;
	uint8_t  tree[_1WIRE_ADDRESS_BIT_SIZE]; // TODO: change to two 64-bit vars
	uint64_t tree_mask;

	uint16_t counter;
	uint16_t value;
	uint64_t address;

	util_old_timer_t timer;
} driver_state_t;


void _onewire_driver_state_reset();
bool _onewire_driver_busy();

void _fsm_onewire_driver_init();
void _fsm_onewire_driver_idle();
void _fsm_onewire_driver_error();

void _fsm_onewire_driver_search_start();
void _fsm_onewire_driver_search_wait_bits();
void _fsm_onewire_driver_search_response();
void _fsm_onewire_driver_search_empty_bits();
void _fsm_onewire_driver_search_confirm_bit_start();
void _fsm_onewire_driver_search_confirm_bit_wait();
void _fsm_onewire_driver_search_iterate();
void _fsm_onewire_driver_search_end();

void _fsm_onewire_driver_read_start();


static const char _1WIRE_DRIVER_TAG[] = "1WRd";

driver_state_t driver_state = {
	.fsm = _fsm_onewire_driver_init
};

void onewire_driver_tick()
{
	if (!driver_state.fsm) {
		_onewire_driver_state_reset();
	}
	driver_state.fsm();
}

void onewire_driver_clear()
{
	_onewire_driver_state_reset();
	onewire_protocol_reset();
}

void onewire_driver_start_search()
{
	if (_onewire_driver_busy()) {
		printTagLog(_1WIRE_DRIVER_TAG, "1WIRE driver is busy, unable to start search");
		return;
	}

	memset(&driver_state.tree, 0, sizeof(driver_state.tree));
	driver_state.need_search      = true;
	driver_state.tree_found       = false;
	driver_state.tree_mask        = 0;
	driver_state.address          = 0;
	driver_state.counter          = 0;
	driver_state.ready            = false;
	driver_state.fsm              = _fsm_onewire_driver_idle;
}

void onewire_driver_next_search()
{
	if (_onewire_driver_busy()) {
		printTagLog(_1WIRE_DRIVER_TAG, "1WIRE driver is busy, unable to start next search");
		return;
	}

	if (!driver_state.tree_found) {
		onewire_driver_start_search();
		return;
	}

	unsigned counter = 0;
	for (uint64_t i = 0; i < __arr_len(driver_state.tree); i++) {
		if (driver_state.tree[i] == _1WIRE_SEARCH_UNDEFINED_BIT) {
			counter++;
		}
	}
	if (!counter) {
		onewire_driver_start_search();
		return;
	}

	uint64_t old_mask = driver_state.tree_mask;
	for (uint64_t i = __arr_len(driver_state.tree) - 1; i > 0; i--) {
		if (driver_state.tree[i] == _1WIRE_SEARCH_UNDEFINED_BIT &&
			!__get_bit(old_mask, i)
		) {
			__set_bit(driver_state.tree_mask, i);
			for (uint64_t j = i + 1; j < _1WIRE_ADDRESS_BIT_SIZE; j++) {
				__reset_bit(driver_state.tree_mask, j);
			}
		}
	}
	if (old_mask == driver_state.tree_mask) {
		onewire_driver_start_search();
		return;
	}


	driver_state.counter     = 0;
	driver_state.need_search = true;
	driver_state.ready       = false;
	driver_state.fsm         = _fsm_onewire_driver_idle;
}

bool onewire_driver_ready()
{
	return driver_state.ready;
}

uint64_t get_onewire_driver_address()
{
	return driver_state.address;
}

void onewire_driver_start_read(uint64_t address)
{
	if (_onewire_driver_busy()) {
		printTagLog(_1WIRE_DRIVER_TAG, "1WIRE driver is busy, unable to read value");
		return;
	}
	driver_state.need_value = true;
	driver_state.address = address;
	driver_state.ready = false;
}

uint16_t get_onewire_driver_value()
{
	return driver_state.value;
}

void _onewire_driver_state_reset()
{
	memset((void*)&driver_state, 0, sizeof(driver_state));
	driver_state.fsm = _fsm_onewire_driver_init;
}

bool _onewire_driver_busy()
{
	return driver_state.need_value || driver_state.need_search;
}

void _fsm_onewire_driver_init()
{
	driver_state.fsm = _fsm_onewire_driver_idle;
}

void _fsm_onewire_driver_idle()
{
	if (driver_state.need_search) {
		driver_state.counter = 0;
		driver_state.fsm = _fsm_onewire_driver_search_start;
	} else if (driver_state.need_value) {
		driver_state.counter = 0;
		driver_state.fsm = _fsm_onewire_driver_read_start;
	}
}

void _fsm_onewire_driver_error()
{
	_onewire_driver_state_reset();
}

void _fsm_onewire_driver_search_start()
{
	uint8_t request[] = { _1WIRE_SEARCH_ROM };
	bool data[sizeof(request) * BITS_IN_BYTE] = {};
    for (unsigned i = 0; i < sizeof(request) * BITS_IN_BYTE; i++) {
    	data[i] = ((request[i / BITS_IN_BYTE] >> (i % BITS_IN_BYTE)) & 0x01);
    }
    onewire_protocol_send_request(data, sizeof(request) * BITS_IN_BYTE, 2);

	util_old_timer_start(&driver_state.timer, _1WIRE_DELAY_MS);
	driver_state.fsm = _fsm_onewire_driver_search_wait_bits;
}

void _fsm_onewire_driver_search_wait_bits()
{
	if (!util_old_timer_wait(&driver_state.timer)) {
		driver_state.fsm = _fsm_onewire_driver_error;
	}

	if (onewire_protocol_result_ready()) {
		driver_state.fsm = _fsm_onewire_driver_search_response;
	}
}

void _fsm_onewire_driver_search_response()
{
	bool* response = onewire_protocol_response();
	uint8_t curr_bit = ((response[0] << 1) | response[1]);
	driver_state.tree[driver_state.counter] = curr_bit;

	switch (curr_bit) {
		case _1WIRE_SEARCH_EMPTY_BIT:
			driver_state.fsm = _fsm_onewire_driver_search_empty_bits;
			break;
		case _1WIRE_SEARCH_UNDEFINED_BIT:
		case _1WIRE_SEARCH_UNIT_BIT:
		case _1WIRE_SEARCH_ZERO_BIT:
			driver_state.counter++;
			driver_state.fsm = _fsm_onewire_driver_search_confirm_bit_start;
			break;
		default:
			printTagLog(_1WIRE_DRIVER_TAG, "1WIRE driver error, unacceptable address bit");
			driver_state.fsm = _fsm_onewire_driver_error;
			break;
	}
}

void _fsm_onewire_driver_search_empty_bits()
{
	if (!driver_state.counter) {
		printTagLog(_1WIRE_DRIVER_TAG, "1WIRE driver error, sensors not found");
		driver_state.fsm = _fsm_onewire_driver_error;
		return;
	}

	driver_state.need_search = false;

	onewire_driver_next_search();

	driver_state.fsm = _fsm_onewire_driver_idle;
}

void _fsm_onewire_driver_search_confirm_bit_start()
{
	uint16_t cur_counter = driver_state.counter - 1;
	uint8_t node = driver_state.tree[cur_counter];
	bool bit = (bool)__get_bit(node, 1);
	if (node == _1WIRE_SEARCH_UNDEFINED_BIT) {
		bit = (bool)__get_bit(driver_state.tree_mask, cur_counter);
	}
	onewire_protocol_send_bit(bit);
	util_old_timer_start(&driver_state.timer, _1WIRE_DELAY_MS);
	driver_state.fsm = _fsm_onewire_driver_search_confirm_bit_wait;
}

void _fsm_onewire_driver_search_confirm_bit_wait()
{
	if (!util_old_timer_wait(&driver_state.timer)) {
		driver_state.fsm = _fsm_onewire_driver_error;
	}

	if (onewire_protocol_result_ready()) {
		driver_state.fsm = _fsm_onewire_driver_search_iterate;
	}
}

void _fsm_onewire_driver_search_iterate()
{
	if (driver_state.counter < _1WIRE_ADDRESS_BIT_SIZE) {
		onewire_protocol_read_bits(2);
		util_old_timer_start(&driver_state.timer, _1WIRE_DELAY_MS);
		driver_state.fsm = _fsm_onewire_driver_search_wait_bits;
	} else {
		driver_state.fsm = _fsm_onewire_driver_search_end;
	}
}

void _fsm_onewire_driver_search_end()
{
	uint8_t buff[sizeof(driver_state.address)] = {};
	for (unsigned i = 0; i < _1WIRE_ADDRESS_BIT_SIZE; i++) {
		bool bit = (driver_state.tree[i] == _1WIRE_SEARCH_UNIT_BIT) ? 1 : 0;
		if (__get_bit(driver_state.tree_mask, i) > 0) {
			bit = 1;
		}
		buff[i / BITS_IN_BYTE] |= (((uint64_t)bit) << (i % BITS_IN_BYTE));
	}
	if (buff[__arr_len(buff) - 1] != onewire_protocol_crc8(buff, __arr_len(buff) - 1)) {
		driver_state.fsm = _fsm_onewire_driver_error;
		return;
	}
	uint8_t tmp = buff[0];
	buff[0] = buff[__arr_len(buff) - 1];
	buff[__arr_len(buff) - 1] = tmp;
	for (unsigned i = 0; i < __arr_len(buff); i++) {
		driver_state.address |= (((uint64_t)buff[i]) << (i * BITS_IN_BYTE));
	}

	driver_state.need_search = false;
	driver_state.tree_found = true;
	driver_state.ready = true;

	driver_state.fsm = _fsm_onewire_driver_idle;
}

void _fsm_onewire_driver_read_start()
{
	printTagLog(_1WIRE_DRIVER_TAG, "1WIRE driver error, read error");
	driver_state.need_value = false;
	driver_state.fsm = _fsm_onewire_driver_idle;
}
