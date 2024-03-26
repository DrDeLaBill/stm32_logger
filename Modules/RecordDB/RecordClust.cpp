/* Copyright © 2024 Georgy E. All rights reserved. */

#include "RecordClust.h"

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

#include "Record.h"
#include "StorageAT.h"
#include "StorageDriver.h"
#include "CodeStopwatch.h"


extern StorageAT* storage;


#if RECORD_ENABLE_CACHE

utl::circle_buffer<RECORD_CACHED_COUNT, RecordClust::cache_t> RecordClust::m_cache;
uint32_t RecordClust::m_cacheAfterId = 0;
bool RecordClust::m_cacheLoaded = false;

#endif


RecordClust::RecordClust(uint32_t recordId, uint16_t recordSize):
 	m_recordId(recordId), m_recordSize(recordSize), m_address(0)
{
    memset(reinterpret_cast<void*>(&m_clust), 0, sizeof(m_clust));
}

record_t& RecordClust::operator[](unsigned i)
{
#if RECORD_CLUST_BEDUG
    BEDUG_ASSERT((i < getCountByRecordSize(m_clust.rcrd_size)), "Cluster records out of range");
#endif
    return m_clust[i];
}

RecordClust::cache_t::cache_t()
{
	memset(reinterpret_cast<void*>(&(this->cluster)), 0, sizeof(cluster));
	this->recordSize = 0;
	this->address = 0;
}

RecordClust::cache_t::cache_t(const cache_t& other)
{
	(*this) = other;
}

RecordClust::cache_t& RecordClust::cache_t::operator=(const cache_t& other)
{
	memcpy(
		reinterpret_cast<void*>(&(this->cluster)),
		reinterpret_cast<void*>(const_cast<record_clust_t*>(&(other.cluster))),
		sizeof(this->cluster)
	);
	this->recordSize = other.recordSize;
	this->address = other.address;
	return *this;
}

RecordClust::cache_t::~cache_t() {}

record_t& RecordClust::record_clust_t::operator[](unsigned i)
{
#if RECORD_CLUST_BEDUG
    BEDUG_ASSERT((i < RecordClust::getCountByRecordSize(rcrd_size)), "Cluster records out of range");
#endif
    return *(reinterpret_cast<record_t*>(&(records[i * rcrd_size])));
}

RecordStatus RecordClust::load(bool validateSize)
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
	this->show();
#endif

    return RECORD_OK;
}

RecordStatus RecordClust::save(record_t *record, uint32_t size)
{
#if RECORD_CLUST_BEDUG
    printTagLog(TAG, "Saving record (size=%lu)", size);
    BEDUG_ASSERT(size <= RECORD_SIZE_MAX, "Size of record is incorrect");
#endif
    if (size <= RECORD_META_SIZE || size > RECORD_SIZE_MAX) {
        return RECORD_ERROR;
    }

    RecordStatus recordStatus = RECORD_OK;

    // 1. find max id
    uint32_t maxId = 0;
    uint32_t newId = 0;
    recordStatus = RecordClust::getMaxId(&maxId); // TODO: assert + update record ID
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
    this->m_recordId = maxId;
    recordStatus = this->load(true);
#if RECORD_CLUST_BEDUG
    BEDUG_ASSERT((recordStatus == RECORD_OK), "Unable to load cluster");
#endif
    if (recordStatus != RECORD_OK) {
        return recordStatus;
    }

    // 4. if sizes has the same values and cluster has empty section, copy current record to tmp
    unsigned emptyIndex = 0;
    {
        bool clustFLag = true;
        bool foundFlag = false;
        for (unsigned i = 0; i < m_clust.count(); i++) {
            if (!(*this)[i].id) {
                emptyIndex = i;
                foundFlag = true;
                break;
            }
        }
        if (!foundFlag) {
            clustFLag = this->createNew();
        }
#if RECORD_CLUST_BEDUG
        BEDUG_ASSERT(clustFLag, "Unable to create cluster");
#endif
        if (!clustFLag) {
            return RECORD_ERROR;
        }
    }
    record->id = newId;
    memcpy(
        reinterpret_cast<void*>(&(*this)[emptyIndex]),
        reinterpret_cast<void*>(record),
        m_recordSize
    );

    // 5. save cluster
    StorageStatus storageStatus = STORAGE_OK;
    {
        storageStatus = storage->rewrite(m_address, PREFIX, newId, reinterpret_cast<uint8_t*>(&m_clust), m_clust.size());
#if RECORD_CLUST_BEDUG
        BEDUG_ASSERT((storageStatus == STORAGE_OK), "Storage save record error");
#endif
        if (storageStatus != STORAGE_OK) {
            return RECORD_ERROR;
        }

        this->m_recordId = newId;
    }

    // 6. load cluster
    recordStatus = this->load(true);
#if RECORD_CLUST_BEDUG
    BEDUG_ASSERT((recordStatus == RECORD_OK), "Error loading the saved record");
    if (recordStatus == RECORD_OK) {
        printTagLog(TAG, "Record cluster saved (address=%lu, id=%lu, record_size=%u)", m_address, newId, m_recordSize);
    }
#endif

    return recordStatus;
}

