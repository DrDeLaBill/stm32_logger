/* Copyright © 2024 Georgy E. All rights reserved. */

#include "app.h"

#include "usb.h"
#include "soul.h"
#include "clock.h"
#include "sensor.h"


app_info_t app_info = {0,};


void app_proccess()
{
	if (!usb_connected()) {
		return;
	}

	uint32_t curr_time = clock_get_timestamp();
	if (__abs_dif(app_info.time, curr_time) > MINUTE_MS / SECOND_MS) {
		clock_save_seconds(curr_time);
	}
	app_info.time = clock_get_timestamp();

	if (app_info.need_mb1_id_update) {
		if (is_status(NEED_ENABLE_SENSORS)) {
			return;
		}
		set_status(NEED_ENABLE_SENSORS);
		sensors_enable();
		sensor_send_new_id(
			app_info.mb1_last_id,
			app_info.mb1_new_id
		);
		app_info.need_mb1_id_update = 0;
		sensors_disable();
		reset_status(NEED_ENABLE_SENSORS);
	}
}
