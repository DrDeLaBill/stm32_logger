/* Copyright © 2024 Georgy E. All rights reserved. */

#include "RecordDB.h"

#include <limits>
#include <cstring>
#include <stdint.h>

#include "log.h"
#include "soul.h"
#include "utils.h"
#include "clock.h"
#include "w25qxx.h"
#include "bmacro.h"
#include "settings.h"

#include "record.h"
#include "StorageAT.h"
#include "deviceInfo.h"
#include "StorageDriver.h"
#include "CodeStopwatch.h"


extern StorageAT* storage;


#if RECORD_ENABLE_CACHE

utl::circle_buffer<RECORD_CACHED_COUNT, record_cache_t> RecordDB::m_cache;
uint32_t RecordDB::m_cacheAfterId = 0;
bool RecordDB::m_cacheLoaded = false;
bool RecordDB::m_recordsExist = true;

#endif


RecordDB::RecordDB(uint32_t targetId):
	m_targetId(targetId), m_address(0), clust({}), record({})
{
	record_cluster_create(&clust);
	cacheRecord(0);
}

RecordDB::RecordDB(const RecordDB& other)
{
	this->m_targetId = other.m_targetId;
	this->m_address  = other.m_address;
	memcpy(
		reinterpret_cast<void*>(&(this->clust)),
		reinterpret_cast<void*>(const_cast<record_clust_t*>(&(other.clust))),
		sizeof(this->clust)
	);
	memcpy(
		reinterpret_cast<void*>(&record),
		reinterpret_cast<void*>(const_cast<record_t*>(&other.record)),
		sizeof(record)
	);
}

RecordDB& RecordDB::operator=(const RecordDB& other)
{
	this->m_targetId = other.m_targetId;
	this->m_address  = other.m_address;
	memcpy(
		reinterpret_cast<void*>(&(this->clust)),
		reinterpret_cast<void*>(const_cast<record_clust_t*>(&(other.clust))),
		sizeof(this->clust)
	);
	memcpy(
		reinterpret_cast<void*>(&record),
		reinterpret_cast<void*>(const_cast<record_t*>(&other.record)),
		sizeof(record)
	);

	return *this;
}

RecordDB::~RecordDB() {}

void RecordDB::cacheRecord(unsigned index)
{
	memcpy(
		reinterpret_cast<void*>(&record),
		reinterpret_cast<void*>(get_record_by_index(&clust, index)),
		sizeof(record)
	);
}

RecordStatus RecordDB::loadNext()
{
    m_targetId += 1;

    return this->load(false);
}

RecordStatus RecordDB::load(bool validateSize)
{
    bool statusFlag = this->loadExist(validateSize);
    if (statusFlag) {
#if RECORD_CLUST_BEDUG
        printTagLog(TAG, "Cluster loaded from address=%lu", m_address);
#endif
    } else {
#if RECORD_CLUST_BEDUG
        printTagLog(TAG, "Unable to load cluster, try to create new");
#endif
        statusFlag = this->createNew();
    }

    if (!statusFlag) {
#if RECORD_CLUST_BEDUG
        printTagLog(TAG, "Unable to create cluster");
#endif
        return RECORD_ERROR;
    }

#if RECORD_CLUST_BEDUG
	printTagLog(TAG, "Cluster loaded");
    record_cluster_show(&clust);
#endif

	bool recordFound = false;
    unsigned id;
    for (unsigned i = 0; i < records_current_count(&clust); i++) {
    	record_t* tmp_record = get_record_by_index(&clust, i);
        if (tmp_record->id == this->m_targetId) {
            recordFound = true;
            id = i;
            break;
        }
    }
    if (!recordFound) {
#if RECORD_BEDUG
        printTagLog(TAG, "Record not found");
#endif
        return RECORD_NO_LOG;
    }

    cacheRecord(id);

#if RECORD_BEDUG
    printTagLog(TAG, "Record loaded (cluster index=%u)", id);
    record_cluster_show(&clust);
    record_show(&clust, id);
#endif

    return RECORD_OK;
}