RecordStatus RecordClust::preLoadClust(const uint32_t address, record_clust_t& clust)
{
    record_clust_t tmpClust = {};
    StorageStatus status = storage->load(address, reinterpret_cast<uint8_t*>(&tmpClust), RecordClust::META_SIZE);
	if (status != STORAGE_OK) {
#if RECORD_CLUST_BEDUG
        printTagLog(TAG, "Preload cluster error: load record cluster meta error=%u", status);
#endif
		return RECORD_ERROR;
	}
	status = storage->load(address, reinterpret_cast<uint8_t*>(&tmpClust), tmpClust.size());
	if (status != STORAGE_OK) {
#if RECORD_CLUST_BEDUG
        printTagLog(TAG, "Preload cluster error: load record cluster error=%u", status);
#endif
		return RECORD_ERROR;
	}
    if (!validate(&tmpClust)) {
#if RECORD_CLUST_BEDUG
        printTagLog(TAG, "Preload cluster error: validation failed (there is an incorrect cluster in the memory), delete cluster from address=%lu", address);
#endif
        BEDUG_ASSERT(storage->clearAddress(address) == STORAGE_OK, "Delete record error");
        return RECORD_ERROR;
    }

    memcpy(
		reinterpret_cast<void*>(&clust),
		reinterpret_cast<void*>(&tmpClust),
		tmpClust.size()
    );

    return RECORD_OK;
}

RecordStatus RecordClust::getLastTime(uint32_t* time)
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

    RecordClust tmpClust;
    RecordStatus recordStatus = preLoadClust(address, tmpClust.m_clust);
	if (recordStatus != RECORD_OK) {
#if RECORD_CLUST_BEDUG
        printTagLog(TAG, "Get last time error: load record cluster error=%u", status);
#endif
		return RECORD_ERROR;
	}

    uint32_t lastTime = 0;
    for (unsigned i = 0; i < tmpClust.records_count(); i++) {
    	if (tmpClust[i].time > lastTime) {
    		lastTime = tmpClust[i].time;
    	}
    }

    *time = lastTime;

    return RECORD_OK;
}

bool RecordClust::validate(record_clust_t* clust)
{
    if (clust->dv_type != settings.dv_type) {
        return false;
    }

    if (clust->sw_id != settings.sw_id) {
        return false;
    }

    if (!clust->rcrd_size || clust->rcrd_size > RECORD_SIZE_MAX) {
    	return false;
    }

    for (unsigned i = 0; i < getCountByRecordSize(clust->rcrd_size); i++) {
    	if ((*clust)[i].id) {
    		return true;
    	}
    }

    return false;
}

