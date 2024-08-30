/* Copyright © 2024 Georgy E. All rights reserved. */

#include "app.h"

#include "usb.h"
#include "soul.h"
#include "clock.h"
#include "sensor.h"


static bool initialized = false;
app_info_t app_info = {0,};


void app_proccess()
{
	if (!usb_connected()) {
		initialized = false;
		return;
	}

	settings.dv_type = DEVICE_TYPE;
	settings.sw_id = SW_VERSION;
	settings.fw_id = FW_VERSION;

	if (!initialized) {
		initialized   = true;
		app_info.time = clock_get_timestamp();
	}
	uint32_t curr_time = clock_get_timestamp();
	if (__abs_dif(app_info.time, curr_time) > MINUTE_MS / SECOND_MS) {
		clock_save_seconds(app_info.time);
	}
	app_info.time = clock_get_timestamp();

	if (app_info.need_mb1_id_update && !app_line2_busy()) {
		set_status(NEED_MODBUS1);
		sensors_enable();
		sensor_send_new_id(
			app_info.mb1_last_id,
			app_info.mb1_new_id
		);
		app_info.need_mb1_id_update = 0;
	} else if (app_info.need_registrate_1wire && !app_line2_busy()) {
		set_status(NEED_1WIRE);
		sensors_enable();
	} else if (app_line2_busy()) {
		sensors_disable();
		reset_status(NEED_1WIRE);
		reset_status(NEED_MODBUS1);
	}
}

bool app_line2_busy()
{
	return is_status(NEED_1WIRE) || is_status(NEED_MODBUS1);
}
