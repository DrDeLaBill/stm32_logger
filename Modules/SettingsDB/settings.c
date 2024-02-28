/* Copyright © 2023 Georgy E. All rights reserved. */

#include "settings.h"

#include <string.h>
#include <stdbool.h>

#include "log.h"
#include "clock.h"
#include "utils.h"
#include "hal_defs.h"


static const char SETTINGS_TAG[] = "STNG";

settings_t settings = {};

settings_info_t stngs_info = {
	.settings_initialized = false,
	.settings_saved       = false,
	.settings_updated     = false,
	.modbus1_status       = { 0 }
};


settings_t* settings_get()
{
	return &settings;
}

void settings_set(settings_t* other)
{
	memcpy((uint8_t*)&settings, (uint8_t*)other, sizeof(settings));
}

void settings_reset(settings_t* other)
{
	printTagLog(SETTINGS_TAG, "Reset settings");
	other->dv_type = DEVICE_TYPE;
	other->sw_id = SW_VERSION;
	other->fw_id = FW_VERSION;
	other->cf_id = CF_VERSION;
	other->record_period = DAY_MS;
	other->send_period = DAY_MS;
	other->record_id = 0;
	memset(other->modbus1_status, SETTINGS_SENSOR_EMPTY, sizeof(other->modbus1_status));
	memset(other->modbus1_value_reg, 0, sizeof(other->modbus1_value_reg));
	memset(other->modbus1_id_reg, 0, sizeof(other->modbus1_id_reg));
}

uint32_t settings_size()
{
	return sizeof(settings_t);
}

bool settings_check(settings_t* other)
{
	if (other->dv_type != DEVICE_TYPE) {
		return false;
	}
	if (other->sw_id != SW_VERSION) {
		return false;
	}
	if (other->fw_id != FW_VERSION) {
		return false;
	}

	return true;
}

void settings_show()
{
	RTC_TimeTypeDef time = {};
	clock_get_rtc_time(&time);

	printPretty("\n");
	printPretty("                    %02u:%02u:%02u\n", time.Hours, time.Minutes, time.Seconds);
	printPretty("####################SETTINGS####################\n");
	printPretty("Device type: %u\n", settings.dv_type);
	printPretty("Software v%u\n", settings.sw_id);
	printPretty("Firmware v%u\n", settings.fw_id);
	printPretty("Configuration ID: %lu\n", settings.cf_id);
	printPretty("Record period: %lu msec\n", settings.record_period);
	printPretty("Send period: %lu msec\n", settings.send_period);
	printPretty("Record id: %lu\n", settings.record_id);
	printPretty("\n");
    printPretty("ID\tSTATUS\tVALREG\tIDREG\n");
    unsigned counter = 0;
    for (unsigned i = 0; i < __arr_len(settings.modbus1_id_reg); i++) {
    	if (settings.modbus1_status[i] == SETTINGS_SENSOR_EMPTY) {
    		continue;
    	}
    	printPretty("%03u\t", i + 1);
    	switch(settings.modbus1_status[i]) {
    	case SETTINGS_SENSOR_THERMAL:
        	gprint("%s\t", "THERMAL");
        	break;
    	case SETTINGS_SENSOR_HUMIDITY:
    		gprint("%s\t", "HUMIDTY");
        	break;
    	case SETTINGS_SENSOR_ANOTHER:
    		gprint("%s\t", "ANOTHER");
        	break;
    	case SETTINGS_SENSOR_ERROR:
    		gprint("%s\t", "ERROR");
        	break;
    	default:
        	printPretty("%s\t", "UNKNWN");
        	break;
    	};
    	gprint("%03u\t", settings.modbus1_value_reg[i]);
    	gprint("%03u\n", settings.modbus1_id_reg[i]);
    	counter++;
    }
    if (!counter) {
    	printPretty("------------EMPTY------------\n");
    }
    printPretty("####################SETTINGS####################\n\n");
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