void RecordClust::show()
{
#if RECORD_CLUST_BEDUG
	RTC_DateTypeDef date = {};
	clock_get_rtc_date(&date);
	RTC_TimeTypeDef time = {};
	clock_get_rtc_time(&time);

	printPretty("               %02u-%02u-20%02u\n", date.Date, date.Month, date.Year);
	printPretty("                %02u:%02u:%02u\n", time.Hours, time.Minutes, time.Seconds);
	printPretty("##############RECORD CLUST###############\n");
	printPretty("Device type: %u\n", m_clust.dv_type);
	printPretty("Software v%02u\n", m_clust.sw_id);
	printPretty("Record size %u\n", m_clust.rcrd_size);
    printPretty("INDEX   RCRDID    TIME       SENSID   VALUE\n");
	for (uint8_t i = 0; i < getCountByRecordSize(m_clust.rcrd_size); i++) {
		if (!(*this)[i].id) {
			break;
		}
	    printPretty("%03u     %09lu %010lu ", i, (*this)[i].id, (*this)[i].time);
	    for (uint8_t j = 0; j < Record::getSensorsCountBySize(m_clust.rcrd_size); j++) {
	    	sensor_t* sensPtr = reinterpret_cast<sensor_t*>((*this)[i].sens);
	    	if (j == 0) {
	    		gprint("%03u      %u\n", sensPtr[j].ID, sensPtr[j].value);
	    	} else {
	    		printPretty("                             %03u      %u\n", sensPtr[j].ID, sensPtr[j].value);
	    	}
	    }
	}
	if (!getCountByRecordSize(m_clust.rcrd_size)) {
        printPretty("------------------EMPTY------------------\n");
	}
	printPretty("##############RECORD CLUST###############\n");
#endif
}

#if RECORD_ENABLE_CACHE

bool RecordClust::checkCachedRecordCLuster()
{
	if (!m_cacheLoaded) {
		return false;
	}

	for (unsigned i = 0; i < m_cache.size(); i++) {
		if (m_cache[i].cluster.hasID(m_recordId) || m_recordId < m_cache[i].cluster.minID()) {
			return true;
		}
	}

	return false;
}

#endif

bool RecordClust::loadExist(bool validateSize)
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
    		if (m_cache[i].cluster.hasID(m_recordId) ||
				m_recordId < m_cache[i].cluster.minID()
			) {
    			tmpClust     = m_cache[i].cluster;
				m_recordSize = m_cache[i].recordSize;
				m_address    = m_cache[i].address;
				address      = m_cache[i].address;
				cacheFound   = true;
				break;
    		}
    	}

    } else {

#endif

		storageStatus = storage->find(FIND_MODE_EQUAL, &address, PREFIX, m_recordId);
		if (storageStatus != STORAGE_OK) {
#if RECORD_CLUST_BEDUG
			printTagLog(TAG, "Unable to find an EQUAL cluster, the NEXT cluster is being searched");
#endif
			storageStatus = storage->find(FIND_MODE_NEXT, &address, PREFIX, m_recordId);
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
    	return false;
    }

#endif


    if (validateSize && m_recordSize > 0 && tmpClust.rcrd_size != m_recordSize) {
#if RECORD_CLUST_BEDUG
        printTagLog(TAG, "The current cluster has another record size, abort search");
#endif
        return false;
    }

    m_address = address;
    memcpy(
		reinterpret_cast<void*>(&m_clust),
		reinterpret_cast<void*>(&tmpClust),
		tmpClust.size()
    );

    return true;
}

bool RecordClust::createNew()
{
    uint32_t address = 0;
    StorageStatus storageStatus = STORAGE_OK;
    StorageFindMode findMode = FIND_MODE_EMPTY;

    storageStatus = storage->find(findMode, &address);
    if (storageStatus == STORAGE_NOT_FOUND || storageStatus == STORAGE_OOM) {
        findMode = FIND_MODE_MIN;
        storageStatus = storage->find(findMode, &address);
    }

#if RECORD_CLUST_BEDUG
    BEDUG_ASSERT((storageStatus == STORAGE_OK), "Unable to find memory for log record");
#endif
    if (storageStatus != STORAGE_OK) {
        return false;
    }

    if (findMode == FIND_MODE_MIN) {
        storageStatus = storage->clearAddress(address);
        set_status(NEED_LOAD_MIN_RECORD);
    }
#if RECORD_CLUST_BEDUG
	if (findMode == FIND_MODE_MIN && storageStatus != STORAGE_OK) {
		BEDUG_ASSERT((storageStatus == STORAGE_OK), "Unable to erase memory for log record");
	}
#endif
    if (storageStatus != STORAGE_OK) {
        return false;
    }

    this->m_clust.dv_type   = settings.dv_type;
    this->m_clust.sw_id     = settings.sw_id;
    this->m_clust.rcrd_size = m_recordSize;
    memset(this->m_clust.records, 0, sizeof(this->m_clust.records));

    this->m_address = address;

    return true;
}

