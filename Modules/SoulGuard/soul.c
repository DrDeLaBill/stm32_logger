/* Copyright © 2023 Georgy E. All rights reserved. */

#include "soul.h"

#include <stdint.h>
#include <stdbool.h>


static soul_t soul = {
	.errors = { 0 }
};


bool is_error(FATAL_ERROR error)
{
	uint8_t err_num = (uint8_t)(error) - 1;
	return (bool)(
		(
			soul.errors[err_num / BITS_IN_BYTE] >>
			(err_num % BITS_IN_BYTE)
		) & 0x01
	);
}

void set_error(FATAL_ERROR error)
{
	uint8_t err_num = (uint8_t)(error) - 1;
	soul.errors[err_num / BITS_IN_BYTE] |= (0x01 << (err_num % BITS_IN_BYTE));
}

void reset_error(FATAL_ERROR error)
{
	uint8_t err_num = (uint8_t)(error) - 1;
	soul.errors[err_num / BITS_IN_BYTE] &= ~(0x01 << (err_num % BITS_IN_BYTE));
}
