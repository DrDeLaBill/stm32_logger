/* Copyright © 2024 Georgy E. All rights reserved. */

#pragma once


#include <stdint.h>

#include "main.h"
#include "Record.h"
#include "RecordStatus.h"


class RecordClust
{
public:
	static const unsigned META_SIZE = sizeof(uint8_t) * 2 + sizeof(uint16_t);
    static const unsigned SZ_MAX = META_SIZE + Record::SZ_MAX;
    typedef struct __attribute__((packed)) _record_clust_t {
    	uint8_t          dv_id;                   // Device type
        uint8_t          sw_id;                   // Software version
        uint16_t         rcrd_size;               // Record structure unit size in cluster
        Record::record_t records[Record::SZ_MAX]; // Pointer to records in cluster
    } record_clust_t;

private:
    static const char* TAG;
    static const char* PREFIX;

    uint32_t m_recordId;
    uint32_t m_recordSize;
    uint32_t m_address;
	record_clust_t m_clust;

	bool validate();
	bool findExist();
	bool createNew();
	RecordStatus getMaxId(uint32_t *maxId);
	uint32_t getRecordsCountBySize(uint32_t structSize);

public:
	RecordClust(uint32_t recordId = 0, uint32_t recordSize = 0);

	Record::record_t& operator[](unsigned i);

	RecordStatus load();
	RecordStatus save(Record::record_t *record, uint32_t size);

	uint32_t count();
	uint32_t size();
};
