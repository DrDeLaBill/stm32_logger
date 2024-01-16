/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _SENSOR_H_
#define _SENSOR_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>

#include "modbus_rtu_master.h"


#define SENSOR_BEDUG       (true)

#define SENSOR_ERROR_VALUE ((uint16_t)0xFFFF)


typedef struct _sensor_info_t {
	bool modbus_initialized;
	bool modbus_timeout;

	uint8_t modbus_errors_count;
} sensor_info_t;


extern sensor_info_t sensor_info;


void sensors_init(void (*response_packet_handler) (modbus_response_t*));
uint8_t sensors_count();
void sensor_timeout();
void sensor_request_value(uint8_t index);
void sensor_send_new_id(uint8_t old_id, uint8_t new_id);


#ifdef __cplusplus
}
#endif


#endif
