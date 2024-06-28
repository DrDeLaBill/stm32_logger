/* Copyright © 2024 Georgy E. All rights reserved. */

#include "onewire_driver.h"

#include "glog.h"
#include "gutils.h"
#include "hal_defs.h"

#include "onewire_prootocl.h"


#define _1WIRE_DELAY_MS             ((uint32_t)100)
#define _1WIRE_DS18B20_DELAY_MS     ((uint32_t)2000)
#define _1WIRE_ADDRESS_BIT_SIZE     (sizeof(uint64_t) * BITS_IN_BYTE)

#define _1WIRE_SEARCH_UNDEFINED_BIT (0x00)
#define _1WIRE_SEARCH_ZERO_BIT      (0x01)
#define _1WIRE_SEARCH_UNIT_BIT      (0x02)
#define _1WIRE_SEARCH_EMPTY_BIT     (0x03)

#define _1WIRE_SEARCH_ROM           ((uint8_t)0xF0)
#define _1WIRE_READ_ROM             ((uint8_t)0x33)
#define _1WIRE_SKIP_ROM             ((uint8_t)0xCC)

#define _1WIRE_DS18B20_CONVERT      ((uint8_t)0x44)
#define _1WIRE_DS18B20_READ         ((uint8_t)0xBE)

#define _1WIRE_DS18B20_BITS_COUNT   (9 * BITS_IN_BYTE)

#define _1WIRE_DS18B20_MSB_SIGN_BIT (3)
#define _1WIRE_DS18B20_LSB_OFFSET   (4)


enum DS18B20_REGS {
	DS18B20_REG_TEMP_LSB = 0,
	DS18B20_REG_TEMP_MSB,
	DS18B20_REG_TEMP_H,
	DS18B20_REG_TEMP_L,
	DS18B20_REG_TEMP_CONF,
	DS18B20_REG_TEMP_R1,
	DS18B20_REG_TEMP_R2,
	DS18B20_REG_TEMP_R3,
	DS18B20_REG_TEMP_CRC
};



typedef struct _driver_state_t {
	void     (*fsm) (void);
	bool     need_search;
	bool     need_value;
	bool     need_convert;
	bool     ready;
	bool     has_response;

	bool     tree_found;
	uint8_t  tree[_1WIRE_ADDRESS_BIT_SIZE]; // TODO: change to two 64-bit vars
	uint64_t tree_mask;

	uint16_t counter;
	int16_t  value;
	uint64_t address;
	uint8_t  value_address[_1WIRE_ADDRESS_BIT_SIZE / BITS_IN_BYTE];

	util_old_timer_t timer;
} driver_state_t;


void _onewire_driver_state_reset();
void _onewire_driver_start_idle();
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

void _fsm_onewire_driver_convert_start();
void _fsm_onewire_driver_convert_wait_send();
void _fsm_onewire_driver_convert_read();
void _fsm_onewire_driver_convert_wait_read();
void _fsm_onewire_driver_read_start();
void _fsm_onewire_driver_read_wait();
void _fsm_onewire_driver_read_recieve();
void _fsm_onewire_driver_read_recieve_wait();
void _fsm_onewire_driver_read_end();


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
	uint8_t last_bit = 0;
	for (uint64_t i = __arr_len(driver_state.tree) - 1; i > 0; i--) {
		if (driver_state.tree[i] == _1WIRE_SEARCH_UNDEFINED_BIT &&
			!__get_bit(old_mask, i)
		) {
			last_bit = i;
			break;
		}
	}
	__set_bit(driver_state.tree_mask, last_bit);
	for (uint64_t i = last_bit + 1; i < _1WIRE_ADDRESS_BIT_SIZE; i++) {
		__reset_bit(driver_state.tree_mask, i);
	}
	if (old_mask == driver_state.tree_mask) {
		onewire_driver_start_search();
		return;
	}


	driver_state.address      = 0;
	driver_state.need_search  = true;
}

void onewire_driver_start_convert()
{
	if (_onewire_driver_busy()) {
		printTagLog(_1WIRE_DRIVER_TAG, "1WIRE driver is busy, unable to start convert");
		return;
	}

	driver_state.need_convert = true;
}

bool onewire_driver_ready()
{
	return driver_state.ready;
}

bool onewire_driver_has_response()
{
	return driver_state.has_response;
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
	driver_state.need_value   = true;
	driver_state.address      = address;
	driver_state.ready        = false;

	uint64_t tmp_address = address;
	for (unsigned i = 0; i < _1WIRE_ADDRESS_BIT_SIZE; i++) {
		bool bit = __get_bit(tmp_address, i);
		if (bit) {
			__set_bit(driver_state.value_address[i / BITS_IN_BYTE], i % BITS_IN_BYTE);
		} else {
			__reset_bit(driver_state.value_address[i / BITS_IN_BYTE], i % BITS_IN_BYTE);
		}
	}
	uint8_t tmp = driver_state.value_address[0];
	driver_state.value_address[0] = driver_state.value_address[__arr_len(driver_state.value_address) - 1];
	driver_state.value_address[__arr_len(driver_state.value_address) - 1] = tmp;
}

