/* Copyright © 2024 Georgy E. All rights reserved. */

#pragma once


#include <stdint.h>

#include "main.h"
#include "bmacro.h"
#include "record.h"
#include "CircleBuffer.h"


class RecordDB
{
protected:
    static constexpr char TAG[]    = "RCDB";
    static constexpr char PREFIX[] = "RDC";

    uint32_t       m_targetId;
    uint32_t       m_address;

    bool loadExist(bool validateSize);
    bool createNew();

public:
    record_clust_t clust;
    record_t       record;

    RecordDB(uint32_t targetId = 0);

    RecordDB(const RecordDB& other);
    RecordDB& operator=(const RecordDB& other);
    ~RecordDB();

    RecordStatus loadNext();
    RecordStatus load(bool validateSize = true);
    RecordStatus save();
    // TODO: RecordStatus erase(uint32_t address); with w25xx.h _flash_erase_data

    static uint32_t getMinID(const record_clust_t& clust);
    static uint32_t getMaxID(const record_clust_t& clust);
    static bool     hasID(const record_clust_t& clust, uint32_t ID);

    static RecordStatus getLastTime(uint32_t* time);
    static RecordStatus getMaxId(uint32_t* maxId);
    static RecordStatus getMinId(uint32_t* minId);
#if RECORD_ENABLE_CACHE
    static RecordStatus updateCache(uint32_t cacheAfterId);
#endif


private:
    void cacheRecord(unsigned index = 0);

    static RecordStatus deleteClust(uint32_t address);
    static RecordStatus preLoadClust(const uint32_t address, record_clust_t& clust);

#if RECORD_ENABLE_CACHE

    static utl::circle_buffer<RECORD_CACHED_COUNT, record_cache_t> m_cache;
    static uint32_t m_cacheAfterId;
    static bool m_cacheLoaded;
    static bool m_recordsExist;

    bool checkCachedRecordCLuster();

#endif

};
