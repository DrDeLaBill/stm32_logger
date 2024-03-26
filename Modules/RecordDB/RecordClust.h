/* Copyright © 2024 Georgy E. All rights reserved. */

#pragma once


#include <stdint.h>

#include "main.h"
#include "bmacro.h"
#include "RecordType.h"
#include "CircleBuffer.h"


class RecordClust
{
public:
    static const unsigned META_SIZE = sizeof(uint8_t) * 2 + sizeof(uint16_t);
    struct __attribute__((packed)) record_clust_t {
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
        // Records counnt
        uint32_t count();
        // Records size
        uint32_t size();
    };

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

    struct cache_t {
    	record_clust_t cluster;
    	uint16_t recordSize;
    	uint32_t address;

    	cache_t();

    	cache_t(const cache_t& other);
    	cache_t& operator=(const cache_t& other);
        ~cache_t();
    };
    static utl::circle_buffer<RECORD_CACHED_COUNT, cache_t> m_cache;
    static uint32_t m_cacheAfterId;
    static bool m_cacheLoaded;

    bool checkCachedRecordCLuster();

#endif

};
