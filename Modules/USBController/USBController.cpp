/* Copyright © 2024 Georgy E. All rights reserved. */

#include "USBController.h"

#include "CodeStopwatch.h"


utl::Timer USBController::timer(GENERAL_TIMEOUT_MS);
bool USBController::updated = false;

USBController::settings_controller_t USBController::settings_controller;
USBController::info_controller_t USBController::info_controller;
USBController::record_controller_t USBController::record_controller;


void USBController::proccess()
{
	utl::CodeStopwatch stopwatch(TAG, GENERAL_TIMEOUT_MS);

	uint8_t counter = 0;
	report_pack_t request = {};
	request.report_id = receive_buf[counter++];

	if (updated && !timer.wait()) {
		set_settings_update_status(true);
		updated = false;
	}

	if (request.report_id != HID_INPUT_REPORT_ID) {
		return;
	}

	request.characteristic_id = utl::deserialize<uint16_t>(&receive_buf[counter])[0];
	counter += sizeof(uint16_t);

	request.index = receive_buf[counter++];

	memcpy(&request.data, &receive_buf[counter], sizeof(request.data));
	counter += sizeof(request.data);

	memcpy(&request.tag, &receive_buf[counter], sizeof(request.tag));
	counter += sizeof(request.tag);

	if (memcmp(request.tag, REPORT_PREFIX, sizeof(request.tag))) {
		clear();
		return;
	}

	uint16_t characteristic_id = request.characteristic_id;
	if (characteristic_id == HID_GETTER_ID) {
		characteristic_id = utl::deserialize<uint16_t>(request.data)[0];
	}

	if (characteristic_id <= settings_controller_t::maxID()) {
#if HID_TABLE_BEDUG
		printTagLog(TAG, "STNG ID: %u", characteristic_id);
#endif
		controllerProccess<settings_controller_t>(&settings_controller, request);
		updated = (request.characteristic_id == HID_GETTER_ID ? updated : true);
		return;
	}

	if (characteristic_id <= info_controller_t::maxID()) {
#if HID_TABLE_BEDUG
		printTagLog(TAG, "INFO ID: %u", characteristic_id);
#endif
		controllerProccess<info_controller_t>(&info_controller, request);
		return;
	}

	if (request.characteristic_id == HID_GETTER_ID &&
		characteristic_id <= record_controller_t::maxID()
	) {
#if HID_TABLE_BEDUG
		printTagLog(TAG, "RCRD ID: %u", characteristic_id);
#endif
		controllerProccess<record_controller_t>(&record_controller, request);
		return;
	}

	BEDUG_ASSERT(characteristic_id <= record_controller.maxID(), "HID characteristic ID is out of range");
}

void USBController::clear()
{
	memset(receive_buf, 0, sizeof(receive_buf));
}

bool USBController::connected()
{
	return HAL_GPIO_ReadPin(WKUP_GPIO_Port, WKUP_Pin);
}