int16_t get_onewire_driver_value()
{
	return driver_state.value;
}

void _onewire_driver_state_reset()
{
	memset((void*)&driver_state, 0, sizeof(driver_state));
	_onewire_driver_start_idle();
}

bool _onewire_driver_busy()
{
	return driver_state.need_value || driver_state.need_search;
}

void _onewire_driver_start_idle()
{
	driver_state.counter      = 0;
	driver_state.need_value   = false;
	driver_state.need_search  = false;
	driver_state.need_convert = false;
	driver_state.ready        = true;
	driver_state.fsm          = _fsm_onewire_driver_idle;
}

void _fsm_onewire_driver_init()
{
	_onewire_driver_start_idle();
	driver_state.need_convert = true;
}

void _fsm_onewire_driver_idle()
{
	if (driver_state.need_convert) {
		driver_state.ready        = false;
		driver_state.has_response = false;
		driver_state.fsm          = _fsm_onewire_driver_convert_start;
	} else if (driver_state.need_search) {
		driver_state.ready        = false;
		driver_state.has_response = false;
		driver_state.fsm          = _fsm_onewire_driver_search_start;
	} else if (driver_state.need_value) {
		memset(&driver_state.tree, 0, sizeof(driver_state.tree));
		driver_state.tree_found   = false;
		driver_state.tree_mask    = 0;
		driver_state.ready        = false;
		driver_state.has_response = false;
		driver_state.fsm          = _fsm_onewire_driver_search_start;
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
		_onewire_driver_start_idle();
		driver_state.has_response = false;
		return;
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
			_onewire_driver_start_idle();
			driver_state.has_response = false;
			break;
	}
}

void _fsm_onewire_driver_search_empty_bits()
{
	if (!driver_state.counter) {
		printTagLog(_1WIRE_DRIVER_TAG, "1WIRE driver error, sensors not found");
		_onewire_driver_start_idle();
		driver_state.has_response = false;
		return;
	}

	driver_state.need_search  = false;
	driver_state.has_response = false;
	driver_state.fsm          = _fsm_onewire_driver_idle;

	if (!driver_state.need_value) {
		onewire_driver_next_search();
	} else {
		_onewire_driver_start_idle();
	}
}

void _fsm_onewire_driver_search_confirm_bit_start()
{
	uint16_t cur_counter = driver_state.counter - 1;
	uint8_t node = driver_state.tree[cur_counter];
	bool bit = (bool)__get_bit(node, 1);
	if (driver_state.need_value) {
		bit = (bool)__get_bit(
			driver_state.value_address[cur_counter / BITS_IN_BYTE],
			(cur_counter % BITS_IN_BYTE)
		);
	} else if (node == _1WIRE_SEARCH_UNDEFINED_BIT) {
		bit = (bool)__get_bit(driver_state.tree_mask, cur_counter);
	}
	onewire_protocol_send_bit(bit);
	util_old_timer_start(&driver_state.timer, _1WIRE_DELAY_MS);
	driver_state.fsm = _fsm_onewire_driver_search_confirm_bit_wait;
}

