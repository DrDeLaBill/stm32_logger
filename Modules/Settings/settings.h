/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef __SETTINGS_H
#define __SETTINGS_H


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>

#include "main.h"


#define DEVICE_TYPE ((uint8_t)0x01)
#define CF_VERSION  ((uint8_t)0x01)
#define SW_VERSION  ((uint8_t)0x01)
#define FW_VERSION  ((uint8_t)0x01)


typedef enum _settings_sensor_status_t {
	SETTINGS_SENSOR_EMPTY    = (uint8_t)0b00000000,
	SETTINGS_SENSOR_THERMAL  = (uint8_t)0b00000001,
	SETTINGS_SENSOR_HUMIDITY = (uint8_t)0b00000010,
	SETTINGS_SENSOR_ANOTHER  = (uint8_t)0b01000000,
	SETTINGS_SENSOR_ERROR    = (uint8_t)0b10000000,
} settings_sensor_status_t;


typedef struct __attribute__((packed)) _settings_t  {
	uint8_t  dv_id; // Device type
    uint8_t  sw_id; // Software version
    uint8_t  fw_id; // Firmware version
    uint32_t cf_id; // Configuration version
	uint32_t record_period;
	uint32_t send_period;
	uint8_t  modbus1_status   [MODBUS_SENS_COUNT];
	uint16_t modbus1_value_reg[MODBUS_SENS_COUNT];
	uint16_t modbus1_id_reg   [MODBUS_SENS_COUNT];
} settings_t;


extern settings_t settings;


typedef struct _settings_info_t {
	bool settings_initialized;
	bool settings_saved;
	bool settings_updated;
} settings_info_t;


/* copy settings to the target */
uint8_t* settings_get();
/* copy settings from the target */
void settings_set(uint8_t* other);
/* reset current settings to default */
void settings_reset();

uint32_t settings_size();

bool settings_check(uint8_t* other);

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
