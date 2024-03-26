/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _USB_CONTROLLER_H_
#define _USB_CONTROLLER_H_


#include <cstdint>

#include "log.h"
#include "utils.h"
#include "com_defs.h"

#include "Timer.h"
#include "variables.h"
#include "DeviceInfo.h"
#include "COMController.h"
#include "COMTableWorker.h"
#include "RecordInterface.h"
#include "SettingsInterface.h"


#define USB_CONTROLLER_BEDUG (true)


struct USBController
{
private:
	static constexpr char TAG[] = "USBc";

	static utl::Timer timer;
	static bool updated;

    using settings_controller_table_t = COMTable<
        COMTuple<uint16_t, SettingsInterface::dv_type>,
        COMTuple<uint8_t,  SettingsInterface::sw_id>,
        COMTuple<uint8_t,  SettingsInterface::fw_id>,
        COMTuple<uint32_t, SettingsInterface::cf_id>,
        COMTuple<uint32_t, SettingsInterface::record_period>, // TODO: max time month
        COMTuple<uint32_t, SettingsInterface::send_period>,
        COMTuple<uint32_t, SettingsInterface::record_id>,
        COMTuple<uint16_t, SettingsInterface::modbus1_status,    __arr_len(settings_t::modbus1_status)>,
        COMTuple<uint16_t, SettingsInterface::modbus1_value_reg, __arr_len(settings_t::modbus1_value_reg)>,
        COMTuple<uint16_t, SettingsInterface::modbus1_id_reg,    __arr_len(settings_t::modbus1_id_reg)>
    >;
    using settings_controller_t = COMTableWorker<settings_controller_table_t, COM_FIRST_KEY>;
    static settings_controller_t settings_controller;

    using info_controller_table_t = COMTable<
		COMTuple<uint32_t, DeviceInfo::time>,
		COMTuple<uint32_t, DeviceInfo::min_id>,
		COMTuple<uint32_t, DeviceInfo::max_id>,
		COMTuple<uint32_t, DeviceInfo::current_id>,
		COMTuple<uint32_t, DeviceInfo::current_count>,
		COMTuple<uint8_t,  DeviceInfo::record_loaded>,
        COMTuple<uint16_t, DeviceInfo::modbus1_value, MODBUS_SENS_COUNT>
    >;
    using info_controller_t = COMTableWorker<info_controller_table_t, settings_controller_t::maxID() + 1>;
    static info_controller_t info_controller;

    using record_controller_table_t = COMTable<
		COMTuple<uint32_t, RecordInterface::id>,
		COMTuple<uint32_t, RecordInterface::time>,
		COMTuple<uint8_t,  RecordInterface::ID,    __arr_len(record_t::sens)>,
		COMTuple<uint16_t, RecordInterface::value, __arr_len(record_t::sens)>
	>;
    using record_controller_t = COMTableWorker<record_controller_table_t, info_controller_t::maxID() + 1>;
    static record_controller_t record_controller;

    void clear();

    template <class controller_t>
    void controllerProccess(controller_t* controller, report_pack_t& request)
    {
    	report_pack_t response     = {};
    	response.characteristic_id = 0;
    	response.index             = request.index;

    	if (request.characteristic_id == COM_GETTER_ID) {
    		response.characteristic_id = utl::deserialize<uint16_t>(request.data)[0];
    		response.index = controller->hid_table.getIndex(response.characteristic_id, response.index);
    	} else {
    		response.characteristic_id = request.characteristic_id;
    		controller->hid_table.setValue(request.characteristic_id, request.data, request.index);
    		timer.start();
    	}

    	controller->hid_table.getValue(response.characteristic_id, response.data, response.index);

    	com_send_report(&response);

#if COM_TABLE_BEDUG
    	printTagLog(TAG, "USB host request:");
    	com_report_show(&request);
    	printTagLog(TAG, "USB device response:");
    	com_report_show(&response);
#endif
    	clear();
    }

public:
    void proccess();

    static bool connected();

};


#endif /* USBCONTROLLER_USBCONTROLLER_H_ */