RecordStatus RecordDB::save()
{
	DeviceInfo::record_loaded::set(0);

#if RECORD_ENABLE_CACHE
	m_recordsExist = true;
#endif

	uint32_t size = record_current_size(&clust);

#if RECORD_CLUST_BEDUG
    printTagLog(TAG, "Saving record (size=%lu)", size);
#endif

    BEDUG_ASSERT(size <= record_max_size(), "Size of record is incorrect");

    if (size <= record_meta_size() || size > record_max_size()) {
        return RECORD_ERROR;
    }

    RecordStatus recordStatus = RECORD_OK;

    // 1. find max id
    uint32_t maxId = 0;
    uint32_t newId = 0;
    recordStatus = RecordDB::getMaxId(&maxId); // TODO: assert + update record ID
    if (recordStatus == RECORD_NO_LOG) {
        maxId = 0;
    } else if (recordStatus != RECORD_OK) {
#if RECORD_CLUST_BEDUG
    	BEDUG_ASSERT(false, "Unable to calculate new ID"); // TODO: assert after 337 line
#endif
        return RECORD_ERROR;
    }
    newId = maxId + 1;

    // 2. nope -----------create record_clust_t variable (tmp)
//    record_clust_t tmpClust = {};

    // 3. load cluster to tmp and validate
    this->m_targetId = maxId;
    RecordDB tmpRecord = *this;
    recordStatus = tmpRecord.load(true);
#if RECORD_CLUST_BEDUG
    BEDUG_ASSERT((recordStatus != RECORD_ERROR), "Unable to load cluster");
#endif
    if (recordStatus == RECORD_ERROR) {
        return recordStatus;
    }

    // 4. if sizes has the same values and cluster has empty section, copy current record to tmp
    unsigned emptyIndex = 0;
    {
        bool clustFLag = true;
        bool foundFlag = false;
        for (unsigned i = 0; i < records_current_count(&tmpRecord.clust); i++) {
        	record_t* tmp_record = get_record_by_index(&tmpRecord.clust, i);
            if (!tmp_record->id) {
                emptyIndex = i;
                foundFlag = true;
                break;
            }
        }
        if (!foundFlag) {
            clustFLag = tmpRecord.createNew();
        }
#if RECORD_CLUST_BEDUG
        BEDUG_ASSERT(clustFLag, "Unable to create cluster");
#endif
        if (!clustFLag) {
            return RECORD_ERROR;
        }
    }

    record_create(&record);
    record.id = newId;

    record_t* clust_record = get_record_by_index(&tmpRecord.clust, emptyIndex);
    memcpy(
        reinterpret_cast<void*>(clust_record),
        reinterpret_cast<void*>(&record),
        record_current_size(&clust)
    );

    // 5. save cluster
    StorageStatus storageStatus = STORAGE_OK;
    {
        storageStatus = storage->rewrite(
			tmpRecord.m_address,
			PREFIX,
			newId,
			reinterpret_cast<uint8_t*>(&tmpRecord.clust),
	        record_cluster_size(&tmpRecord.clust)
		);
        BEDUG_ASSERT((storageStatus == STORAGE_OK), "Storage save record error");
        if (storageStatus != STORAGE_OK) {
            return RECORD_ERROR;
        }

        this->m_targetId = newId;
    }

    // 6. load cluster
    recordStatus = this->load(true);
    BEDUG_ASSERT((recordStatus == RECORD_OK), "Error loading the saved record");
#if RECORD_CLUST_BEDUG
    if (recordStatus == RECORD_OK) {
        printTagLog(TAG, "Record cluster saved (address=%lu, id=%lu, record_size=%u)", m_address, newId, record_current_size(&clust));
    }
#endif

    if (recordStatus == RECORD_OK) {
    	set_status(NEED_LOAD_MIN_RECORD);
    	set_status(NEED_LOAD_MAX_RECORD);
#ifdef RECORD_BEDUG
        record_cluster_show(&clust);
    	record_show(&clust, emptyIndex);
    } else {
        printTagLog(TAG, "New record was not saved");
#endif
    }

    return recordStatus;
}

