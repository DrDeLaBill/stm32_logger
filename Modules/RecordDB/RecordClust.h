/* Copyright © 2024 Georgy E. All rights reserved. */

#pragma once


#include <stdint.h>

#include "main.h"
#include "bmacro.h"
#include "RecordType.h"


class RecordClust
{
public:
    static const unsigned META_SIZE = sizeof(uint8_t) * 2 + sizeof(uint16_t);
    typedef struct __attribute__((packed)) _record_clust_t {
    	// Device type
        uint8_t  dv_type;
        // Software version
        uint8_t  sw_id;
        // Record structure unit size in cluster
        uint16_t rcrd_size;
        // Buffer for record/s
        uint8_t  records[RECORD_SIZE_MAX];

        record_t& operator[](unsigned i);

        uint32_t minID();
        uint32_t maxID();
        bool hasID(uint32_t ID);
        uint32_t count();
        uint32_t size();
    } record_clust_t;

protected:
    static constexpr char TAG[] = "RDC";
    static constexpr char PREFIX[] = "RDC";

    uint32_t m_recordId;
    uint16_t m_recordSize;
    uint32_t m_address;
    record_clust_t m_clust;

    bool loadExist(bool validateSize);
    bool createNew();

    static uint32_t getCountByRecordSize(uint32_t recordSize);
    static bool validate(record_clust_t* clust);

public:
    RecordClust(uint32_t recordId = 0, uint16_t recordSize = 0);

    record_t& operator[](unsigned i);

    RecordStatus load(bool validateSize = true);
    RecordStatus save(record_t *record, uint32_t size);
    // TODO: RecordStatus erase(uint32_t address); with w25xx.h _flash_erase_data

    uint32_t records_count();
    uint32_t record_size();
    uint32_t structure_size();
    void show();

    static RecordStatus getLastTime(uint32_t* time);
    static RecordStatus getMaxId(uint32_t* maxId);
    static RecordStatus getMinId(uint32_t* minId);
#if RECORD_ENABLE_CACHE
    static RecordStatus updateCache(uint32_t cacheAfterId);
#endif


private:
    static RecordStatus deleteClust(uint32_t address);
    static RecordStatus preLoadClust(const uint32_t address, record_clust_t& clust);

#if RECORD_ENABLE_CACHE

    static record_clust_t m_cache[RECORD_CACHED_COUNT]; // TODO: use circle buffer
    static uint16_t m_cachedRecordSize[RECORD_CACHED_COUNT];
    static uint32_t m_cachedAddress[RECORD_CACHED_COUNT];
    static uint32_t m_cacheAfterId;

    bool checkCachedRecordCLuster();

#endif

};
