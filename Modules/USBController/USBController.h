/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _USB_CONTROLLER_H_
#define _USB_CONTROLLER_H_


#include <cstdint>

#include "usbd_customhid.h"

#include "log.h"
#include "utils.h"
#include "hid_defs.h"

#include "Timer.h"
#include "variables.h"
#include "DeviceInfo.h"
#include "HIDController.h"
#include "HIDTableWorker.h"
#include "RecordInterface.h"
#include "SettingsInterface.h"


#define USB_CONTROLLER_BEDUG (true)


struct USBController
{
private:
	static constexpr char TAG[] = "USBc";

	static utl::Timer timer;
	static bool updated;

    using settings_controller_table_t = HIDTable<
        HIDTuple<uint16_t, SettingsInterface::dv_type>,
        HIDTuple<uint8_t,  SettingsInterface::sw_id>,
        HIDTuple<uint8_t,  SettingsInterface::fw_id>,
        HIDTuple<uint32_t, SettingsInterface::cf_id>,
        HIDTuple<uint32_t, SettingsInterface::record_period>, // TODO: max time month
        HIDTuple<uint32_t, SettingsInterface::send_period>,
        HIDTuple<uint32_t, SettingsInterface::record_id>,
        HIDTuple<uint16_t, SettingsInterface::modbus1_status,    __arr_len(settings_t::modbus1_status)>,
        HIDTuple<uint16_t, SettingsInterface::modbus1_value_reg, __arr_len(settings_t::modbus1_value_reg)>,
        HIDTuple<uint16_t, SettingsInterface::modbus1_id_reg,    __arr_len(settings_t::modbus1_id_reg)>
    >;
    using settings_controller_t = HIDTableWorker<settings_controller_table_t, HID_FIRST_KEY>;
    static settings_controller_t settings_controller;

    using info_controller_table_t = HIDTable<
		HIDTuple<uint32_t, DeviceInfo::time>,
		HIDTuple<uint32_t, DeviceInfo::min_id>,
		HIDTuple<uint32_t, DeviceInfo::max_id>,
		HIDTuple<uint32_t, DeviceInfo::current_id>,
		HIDTuple<uint32_t, DeviceInfo::current_count>,
		HIDTuple<uint8_t,  DeviceInfo::record_loaded>
    >;
    using info_controller_t = HIDTableWorker<info_controller_table_t, settings_controller_t::maxID() + 1>;
    static info_controller_t info_controller;

    using record_controller_table_t = HIDTable<
		HIDTuple<uint32_t, RecordInterface::id>,
		HIDTuple<uint32_t, RecordInterface::time>,
		HIDTuple<uint8_t,  RecordInterface::ID,    __arr_len(record_t::sens)>,
		HIDTuple<uint16_t, RecordInterface::value, __arr_len(record_t::sens)>
	>;
    using record_controller_t = HIDTableWorker<record_controller_table_t, info_controller_t::maxID() + 1>;
    static record_controller_t record_controller;

    void clear();

    template <class controller_t>
    void controllerProccess(controller_t* controller, report_pack_t& request)
    {
    	extern USBD_HandleTypeDef hUsbDeviceFS;

    	report_pack_t response     = {};
    	response.report_id         = HID_OUTPUT_REPORT_ID;
    	response.characteristic_id = 0;
    	response.index             = request.index;
    	memcpy(response.tag, REPORT_PREFIX, sizeof(response.tag));

    	if (request.characteristic_id == HID_GETTER_ID) {
    		response.characteristic_id = utl::deserialize<uint16_t>(request.data)[0];
    		response.index = controller->hid_table.getIndex(response.characteristic_id, response.index);
    	} else {
    		response.characteristic_id = request.characteristic_id;
    		controller->hid_table.setValue(request.characteristic_id, request.data, request.index);
    		updated = true;
    		timer.start();
    	}

    	controller->hid_table.getValue(response.characteristic_id, response.data, response.index);

    	USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, reinterpret_cast<uint8_t*>(&response), sizeof(response));
#if HID_TABLE_BEDUG
    	printTagLog(TAG, "USB host request:");
    	hid_report_show(&request);
    	printTagLog(TAG, "USB device response:");
    	hid_report_show(&response);
#endif
    	clear();
    }

public:
    void proccess();

    static bool connected();

};


#endif /* USBCONTROLLER_USBCONTROLLER_H_ */
