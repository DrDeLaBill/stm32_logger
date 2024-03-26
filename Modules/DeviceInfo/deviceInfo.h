/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _DEVICE_INFO_H_
#define _DEVICE_INFO_H_


#include <cstdint>

#include "settings.h"


class DeviceInfo
{
private:
	static uint16_t m_modbus1_value[MODBUS_SENS_COUNT];

public:
    typedef struct _info_t {
        uint32_t time;
        uint32_t min_id;
        uint32_t max_id;
        uint32_t current_id;
        uint32_t current_count;
        uint8_t  record_loaded;
    } info_t;

protected:
    static info_t info;

public:
    struct time
    {
        static void set(uint32_t value, unsigned index = 0);
        static uint32_t get(unsigned index = 0);
        static unsigned index(unsigned) { return 0; }
    };
    struct min_id
    {
    	static void set(uint32_t value, unsigned index = 0);
    	static uint32_t get(unsigned index = 0);
    	static unsigned index(unsigned) { return 0; }
    };
    struct max_id
    {
    	static void set(uint32_t value, unsigned index = 0);
    	static uint32_t get(unsigned index = 0);
    	static unsigned index(unsigned) { return 0; }
    };
    struct current_id
    {
    	static void set(uint32_t value, unsigned index = 0);
    	static uint32_t get(unsigned index = 0);
    	static unsigned index(unsigned) { return 0; }
    };
    struct current_count
    {
    	static void set(uint32_t value, unsigned index = 0);
    	static uint32_t get(unsigned index = 0);
    	static unsigned index(unsigned) { return 0; }
    };
    struct record_loaded
    {
    	static void set(uint32_t value, unsigned index = 0);
    	static uint32_t get(unsigned index = 0);
    	static unsigned index(unsigned) { return 0; }
    };
    struct modbus1_value
    {
        static void set(uint32_t value, unsigned index = 0);
        static uint32_t get(unsigned index = 0);
        static unsigned index(unsigned index = 0);
    };

};

#endif // DEVICEINFO_H