RecordStatus RecordDB::preLoadClust(const uint32_t address, record_clust_t& clust)
{
    record_clust_t tmpClust = {};
    StorageStatus status = storage->load(address, reinterpret_cast<uint8_t*>(&tmpClust), record_cluster_meta_size());
	if (status != STORAGE_OK) {
#if RECORD_CLUST_BEDUG
        printTagLog(TAG, "Preload cluster error: load record cluster meta error=%u", status);
#endif
		return RECORD_ERROR;
	}
	status = storage->load(address, reinterpret_cast<uint8_t*>(&tmpClust), record_cluster_size(&tmpClust));
	if (status != STORAGE_OK) {
#if RECORD_CLUST_BEDUG
        printTagLog(TAG, "Preload cluster error: load record cluster error=%u", status);
#endif
		return RECORD_ERROR;
	}
    if (!record_cluster_validate(&tmpClust)) {
#if RECORD_CLUST_BEDUG
        printTagLog(TAG, "Preload cluster error: validation failed (there is an incorrect cluster in the memory), delete cluster from address=%lu", address);
#endif
        BEDUG_ASSERT(storage->clearAddress(address) == STORAGE_OK, "Delete record error");
        return RECORD_ERROR;
    }

    memcpy(
		reinterpret_cast<void*>(&clust),
		reinterpret_cast<void*>(&tmpClust),
		record_cluster_size(&tmpClust)
    );

    return RECORD_OK;
}

RecordStatus RecordDB::getLastTime(uint32_t* time)
{
	utl::CodeStopwatch stopwatch(TAG, GENERAL_TIMEOUT_MS);

	uint32_t address = 0;
	StorageStatus status = storage->find(FIND_MODE_MAX, &address, PREFIX);
    if (status != STORAGE_OK) {
#if RECORD_CLUST_BEDUG
        printTagLog(TAG, "Get last time error: unable to find a cluster with error=%u", status);
#endif
        return RECORD_ERROR;
    }

    RecordDB tmpClust;
    RecordStatus recordStatus = preLoadClust(address, tmpClust.clust);
	if (recordStatus != RECORD_OK) {
#if RECORD_CLUST_BEDUG
        printTagLog(TAG, "Get last time error: load record cluster error=%u", status);
#endif
		return RECORD_ERROR;
	}

    uint32_t lastTime = 0;
    for (unsigned i = 0; i < records_current_count(&tmpClust.clust); i++) {
    	record_t* tmp_record = get_record_by_index(&tmpClust.clust, i);
    	if (tmp_record->time > lastTime) {
    		lastTime = tmp_record->time;
    	}
    }

    *time = lastTime;

    return RECORD_OK;
}

#if RECORD_ENABLE_CACHE

bool RecordDB::checkCachedRecordCLuster()
{
	if (!m_cacheLoaded) {
		return false;
	}

	for (unsigned i = 0; i < m_cache.size(); i++) {
		if (hasID(m_cache[i].cluster, m_targetId)) {
			return true;
		}
	}

	return false;
}

#endif

bool RecordDB::loadExist(bool validateSize)
{
    uint32_t address = 0;
    StorageStatus storageStatus = STORAGE_OK;

    record_clust_t tmpClust = {};

#if RECORD_ENABLE_CACHE

    bool cacheFound = false;
    if (checkCachedRecordCLuster()) {
#	if RECORD_CLUST_BEDUG
        printTagLog(TAG, "Use cached cluster");
#	endif
    	for (unsigned i = 0; i < m_cache.size(); i++) {
    		if (hasID(m_cache[i].cluster, m_targetId)) {
    			memcpy(
					reinterpret_cast<void*>(&tmpClust),
					reinterpret_cast<void*>(&m_cache[i].cluster),
					record_cluster_size(&m_cache[i].cluster)
				);
				m_address    = m_cache[i].address;
				address      = m_cache[i].address;
				cacheFound   = true;
				break;
    		}
    	}

    } else {

#endif

		storageStatus = storage->find(FIND_MODE_EQUAL, &address, PREFIX, m_targetId);
		if (storageStatus != STORAGE_OK) {
#if RECORD_CLUST_BEDUG
			printTagLog(TAG, "Unable to find an EQUAL cluster, the NEXT cluster is being searched");
#endif
			storageStatus = storage->find(FIND_MODE_NEXT, &address, PREFIX, m_targetId);
		}

		if (storageStatus != STORAGE_OK) {
#if RECORD_CLUST_BEDUG
			printTagLog(TAG, "Unable to find an NEXT cluster, the MAX ID cluster is being searched");
#endif
			storageStatus = storage->find(FIND_MODE_MAX, &address, PREFIX);
		}

		if (storageStatus != STORAGE_OK) {
#if RECORD_CLUST_BEDUG
			printTagLog(TAG, "Unable to find a cluster with error=%u", storageStatus);
#endif
			return false;
		}

		RecordStatus recordStatus = preLoadClust(address, tmpClust);
		if (recordStatus != RECORD_OK) {
#if RECORD_CLUST_BEDUG
			printTagLog(TAG, "Load record cluster error=%u", recordStatus);
#endif
			return false;
		}

#if RECORD_ENABLE_CACHE

    }

    if (checkCachedRecordCLuster() && !cacheFound) {
#if RECORD_CLUST_BEDUG
        printTagLog(TAG, "Use cached cluster error - not found");
#endif
        m_recordsExist = false;
        m_cacheLoaded = false;
        m_cache.pop_front();
    	return false;
    }

#endif


    if (validateSize && !record_cluster_validate_size(&clust, &tmpClust)) {
#if RECORD_CLUST_BEDUG
        printTagLog(TAG, "The current cluster has another record size, abort search");
#endif
        return false;
    }

    m_address = address;
    memcpy(
		reinterpret_cast<void*>(&clust),
		reinterpret_cast<void*>(&tmpClust),
		record_cluster_size(&tmpClust)
    );

    return true;
}

