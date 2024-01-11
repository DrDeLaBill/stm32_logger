/* Copyright © 2024 Georgy E. All rights reserved. */

#pragma once


#include <stdint.h>

#include "main.h"
#include "StorageAT.h"
#include "RecordStatus.h"


class Record
{
public:
	typedef struct __attribute__((packed)) _register_t {
		uint8_t  ID;
		uint16_t value;
	} register_t;

	static const unsigned META_SIZE = sizeof(uint32_t) * 2;
	static const unsigned REGS_SIZE_MAX = sizeof(uint16_t) * MODBUS_SENS_COUNT;
    static const unsigned SZ_MAX = META_SIZE + REGS_SIZE_MAX;
    typedef struct __attribute__((packed)) _reocrd_t {
        uint32_t   id;                  // Record ID
        uint32_t   time;                // Record time
        register_t regs[REGS_SIZE_MAX]; // Record registers values
    } record_t;

    record_t record;

    Record(uint32_t recordId);

    register_t& operator[](const unsigned i);

    RecordStatus save();
    RecordStatus load();
    RecordStatus loadNext();

    unsigned size();
    unsigned count();

private:
    static const char* TAG;
    static const char* PREFIX;

	uint32_t m_recordId;
	uint16_t m_regsCount;
};
