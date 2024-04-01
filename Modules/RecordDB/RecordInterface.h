#ifndef _RECORD_INTERFACE_H_
#define _RECORD_INTERFACE_H_


#include "Record.h"


struct RecordInterface
{
protected:
	static record_t record;

	static unsigned __get_index(unsigned index);

public:
    struct id
    {
        static void set(uint64_t value, unsigned index = 0);
        static uint64_t get(unsigned index = 0);
        static unsigned index(unsigned) { return 0; }
    };
    struct time
    {
        static void set(uint64_t value, unsigned index = 0);
        static uint64_t get(unsigned index = 0);
        static unsigned index(unsigned) { return 0; }
    };
    struct MODBUS1_ID
    {
        static void set(uint64_t value, unsigned index = 0);
        static uint64_t get(unsigned index = 0);
        static unsigned index(unsigned index = 0);
    };
    struct MODBUS1_value
    {
        static void set(uint64_t value, unsigned index = 0);
        static uint64_t get(unsigned index = 0);
        static unsigned index(unsigned index = 0);
    };
    struct _1WIRE_ADDR
    {
        static void set(uint64_t value, unsigned index = 0);
        static uint64_t get(unsigned index = 0);
        static unsigned index(unsigned index = 0);
    };
    struct _1WIRE_value
    {
        static void set(uint64_t value, unsigned index = 0);
        static uint64_t get(unsigned index = 0);
        static unsigned index(unsigned index = 0);
    };
};


#endif
