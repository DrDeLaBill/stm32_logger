/* Copyright © 2023 Georgy E. All rights reserved. */

#include "settings.h"

#include <string.h>
#include <stdbool.h>

#include "log.h"


settings_t settings = {};


uint8_t* settings_get()
{
	return (uint8_t*)&settings;
}

void settings_set(uint8_t* other)
{
	memcpy((uint8_t*)&settings, other, sizeof(settings));
}

void settings_reset()
{
	settings.dv_id = DEVICE_TYPE;
	settings.sw_id = SW_VERSION;
	settings.fw_id = FW_VERSION;
	settings.cf_id = CF_VERSION;
	settings.record_period = 0;
	settings.send_period = 0;
	memset(settings.modbus1_status, SETTINGS_SENSOR_EMPTY, sizeof(settings.modbus1_status));
	memset(settings.modbus1_reg_value, 0, sizeof(settings.modbus1_reg_value));
	memset(settings.modbus1_reg_id, 0, sizeof(settings.modbus1_reg_id));
}

uint32_t settings_size()
{
	return sizeof(settings);
}

bool settings_check(uint8_t* other)
{
	settings_t* tmp_settings = (settings_t*)other;
	if (tmp_settings->dv_id != DEVICE_TYPE) {
		return false;
	}
	if (tmp_settings->sw_id != SW_VERSION) {
		return false;
	}
	if (tmp_settings->fw_id != FW_VERSION) {
		return false;
	}

	return true;
}

void settings_show()
{
    printLog("---------------settings show plug--------------\n");
}