RecordStatus RecordClust::getMaxId(uint32_t* maxId)
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

    *maxId = tmpClust.maxID();

#if RECORD_CLUST_BEDUG
    printTagLog(TAG, "MAX ID received from address=%lu id=%lu", address, *maxId);
#endif

    return RECORD_OK;
}

RecordStatus RecordClust::getMinId(uint32_t* minId)
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

    *minId = tmpClust.minID();

#if RECORD_CLUST_BEDUG
    printTagLog(TAG, "MIN ID received from address=%lu id=%lu", address, *minId);
#endif

    return RECORD_OK;
}

#if RECORD_ENABLE_CACHE

RecordStatus RecordClust::updateCache(uint32_t cacheAfterId)
{
	if (m_cache[0].cluster.hasID(cacheAfterId + 1)) {
		return RECORD_OK;
	}

	unsigned index = 0;
	uint32_t maxID = 0;
	bool maxIDFound = false;
	for (unsigned i = 0; i < m_cache.size(); i++) {
		if (m_cache[i].cluster.hasID(cacheAfterId + 1)) {
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
			maxID = m_cache[i].cluster.maxID();
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

		cache_t tmpCache;
		tmpCache.cluster = tmpClust;
		tmpCache.recordSize = tmpClust.rcrd_size;
		tmpCache.address = address;
		m_cache.push_back(tmpCache);

		maxID = tmpClust.maxID();

#if RECORD_CLUST_BEDUG
		printTagLog(TAG, "Cache updated");
#endif
	}

	m_cacheLoaded = true;

	return RECORD_OK;
}

#endif

uint32_t RecordClust::getCountByRecordSize(uint32_t recordSize)
{
#if RECORD_CLUST_BEDUG
    BEDUG_ASSERT((recordSize > 0), "Record size must be large than 0");
#endif
    if (recordSize == 0) {
        return 0;
    }

    uint32_t payload_clust = Page::PAYLOAD_SIZE - RecordClust::META_SIZE;
    return (recordSize > payload_clust) ? 1 : (payload_clust / recordSize);
}

uint32_t RecordClust::record_clust_t::minID()
{
	uint32_t minId = std::numeric_limits<uint32_t>::max();
	for (unsigned i = 0; i < getCountByRecordSize(rcrd_size); i++) {
		uint32_t tmpId = (*this)[i].id;
		if (tmpId && minId > tmpId) {
			minId = tmpId;
		}
	}
	return minId;
}

uint32_t RecordClust::record_clust_t::maxID()
{
	uint32_t maxId = 0;
	for (unsigned i = 0; i < getCountByRecordSize(rcrd_size); i++) {
		uint32_t tmpId = (*this)[i].id;
		if (tmpId && maxId < tmpId) {
			maxId = tmpId;
		}
	}
	return maxId;
}

bool RecordClust::record_clust_t::hasID(uint32_t ID)
{
	return minID() <= ID && ID <= maxID();
}

uint32_t RecordClust::record_clust_t::count()
{
    return getCountByRecordSize(this->rcrd_size);
}

uint32_t RecordClust::record_clust_t::size()
{
#if RECORD_CLUST_BEDUG
    BEDUG_ASSERT((this->rcrd_size > 0), "Record size must be large than 0");
#endif
	if (!this->count()) {
		return 0;
	}

    return RecordClust::META_SIZE + this->rcrd_size * this->count();
}

uint32_t RecordClust::records_count()
{
	return this->m_clust.count();
}

uint32_t RecordClust::record_size()
{
	return this->m_clust.rcrd_size;
}

uint32_t RecordClust::structure_size()
{
	return this->m_clust.size();
}
