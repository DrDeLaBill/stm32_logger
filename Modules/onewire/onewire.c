/* Copyright © 2024 Georgy E. All rights reserved. */

#include "onewire.h"

#include <string.h>

#include "main.h"

#include "utils.h"
#include "bmacro.h"
#include "hal_defs.h"

#define COMMAND_MAX_LENGTH   (10 * BITS_IN_BYTE) // TODO: too big

#define IDLE_DELAY_US        ((uint16_t)100)
#define START_DELAY_US       ((uint16_t)100)
#define RESET_DELAY_US       ((uint16_t)600)
#define RESET_WAIT_US        ((uint16_t)15)
#define WAIT_PRESENSE_US     ((uint16_t)40)
#define WAIT_PRESENSE_END_US ((uint16_t)300)
#define RESET_DATA_DELAY_US  ((uint16_t)2)
#define SLOT_DELAY_US        ((uint16_t)60)
#define WAIT_DATA_DELAY_US   ((uint16_t)15)
#define END_DATA_DELAY_US    ((uint16_t)5)
#define WAIT_SLAVE_BIT_US    ((uint16_t)5)

#define SET_BUS()      (_1WIRE_GPIO_Port->ODR |= _1WIRE_Pin)
#define RESET_BUS()    (_1WIRE_GPIO_Port->ODR &= ~(_1WIRE_Pin))
#define READ_BUS()     (_1WIRE_GPIO_Port->IDR & _1WIRE_Pin)


typedef struct _onewire_t {
	void (*fsm) (void);
	bool     need_reset;
	bool     ready;
	uint16_t value;

	bool     data[COMMAND_MAX_LENGTH];
	uint16_t count;
	uint16_t need_count;
	uint8_t  bit_idx;
} onewire_t;


void _onewire_state_reset();
void _configure_timer(uint16_t target_usec);
void _onewire_state_reset();
bool _onewire_busy();

void _fsm_onewire_init();
void _fsm_onewire_idle();
void _fsm_onewire_error();
void _fsm_onewire_write_reset_start();
void _fsm_onewire_write_reset_end();
void _fsm_onewire_wait_presense_start();
void _fsm_onewire_wait_presense_end();
void _fsm_onewire_send_bit_begin();
void _fsm_onewire_send_bit_set();
void _fsm_onewire_send_bit_end();
void _fsm_onewire_send_iterate_bit();
void _fsm_onewire_read_bit_begin();
void _fsm_onewire_read_bit_set();
void _fsm_onewire_read_bit_end();
void _fsm_onewire_read_iterate_bit();
void _fsm_onewire_end();


static const uint8_t ONEWIRE_CRC8_TABLE[] = {
    0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
	157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220,
	35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98,
	190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
	70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
	219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
	101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
	248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
	140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
	17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
	175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
	50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
	202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139,
	87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
	233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
	116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53
};


onewire_t _1wire_state = {
	.fsm = _fsm_onewire_init
};


void onewire_proccess()
{
	if (_1wire_state.need_reset || !_1wire_state.fsm) {
		_onewire_state_reset();
	}
	LED_GPIO_Port->ODR |= LED_Pin; // TODO: remove after 1wire test
	_1wire_state.fsm();
	LED_GPIO_Port->ODR &= ~LED_Pin; // TODO: remove after 1wire test
}

void onewire_read_bits(uint8_t count)
{
	if (_onewire_busy()) {
		BEDUG_ASSERT(false, "1WIRE is already busy");
		return;
	}

	memset(_1wire_state.data, 0, sizeof(_1wire_state.data));
	_1wire_state.need_count = count;
	_1wire_state.ready = false;
}

void onewire_send_bit(uint8_t bit)
{
	if (_onewire_busy()) {
		BEDUG_ASSERT(false, "1WIRE is already busy");
		return;
	}

	memset(_1wire_state.data, 0, sizeof(_1wire_state.data));
	_1wire_state.data[0] = (bool)bit;
	_1wire_state.count = 1;
	_1wire_state.ready = false;
}

void onewire_send_request(bool* data, uint16_t bitCount, uint16_t needBitCount)
{
	if (_onewire_busy()) {
		BEDUG_ASSERT(false, "1WIRE is already busy");
		return;
	}

	BEDUG_ASSERT(bitCount <= __arr_len(_1wire_state.data), "1WIRE bit request buffer is out of range");
	BEDUG_ASSERT(needBitCount <= __arr_len(_1wire_state.data), "1WIRE bit response buffer is out of range");

	if (bitCount > __arr_len(_1wire_state.data)) {
		bitCount = __arr_len(_1wire_state.data);
	}
	if (needBitCount > __arr_len(_1wire_state.data)) {
		needBitCount = __arr_len(_1wire_state.data);
	}

	for (unsigned i = 0; i < bitCount; i++) {
		_1wire_state.data[i] = data[i];
	}
	_1wire_state.count = bitCount;
	_1wire_state.need_count = needBitCount;
	_1wire_state.ready = false;
}

bool onewire_result_ready()
{
	return _1wire_state.ready;
}

bool* onewire_response()
{
	return _1wire_state.data;
}

void onewire_reset()
{
	_1wire_state.need_reset = true;
}

uint8_t onewire_crc8(uint8_t* data, uint8_t len)
{
    uint8_t crc = 0x00;

    while (len--) {
        crc = ONEWIRE_CRC8_TABLE[crc ^ *data];
        data++;
    }

    return crc;
}

