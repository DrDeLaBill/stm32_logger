/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _DEVICE_INFO_H_
#define _DEVICE_INFO_H_


#include <cstdint>

#include "settings.h"


class DeviceInfo
{
private:
	static int16_t m_modbus1_value[MODBUS_SENS_COUNT];
	static int16_t m_1wire_value[MODBUS_SENS_COUNT];

public:
    typedef struct _info_t {
        uint64_t time;
        uint64_t min_id;
        uint64_t max_id;
        uint64_t current_id;
        uint64_t current_mbodbus1_count;
        uint64_t current_1wire_count;
        uint8_t  record_loaded;
    } info_t;

protected:
    static info_t info;

public:
    struct time
    {
        static void set(uint64_t value, unsigned index = 0);
        static uint64_t get(unsigned index = 0);
        static unsigned index(unsigned) { return 0; }
    };
    struct min_id
    {
    	static void set(uint64_t value, unsigned index = 0);
    	static uint64_t get(unsigned index = 0);
    	static unsigned index(unsigned) { return 0; }
    };
    struct max_id
    {
    	static void set(uint64_t value, unsigned index = 0);
    	static uint64_t get(unsigned index = 0);
    	static unsigned index(unsigned) { return 0; }
    };
    struct current_id
    {
    	static void set(uint64_t value, unsigned index = 0);
    	static uint64_t get(unsigned index = 0);
    	static unsigned index(unsigned) { return 0; }
    };
    struct current_mbodbus1_count
    {
    	static void set(uint64_t value, unsigned index = 0);
    	static uint64_t get(unsigned index = 0);
    	static unsigned index(unsigned) { return 0; }
    };
    struct current_1wire_count
    {
    	static void set(uint64_t value, unsigned index = 0);
    	static uint64_t get(unsigned index = 0);
    	static unsigned index(unsigned) { return 0; }
    };
    struct record_loaded
    {
    	static void set(uint64_t value, unsigned index = 0);
    	static uint64_t get(unsigned index = 0);
    	static unsigned index(unsigned) { return 0; }
    };
    struct need_registrate_1wire
    {
    	static void set(uint64_t value, unsigned index = 0);
    	static uint64_t get(unsigned index = 0);
    	static unsigned index(unsigned) { return 0; }
    };
    struct modbus1_last_value
    {
        static void set(int16_t value, unsigned index = 0);
        static int16_t get(unsigned index = 0);
        static unsigned index(unsigned index = 0);
    };
    struct _1wire_last_value
    {
        static void set(int16_t value, unsigned index = 0);
        static int16_t get(unsigned index = 0);
        static unsigned index(unsigned index = 0);
    };
    struct _1wire_registrate
    {
        static void set(uint64_t value, unsigned index = 0);
        static uint64_t get(unsigned index = 0);
        static unsigned index(unsigned index = 0);
    };

};

#endif // DEVICEINFO_H
