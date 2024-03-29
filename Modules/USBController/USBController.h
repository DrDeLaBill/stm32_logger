/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _USBCONTROLLER_H_
#define _USBCONTROLLER_H_


#include <cstdint>

#include "HIDController.h"
#include "RecordInterface.h"
#include "SettingsInterface.h"


struct USBController
{
private:
    using hid_table_t = HIDTable<
        HIDTuple<uint16_t, SettingsInterface::dv_type>,
        HIDTuple<uint8_t,  SettingsInterface::sw_id>,
        HIDTuple<uint8_t,  SettingsInterface::fw_id>,
        HIDTuple<uint32_t, SettingsInterface::cf_id>,
        HIDTuple<uint32_t, SettingsInterface::record_period>,
        HIDTuple<uint32_t, SettingsInterface::send_period>,
        HIDTuple<uint32_t, SettingsInterface::record_id>,
        HIDTuple<uint16_t, SettingsInterface::modbus1_status,    __arr_len(settings_t::modbus1_status)>,
        HIDTuple<uint16_t, SettingsInterface::modbus1_value_reg, __arr_len(settings_t::modbus1_value_reg)>,
        HIDTuple<uint16_t, SettingsInterface::modbus1_id_reg,    __arr_len(settings_t::modbus1_id_reg)>
//        HIDTuple<uint32_t, RecordInterface::id>,
//        HIDTuple<uint32_t, RecordInterface::time>
//        HIDTuple<uint8_t,  DeviceRecord::IDs>,
//        HIDTuple<uint16_t, DeviceRecord::values> // TODO: ids and values for HID
    >;

    HIDController<hid_table_t> hid_controller;

    void clear();

public:
    void proccess();

};


#endif /* USBCONTROLLER_USBCONTROLLER_H_ */
