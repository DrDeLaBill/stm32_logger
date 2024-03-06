/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _SETTINGS_INTERFACE_H_
#define _SETTINGS_INTERFACE_H_


#include "settings.h"


struct SettingsInterface
{
	static constexpr char TAG[] = "ISTG";

    struct dv_type
    {
        void set(uint32_t value, unsigned index = 0);
        uint32_t get(unsigned index = 0);
        unsigned index(unsigned) { return 0; }
    };
    struct sw_id
    {
        void set(uint32_t value, unsigned index = 0);
        uint32_t get(unsigned index = 0);
        unsigned index(unsigned) { return 0; }
    };
    struct fw_id
    {
        void set(uint32_t value, unsigned index = 0);
        uint32_t get(unsigned index = 0);
        unsigned index(unsigned) { return 0; }
    };
    struct cf_id
    {
        void set(uint32_t value, unsigned index = 0);
        uint32_t get(unsigned index = 0);
        unsigned index(unsigned) { return 0; }
    };
    struct record_period
    {
        void set(uint32_t value, unsigned index = 0);
        uint32_t get(unsigned index = 0);
        unsigned index(unsigned) { return 0; }
    };
    struct send_period
    {
        void set(uint32_t value, unsigned index = 0);
        uint32_t get(unsigned index = 0);
        unsigned index(unsigned) { return 0; }
    };
    struct record_id
    {
        void set(uint32_t value, unsigned index = 0);
        uint32_t get(unsigned index = 0);
        unsigned index(unsigned) { return 0; }
    };
    struct modbus1_status
    {
        void set(uint32_t value, unsigned index = 0);
        uint32_t get(unsigned index = 0);
        unsigned index(unsigned index = 0);
    };
    struct modbus1_value_reg
    {
        void set(uint32_t value, unsigned index = 0);
        uint32_t get(unsigned index = 0);
        unsigned index(unsigned index = 0);
    };
    struct modbus1_id_reg
    {
        void set(uint32_t value, unsigned index = 0);
        uint32_t get(unsigned index = 0);
        unsigned index(unsigned index = 0);
    };
    struct time
    {
        void set(uint32_t value, unsigned index = 0);
        uint32_t get(unsigned index = 0);
        unsigned index(unsigned index = 0) { return 0; }
    };
};


#endif