bool RecordDB::createNew()
{
    uint32_t address = 0;
    StorageStatus storageStatus = STORAGE_OK;
    StorageFindMode findMode = FIND_MODE_EMPTY;

    storageStatus = storage->find(findMode, &address);
    if (storageStatus == STORAGE_NOT_FOUND || storageStatus == STORAGE_OOM) {
        findMode = FIND_MODE_MIN;
        storageStatus = storage->find(findMode, &address);
    }

    BEDUG_ASSERT((storageStatus == STORAGE_OK), "Unable to find memory for log record");
    if (storageStatus != STORAGE_OK) {
        return false;
    }

    if (findMode == FIND_MODE_MIN) {
        storageStatus = storage->clearAddress(address);
        set_status(NEED_LOAD_MIN_RECORD);
    }
	if (findMode == FIND_MODE_MIN && storageStatus != STORAGE_OK) {
		BEDUG_ASSERT((storageStatus == STORAGE_OK), "Unable to erase memory for log record");
	}
    if (storageStatus != STORAGE_OK) {
        return false;
    }

    this->m_address = address;

    record_cluster_create(&clust);

    return true;
}

RecordStatus RecordDB::getMaxId(uint32_t* maxId)
{
	utl::CodeStopwatch stopwatch(TAG, GENERAL_TIMEOUT_MS);

    uint32_t address = 0;
    StorageStatus storageStatus = STORAGE_OK;

    storageStatus = storage->find(FIND_MODE_MAX, &address, PREFIX);
    if (storageStatus == STORAGE_NOT_FOUND) {
#if RECORD_CLUST_BEDUG
        printTagLog(TAG, "MAX ID not found, reset ID");
#endif
        return RECORD_NO_LOG;
    }

    BEDUG_ASSERT((storageStatus == STORAGE_OK), "Storage find error");
    if (storageStatus != STORAGE_OK) {
        return RECORD_ERROR;
    }

    record_clust_t tmpClust = {};
    RecordStatus recordStatus = preLoadClust(address, tmpClust);
	if (recordStatus != RECORD_OK) {
#if RECORD_CLUST_BEDUG
        printTagLog(TAG, "Load record cluster error=%u", recordStatus);
#endif
		return recordStatus;
	}

    *maxId = getMaxID(tmpClust);

#if RECORD_CLUST_BEDUG
    printTagLog(TAG, "MAX ID received from address=%lu id=%lu", address, *maxId);
#endif

    return RECORD_OK;
}

RecordStatus RecordDB::getMinId(uint32_t* minId)
{
	utl::CodeStopwatch stopwatch(TAG, GENERAL_TIMEOUT_MS);

    uint32_t address = 0;
    StorageStatus storageStatus = STORAGE_OK;

    storageStatus = storage->find(FIND_MODE_MIN, &address, PREFIX);
    if (storageStatus == STORAGE_NOT_FOUND) {
#if RECORD_CLUST_BEDUG
        printTagLog(TAG, "MIN ID not found, reset ID");
#endif
        return RECORD_NO_LOG;
    }

#if RECORD_CLUST_BEDUG
    BEDUG_ASSERT((storageStatus == STORAGE_OK), "Storage find error");
#endif
    if (storageStatus != STORAGE_OK) {
        return RECORD_ERROR;
    }

    record_clust_t tmpClust = {};
    RecordStatus recordStatus = preLoadClust(address, tmpClust);
	if (recordStatus != RECORD_OK) {
#if RECORD_CLUST_BEDUG
        printTagLog(TAG, "Load record cluster error=%u", recordStatus);
#endif
		return recordStatus;
	}

    *minId = getMinID(tmpClust);

#if RECORD_CLUST_BEDUG
    printTagLog(TAG, "MIN ID received from address=%lu id=%lu", address, *minId);
#endif

    return RECORD_OK;
}

