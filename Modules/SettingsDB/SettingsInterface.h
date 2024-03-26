/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _SETTINGS_INTERFACE_H_
#define _SETTINGS_INTERFACE_H_


#include "settings.h"


struct SettingsInterface
{
	static constexpr char TAG[] = "ISTG";

    struct dv_type
    {
        static void set(uint32_t value, unsigned index = 0);
        static uint32_t get(unsigned index = 0);
        static unsigned index(unsigned) { return 0; }
    };
    struct sw_id
    {
        static void set(uint32_t value, unsigned index = 0);
        static uint32_t get(unsigned index = 0);
        static unsigned index(unsigned) { return 0; }
    };
    struct fw_id
    {
        static void set(uint32_t value, unsigned index = 0);
        static uint32_t get(unsigned index = 0);
        static unsigned index(unsigned) { return 0; }
    };
    struct cf_id
    {
        static void set(uint32_t value, unsigned index = 0);
        static uint32_t get(unsigned index = 0);
        static unsigned index(unsigned) { return 0; }
    };
    struct record_period
    {
        static void set(uint32_t value, unsigned index = 0);
        static uint32_t get(unsigned index = 0);
        static unsigned index(unsigned) { return 0; }
    };
    struct send_period
    {
        static void set(uint32_t value, unsigned index = 0);
        static uint32_t get(unsigned index = 0);
        static unsigned index(unsigned) { return 0; }
    };
    struct record_id
    {
        static void set(uint32_t value, unsigned index = 0);
        static uint32_t get(unsigned index = 0);
        static unsigned index(unsigned) { return 0; }
    };
    struct modbus1_status
    {
        static void set(uint32_t value, unsigned index = 0);
        static uint32_t get(unsigned index = 0);
        static unsigned index(unsigned index = 0);
    };
    struct modbus1_value_reg
    {
        static void set(uint32_t value, unsigned index = 0);
        static uint32_t get(unsigned index = 0);
        static unsigned index(unsigned index = 0);
    };
    struct modbus1_id_reg
    {
        static void set(uint32_t value, unsigned index = 0);
        static uint32_t get(unsigned index = 0);
        static unsigned index(unsigned index = 0);
    };

};


#endif