void _fsm_onewire_driver_search_confirm_bit_wait()
{
	if (!util_old_timer_wait(&driver_state.timer)) {
		_onewire_driver_start_idle();
		driver_state.has_response = false;
		return;
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
	if (driver_state.need_value) {
		driver_state.fsm = _fsm_onewire_driver_read_start;
		return;
	}

	uint8_t buff[sizeof(driver_state.address)] = {};
	for (unsigned i = 0; i < _1WIRE_ADDRESS_BIT_SIZE; i++) {
		bool bit = (driver_state.tree[i] == _1WIRE_SEARCH_UNIT_BIT) ? 1 : 0;
		if (__get_bit(driver_state.tree_mask, i) > 0) {
			bit = 1;
		}
		buff[i / BITS_IN_BYTE] |= (((uint64_t)bit) << (i % BITS_IN_BYTE));
	}
	if (buff[__arr_len(buff) - 1] != onewire_protocol_crc8(buff, __arr_len(buff) - 1)) {
		_onewire_driver_start_idle();
		driver_state.has_response = false;
		return;
	}

	uint8_t tmp = buff[0];
	buff[0] = buff[__arr_len(buff) - 1];
	buff[__arr_len(buff) - 1] = tmp;
	driver_state.address = 0;
	for (unsigned i = 0; i < __arr_len(buff); i++) {
		driver_state.address |= (((uint64_t)buff[i]) << (i * BITS_IN_BYTE));
	}

	driver_state.tree_found  = true;
	driver_state.has_response = true;

	_onewire_driver_start_idle();
}

void _fsm_onewire_driver_convert_start()
{
	uint8_t request[] = { _1WIRE_SKIP_ROM, _1WIRE_DS18B20_CONVERT };
	bool data[sizeof(request) * BITS_IN_BYTE] = {};
    for (unsigned i = 0; i < sizeof(request) * BITS_IN_BYTE; i++) {
    	data[i] = ((request[i / BITS_IN_BYTE] >> (i % BITS_IN_BYTE)) & 0x01);
    }
    onewire_protocol_send_request(data, sizeof(request) * BITS_IN_BYTE, 2);

	util_old_timer_start(&driver_state.timer, _1WIRE_DS18B20_DELAY_MS);
	driver_state.fsm = _fsm_onewire_driver_convert_wait_send;
}

void _fsm_onewire_driver_convert_wait_send()
{
	if (!util_old_timer_wait(&driver_state.timer)) {
		onewire_protocol_reset();
		_onewire_driver_start_idle();
		driver_state.has_response = false;
		return;
	}

	if (onewire_protocol_result_ready()) {
		driver_state.fsm = _fsm_onewire_driver_convert_read;
	}
}

void _fsm_onewire_driver_convert_read()
{
	onewire_protocol_read_bits(1);
	util_old_timer_start(&driver_state.timer, _1WIRE_DELAY_MS);
	driver_state.fsm = _fsm_onewire_driver_convert_wait_read;
}

void _fsm_onewire_driver_convert_wait_read()
{
	if (!util_old_timer_wait(&driver_state.timer)) {
		onewire_protocol_reset();
		_onewire_driver_start_idle();
		driver_state.has_response = false;
		return;
	}

	if (!onewire_protocol_result_ready()) {
		return;
	}

	if (onewire_protocol_response()[0]) {
		_onewire_driver_start_idle();
		driver_state.has_response = true;
	} else {
		driver_state.fsm = _fsm_onewire_driver_convert_read;
	}
}

void _fsm_onewire_driver_read_start()
{
    onewire_protocol_send_byte(_1WIRE_DS18B20_READ);

	util_old_timer_start(&driver_state.timer, _1WIRE_DELAY_MS);
	driver_state.fsm = _fsm_onewire_driver_read_wait;
}

void _fsm_onewire_driver_read_wait()
{
	if (!util_old_timer_wait(&driver_state.timer)) {
		onewire_protocol_reset();
		_onewire_driver_start_idle();
		return;
	}

	if (onewire_protocol_result_ready()) {
		driver_state.fsm = _fsm_onewire_driver_read_recieve;
	}
}

void _fsm_onewire_driver_read_recieve()
{
	onewire_protocol_read_bits(_1WIRE_DS18B20_BITS_COUNT);

	util_old_timer_start(&driver_state.timer, _1WIRE_DELAY_MS);
	driver_state.fsm = _fsm_onewire_driver_read_recieve_wait;
}

void _fsm_onewire_driver_read_recieve_wait()
{
	if (!util_old_timer_wait(&driver_state.timer)) {
		onewire_protocol_reset();
		_onewire_driver_start_idle();
		driver_state.has_response = false;
		return;
	}

	if (!onewire_protocol_result_ready()) {
		return;
	}

	uint8_t buff[_1WIRE_DS18B20_BITS_COUNT / BITS_IN_BYTE] = {};
	for (unsigned i = 0; i < _1WIRE_DS18B20_BITS_COUNT; i++) {
		buff[i / BITS_IN_BYTE] |= ((onewire_protocol_response()[i]) << (i % BITS_IN_BYTE));
	}
	if (buff[__arr_len(buff) - 1] != onewire_protocol_crc8(buff, __arr_len(buff) - 1)) {
		_onewire_driver_start_idle();
		driver_state.has_response = false;
		return;
	}

	uint8_t lsb = buff[DS18B20_REG_TEMP_LSB];
	uint8_t msb = buff[DS18B20_REG_TEMP_MSB];
	driver_state.value  = ((0x07 & msb) << _1WIRE_DS18B20_LSB_OFFSET);
	driver_state.value |= (lsb >> _1WIRE_DS18B20_LSB_OFFSET);
	driver_state.value *= 10;
	driver_state.value += (lsb & 0x0F);
	if (__get_bit(msb, _1WIRE_DS18B20_MSB_SIGN_BIT)) {
		driver_state.value *= -1;
	}

	printTagLog("OWd", "value %d.%d", driver_state.value / 10, __abs(driver_state.value % 10));

	driver_state.fsm = _fsm_onewire_driver_read_end;
}

void _fsm_onewire_driver_read_end()
{
	_onewire_driver_start_idle();
	driver_state.has_response = true;
}