#if RECORD_ENABLE_CACHE

RecordStatus RecordDB::updateCache(uint32_t cacheAfterId)
{
	if (!m_recordsExist) {
		return RECORD_NO_LOG;
	}

	if (hasID(m_cache[0].cluster, cacheAfterId + 1)) {
		return RECORD_OK;
	}

	unsigned index = 0;
	uint32_t maxID = 0;
	bool maxIDFound = false;
	for (unsigned i = 0; i < m_cache.size(); i++) {
		if (hasID(m_cache[i].cluster, cacheAfterId + 1)) {
			maxIDFound = true;
			break;
		}
		index++;
	}

	if (index == 0) {
		return RECORD_OK;
	}

#if RECORD_CLUST_BEDUG
	printTagLog(TAG, "Update cache (length=%u)", m_cache.size());
#endif

	if (maxIDFound) {
		m_cache.pop_front();
		for (unsigned i = 0; i < m_cache.size() - index; i++) {
			maxID = getMaxID(m_cache[i].cluster);
		}
#if RECORD_CLUST_BEDUG
		printTagLog(TAG, "Remove cache index=[%u->%u)", 0, index);
#endif
	} else {
		m_cache.clear();
		m_cacheLoaded = false;
		index = m_cache.size();
		maxID = cacheAfterId;
	}

	StorageStatus storageStatus = STORAGE_OK;
	uint32_t address = 0;
	for (unsigned i = 0; i < index; i++) {
		storageStatus = storage->find(FIND_MODE_NEXT, &address, PREFIX, maxID);
		if (storageStatus == STORAGE_NOT_FOUND) {
			m_recordsExist = false;
			break;
		}
	    if (storageStatus != STORAGE_OK) {
#if RECORD_CLUST_BEDUG
	        printTagLog(TAG, "Unable to find a cluster after ID %lu with error=%u", maxID, storageStatus);
#endif
	        return RECORD_ERROR;
	    }

	    record_clust_t tmpClust = {};
	    RecordStatus recordStatus = preLoadClust(address, tmpClust);
		if (recordStatus != RECORD_OK) {
#if RECORD_CLUST_BEDUG
			printTagLog(TAG, "Load record cluster after ID %lu error=%u", maxID, recordStatus);
#endif
			return RECORD_ERROR;
		}

		record_cache_t tmpCache;
		memcpy(
			reinterpret_cast<void*>(&tmpCache.cluster),
			reinterpret_cast<void*>(&tmpClust),
			record_cluster_size(&tmpClust)
		);
		tmpCache.address    = address;
		m_cache.push_back(tmpCache);

		maxID = getMaxID(tmpClust);

#if RECORD_CLUST_BEDUG
		printTagLog(TAG, "Cache updated");
#endif
	}

	m_cacheLoaded = true;

	return RECORD_OK;
}

#endif

uint32_t RecordDB::getMinID(const record_clust_t& clust)
{
	if (!record_cluster_validate(&clust)) {
		return 0;
	}
	uint32_t minId = std::numeric_limits<uint32_t>::max();
	for (unsigned i = 0; i < records_current_count(&clust); i++) {
		uint32_t tmpId = get_record_by_index(&clust, i)->id;
		if (tmpId && minId > tmpId) {
			minId = tmpId;
		}
	}
	return minId;
}

uint32_t RecordDB::getMaxID(const record_clust_t& clust)
{
	if (!record_cluster_validate(&clust)) {
		return 0;
	}
	uint32_t maxId = 0;
	for (unsigned i = 0; i < records_current_count(&clust); i++) {
		uint32_t tmpId = get_record_by_index(&clust, i)->id;
		if (tmpId && maxId < tmpId) {
			maxId = tmpId;
		}
	}
	return maxId;
}

bool RecordDB::hasID(const record_clust_t& clust, uint32_t ID)
{
	if (!record_cluster_validate(&clust)) {
		return false;
	}
	return getMinID(clust) <= ID && ID <= getMaxID(clust);
}
