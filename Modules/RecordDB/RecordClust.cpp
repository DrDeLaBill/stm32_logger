/* Copyright © 2024 Georgy E. All rights reserved. */

#include "RecordClust.h"

#include <cstring>
#include <stdint.h>

#include "log.h"
#include "bmacro.h"
#include "w25qxx.h"
#include "settings.h"
#include "StorageAT.h"
#include "StorageDriver.h"


const char* RecordClust::PREFIX = "RDC";
const char* RecordClust::TAG = "RDC";


RecordClust::RecordClust(uint32_t recordId, uint32_t recordSize):
		m_recordId(recordId), m_recordSize(recordSize), m_address(0)
{
	memset(reinterpret_cast<size_t*>(&m_clust), 0, sizeof(m_clust));
}

Record::record_t& RecordClust::operator[](unsigned i)
{
#if RECORD_BEDUG
	BEDUG_ASSERT((i < getRecordsCountBySize(m_recordSize)), "Cluster records out of range");
#endif
	return this->m_clust.records[i];
}

RecordStatus RecordClust::load()
{
	bool statusFlag = this->findExist();
	if (statusFlag) {
#if RECORD_BEDUG
		printTagLog(TAG, "cluster loaded from address=%08X", (unsigned int)m_address);
#endif
	} else {
		statusFlag = this->createNew();
	}

	if (!statusFlag) {
		return RECORD_ERROR;
	}

	StorageDriver storageDriver;
	StorageAT storage(
		flash_w25qxx_get_pages_count(),
		&storageDriver
	);
	StorageStatus storageStatus = STORAGE_OK;
	record_clust_t tmpClust = {};

	storageStatus = storage.load(m_address, reinterpret_cast<uint8_t*>(&tmpClust), this->size());
#if RECORD_BEDUG
	BEDUG_ASSERT((storageStatus == STORAGE_OK), "Storage error");
#endif
	if (storageStatus != STORAGE_OK) {
		return RECORD_ERROR;
	}

	memcpy(reinterpret_cast<void*>(&m_clust), reinterpret_cast<void*>(&tmpClust), this->size());

	return RECORD_OK;
}

RecordStatus RecordClust::save(Record::record_t *record, uint32_t size)
{
#if RECORD_BEDUG
	printTagLog(TAG, "Saving record (size=%lu)", size);
	BEDUG_ASSERT((size > Record::SZ_MAX), "Size of record is large than MAX");
#endif
	if (size > Record::SZ_MAX) {
		return RECORD_ERROR;
	}

	RecordStatus recordStatus = RECORD_OK;

	// 1. find max id
	uint32_t maxId = 0;
	uint32_t newId = 0;
	recordStatus = getMaxId(&maxId);
	if (recordStatus == RECORD_NO_LOG) {
		maxId = 0;
	}
#if RECORD_BEDUG
	BEDUG_ASSERT((recordStatus != RECORD_ERROR), "Unable to calculate new ID");
#endif
	if (recordStatus == RECORD_ERROR) {
		return RECORD_ERROR;
	}
	newId = maxId + 1;

	// 2. nope -----------create record_clust_t variable (tmp)
//	record_clust_t tmpClust = {};

	// 3. load cluster to tmp and validate
	this->m_recordId = maxId;
	recordStatus = this->load();
#if RECORD_BEDUG
	BEDUG_ASSERT((recordStatus == RECORD_OK), "Unable to load cluster");
#endif
	if (recordStatus != RECORD_OK) {
		return recordStatus;
	}

	// 4. check that sizes has the same values
	{
		bool clustFLag = true;
		if (m_clust.rcrd_size != size) {
			clustFLag = this->createNew();
		}
	// 5. if it is false, create new cluster
#if RECORD_BEDUG
		BEDUG_ASSERT(clustFLag, "Unable to create cluster");
#endif
		if (!clustFLag) {
			return RECORD_ERROR;
		}
	}

	// 6. if it is true and cluster has empty section, copy current record to tmp
	unsigned emptyIndex = 0;
	{
		bool clustFLag = true;
		bool foundFlag = false;
		for (unsigned i = 0; i < this->count(); i++) {
			if (!m_clust.records[i].id) {
				emptyIndex = i;
				foundFlag = true;
				break;
			}
		}
		if (!foundFlag) {
			clustFLag = this->createNew();
		}
#if RECORD_BEDUG
		BEDUG_ASSERT(clustFLag, "Unable to create cluster");
#endif
		if (!clustFLag) {
			return RECORD_ERROR;
		}
	}
	memcpy(
		reinterpret_cast<void*>(&m_clust.records[emptyIndex]),
		reinterpret_cast<void*>(&record),
		m_recordSize
	);
	record->id = newId;

	// 7. save cluster
	StorageDriver storageDriver;
	StorageAT storage(
		flash_w25qxx_get_pages_count(),
		&storageDriver
	);
	StorageStatus storageStatus = STORAGE_OK;

	storageStatus = storage.rewrite(m_address, PREFIX, newId, reinterpret_cast<uint8_t*>(&m_clust), this->size());
#if RECORD_BEDUG
	BEDUG_ASSERT((storageStatus == STORAGE_OK), "Storage save record error");
#endif
	if (storageStatus != STORAGE_OK) {
		return RECORD_ERROR;
	}

	// 8. load cluster
	recordStatus = this->load();
#if RECORD_BEDUG
	BEDUG_ASSERT((recordStatus == RECORD_OK), "Error loading the saved record");
	if (recordStatus == RECORD_OK) {
		printTagLog(TAG, "Record saved (address=%lu, id=%lu)", m_address, newId);
	}
#endif

	return recordStatus;
}