void _configure_timer(uint16_t target_usec)
{
	_1WIRE_TIM.Instance->CNT = 0;
	_1WIRE_TIM.Instance->ARR = target_usec;
}

void _onewire_state_reset()
{
	memset((void*)&_1wire_state, 0, sizeof(_1wire_state));
	_1wire_state.fsm = _fsm_onewire_init;
}

bool _onewire_busy()
{
	return _1wire_state.count || _1wire_state.need_count;
}

void _fsm_onewire_init()
{
	_onewire_state_reset();

	SET_BUS();
	_configure_timer(IDLE_DELAY_US);

	_1wire_state.fsm = _fsm_onewire_idle;
}

void _fsm_onewire_idle()
{
	if (_1wire_state.count == 1) {
		SET_BUS();
		_configure_timer(WAIT_DATA_DELAY_US);
		_1wire_state.fsm = _fsm_onewire_send_bit_begin;
	} else if (_1wire_state.count > 0) {
		SET_BUS();
		_configure_timer(START_DELAY_US);
		_1wire_state.fsm = _fsm_onewire_write_reset_start;
	} else if (_1wire_state.need_count > 0) {
		SET_BUS();
		_configure_timer(START_DELAY_US);
		_1wire_state.fsm = _fsm_onewire_read_bit_begin;
	}
}

void _fsm_onewire_error()
{
	_configure_timer(IDLE_DELAY_US);

	_onewire_state_reset();
	// TODO: error action

	_1wire_state.fsm = _fsm_onewire_end;
}

void _fsm_onewire_write_reset_start()
{
	_configure_timer(RESET_DELAY_US);
	RESET_BUS();

	_1wire_state.fsm = _fsm_onewire_write_reset_end;
}

void _fsm_onewire_write_reset_end()
{
	_configure_timer(RESET_WAIT_US);
	SET_BUS();

	_1wire_state.fsm = _fsm_onewire_wait_presense_start;
}

void _fsm_onewire_wait_presense_start()
{
	if (READ_BUS()) {
		_1wire_state.fsm = _fsm_onewire_wait_presense_end;
	} else {
		_1wire_state.fsm = _fsm_onewire_error;
	}

	_configure_timer(WAIT_PRESENSE_US);
}

void _fsm_onewire_wait_presense_end()
{
	if (READ_BUS()) {
		_1wire_state.fsm = _fsm_onewire_error;
	} else {
		_1wire_state.fsm = _fsm_onewire_send_bit_begin;
	}
	_configure_timer(WAIT_PRESENSE_END_US);
}

void _fsm_onewire_send_bit_begin()
{
	if (!_1wire_state.count) {
		_1wire_state.fsm = _fsm_onewire_send_iterate_bit;
		return;
	}

	_configure_timer(RESET_DATA_DELAY_US);
	RESET_BUS();

	_1wire_state.fsm = _fsm_onewire_send_bit_set;
}

void _fsm_onewire_send_bit_set()
{
	if (_1wire_state.data[_1wire_state.bit_idx]) {
		SET_BUS();
	} else {
		RESET_BUS();
	}
	_configure_timer(SLOT_DELAY_US);

	_1wire_state.fsm = _fsm_onewire_send_bit_end;
}

void _fsm_onewire_send_bit_end()
{
	_configure_timer(END_DATA_DELAY_US);
	SET_BUS();

	_1wire_state.fsm = _fsm_onewire_send_iterate_bit;
}

void _fsm_onewire_send_iterate_bit()
{
	_1wire_state.bit_idx++;
	if (_1wire_state.bit_idx < _1wire_state.count) {
		_1wire_state.fsm = _fsm_onewire_send_bit_begin;
	} else {
		_configure_timer(SLOT_DELAY_US);
		_1wire_state.bit_idx = 0;
		_1wire_state.fsm = _fsm_onewire_read_bit_begin;
	}
}

void _fsm_onewire_read_bit_begin()
{
	if (!_1wire_state.need_count) {
		_1wire_state.fsm = _fsm_onewire_read_iterate_bit;
		return;
	}

	_configure_timer(RESET_DATA_DELAY_US);
	RESET_BUS();

	_1wire_state.fsm = _fsm_onewire_read_bit_set;
}

void _fsm_onewire_read_bit_set()
{
	_configure_timer(WAIT_SLAVE_BIT_US);
	SET_BUS();

	_1wire_state.fsm = _fsm_onewire_read_bit_end;
}

void _fsm_onewire_read_bit_end()
{
	_1wire_state.data[_1wire_state.bit_idx] = READ_BUS();

	_configure_timer(SLOT_DELAY_US);

	_1wire_state.fsm = _fsm_onewire_read_iterate_bit;
}

void _fsm_onewire_read_iterate_bit()
{
	_1wire_state.bit_idx++;
	if (_1wire_state.bit_idx < _1wire_state.need_count) {
		_1wire_state.fsm = _fsm_onewire_read_bit_begin;
	} else {
		memset((void*)&_1wire_state.data[_1wire_state.need_count], 0, sizeof(_1wire_state.data) - _1wire_state.need_count);
		_1wire_state.ready      = true;
		_1wire_state.count      = 0;
		_1wire_state.need_count = 0;
		_1wire_state.fsm        = _fsm_onewire_end;
	}
	_configure_timer(WAIT_DATA_DELAY_US);
}

void _fsm_onewire_end()
{
	_1wire_state.bit_idx = 0;
	_1wire_state.fsm     = _fsm_onewire_idle;
}
