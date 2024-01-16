/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef _SETTINGS_H_
#define _SETTINGS_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>

#include "main.h"


#define DEVICE_TYPE ((uint16_t)0x0001)
#define SW_VERSION  ((uint8_t)0x03)
#define FW_VERSION  ((uint8_t)0x01)
#define CF_VERSION  ((uint8_t)0x01)


typedef enum _sensor_status_t {
	SETTINGS_SENSOR_EMPTY    = (uint16_t)0x0000,
	SETTINGS_SENSOR_THERMAL  = (uint16_t)0x0001,
	SETTINGS_SENSOR_HUMIDITY = (uint16_t)0x0002,
	SETTINGS_SENSOR_ANOTHER  = (uint16_t)0x4000,
	SETTINGS_SENSOR_ERROR    = (uint16_t)0x8000,
} sensor_status_t;


typedef struct __attribute__((packed)) _settings_t  {
	 // Device type
	uint16_t  dv_type;
	 // Software version
    uint8_t  sw_id;
    // Firmware version
    uint8_t  fw_id;
    // Configuration version
    uint32_t cf_id;
    // Log record period time in ms
	uint32_t record_period;
    // Send record period time in ms
	uint32_t send_period;
	 // Last sended record ID
	uint32_t record_id;

	// The ID sensor on the bus is the N-1 index in the arrays
	// MODBUS 1 sensors statuses
	uint16_t modbus1_status   [MODBUS_SENS_COUNT];
	// MODBUS 1 sensor register IDs for reading values
	uint16_t modbus1_value_reg[MODBUS_SENS_COUNT];
	// MODBUS 1 sensor register IDs for setting new sensor ids
	uint16_t modbus1_id_reg   [MODBUS_SENS_COUNT];
} settings_t;


extern settings_t settings;


typedef struct _settings_info_t {
	bool settings_initialized;
	bool settings_saved;
	bool settings_updated;
} settings_info_t;


/* copy settings to the target */
settings_t* settings_get();
/* copy settings from the target */
void settings_set(settings_t* other);
/* reset current settings to default */
void settings_reset(settings_t* other);

uint32_t settings_size();

bool settings_check(settings_t* other);

void settings_show();

bool is_settings_saved();
bool is_settings_updated();
bool is_settings_initialized();

void set_settings_initialized();
void set_settings_save_status(bool state);
void set_settings_update_status(bool state);


#ifdef __cplusplus
}
#endif


#endif
