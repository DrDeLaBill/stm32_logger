/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _APP_H_
#define _APP_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>

#include "gutils.h"
#include "settings.h"


typedef struct _app_info_t {
	uint8_t mb1_last_id;
	uint8_t mb1_new_id;
	uint8_t need_mb1_id_update;
	uint32_t time;
	uint8_t need_registrate_1wire;
	int16_t modbus1_last_value[__arr_len(settings.modbus1_status)];
	int16_t _1wire_last_value[__arr_len(settings._1wire_address)];
	uint64_t _1wire_registrate[__arr_len(settings._1wire_address)];
} app_info_t;


extern app_info_t app_info;


void app_proccess();


#ifdef __cplusplus
}
#endif


#endif
