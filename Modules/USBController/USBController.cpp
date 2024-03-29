/* Copyright © 2024 Georgy E. All rights reserved. */

#include "USBController.h"

#include "usbd_customhid.h"

#include "hid_defs.h"
#include "variables.h"


void USBController::proccess()
{
	uint8_t counter = 0;
	report_pack_t request = {};
	request.report_id = receive_buf[counter++];

	if (request.report_id != HID_INPUT_REPORT_ID) {
		return;
	}

	extern USBD_HandleTypeDef hUsbDeviceFS;

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

	report_pack_t response = {};
	response.report_id         = HID_OUTPUT_REPORT_ID;
	response.characteristic_id = 0;
	response.index             = request.index;
	memcpy(response.tag, REPORT_PREFIX, sizeof(response.tag));

	if (request.characteristic_id == HID_GETTER_ID) {
		response.characteristic_id = utl::deserialize<uint16_t>(request.data)[0];
		hid_controller.getValue(response.characteristic_id, response.data, response.index);
	} else {
		response.characteristic_id = request.characteristic_id;
		hid_controller.setValue(request.characteristic_id, request.data, request.index);
		hid_controller.getValue(response.characteristic_id, response.data, response.index);
	}

	USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, reinterpret_cast<uint8_t*>(&response), sizeof(response));
	clear();
}

void USBController::clear()
{
	memset(receive_buf, 0, sizeof(receive_buf));
}
