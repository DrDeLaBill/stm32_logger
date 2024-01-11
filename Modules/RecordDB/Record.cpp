/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Record.h"

#include <cstring>
#include <stdint.h>

#include "log.h"
#include "clock.h"
#include "settings.h"
#include "StorageAT.h"
#include "RecordClust.h"
#include "RecordStatus.h"
#include "StorageDriver.h"


const char* Record::PREFIX = "RCR";
const char* Record::TAG = "RCR";


Record::Record(uint32_t recordId): m_recordId(recordId), m_regsCount(0)
{
	memset(reinterpret_cast<size_t*>(&record), 0, sizeof(record));
}

Record::register_t& Record::operator[](const unsigned i)
{
	return this->record.regs[i];
}

RecordStatus Record::load()
{
	RecordClust clust(m_recordId, this->size());

	RecordStatus recordStatus = clust.load();
	if (recordStatus != RECORD_OK) {
		return recordStatus;
	}

	bool recordFound = false;
	unsigned id;
	for (unsigned i = 0; i < clust.count(); i++) {
		if (clust[i].id == this->m_recordId) {
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

	memcpy(reinterpret_cast<void*>(&(this->record)), reinterpret_cast<void*>(&(clust[id])), this->size());

#if RECORD_BEDUG
	printTagLog(TAG, "Record loaded index=%u", id);
#endif

	return RECORD_OK;
}

RecordStatus Record::loadNext()
{
	RecordClust clust(m_recordId + 1, this->size());

	RecordStatus recordStatus = clust.load();
	if (recordStatus != RECORD_OK) {
		return recordStatus;
	}

    bool recordFound = false;
	unsigned idx;
	uint32_t curId = 0xFFFFFFFF;
	for (unsigned i = 0; i < clust.count(); i++) {
		if (clust[i].id > m_recordId && curId > clust[i].id) {
			curId = clust[i].id;
			recordFound = true;
			idx = i;
			break;
		}
	}

    if (!recordFound) {
#if RECORD_BEDUG
        printTagLog(TAG, "Record not found");
#endif
        return RECORD_NO_LOG;
    }

    memcpy(
		reinterpret_cast<void*>(&(this->record)),
		reinterpret_cast<void*>(&(clust[idx])),
		this->size()
	);

#if RECORD_BEDUG
    printTagLog(TAG, "Next record loaded index=%u", idx);
#endif

    return RECORD_OK;
}

RecordStatus Record::save()
{
	RecordClust clust;
	RecordStatus recordStatus = RECORD_OK;

	record.time = clock_get_timestamp();
	recordStatus = clust.save(&record, this->size());

#ifdef RECORD_BEDUG
	if (recordStatus != RECORD_OK) {
		printTagLog(TAG, "New record was not saved");
	}
#endif

    return recordStatus;
}

unsigned Record::size()
{
	return META_SIZE + m_regsCount * sizeof(uint16_t);
}

unsigned Record::count()
{
	return m_regsCount;
}
