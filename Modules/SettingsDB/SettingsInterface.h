/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _SETTINGS_INTERFACE_H_
#define _SETTINGS_INTERFACE_H_


#include <cstdint>

#include "settings.h"


struct SettingsInterface
{
public:
	static constexpr char TAG[] = "ISTG";

    typedef struct _info_t {
        uint8_t mb1_last_id;
        uint8_t mb1_new_id;
        uint8_t need_mb1_id_update;
    } info_t;

protected:
    static info_t info;

public:

    struct dv_type
    {
        static void set(uint64_t value, unsigned index = 0);
        static uint64_t get(unsigned index = 0);
        static unsigned index(unsigned) { return 0; }
    };
    struct sw_id
    {
        static void set(uint64_t value, unsigned index = 0);
        static uint64_t get(unsigned index = 0);
        static unsigned index(unsigned) { return 0; }
    };
    struct fw_id
    {
        static void set(uint64_t value, unsigned index = 0);
        static uint64_t get(unsigned index = 0);
        static unsigned index(unsigned) { return 0; }
    };
    struct cf_id
    {
        static void set(uint64_t value, unsigned index = 0);
        static uint64_t get(unsigned index = 0);
        static unsigned index(unsigned) { return 0; }
    };
    struct record_period
    {
        static void set(uint64_t value, unsigned index = 0);
        static uint64_t get(unsigned index = 0);
        static unsigned index(unsigned) { return 0; }
    };
    struct send_period
    {
        static void set(uint64_t value, unsigned index = 0);
        static uint64_t get(unsigned index = 0);
        static unsigned index(unsigned) { return 0; }
    };
    struct record_id
    {
        static void set(uint64_t value, unsigned index = 0);
        static uint64_t get(unsigned index = 0);
        static unsigned index(unsigned) { return 0; }
    };
    struct modbus1_status
    {
        static void set(uint64_t value, unsigned index = 0);
        static uint64_t get(unsigned index = 0);
        static unsigned index(unsigned index = 0);
    };
    struct modbus1_value_reg
    {
        static void set(uint64_t value, unsigned index = 0);
        static uint64_t get(unsigned index = 0);
        static unsigned index(unsigned index = 0);
    };
    struct modbus1_id_reg
    {
        static void set(uint64_t value, unsigned index = 0);
        static uint64_t get(unsigned index = 0);
        static unsigned index(unsigned index = 0);
    };
    struct _1wire_address
    {
        static void set(uint64_t value, unsigned index = 0);
        static uint64_t get(unsigned index = 0);
        static unsigned index(unsigned index = 0);
    };
    struct mb1_last_id
    {
        static void set(uint8_t value, unsigned index = 0);
        static uint8_t get(unsigned index = 0);
        static unsigned index(unsigned) { return 0; }
    };
    struct mb1_new_id
    {
        static void set(uint8_t value, unsigned index = 0);
        static uint8_t get(unsigned index = 0);
        static unsigned index(unsigned) { return 0; }
    };
    struct need_mb1_id_update
    {
        static void set(uint8_t value, unsigned index = 0);
        static uint8_t get(unsigned index = 0);
        static unsigned index(unsigned) { return 0; }
    };

};


#endif
