/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _RECORD_H_
#define _RECORD_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>

#include "gutils.h"
#include "settings.h"


#define RECORD_BEDUG (1)


typedef enum _record_status_t {
    RECORD_OK = 0,
    RECORD_ERROR,
} record_status_t;


typedef struct _reccord_t {
	uint32_t id;
	uint16_t version;

	uint16_t mb1_count;
	uint16_t _1w_count;

	uint8_t  mb1_id[__arr_len(settings.modbus1_status)];
	int16_t  mb1_value[__arr_len(settings.modbus1_status)];

	uint64_t _1w_id[__arr_len(settings._1wire_address)];
	int16_t  _1w_value[__arr_len(settings._1wire_address)];
} record_t;


record_status_t record_save(const record_t* record);

void record_show(const record_t* record);


#ifdef __cplusplus
}
#endif


#endif
