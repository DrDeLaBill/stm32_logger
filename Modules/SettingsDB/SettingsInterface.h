#ifndef _SETTINGS_INTERFACE_H_
#define _SETTINGS_INTERFACE_H_


#include "settings.h"


struct SettingsInterface
{
    struct dv_type           { uint16_t* operator()() { return &settings.dv_type; } };
    struct sw_id             { uint8_t*  operator()() { return &settings.sw_id; } };
    struct fw_id             { uint8_t*  operator()() { return &settings.fw_id; } };
    struct cf_id             { uint32_t* operator()() { return &settings.cf_id; } };
    struct record_period     { uint32_t* operator()() { return &settings.record_period; } };
    struct send_period       { uint32_t* operator()() { return &settings.send_period; } };
    struct record_id         { uint32_t* operator()() { return &settings.record_id; } };
    struct modbus1_status    { uint16_t* operator()() { return settings.modbus1_status; } };
    struct modbus1_value_reg { uint16_t* operator()() { return settings.modbus1_value_reg; } };
    struct modbus1_id_reg    { uint16_t* operator()() { return settings.modbus1_id_reg; } };
};


#endif
