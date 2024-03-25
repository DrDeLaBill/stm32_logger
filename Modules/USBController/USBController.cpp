/* Copyright © 2024 Georgy E. All rights reserved. */

#include "USBController.h"

#include "usbd_cdc_if.h"

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

	if (updated && !timer.wait()) {
		set_settings_update_status(true);
		updated = false;
	}

	request.flag = UserRxBufferFS[counter];
	if (request.flag != COM_SEND_FLAG) {
		return;
	}
	counter++;

	request.characteristic_id = utl::deserialize<uint16_t>(&UserRxBufferFS[counter])[0];
	counter += sizeof(uint16_t);

	request.index = UserRxBufferFS[counter++];

	memcpy(request.data, &UserRxBufferFS[counter], sizeof(request.data));
	counter += sizeof(request.data);

	memcpy(&request.crc, &UserRxBufferFS[counter], sizeof(request.crc));
	counter += sizeof(request.crc);

	if (!request.crc) {
		return;
	}

	if (request.crc != com_get_crc(&request)) {
		clear();
		return;
	}

	uint16_t characteristic_id = request.characteristic_id;
	if (characteristic_id == COM_GETTER_ID) {
		characteristic_id = utl::deserialize<uint16_t>(request.data)[0];
	}

	if (characteristic_id <= settings_controller_t::maxID()) {
#if COM_TABLE_BEDUG
		printTagLog(TAG, "STNG ID: %u", characteristic_id);
#endif
		controllerProccess<settings_controller_t>(&settings_controller, request);
		updated = (request.characteristic_id == COM_GETTER_ID ? updated : true);
		return;
	}

	if (characteristic_id <= info_controller_t::maxID()) {
#if COM_TABLE_BEDUG
		printTagLog(TAG, "INFO ID: %u", characteristic_id);
#endif
		controllerProccess<info_controller_t>(&info_controller, request);
		return;
	}

	if (request.characteristic_id == COM_GETTER_ID &&
		characteristic_id <= record_controller_t::maxID()
	) {
#if COM_TABLE_BEDUG
		printTagLog(TAG, "RCRD ID: %u", characteristic_id);
#endif
		controllerProccess<record_controller_t>(&record_controller, request);
		return;
	}

	BEDUG_ASSERT(characteristic_id <= record_controller.maxID(), "HID characteristic ID is out of range");
}

void USBController::clear()
{
	memset(UserRxBufferFS, 0, sizeof(UserRxBufferFS));
}

bool USBController::connected()
{
	return HAL_GPIO_ReadPin(WKUP_GPIO_Port, WKUP_Pin);
}
