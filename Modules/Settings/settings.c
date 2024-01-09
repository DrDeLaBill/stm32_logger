/* Copyright © 2023 Georgy E. All rights reserved. */

#include "settings.h"

#include <string.h>
#include <stdbool.h>

#include "log.h"
#include "utils.h"


settings_t settings = {};

settings_info_t stngs_info = {
	.settings_initialized = false,
	.settings_saved       = true,
	.settings_updated     = false
};


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
	memset(settings.modbus1_value_reg, 0, sizeof(settings.modbus1_value_reg));
	memset(settings.modbus1_id_reg, 0, sizeof(settings.modbus1_id_reg));
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
    printLog(
		"\n\n####################SETTINGS####################\n"
		"Device ID: %u\n"
		"Software v%u\n"
		"Firmware v%u\n"
		"Configuration ID: %lu\n"
		"Record period: %lu sec\n"
		"Send period: %lu sec\n"
		"\n",
		settings.dv_id,
		settings.sw_id,
		settings.fw_id,
		settings.cf_id,
		settings.record_period,
		settings.send_period
    );
    printLog("ID\tSTATUS\tVALREG\tIDREG\n");
    for (unsigned i = 0; i < __arr_len(settings.modbus1_id_reg); i++) {
    	printLog("%03u\t", i);
    	switch(settings.modbus1_status[i]) {
    	case SETTINGS_SENSOR_EMPTY:
        	printLog("%s\t", "EMPTY");
        	break;
    	case SETTINGS_SENSOR_THERMAL:
        	printLog("%s\t", "THERMAL");
        	break;
    	case SETTINGS_SENSOR_HUMIDITY:
        	printLog("%s\t", "HUMIDTY");
        	break;
    	case SETTINGS_SENSOR_ANOTHER:
        	printLog("%s\t", "ANOTHER");
        	break;
    	case SETTINGS_SENSOR_ERROR:
        	printLog("%s\t", "ERROR");
        	break;
    	default:
        	printLog("%s\t", "UNKNWN");
        	break;
    	};
    	printLog("%03u\t", settings.modbus1_value_reg[i]);
    	printLog("%03u\n", settings.modbus1_id_reg[i]);
    }
    printLog("####################SETTINGS####################\n\n");
}

bool is_settings_saved()
{
	return stngs_info.settings_saved;
}

bool is_settings_updated()
{
	return stngs_info.settings_updated;
}

bool is_settings_initialized()
{
	return stngs_info.settings_initialized;
}

void set_settings_initialized()
{
	stngs_info.settings_initialized = true;
}

void set_settings_save_status(bool state)
{
	if (state) {
		stngs_info.settings_updated = false;
	}
	stngs_info.settings_saved = state;
}

void set_settings_update_status(bool state)
{
	if (state) {
		stngs_info.settings_saved = false;
	}
	stngs_info.settings_updated = state;
}