bool RecordClust::validate()
{
	if (m_clust.dv_id != settings.dv_id) {
		return false;
	}

	if (m_clust.sw_id != settings.sw_id) {
		return false;
	}

	return true;
}

bool RecordClust::findExist()
{
	StorageDriver storageDriver;
	StorageAT storage(
		flash_w25qxx_get_pages_count(),
		&storageDriver
	);
	uint32_t address = 0;
	StorageStatus storageStatus = STORAGE_OK;

	storageStatus = storage.find(FIND_MODE_EQUAL, &address, PREFIX, m_recordId);

	if (storageStatus != STORAGE_OK) {
#if RECORD_BEDUG
		printTagLog(TAG, "Unable to find an EQUAL cluster, the NEXT cluster is being searched");
#endif
		storageStatus = storage.find(FIND_MODE_NEXT, &address, PREFIX, m_recordId);
	}

	if (storageStatus != STORAGE_OK) {
#if RECORD_BEDUG
		printTagLog(TAG, "Unable to find an NEXT cluster, the MAX ID cluster is being searched");
#endif
		storageStatus = storage.find(FIND_MODE_MAX, &address, PREFIX);
	}

	if (storageStatus != STORAGE_OK) {
#if RECORD_BEDUG
		printTagLog(TAG, "Unable to find a cluster with error=%u", storageStatus);
#endif
		return false;
	}

	if (!this->validate()) {
		BEDUG_ASSERT(storage.deleteData(address) == STORAGE_OK, "Delete record error");
		return false;
	}

	if (m_recordSize > 0 &&  m_clust.rcrd_size != m_recordSize) {
#if RECORD_BEDUG
		printTagLog(TAG, "The current cluster has another record size, abort search");
#endif
		return false;
	}

	m_address = address;

	return true;
}

bool RecordClust::createNew()
{
	StorageDriver storageDriver;
	StorageAT storage(
		flash_w25qxx_get_pages_count(),
		&storageDriver
	);
	uint32_t address = 0;
	StorageStatus storageStatus = STORAGE_OK;
	StorageFindMode findMode = FIND_MODE_EMPTY;

	storageStatus = storage.find(findMode, &address);
	if (storageStatus == STORAGE_NOT_FOUND || storageStatus == STORAGE_OOM) {
		findMode = FIND_MODE_MIN;
		storageStatus = storage.find(findMode, &address);
	}

#if RECORD_BEDUG
	BEDUG_ASSERT((storageStatus == STORAGE_OK), "Unable to find memory for log record");
#endif
	if (storageStatus != STORAGE_OK) {
		return false;
	}

	if (findMode == FIND_MODE_MIN) {
		storageStatus = storage.deleteData(address);
	}
#if RECORD_BEDUG
	BEDUG_ASSERT((storageStatus == STORAGE_OK), "Unable to erase memory for log record");
#endif
	if (storageStatus != STORAGE_OK) {
		return false;
	}

	this->m_clust.dv_id     = settings.dv_id;
	this->m_clust.sw_id     = settings.sw_id;
	this->m_clust.rcrd_size = m_recordSize;
	memset(this->m_clust.records, 0, sizeof(this->m_clust.records));

	this->m_address = address;

	return true;
}

RecordStatus RecordClust::getMaxId(uint32_t* maxId)
{
	StorageDriver storageDriver;
	StorageAT storage(
		flash_w25qxx_get_pages_count(),
		&storageDriver
	);
    uint32_t address = 0;
	StorageStatus storageStatus = STORAGE_OK;

	storageStatus = storage.find(FIND_MODE_MAX, &address, PREFIX);
    if (storageStatus == STORAGE_NOT_FOUND) {
#if RECORD_BEDUG
        printTagLog(TAG, "MAX ID not found, reset ID");
#endif
        return RECORD_NO_LOG;
    }

#if RECORD_BEDUG
	BEDUG_ASSERT((storageStatus == STORAGE_OK), "Storage find error");
#endif
    if (storageStatus != STORAGE_OK) {
        return RECORD_ERROR;
    }

    record_clust_t tmpClust;
    storageStatus = storage.load(address, reinterpret_cast<uint8_t*>(&tmpClust), sizeof(tmpClust));
#if RECORD_BEDUG
	BEDUG_ASSERT((storageStatus == STORAGE_OK), "Storage load error");
#endif
    if (storageStatus != STORAGE_OK) {
        return RECORD_ERROR;
    }

    *maxId = 0;
    for (unsigned i = 0; i < this->count(); i++) {
    	if (*maxId < tmpClust.records[i].id) {
    		*maxId = tmpClust.records[i].id;
    	}
    }

#if RECORD_BEDUG
    printTagLog(TAG, "new ID received from address=%08X id=%lu", (unsigned int)address, *maxId);
#endif

    return RECORD_OK;
}

uint32_t RecordClust::getRecordsCountBySize(uint32_t structSize)
{
#if RECORD_BEDUG
	BEDUG_ASSERT((structSize > 0), "Record size must be large than 0");
#endif
	if (structSize == 0) {
		return 1;
	}
	uint32_t payload_clust = Page::PAYLOAD_SIZE - RecordClust::META_SIZE;
	return (structSize > payload_clust) ? 1 : (payload_clust / structSize);
}

uint32_t RecordClust::count()
{
	return getRecordsCountBySize(m_recordSize);
}

uint32_t RecordClust::size()
{
	return META_SIZE + (m_recordSize * count());
}
