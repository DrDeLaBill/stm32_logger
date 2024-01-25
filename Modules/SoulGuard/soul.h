/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef __SOUL_H
#define __SOUL_H


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>

#include "utils.h"
#include "hal_defs.h"


typedef enum _FATAL_ERROR {
	INTERNAL_ERROR = 1,
	STACK_ERROR,
	RAM_ERROR,
	MODBUS_ERROR,
	MEMORY_ERROR,
//	RTC_ERROR, // TODO: this is not fatal
	SETTINGS_ERROR,

	/* Paste errors to the top */
	END_ERRORS
} FATAL_ERROR;


typedef struct _soul_t {
	uint8_t errors[__div_up(END_ERRORS-1, BITS_IN_BYTE)];
} soul_t;


bool is_error(FATAL_ERROR error);
void set_error(FATAL_ERROR error);
void reset_error(FATAL_ERROR error);


#ifdef __cplusplus
}
#endif


#endif
